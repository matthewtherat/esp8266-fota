#include "params.h"
#include "status.h"
#include "webadmin.h"
#include "httpd.h"
#include "uns.h"
#include "http.h"

#include <upgrade.h>
#include <osapi.h>
#include <mem.h>



//#define HTML_HEADER \
//    "<!DOCTYPE html><html>" \
//    "<head><title>ESP8266 Firstboot config</title></head><body>\r\n" 
//
//#define HTML_FOOTER "\r\n</body></html>\r\n"
//
//#define HTML_INDEX \
//    HTML_HEADER \
//    "<h4>Welcome to %s Web Administration</h4><br />" \
//    "<a href=\"/params\">Params</a><br />" \
//    HTML_FOOTER


#define WEBADMIN_ERR_FLASHREAD        -100
#define WEBADMIN_ERR_SAVEPARAMS       -101
#define WEBADMIN_UNKNOWNFIELD         -102
#define WEBADMIN_ERR_FLASHWRITE       -103
#define WEBADMIN_ERR_FLASHWRPROTECT   -104
#define WEBADMIN_BUFFSIZE             1024
#define SEC_SIZE                      SPI_FLASH_SEC_SIZE


#if SPI_SIZE_MAP == 2
#define INDEXHTML_SECTOR        0x77
#elif SPI_SIZE_MAP == 4
#define INDEXHTML_SECTOR        0x220
#elif SPI_SIZE_MAP == 6
#define INDEXHTML_SECTOR        0x170
#endif


static struct params *params;
static char buff[WEBADMIN_BUFFSIZE];
static size16_t bufflen;

#define SECTFMT     "0x%4X"

struct fileserve {
    uint32_t remain;
    uint32_t sect;
};


static ICACHE_FLASH_ATTR
httpd_err_t _index_chunk_sent(struct httpd_session *s) {
    httpd_err_t err;
    struct fileserve *f = (struct fileserve *) s->reverse;
    size16_t available = HTTPD_RESP_LEN(s);
    uint16_t chunklen;

    if (available) {
        return HTTPD_OK;
    }
    
    if (f == NULL) {
        return HTTPD_OK;
    }

    char *tmp = os_zalloc(SEC_SIZE);
    if (f->remain) {
        /* Read sector:  */
        f->sect++;
        err = spi_flash_read(f->sect * SEC_SIZE, (uint32_t*)tmp, SEC_SIZE);
        if (err) {
            ERROR("SPI Flash read failed: %d", err);
            goto reterr;
        }
        
        chunklen = MIN(f->remain, SEC_SIZE);
        /* Remain: %u chunklen: %u */
        err = httpd_send(s, tmp, chunklen);
        if (err) {
            goto reterr;
        }
        f->remain -= chunklen;
    }
    
    if (f->remain == 0){
        /* Finalize */
        WDTCHECK(s);
        httpd_response_finalize(s, HTTPD_FLAG_NONE);
        goto retok;
    } 
    else if(!HTTPD_SCHEDULE(HTTPD_SIG_RECVUNHOLD, s)) {
        err = HTTPD_ERR_TASKQ_FULL;
        goto reterr;
    }

retok:
    /* Free */
    os_free(tmp);
    os_free(f);
    s->reverse = NULL;
    WDTCHECK(s);
    return HTTPD_OK;

reterr:
    /* Free */
    os_free(tmp);
    os_free(f);
    s->reverse = NULL;
    return err;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_index_get(struct httpd_session *s) {
    httpd_err_t err;
    size16_t sect;
    char *tmp = os_zalloc(SEC_SIZE);
    uint16_t chunklen;
    uint8_t offset = sizeof(uint32_t);
    uint32_t remain;

    sect = INDEXHTML_SECTOR;
    /* Read sector:  */
    err = spi_flash_read(sect * SEC_SIZE, (uint32_t*)tmp, SEC_SIZE);
    if (err) {
        ERROR("SPI Flash read failed: %d", err);
        goto reterr;
    }
    
    /* Find file size */
    remain = ((uint32_t*) tmp)[0];
    //os_memcpy(&remain, tmp, 4);
    
    /* Response headers */
    struct httpd_header deflate = {"Content-Encoding", "deflate"};
    
    /* Start response: %u */
    s->sentcb = _index_chunk_sent;
    err = httpd_response_start(s, HTTPSTATUS_OK, &deflate, 1, 
            HTTPHEADER_CONTENTTYPE_HTML, remain, HTTPD_FLAG_NONE);
    if (err) {
        goto reterr;
    }

    chunklen = MIN(remain, SEC_SIZE - offset);
    err = httpd_send(s, tmp + offset, chunklen);
    if (err) {
        goto reterr;
    }
    remain -= chunklen;
    struct fileserve *f = os_zalloc(sizeof(struct fileserve));            
    f->remain = remain;
    f->sect = sect;
    s->reverse = f;

retok:
    os_free(tmp);
    return HTTPD_OK;

reterr:
    os_free(tmp);
    return err;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_index_post(struct httpd_session *s) {
    httpd_err_t err;
    size16_t avail = HTTPD_REQ_LEN(s);
    size16_t sect;
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    uint8_t offset = sizeof(uint32_t);
    size32_t chunklen;
    size32_t wlen;
     
    WDTCHECK(s);
    if ((avail < (SEC_SIZE - offset)) && more) {
        /* More */
        s->request.handlercalls--;
        return HTTPD_MORE;
    }
   
    char *tmp = os_zalloc(SEC_SIZE);
    /* initialize, wc: %u */
    if (s->request.handlercalls == 1) {
        os_memcpy(tmp, &s->request.contentlen, offset);
    }

    sect = INDEXHTML_SECTOR;

    while (true) {
        chunklen = MIN(SEC_SIZE - offset, avail);
        INFO("sector: "SECTFMT" chunklen: %04d More: %07d ", sect, chunklen, 
                more);
        HTTPD_RECV(s, tmp + offset, chunklen);
    
        /* Erase sector:  */
        spi_flash_erase_protect_enable();
        if (!spi_flash_erase_protect_disable()) {
            err = WEBADMIN_ERR_FLASHWRPROTECT;
            ERROR("Cannot spi_flash_erase_protect_disable(void)");
            goto reterr;
        }
        err = spi_flash_erase_sector(INDEXHTML_SECTOR + sect);
        if (err) {
            ERROR("Erase sector "SECTFMT" error: %d: ", sect, err);
            goto reterr;
        }

        wlen = chunklen + offset;
        if (wlen % 4) {
            wlen += 4 - (wlen % 4);
        }
        /* Write sector:  */
        
        err = spi_flash_write(sect * SEC_SIZE, (uint32_t*)tmp, wlen);
        if (err) {
            goto reterr;
        }
        
        avail = HTTPD_REQ_LEN(s);
        if ((!avail) && (!more)) {
            goto retok;
        }
        offset = 0;
        sect++; 
    }; 
    
    if (more) {
        /* Unhold */
        if(!HTTPD_SCHEDULE(HTTPD_SIG_RECVUNHOLD, s)) {
            err = HTTPD_ERR_TASKQ_FULL;
            goto reterr;
        }
    }
    goto retmore;

retok:
    /* Terminating. */
    WDTCHECK(s);
    os_free(tmp);
    return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, NULL, 0);

retmore:
    os_free(tmp);
    return HTTPD_MORE;

reterr:
    os_free(tmp);
    return err;
}


static ICACHE_FLASH_ATTR
void _toggleboot() {
    system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
    system_upgrade_reboot();
}


static ICACHE_FLASH_ATTR
httpd_err_t _firmware_write(struct httpd_session *s, size16_t len) {
    char tmp[SPI_FLASH_SEC_SIZE];
    if (!len) {
        return HTTPD_OK;
    }
    system_upgrade_erase_flash(SPI_FLASH_SEC_SIZE);
    /* Reading */
    system_soft_wdt_feed();
    HTTPD_RECV(s, tmp, len);
    system_upgrade(tmp, len);
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_firmware_upgrade(struct httpd_session *s) {
    httpd_err_t err;
    size16_t avail = HTTPD_REQ_LEN(s);
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    size32_t chunk;
     
    /* initialize */
    if (s->req_rb.writecounter == 0) {
        system_upgrade_init();
        system_upgrade_flag_set(UPGRADE_FLAG_START);
    }
    
    if ((avail < SPI_FLASH_SEC_SIZE) && more) {
        return HTTPD_MORE;
    }
    
    do {
        if ((!avail) && (!more)) {
            /* Terminating. */
            status_update(200, 200, 5, _toggleboot);
            return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, "Rebooting"CR, 11);
        }

        /* Write */
        chunk = MIN(SPI_FLASH_SEC_SIZE, avail);
        INFO("FW: %04d More: %07d ", chunk, more);
        system_soft_wdt_feed();
        err = _firmware_write(s, chunk);
        if (err) {
            return err;
        }
        
        avail = HTTPD_REQ_LEN(s);
    } while((avail >= SPI_FLASH_SEC_SIZE) || (!more));
    
    if (more) {
        /* Unhold */
        if(!HTTPD_SCHEDULE(HTTPD_SIG_RECVUNHOLD, s)) {
            return HTTPD_ERR_TASKQ_FULL;
        }
    }
    return HTTPD_MORE;
}

static ICACHE_FLASH_ATTR
void discovercb(struct unsrecord *rec, void *arg) {
    struct httpd_session *s = (struct httpd_session *) arg;
    int bufflen = os_sprintf(buff, "%s "IPSTR"\n", rec->fullname, 
            IP2STR(&rec->address));
    HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_uns_discover(struct httpd_session *s) {
    char *pattern = rindex(s->request.path, '/') + 1;
    return uns_discover(pattern, discovercb, s);
}


static ICACHE_FLASH_ATTR
httpd_err_t _params_cb(struct httpd_session *s, const char *field, 
        const char *value) {
    char *target;
    /* Compare: %s */
    system_soft_wdt_feed();
    if (os_strcmp(field, "zone") == 0) {
        target = params->zone;
    }
    else if (os_strcmp(field, "name") == 0) {
        target = params->name;
    }
    else if (os_strcmp(field, "ap_psk") == 0) {
        target = params->ap_psk;
    }
    else if (os_strcmp(field, "ssid") == 0) {
        target = params->station_ssid;
    }
    else if (os_strcmp(field, "psk") == 0) {
        target = params->station_psk;
    }
    else {
        return WEBADMIN_UNKNOWNFIELD;;
    }

    if (value == NULL) {
        value = "";
    }
    /* Copy */
    
    //INFO("Updating params: %s", field);
    os_strcpy(target, value);
    /* After Copy */
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_params_post(struct httpd_session *s) {
    httpd_err_t err;
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    if (more) {
        return HTTPD_OK;
    }
   
    /* parse */
    err = httpd_form_urlencoded_parse(s, _params_cb);
    if (err) {
        return err;
    }
    if (!params_save(params)) {
        return WEBADMIN_ERR_SAVEPARAMS;
    }

    bufflen = os_sprintf(buff, 
            "Params has been saved, Rebooting in 4 seconds."CR);
    err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    if (err) {
        return err;
    }
    INFO("Rebooting...");
    status_update(500, 500, 1, system_restart);
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_params_get(struct httpd_session *s) {
#define PARAMS_JSON "{" \
    "\"zone\": \"%s\"," \
    "\"name\": \"%s\"," \
    "\"ap_psk\": \"%s\"," \
    "\"ssid\": \"%s\"," \
    "\"psk\": \"%s\"" \
    "}"


    bufflen = os_sprintf(buff, PARAMS_JSON, 
            params->zone, 
            params->name, 
            params->ap_psk, 
            params->station_ssid, 
            params->station_psk);
    return HTTPD_RESPONSE_JSON(s, HTTPSTATUS_OK, buff, bufflen);
}


//static ICACHE_FLASH_ATTR
//httpd_err_t webadmin_favicon(struct httpd_session *s) {
//    #define FAVICON_SIZE    495
//
//    #if SPI_SIZE_MAP == 2
//    #define FAVICON_FLASH_SECTOR    0x77    
//    #elif SPI_SIZE_MAP == 4
//    #define FAVICON_FLASH_SECTOR    0x200    
//    #elif SPI_SIZE_MAP == 6
//    #define FAVICON_FLASH_SECTOR    0x200    
//    #endif
//   
//
//    char buf[4 * 124];
//    //os_memset(buff, 0, 4 * 124);
//    int result = spi_flash_read(
//            FAVICON_FLASH_SECTOR * SPI_FLASH_SEC_SIZE,
//            (uint32_t*) buf,
//            4 * 124
//        );
//    
//    if (result != SPI_FLASH_RESULT_OK) {
//        ERROR("SPI Flash write failed: %d", result);
//        return WEBADMIN_ERR_FLASHREAD;
//    }
//    return HTTPD_RESPONSE_ICON(s, HTTPSTATUS_OK, buf, FAVICON_SIZE);
//}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_toggle_boot(struct httpd_session *s) {
    httpd_err_t err;
    uint8_t image = system_upgrade_userbin_check();
    bufflen = os_sprintf(buff, "Rebooting to user%d mode..."CR, image + 1);
    err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    if (err) {
        return err;
    }
    status_update(500, 500, 1, _toggleboot);
    return HTTPD_OK;
}


static
void httpcb(int status, char *body, void *arg) {
    struct httpd_session *s = (struct httpd_session *) arg;
    httpd_err_t err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, body, strlen(body));
    if (err) {
        httpd_tcp_print_err(err);
    }
}

#define SYSINFO \
    "Image:      %s"CR \
    "Boot:       user%d"CR \
    "Version:    %s"CR \
    "Uptime:     %u"CR \
    "Free mem:   %d"CR \
    "RTC:        %u"CR


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_sysinfo(struct httpd_session *s) {
    if (strlen(s->request.path) <= 1) {
        uint8_t image = system_upgrade_userbin_check();
        bufflen = os_sprintf(buff, SYSINFO, 
            __name__,
            image + 1,
            __version__,
            system_get_time(),
            system_get_free_heap_size(),
            system_get_rtc_time()
        );
        return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    }
    
    char *pattern = rindex(s->request.path, '/');
    pattern++;
    DEBUG("Trying UNS for: %s\n", pattern);
    http_nobody_uns(pattern, "INFO", "/", httpcb, s);
}


//static ICACHE_FLASH_ATTR
//httpd_err_t webadmin_index_get(struct httpd_session *s) {
//    WDTCHECK(s);
//    status_update(50, 100, 10, NULL);
//    bufflen = os_sprintf(buff, HTML_INDEX, params->name);
//    return HTTPD_RESPONSE_HTML(s, HTTPSTATUS_OK, buff, bufflen);
//}


static struct httpd_route routes[] = {
    /* Upgrade firmware over the air (wifi) */
    {"UPGRADE",    "/firmware",           webadmin_firmware_upgrade  },

    /* Feel free to change these handlers */
    {"DISCOVER",   "/uns",                webadmin_uns_discover      },
    {"POST",       "/params",             webadmin_params_post       },
    {"GET",        "/params.json",        webadmin_params_get        },
//    {"GET",        "/favicon.ico",        webadmin_favicon           },
    {"TOGGLE",     "/boots",              webadmin_toggle_boot       },
    {"INFO",       "/",                   webadmin_sysinfo           },
    {"GET",        "/",                   webadmin_index_get         },
    {"POST",       "/",                   webadmin_index_post        },
    { NULL }
};


ICACHE_FLASH_ATTR
int webadmin_start(struct params *_params) {
    err_t err;
    params = _params;
    err = httpd_init(routes);
    if (err) {
        ERROR("Cannot init httpd: %d", err);
    }
    return OK;
}


ICACHE_FLASH_ATTR
void webadmin_stop() {
    httpd_deinit();
}

