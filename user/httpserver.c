#include "httpserver.h"
#include "debug.h"

#include <osapi.h>
#include <user_interface.h>
#include <mem.h>

#include <ets_sys.h>
#include <c_types.h>
#include <os_type.h>

// TODO: Max connection: 1

static HttpServer *server;
static struct mdns_info mdns;
static struct mdns_info mdns2;


static ICACHE_FLASH_ATTR
int _parse_request(char *data, uint16_t length, Request *req) {
	char *cursor;

	req->verb = data;
	cursor = os_strchr(data, ' ');
	cursor[0] = 0;

	req->path = ++cursor;
	cursor = os_strchr(cursor, ' ');
	cursor[0] = 0;

	req->content_type = os_strstr(++cursor, "Content-Type:");
	if (req->content_type != NULL) {
		cursor = os_strstr(++cursor, "\r\n");
		cursor[0] = 0;
		cursor += 2;
	}

	req->content_type = os_strstr(++cursor, "Content-Length:");
	if (req->content_length != NULL) {
		cursor = os_strstr(++cursor, "\r\n");
		cursor[0] = 0;
		cursor += 2;
	}

	req->body = (char*)os_strstr(cursor, "\r\n\r\n");
	if (req->body != NULL) {
		req->body += 4;
		req->body_length = length - (req->body - data);	
	}
	return OK;
}


static ICACHE_FLASH_ATTR
int _read_header(char *data, uint16_t length) {
	// TODO: max header length check !
	char *buff = server->buff_header;
	char *cursor = os_strstr(data, "\r\n\r\n");
	char *headers;
	uint16_t content_type_len;

	uint16_t l = (cursor == NULL)? length: (cursor - data) + 4;
	os_memcpy(&buff[server->buff_header_length], data, l);
	server->buff_header_length += l;

	if (cursor == NULL) {
		// Request for more data, incomplete http header
		return 0;
	}
	
	Request *req = &server->request;
	req->verb = buff;
	cursor = os_strchr(buff, ' ');
	cursor[0] = 0;

	req->path = ++cursor;
	cursor = os_strchr(cursor, ' ');
	cursor[0] = 0;
	headers = cursor + 1;

	req->content_type = os_strstr(headers, "Content-Type:");
	if (req->content_type != NULL) {
		cursor = os_strstr(req->content_type, "\r\n");
		if (cursor == NULL) {
			return -1;
		}
		content_type_len = cursor - req->content_type;
	}

	cursor = os_strstr(headers, "Content-Length:");
	if (cursor != NULL) {
		req->content_length = atoi(cursor + 16);
		cursor = os_strstr(cursor, "\r\n");
		if (cursor == NULL) {
			return -2;
		}
	}

	// Terminating the strings
	
	if (req->content_type != NULL) {
		req->content_type[content_type_len] = 0;
	}
	return l;
}


static ICACHE_FLASH_ATTR
void _cleanup_request() {
	os_memset(server->buff_header, 0, HTTP_HEADER_BUFFER_SIZE);
	os_memset(&server->request, 0, sizeof(Request));
	server->body_read_size = 0;
	server->buff_header_length = 0;
	server->status = HSS_IDLE;
}


static ICACHE_FLASH_ATTR
void _client_recv(void *arg, char *data, uint16_t length) {
	uint16_t remaining;
	int readsize;
    struct espconn *conn = arg;
	Request *req = &server->request;

	if (server->status < HSS_BODY) {
		server->status = HSS_HEADER;
		readsize = _read_header(data, length);
		if (readsize < 0) {
			os_printf("Invalid Header: %d\r\n", readsize);
			_cleanup_request();
			espconn_disconnect(conn);
		}
		else if (readsize == 0) {
			// Incomplete header
			os_printf("Incomplete Header: %d\r\n", readsize);
			return;
		}
		remaining = length - readsize;
		os_printf("--> %s %s type: %s length: %d, remaining: %d-%d=%d\r\n", 
				req->verb,
				req->path,
				req->content_type,
				req->content_length,
				length,
				readsize,
				remaining
		);
		server->status = HSS_BODY;
	}
	else {
		remaining = length;
	}
	
	server->body_read_size += remaining;
	uint32_t needed = req->content_length - server->body_read_size;
	if (remaining >= needed) {
		_cleanup_request();
	}
	// TODO: Dispatch
	//_dispatch(&server->req);
	

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
void _client_recon(void *arg, char err) {
    struct espconn *conn = arg;
    os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", 
			conn->proto.tcp->remote_ip[0],
    		conn->proto.tcp->remote_ip[1],
			conn->proto.tcp->remote_ip[2],
    		conn->proto.tcp->remote_ip[3],
			conn->proto.tcp->remote_port, 
			err
	);
}



static ICACHE_FLASH_ATTR
void _client_disconnected(void *arg) {
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
	s->buff_header = (char*)os_zalloc(HTTP_HEADER_BUFFER_SIZE);
	s->status = HSS_IDLE;
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


ICACHE_FLASH_ATTR
void httpserver_stop() {
	if (server == NULL) {
		return;
	}
	espconn_disconnect(&server->connection);
	espconn_delete(&server->connection);
	if (server->buff_header != NULL) {
		os_free(server->buff_header);
	}
	server = NULL;
}


