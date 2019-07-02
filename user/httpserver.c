#include "httpserver.h"

#include <osapi.h>
#include <user_interface.h>
#include <mem.h>

#include <ets_sys.h>
#include <c_types.h>
#include <os_type.h>


int ICACHE_FLASH_ATTR 
httpserver_mdns_init(HttpServer *s) {
	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);
	s->mdns.host_name = s->hostname;
	s->mdns.ipAddr = ipconfig.ip.addr; //ESP8266 Station IP
	s->mdns.server_name = HTTPSERVER_NAME;
	s->mdns.server_port = HTTPSERVER_PORT;
	s->mdns.txt_data[0] = "version = "HTTPSERVER_VERSION;
	//info->txt_data[1] = "user1 = data1";
	//info->txt_data[2] = "user2 = data2";
	espconn_mdns_init(&s->mdns);
}


int ICACHE_FLASH_ATTR 
httpserver_init(HttpServer *s) {
#ifdef HTTP_MDNS
	httpserver_mdns_init(s);
#endif
//    s->connection.type = ESPCONN_TCP;
//    s->connection.state = ESPCONN_NONE;
//    s->connection.proto.tcp = &esptcp;
//    s->connection.proto.tcp->local_port = HTTPSERVER_PORT;
//    os_printf("HTTP Server is listening on: %d.%d.%d.%d:%d\n", 
//			s->connection.proto.tcp->local_ip[0],
//        	s->connection.proto.tcp->local_ip[1],
//			s->connection.proto.tcp->local_ip[2],
//        	s->connection.proto.tcp->local_ip[3],
//			s->connection.proto.tcp->local_port
//	);

//    espconn_regist_connectcb(&esp_conn, fb_webserver_connected);
//    espconn_accept(&esp_conn);
//	espconn_tcp_set_max_con_allow(&esp_conn, 1);
//	espconn_regist_time(&esp_conn, WA_TIMEOUT, 1);

}


void ICACHE_FLASH_ATTR
httpserver_stop(HttpServer *s) {
	espconn_disconnect(&s->connection);
	espconn_delete(&s->connection);
}


