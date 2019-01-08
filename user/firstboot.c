#include <user_interface.h>
#include <osapi.h>
#include <mem.h>
#include <ets_sys.h>
#include <c_types.h>
#include <os_type.h>
#include <espconn.h>

#include "debug.h"
#include "wifi.h"
#include "firstboot.h"


#define FB_RESPONSE_HEADER_FORMAT \
	"HTTP/1.0 200 OK\r\n"\
	"Content-Length: %d\r\n"\
	"Server: lwIP/1.4.0\r\n"

#define FB_CONTENT_LENGTH_FORMAT \
	"Content-type: application/json\r\n"\
	"Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n"\
	"Pragma: no-cache\r\n\r\n"

#define FB_BAD_REQUEST_FORMAT \
	"HTTP/1.0 400 BadRequest\r\n"\
	"Content-Length: 0\r\n"\
	"Server: lwIP/1.4.0\r\n\n"

/*
GET / HTTP/1.1
Host: 192.168.43.1
Connection: keep-alive
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
Save-Data: on
User-Agent: Mozilla/5.0 (Linux; Android 8.0.0; SM-G955F) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.99 Mobile Safari/537.36
Accept-Encoding: gzip, deflate
Accept-Language: en-US,en;q=0.9,fa;q=0.8,ru;q=0.7

*/

static void ICACHE_FLASH_ATTR
data_send(void *arg, bool responseOK, char *body) {
    uint16 length = 0;
    char *pbuf = NULL;
    char httphead[256];
    struct espconn *ptrespconn = arg;
    os_memset(httphead, 0, 256);

    if (responseOK) {
        os_sprintf(
				httphead, 
				FB_RESPONSE_HEADER_FORMAT,
                body ? os_strlen(body) : 0
			);

        if (body) {
            os_sprintf(
					httphead + os_strlen(httphead),
                    FB_CONTENT_LENGTH_FORMAT
				);
            length = os_strlen(httphead) + os_strlen(body);
            pbuf = (char *)os_zalloc(length + 1);
            os_memcpy(pbuf, httphead, os_strlen(httphead));
            os_memcpy(pbuf + os_strlen(httphead), body, os_strlen(body));
        } else {
            os_sprintf(httphead + os_strlen(httphead), "\n");
            length = os_strlen(httphead);
        }
    } else {
        os_sprintf(httphead, FB_BAD_REQUEST_FORMAT);
        length = os_strlen(httphead);
    }

    if (body) {
        espconn_sent(ptrespconn, pbuf, length);
    } else {
        espconn_sent(ptrespconn, httphead, length);
    }

    if (pbuf) {
        os_free(pbuf);
        pbuf = NULL;
    }
}

typedef enum httpverb {
	GET,
	POST
} HTTPVerb;

typedef struct request {
	HTTPVerb verb;
	char *body;
	uint32_t body_length;
} Request;

static void ICACHE_FLASH_ATTR
parse_request(char *data, uint32_t length, Request *req) {
}


static void ICACHE_FLASH_ATTR
fb_webserver_recv(void *arg, char *data, unsigned short length) {
	//os_printf("-%u-> %s\r\n", length, data);
	//Request *req = (Request*)zalloc(sizeof(Request));
	//parse_request(data, length, req);
	//os_strncpy(req, data, length);
	//os_printf("--> %s\r\n", length, data);
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
void fb_webserver_listen(void *arg)
{
    struct espconn *pesp_conn = arg;
    espconn_regist_recvcb(pesp_conn, fb_webserver_recv);
    espconn_regist_reconcb(pesp_conn, fb_webserver_recon);
    espconn_regist_disconcb(pesp_conn, fb_webserver_disconnected);
}


void ICACHE_FLASH_ATTR
fb_webserver_init(uint32 port)
{
    LOCAL struct espconn esp_conn;
    LOCAL esp_tcp esptcp;

    esp_conn.type = ESPCONN_TCP;
    esp_conn.state = ESPCONN_NONE;
    esp_conn.proto.tcp = &esptcp;
    esp_conn.proto.tcp->local_port = port;
    espconn_regist_connectcb(&esp_conn, fb_webserver_listen);
    espconn_accept(&esp_conn);
}


void ICACHE_FLASH_ATTR
fb_start() {
	uint8_t mac[6];

	/***add by tzx for saving ip_info to avoid dhcp_client start****/
    struct dhcp_client_info dhcp_info;
    struct ip_info sta_info;
    system_rtc_mem_read(64, &dhcp_info, sizeof(struct dhcp_client_info));
	if(dhcp_info.flag == 0x01 ) {
		if (true == wifi_station_dhcpc_status()) {
			wifi_station_dhcpc_stop();
		}
		sta_info.ip = dhcp_info.ip_addr;
		sta_info.gw = dhcp_info.gw;
		sta_info.netmask = dhcp_info.netmask;
		if ( true != wifi_set_ip_info(STATION_IF, &sta_info)) {
			os_printf("set default ip wrong\n");
		}
	}
    os_memset(&dhcp_info, 0, sizeof(struct dhcp_client_info));
    system_rtc_mem_write(64, &dhcp_info, sizeof(struct rst_info));


	// Get the device mac address
	bool ok = wifi_get_macaddr(SOFTAP_IF, &mac[0]);
	if (!ok) {
		ERROR("Cannot get softap macaddr\r\n");
	}

	// initialization
    struct softap_config *config = (struct softap_config *) \
			os_zalloc(sizeof(struct softap_config));

	// Get soft-AP config first.
    wifi_softap_get_config(config);     

	// Updating ssid and password
	os_sprintf(config->ssid, "esp8266_"MACSTR, MAC2STR(mac));
	INFO("SSID: %s\r\n", config->ssid);
    config->ssid_len = 0; 
    os_sprintf(config->password, FB_SOFTAP_PSK);
    config->authmode = AUTH_WPA_WPA2_PSK;
    config->max_connection = 4;
	config->channel = 5;	
	config->beacon_interval = 120;

	// Set ESP8266 soft-AP config
    ok = wifi_softap_set_config(config); 
    os_free(config);
	if (!ok) {
		ERROR("Cannot set softap config\r\n");
		return;
	}
    wifi_set_opmode(SOFTAP_MODE);

    struct station_info * station = wifi_softap_get_station_info();
    while (station) {
        os_printf("bssid : MACSTR, ip : IPSTR/n", MAC2STR(station->bssid), 
				IP2STR(&station->ip));
        station = STAILQ_NEXT(station, next);
    }

	// Free it by calling functionss
    wifi_softap_free_station_info(); 
    wifi_softap_dhcps_stop(); // disable soft-AP DHCP server
    struct ip_info info;
    IP4_ADDR(&info.ip, 192, 168, 43, 1); // set IP
    IP4_ADDR(&info.gw, 192, 168, 43, 1); // set gateway
    IP4_ADDR(&info.netmask, 255, 255, 255, 0); // set netmask
    wifi_set_ip_info(SOFTAP_IF, &info);
    struct dhcps_lease dhcp_lease;
    IP4_ADDR(&dhcp_lease.start_ip, 192, 168, 43, 100);
    IP4_ADDR(&dhcp_lease.end_ip, 192, 168, 43, 105);
    wifi_softap_set_dhcps_lease(&dhcp_lease);
    wifi_softap_dhcps_start(); // enable soft-AP DHCP server
}

