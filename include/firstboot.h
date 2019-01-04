#ifndef _FIRSTBOOT_H__
#define _FIRSTBOOT_H__

#define FB_SOFTAP_CHANNEL		7
#define FB_SOFTAP_PSK			"esp-8266"

struct dhcp_client_info {
	ip_addr_t ip_addr;
	ip_addr_t netmask;
	ip_addr_t gw;
	uint8 flag;
	uint8 pad[3];
};

#endif
