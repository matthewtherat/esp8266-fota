#include "httpserver.h"
#include "debug.h"

#include <osapi.h>
#include <user_interface.h>
#include <mem.h>

#include <ets_sys.h>
#include <c_types.h>
#include <os_type.h>


static HttpServer *server;
static struct mdns_info mdns;
static struct mdns_info mdns2;


static void ICACHE_FLASH_ATTR
void _client_recv(void *arg, char *data, uint16_t length) {
	Request req;
	if(OK != _parse_request(data, length, &req)) {
		return;
	}

	os_printf("--> Verb: %s Length: %d Body: %s\r\n", 
			req.verb, 
			req.body_length, 
			req.body
	);
	
	// Dispatch
}


static ICACHE_FLASH_ATTR 
void _mdns_init(HttpServer *s) {
	struct ip_info ipconfig;
	wifi_set_broadcast_if(STATIONAP_MODE);

	wifi_get_ip_info(STATION_IF, &ipconfig);
	mdns.ipAddr = ipconfig.ip.addr; //ESP8266 Station IP
	mdns.host_name = s->hostname;
	mdns.server_name = HTTPSERVER_NAME;
	mdns.server_port = HTTPSERVER_PORT;
	mdns.txt_data[0] = "version = "HTTPSERVER_VERSION;
	espconn_mdns_init(&mdns);
}


static ICACHE_FLASH_ATTR
void _client_recon(void *arg, char err)
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
void _client_disconnected(void *arg)
{
    struct espconn *conn = arg;
    os_printf("Client %d.%d.%d.%d:%d disconnect\n", 
			conn->proto.tcp->remote_ip[0],
        	conn->proto.tcp->remote_ip[1],
			conn->proto.tcp->remote_ip[2],
        	conn->proto.tcp->remote_ip[3],
			conn->proto.tcp->remote_port
	);
}


static ICACHE_FLASH_ATTR
void _client_connected(void *arg)
{
    struct espconn *conn = arg;
    espconn_regist_recvcb(conn, _client_recv);
    espconn_regist_reconcb(conn, _client_recon);
    espconn_regist_disconcb(conn, _client_disconnected);
}


ICACHE_FLASH_ATTR 
int httpserver_init(HttpServer *s) {
	int err; 
	server = s;
#ifdef HTTPSERVER_MDNS
	_mdns_init(s);
#endif
    s->connection.type = ESPCONN_TCP;
    s->connection.state = ESPCONN_NONE;
    s->connection.proto.tcp = &s->esptcp;
    s->connection.proto.tcp->local_port = HTTPSERVER_PORT;
    os_printf("HTTP Server is listening on: %d.%d.%d.%d:%d\n", 
			s->connection.proto.tcp->local_ip[0],
        	s->connection.proto.tcp->local_ip[1],
			s->connection.proto.tcp->local_ip[2],
        	s->connection.proto.tcp->local_ip[3],
			s->connection.proto.tcp->local_port
	);

	espconn_regist_connectcb(&s->connection, _client_connected);
	espconn_accept(&s->connection);
	espconn_tcp_set_max_con_allow(&s->connection, 1);
	espconn_regist_time(&s->connection, HTTPSERVER_TIMEOUT, 1);
	return OK;
}


void ICACHE_FLASH_ATTR
httpserver_stop() {
	espconn_disconnect(&server->connection);
	espconn_delete(&server->connection);
	server = NULL;
}


