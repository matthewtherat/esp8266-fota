#include <user_interface.h>
#include <osapi.h>
#include <espconn.h>
#include <mem.h>
#include <ets_sys.h>
#include <c_types.h>
#include <os_type.h>

#include "debug.h"
#include "wifi.h"
#include "firstboot.h"
#include "params.h"


#define FB_RESPONSE_HEADER_FORMAT \
	"HTTP/1.0 200 OK\r\n"\
	"Content-Length: %d\r\n"\
	"Server: lwIP/1.4.0\r\n"\
	"Content-type: text/html\r\n"\
	"Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n"\
	"Pragma: no-cache\r\n\r\n"

#define FB_BAD_REQUEST_FORMAT \
	"HTTP/1.0 400 BadRequest\r\n"\
	"Content-Length: 0\r\n"\
	"Server: lwIP/1.4.0\r\n"

#define HTML_HEADER \
	"<!DOCTYPE html><html>" \
	"<head><title>ESP8266 Firstboot config</title></head><body>\r\n" 
#define HTML_FOOTER "\r\n</body></html>\r\n"
#define HTML_FORM \
	HTML_HEADER \
	"<form method=\"post\">" \
	"name: <input name=\"name\" value=\"%s\"/><br/>" \
	"AP PSK: <input name=\"ap_psk\" value=\"%s\"/><br/>" \
	"SSID: <input name=\"ssid\" value=\"%s\"/><br/>" \
	"PSK: <input name=\"psk\" value=\"%s\"/><br/>" \
	"<input type=\"submit\" value=\"Reboot\" />" \
	"</form>" \
	HTML_FOOTER


static struct espconn esp_conn;
static esp_tcp esptcp;


static void ICACHE_FLASH_ATTR
send_response(bool ok, const char *response_buffer) {
	uint16_t total_length = 0;
	uint16_t head_length = 0;
    char *send_buffer = NULL;
    char httphead[256];
    os_memset(httphead, 0, 256);
	uint16_t response_length = (ok && response_buffer != NULL) ? \
		os_strlen(response_buffer): 0;

	os_sprintf(
			httphead, 
			ok? FB_RESPONSE_HEADER_FORMAT: FB_BAD_REQUEST_FORMAT, 
			response_length
		);
	head_length = os_strlen(httphead);	
    total_length = head_length + response_length;
    send_buffer = (char *)os_zalloc(total_length + 1);
	// Write head
    os_memcpy(send_buffer, httphead, head_length);

	// Body
    if (response_length > 0) {
        os_memcpy(send_buffer+head_length, response_buffer, response_length);
    }

	espconn_sent(&esp_conn, send_buffer, total_length);
    os_free(send_buffer);
}


static void ICACHE_FLASH_ATTR
fb_serve_form() {
	char *buffer = (char*) os_zalloc(1024);
	Params params;
	if (!params_load(&params)) {
		params.station_ssid[0] = 0;
	}
	os_sprintf(buffer, HTML_FORM, params.device_name, params.ap_psk, 
			params.station_ssid, params.station_psk);
	send_response(true, buffer);	
    os_free(buffer);
}


static void ICACHE_FLASH_ATTR
fb_update_params_field(Params *out, const char *field, const char *value) {
	INFO("Updating Field: %s with value: %s\r\n", field, value);
	char *target;
	if (os_strcmp(field, "name") == 0) {
		target = (char*)&out->device_name;
	}
	else if (os_strcmp(field, "ap_psk") == 0) {
		target = (char*)&out->ap_psk;
	}
	else if (os_strcmp(field, "ssid") == 0) {
		target = (char*)&out->station_ssid;
	}
	else if (os_strcmp(field, "psk") == 0) {
		target = (char*)&out->station_psk;
	}
	else return;
	os_strcpy(target, value);
}


static void ICACHE_FLASH_ATTR
fb_parse_form(const char *form, Params *out) {
	char *field = (char*)&form[0];
	char *value;
	char *tmp;

	while (true) {
		value = os_strstr(field, "=") + 1;
		(value-1)[0] = 0;
		tmp  = os_strstr(value, "&");
		if (tmp != NULL) {
			tmp[0] = 0;
		}
		fb_update_params_field(out, field, value);
		if (tmp == NULL) {
			return;
		}
		field = tmp + 1;
	}
}


static Error ICACHE_FLASH_ATTR
fb_parse_request(char *data, uint16_t length, Request *req) {
	char *cursor;
	if (os_strncmp(data, "GET", 3) == 0) {
		req->verb = GET;
		req->body_length = 0;
		req->body =  NULL; 	
		return OK;
	}
	
	if (os_strncmp(data, "POST", 4) == 0) {
		req->verb = POST;
		req->body = (char*)os_strstr(data, "\r\n\r\n");
		if (req->body == NULL) {
			goto error;
		}
		req->body += 4;
		req->body_length = length - (req->body - data);	
		return OK;
	}

error:
	req->body_length = 0;
	send_response(false, NULL);
	return 1;

}


static void ICACHE_FLASH_ATTR
fb_webserver_recv(void *arg, char *data, uint16_t length) {
	Request req;
	if(OK != fb_parse_request(data, length, &req)) {
		return;
	}

	os_printf("--> Verb: %s Length: %d Body: %s\r\n", 
			req.verb == 0 ? "GET" : "POST", 
			req.body_length, 
			req.body
	);
	if (req.verb == GET) {
		fb_serve_form();
	}
	else {
		Params params;
		fb_parse_form(req.body, &params);
		if(!params_save(&params)) {
			ERROR("Cannot save params\r\n");
			send_response(false, "Error Saving parameters");
		}
		
		send_response(true, "Update Successfull, Rebooting...");
		system_restart();
	}
}


static ICACHE_FLASH_ATTR
void fb_webserver_recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = arg;
    os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", 
			pesp_conn->proto.tcp->remote_ip[0],
    		pesp_conn->proto.tcp->remote_ip[1],
			pesp_conn->proto.tcp->remote_ip[2],
    		pesp_conn->proto.tcp->remote_ip[3],
			pesp_conn->proto.tcp->remote_port, 
			err
	);
}


static ICACHE_FLASH_ATTR
void fb_webserver_disconnected(void *arg)
{
    struct espconn *pesp_conn = arg;

    os_printf("webserver's %d.%d.%d.%d:%d disconnect\n", 
			pesp_conn->proto.tcp->remote_ip[0],
        	pesp_conn->proto.tcp->remote_ip[1],
			pesp_conn->proto.tcp->remote_ip[2],
        	pesp_conn->proto.tcp->remote_ip[3],
			pesp_conn->proto.tcp->remote_port
	);
}


static ICACHE_FLASH_ATTR
void fb_webserver_connected(void *arg)
{
    struct espconn *pesp_conn = arg;
    espconn_regist_recvcb(pesp_conn, fb_webserver_recv);
    espconn_regist_reconcb(pesp_conn, fb_webserver_recon);
    espconn_regist_disconcb(pesp_conn, fb_webserver_disconnected);

}



void ICACHE_FLASH_ATTR
fb_start() {
    esp_conn.type = ESPCONN_TCP;
    esp_conn.state = ESPCONN_NONE;
    esp_conn.proto.tcp = &esptcp;
    esp_conn.proto.tcp->local_port = 80;
    os_printf("First boot webserver is listening on: %d.%d.%d.%d:%d\n", 
			esp_conn.proto.tcp->local_ip[0],
        	esp_conn.proto.tcp->local_ip[1],
			esp_conn.proto.tcp->local_ip[2],
        	esp_conn.proto.tcp->local_ip[3],
			esp_conn.proto.tcp->local_port
	);

    espconn_regist_connectcb(&esp_conn, fb_webserver_connected);
    espconn_accept(&esp_conn);
}


void ICACHE_FLASH_ATTR
fb_stop() {
	espconn_disconnect(&esp_conn);
	espconn_delete(&esp_conn);
}


static struct mdns_info info;

void ICACHE_FLASH_ATTR
fb_mdns_init(Params *params) {

	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);
	info.host_name = params->device_name;
	info.ipAddr = ipconfig.ip.addr; //ESP8266 Station IP
	info.server_name = "firstboot";
	info.server_port = 80;
	info.txt_data[0] = "version = 1.0";
	//info->txt_data[1] = "user1 = data1";
	//info->txt_data[2] = "user2 = data2";
	espconn_mdns_init(&info);
}



