#include "params.h"
//#include "fotaweb.h"
#include "status.h"
#include "webadmin.h"
#include "httpd.h"
#include "uns.h"
#include "http.h"

#include <upgrade.h>
#include <osapi.h>
#include <mem.h>



#define HTML_HEADER \
    "<!DOCTYPE html><html>" \
    "<head><title>ESP8266 Firstboot config</title></head><body>\r\n" 

#define HTML_FOOTER "\r\n</body></html>\r\n"

#define HTML_INDEX \
    HTML_HEADER \
    "<h4>Welcome to %s Web Administration</h4><br />" \
    "<a href=\"/params\">Params</a><br />" \
    HTML_FOOTER

#define HTML_FORM \
    HTML_HEADER \
    "<form action=\"/params\" method=\"post\">" \
    "<h4>Settings</h4>" \
    "zone: <input name=\"zone\" value=\"%s\"/><br/>" \
    "name: <input name=\"name\" value=\"%s\"/><br/>" \
    "AP PSK: <input name=\"ap_psk\" value=\"%s\"/><br/>" \
    "SSID: <input name=\"ssid\" value=\"%s\"/><br/>" \
    "PSK: <input name=\"psk\" value=\"%s\"/><br/>" \
    "<input type=\"submit\" value=\"Reboot\" />" \
    "</form>" \
    HTML_FOOTER



//void discovercb(struct unsrecord *rec, void *arg) {
//    char buffer[128];
//    struct httpd_session *s = (struct httpd_session *) arg;
//    int len = os_sprintf(buffer, "%s "IPSTR"\n", rec->fullname, 
//            IP2STR(&rec->address));
//    httpd_response_text(s, HTTPSTATUS_OK, buffer, len);
//}
//
//
//static ICACHE_FLASH_ATTR
//void webadmin_uns_discover(struct httpd_session *s, char *body, 
//        uint32_t bodylen, uint32_t more) {
//    char *pattern = rindex(s->path, '/') + 1;
//    uns_discover(pattern, discovercb, s);
//}


#define FAVICON_SIZE    495

#if SPI_SIZE_MAP == 2
#define FAVICON_FLASH_SECTOR    0x77    
#elif SPI_SIZE_MAP == 6
#define FAVICON_FLASH_SECTOR    0x200    
#endif


#define WEBADMIN_ERR_FLASHREAD   -100
#define WEBADMIN_ERR_SAVEPARAMS  -101
#define WEBADMIN_UNKNOWNFIELD    -102

#define WEBADMIN_BUFFSIZE   1024

static Params *params;
static char buff[WEBADMIN_BUFFSIZE];
static size16_t bufflen;


static ICACHE_FLASH_ATTR
httpd_err_t _params_cb(struct httpd_session *s, const char *field, 
        const char *value) {
    char *target;
    if (os_strcmp(field, "zone") == 0) {
        target = (char*)&params->zone;
    }
    else if (os_strcmp(field, "name") == 0) {
        target = (char*)&params->name;
    }
    else if (os_strcmp(field, "ap_psk") == 0) {
        target = (char*)&params->ap_psk;
    }
    else if (os_strcmp(field, "ssid") == 0) {
        target = (char*)&params->station_ssid;
    }
    else if (os_strcmp(field, "psk") == 0) {
        target = (char*)&params->station_psk;
    }
    else {
        return WEBADMIN_UNKNOWNFIELD;;
    }

    if (value == NULL) {
        value = "";
    }
    os_strcpy(target, value);
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_set_params(struct httpd_session *s) {
    httpd_err_t err;
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    if (more) {
        return HTTPD_OK;
    }
    
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
    INFO("Rebooting");
    status_update(500, 500, 2, system_restart);
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_params_get(struct httpd_session *s) {
    bufflen = os_sprintf(buff, HTML_FORM, 
            params->zone, 
            params->name, 
            params->ap_psk, 
            params->station_ssid, 
            params->station_psk);
    return HTTPD_RESPONSE_HTML(s, HTTPSTATUS_OK, buff, bufflen);
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_favicon(struct httpd_session *s) {
    char buf[4 * 124];
    //os_memset(buff, 0, 4 * 124);
    CHK("Reading flash");
    int result = spi_flash_read(
            FAVICON_FLASH_SECTOR * SPI_FLASH_SEC_SIZE,
            (uint32_t*) buf,
            4 * 124
        );
    
    if (result != SPI_FLASH_RESULT_OK) {
        ERROR("SPI Flash write failed: %d", result);
        return WEBADMIN_ERR_FLASHREAD;
    }
    CHK("Response");
    return HTTPD_RESPONSE_ICON(s, HTTPSTATUS_OK, buf, FAVICON_SIZE);
}


static ICACHE_FLASH_ATTR
void reboot_fotamode() {
    system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
    system_upgrade_reboot();
}


static ICACHE_FLASH_ATTR
httpd_err_t app_reboot(struct httpd_session *s) {
    httpd_err_t err;
    uint8_t image = system_upgrade_userbin_check();
    bufflen = os_sprintf(buff, "Rebooting to %s mode...\r\n",
        image == UPGRADE_FW_BIN1? "app": "FOTA");
    err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    if (err) {
        return err;
    }
    status_update(500, 500, 1, reboot_fotamode);
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


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_sysinfo(struct httpd_session *s, char *body, 
        uint32_t body_length, uint32_t more) {
    if (strlen(s->request.path) <= 1) {
        bufflen = os_sprintf(buff, "%d Free mem: %d.\n", 
            system_get_time() / 1000000,
            system_get_free_heap_size()
        );
        return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    }
    
    char *pattern = rindex(s->request.path, '/');
    pattern++;
    DEBUG("Trying UNS for: %s\n", pattern);
    http_nobody_uns(pattern, "INFO", "/", httpcb, s);
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_index(struct httpd_session *s) {
    status_update(50, 100, 10, NULL);
    bufflen = os_sprintf(buff, HTML_INDEX, params->name);
    return HTTPD_RESPONSE_HTML(s, HTTPSTATUS_OK, buff, bufflen);
}


static struct httpd_route routes[] = {
    //{"DISCOVER", "/uns",             webadmin_uns_discover           },
    //{"POST",     "/firmware",        fotaweb_upgrade_firmware        },
    {"POST",     "/params",          webadmin_set_params             },
    {"GET",      "/params",          webadmin_params_get             },
    {"GET",      "/favicon.ico",     webadmin_favicon                },
    {"APP",      "/",                app_reboot                      },
    {"INFO",     "/",                webadmin_sysinfo                },
    {"GET",      "/",                webadmin_index                  },
    { NULL }
};


ICACHE_FLASH_ATTR
int webadmin_start(Params *_params) {
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

