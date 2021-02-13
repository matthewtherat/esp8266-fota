#include "params.h"
#include "multipart.h"
#include "fota.h"
#include "querystring.h"
#include "status.h"
#include "debug.h"
#include "webadmin.h"
#include "httpd.h"

#include <upgrade.h>
#include <osapi.h>
#include <mem.h>


#define FAVICON_SIZE	495

#if SPI_SIZE_MAP == 2
#define FAVICON_FLASH_SECTOR	0x77	
#elif SPI_SIZE_MAP == 6
#define FAVICON_FLASH_SECTOR	0x200	
#endif

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
	"<h4>Firmware</h4>" \
	"<form action=\"/firmware\" method=\"post\" " \
	"enctype=\"multipart/form-data\">" \
	"<input name=\"firmware\" type=\"file\"/><br/>" \
	"<input type=\"submit\" value=\"Upgrade\" />" \
	"</form>" \
	HTML_FOOTER

static Params *params;
static ETSTimer ff;

#define BUFFSIZE	2048

static Multipart mp;
static char buff[BUFFSIZE];
static RingBuffer rb = {BUFFSIZE, 0, 0, buff};


#define IP_FMT    "%d.%d.%d.%d"
#define IPPORT_FMT    IP_FMT":%d"
#define unpack_ip(ip) ip[0], ip[1], ip[2], ip[3]
#define lclinfo(t) unpack_ip((t)->local_ip), (t)->local_port
#define rmtinfo(t) unpack_ip((t)->remote_ip), (t)->remote_port


void discovercb(char *hostname, int hlen, remot_info* remoteinfo) {
    os_printf("D: %s "IP_FMT"\n", hostname, rmtinfo(remoteinfo));
}

static ICACHE_FLASH_ATTR
void webadmin_sysinfo(struct httprequest *req, char *body, 
        uint32_t body_length, uint32_t more) {
	char buffer[1024];
    uns_discover(params->zone, "foo", discovercb);
    int len = os_sprintf(buffer, "Free mem: %d.\n", 
            system_get_free_heap_size());
	httpd_response_text(req, HTTPSTATUS_OK, buffer, len);
}


static ICACHE_FLASH_ATTR
void webadmin_uns_discover(struct httprequest *req, char *body, 
        uint32_t body_length, uint32_t more) {
	char buffer[1024];
    uns_discover(params->zone, "foo", discovercb);
    int len = os_sprintf(buffer, "Discover sent.\n");
	httpd_response_text(req, HTTPSTATUS_OK, buffer, len);
}


static ICACHE_FLASH_ATTR
void app_reboot(struct httprequest *req, char *body, uint32_t body_length, 
		uint32_t more) {
	char buffer[256];
	int len = os_sprintf(buffer, "Rebooting to app mode...\r\n");
	httpd_response_text(req, HTTPSTATUS_OK, buffer, len);
    os_delay_us(2000);
	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
	system_upgrade_reboot();
}


void ff_func(void *arg) {
    params->apploaded = 1;
	if (!params_save(params)) {
        ERROR("Cannot save apploaded flag paramter.\r\n"); 
        return;
    }
    os_delay_us(1000);
	fota_finalize();
}


void _mp_callback(MultipartField *f, char *body, Size bodylen, 
		bool last) {
	if (os_strncmp(f->name, "firmware", 8) != 0) {
		return;
	}
	fota_feed(body, bodylen, last);
	//os_printf("total: %d, Chunk len: %d last: %d\r\n", ll, bodylen, last);
	//if (last) {
	//	//espconn_recv_unhold(request->conn);
	//	httpd_response_text(request, HTTPSTATUS_OK, "Done", 4);
	//	fota_finalize();
	//}

}


static ICACHE_FLASH_ATTR
void webadmin_upgrade_firmware(struct httprequest *req, char *body, 
        uint32_t body_length, uint32_t more) {

	int err;
	if (body_length <= 0) {
		return;
	}
	
	if (mp.status == MP_IDLE) {
		err = mp_init(&mp, req->contenttype, _mp_callback);
		if (err != MP_OK) {
			os_printf("Cannot init multipart: %d\r\n", err);
			goto badrequest;
		}
		rb_reset(&rb);
		fota_init();
        status_update(1000, 1000, INFINITE, NULL);
	}
	
	espconn_recv_hold(req->conn);
	if ((err = rb_safepush(&rb, body, body_length)) == RB_FULL) {
		goto badrequest;
	}

    err = mp_feedbybuffer(&mp, &rb);
	espconn_recv_unhold(req->conn);
	switch (err) {
		case MP_DONE:
			goto done;

		case MP_MORE:
			return;

		default:
			goto badrequest;
	}

done:
	mp_close(&mp);
	httpd_response_text(req, HTTPSTATUS_OK, "Rebooting...", 12);
	os_timer_disarm(&ff);
	os_timer_setfn(&ff, (os_timer_func_t *)ff_func, NULL);
	os_timer_arm(&ff, 2000, 0);
	return;

badrequest:
	mp_close(&mp);
    status_update(100, 100, 3, NULL);
	httpd_response_notok(req, HTTPSTATUS_BADREQUEST);
}


static ICACHE_FLASH_ATTR
void _update_params_field(const char *field, const char *value) {

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
	else return;
	os_strcpy(target, value);
}


static ICACHE_FLASH_ATTR
void webadmin_get_params(struct httprequest *req, char *body, 
        uint32_t body_length, uint32_t more) {

	char buffer[1024];
	int len = os_sprintf(buffer, HTML_FORM, 
			params->zone, 
			params->name, 
			params->ap_psk, 
			params->station_ssid, 
			params->station_psk);
	httpd_response_html(req, HTTPSTATUS_OK, buffer, len);
}


static ICACHE_FLASH_ATTR
void webadmin_set_params(struct httprequest *req, char *body, 
        uint32_t body_length, uint32_t more) {

	body[body_length] = 0;
	querystring_parse(body, _update_params_field);  
	if (!params_save(params)) {
		httpd_response_notok(req, HTTPSTATUS_SERVERERROR);
		return;
	}
	system_restart();
}


static ICACHE_FLASH_ATTR
void webadmin_favicon(struct httprequest *req, char *body, uint32_t body_length, 
		uint32_t more) {
	
	char buffer[4 * 124];
	int result = spi_flash_read(
			FAVICON_FLASH_SECTOR * SPI_FLASH_SEC_SIZE,
			(uint32_t*) buffer,
			4 * 124
		);
	if (result != SPI_FLASH_RESULT_OK) {
		os_printf("SPI Flash write failed: %d\r\n", result);
		httpd_response_notok(req, HTTPSTATUS_SERVERERROR);
		return;
	}
	httpd_response(req, HTTPSTATUS_OK, "image/x-icon", buffer, 495, 
            NULL, 0);
}


static ICACHE_FLASH_ATTR
void webadmin_index(struct httprequest *req, char *body, uint32_t body_length, 
		uint32_t more) {
	char buffer[1024];
	int len = os_sprintf(buffer, HTML_INDEX, params->name);
	httpd_response_html(req, HTTPSTATUS_OK, buffer, len);
}


static struct httproute routes[] = {
	{"DISCOVER","/uns",		        webadmin_uns_discover   		},
	{"POST",	"/firmware",		webadmin_upgrade_firmware		},
	{"POST", 	"/params",			webadmin_set_params				},
	{"GET",  	"/params",			webadmin_get_params				},
	{"GET",  	"/favicon.ico",		webadmin_favicon				},
	{"APP",     "/",                app_reboot                      },
	{"INFO",    "/",                webadmin_sysinfo                },
	{"GET",  	"/",				webadmin_index					},
	{ NULL }
};


static struct httpd httpd;


ICACHE_FLASH_ATTR
int webadmin_start(Params *_params) {
    err_t err;
	params = _params;
	err = httpd_init(&httpd, routes);
    if (err) {
        ERROR("Cannot init httpd: %d\r\n", err);
    }
	return OK;
}


ICACHE_FLASH_ATTR
void webadmin_stop() {
	httpd_stop(&httpd);
}

