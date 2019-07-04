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
static char *buff_header;


static ICACHE_FLASH_ATTR
int _read_header(char *data, uint16_t length) {
	// TODO: max header length check !
	char *cursor = os_strstr(data, "\r\n\r\n");
	char *headers;
	uint16_t content_type_len;
	Request *req = &server->request;

	uint16_t l = (cursor == NULL)? length: (cursor - data) + 4;
	os_memcpy(buff_header + req->buff_header_length, data, l);
	req->buff_header_length += l;

	if (cursor == NULL) {
		// Request for more data, incomplete http header
		return 0;
	}
	
	req->verb = buff_header;
	cursor = os_strchr(buff_header, ' ');
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
	Request *req = &server->request;
	os_memset(buff_header, 0, HTTP_HEADER_BUFFER_SIZE);
	os_memset(req, 0, sizeof(Request));
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
	
	if (req->content_length == 0) {
		_cleanup_request();
		return;
	}

	// Consuming body
	uint32_t needed = (req->content_length + 2) - req->body_read_size;
	os_printf("Reading body: %d\r\n", remaining);
	req->body_read_size += remaining;
	if (remaining >= needed) {
		os_printf("Cleaning up: %d\r\n", req->body_read_size);
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
void _client_recon(void *arg, int8_t err) {
    struct espconn *conn = arg;
	os_printf("HTTPServer "IP_FORMAT" err %d reconnecting...\r\n",  
			unpack_tcp(conn->proto.tcp),
			err
		);
}


static ICACHE_FLASH_ATTR
void _client_disconnected(void *arg) {
    struct espconn *conn = arg;
	os_printf("Client "IP_FORMAT" has been disconnected.\r\n",  
			unpack_tcp(conn->proto.tcp)
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
	buff_header = (char*)os_zalloc(HTTP_HEADER_BUFFER_SIZE);
	s->status = HSS_IDLE;
	server = s;
#ifdef HTTPSERVER_MDNS
	_mdns_init(s);
#endif
    s->connection.type = ESPCONN_TCP;
    s->connection.state = ESPCONN_NONE;
    s->connection.proto.tcp = &s->esptcp;
    s->connection.proto.tcp->local_port = HTTPSERVER_PORT;
	os_printf("HTTP Server is listening on: "IP_FORMAT"\r\n",  
			unpack_tcp(s->connection.proto.tcp)
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
	if (buff_header != NULL) {
		os_free(buff_header);
	}
	server = NULL;
}

