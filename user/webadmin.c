
#define HTML_HEADER \
	"<!DOCTYPE html><html>" \
	"<head><title>ESP8266 Firstboot config</title></head><body>\r\n" 
#define HTML_FOOTER "\r\n</body></html>\r\n"
#define HTML_FORM \
	HTML_HEADER \
	"<form action=\"/params\" method=\"post\">" \
	"<h4>Settings</h4>" \
	"name: <input name=\"name\" value=\"%s\"/><br/>" \
	"AP PSK: <input name=\"ap_psk\" value=\"%s\"/><br/>" \
	"SSID: <input name=\"ssid\" value=\"%s\"/><br/>" \
	"PSK: <input name=\"psk\" value=\"%s\"/><br/>" \
	"<input type=\"submit\" value=\"Reboot\" />" \
	"</form>" \
	"<h4>Firmware</h4>" \
	"<form method=\"upgrade\">" \
	"<input name=\"firmware\" type=\"file\"/><br/>" \
	"<input type=\"submit\" value=\"Upgrade\" />" \
	"</form>" \
	HTML_FOOTER
//static void ICACHE_FLASH_ATTR
//fb_update_params_field(Params *out, const char *field, const char *value) {
//	INFO("Updating Field: %s with value: %s\r\n", field, value);
//	char *target;
//	if (os_strcmp(field, "name") == 0) {
//		target = (char*)&out->device_name;
//	}
//	else if (os_strcmp(field, "ap_psk") == 0) {
//		target = (char*)&out->ap_psk;
//	}
//	else if (os_strcmp(field, "ssid") == 0) {
//		target = (char*)&out->station_ssid;
//	}
//	else if (os_strcmp(field, "psk") == 0) {
//		target = (char*)&out->station_psk;
//	}
//	else return;
//	os_strcpy(target, value);
//}
//static void ICACHE_FLASH_ATTR
//fb_parse_form(const char *form, Params *out) {
//	char *field = (char*)&form[0];
//	char *value;
//	char *tmp;
//
//	while (true) {
//		value = os_strstr(field, "=") + 1;
//		(value-1)[0] = 0;
//		tmp  = os_strstr(value, "&");
//		if (tmp != NULL) {
//			tmp[0] = 0;
//		}
//		fb_update_params_field(out, field, value);
//		if (tmp == NULL) {
//			return;
//		}
//		field = tmp + 1;
//	}
//}
#include "params.h"
#include "httpserver.h"

#include <osapi.h>


static Params *params;


static ICACHE_FLASH_ATTR
void webadmin_index(Request *req, char *body, uint32_t body_length, 
		uint32_t more) {
	os_printf("Handling: %s %s\r\n", req->verb, req->path);
	httpserver_response_text(HTTPSTATUS_OK, 7, "Hello\r\n");
}


static ICACHE_FLASH_ATTR
void webadmin_get_params(Request *req, char *body, uint32_t body_length, 
		uint32_t more) {
	os_printf("Handling: %s %s\r\n", req->verb, req->path);

	//char *buffer = (char*) os_zalloc(1024);
	//Params params;
	//if (!params_load(&params)) {
	//	params.station_ssid[0] = 0;
	//}
	char buffer[1024];
	int len = os_sprintf(buffer, HTML_FORM, 
			params->device_name, 
			params->ap_psk, 
			params->station_ssid, 
			params->station_psk);
	httpserver_response_text(HTTPSTATUS_OK, len, buffer);
    //os_free(buffer);

}


static HttpServer httpserver;
static HttpRoute routes[] = {
	{"GET", "/params", webadmin_get_params},
	{"GET", "/", webadmin_index},
	NULL
};


int ICACHE_FLASH_ATTR
webadmin_start(Params *_params) {
	params = _params;
	httpserver.hostname = params->device_name;
	httpserver.routes_length = 2;
	httpserver.routes = routes;
	httpserver_init(&httpserver);
	return OK;
}


void ICACHE_FLASH_ATTR
webadmin_stop() {
	httpserver_stop();
}



