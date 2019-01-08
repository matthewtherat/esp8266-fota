#ifndef _FIRSTBOOT_H__
#define _FIRSTBOOT_H__

#define FB_SOFTAP_CHANNEL		7
#define FB_SOFTAP_PSK			"esp-8266"
#define FB_HTTPSERVER_PORT 80
#define FB_URL_SIZE 10

struct dhcp_client_info {
	ip_addr_t ip_addr;
	ip_addr_t netmask;
	ip_addr_t gw;
	uint8 flag;
	uint8 pad[3];
};


void user_webserver_init(uint32 port);


#endif
