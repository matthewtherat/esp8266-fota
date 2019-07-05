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
static char *buff_header;


//static void ICACHE_FLASH_ATTR
//send_response(bool ok, const char *response_buffer) {
//	uint16_t total_length = 0;
//	uint16_t head_length = 0;
//    char *send_buffer = NULL;
//    char httphead[256];
//    os_memset(httphead, 0, 256);
//	uint16_t response_length = (ok && response_buffer != NULL) ? \
//		os_strlen(response_buffer): 0;
//
//	os_sprintf(
//			httphead, 
//			ok? FB_RESPONSE_HEADER_FORMAT: FB_BAD_REQUEST_FORMAT, 
//			response_length
//		);
//	head_length = os_strlen(httphead);	
//    total_length = head_length + response_length;
//    send_buffer = (char *)os_zalloc(total_length + 1);
//	// Write head
//    os_memcpy(send_buffer, httphead, head_length);
//
//	// Body
//    if (response_length > 0) {
//        os_memcpy(send_buffer+head_length, response_buffer, response_length);
//    }
//
//	espconn_sent(&esp_conn, send_buffer, total_length);
//    os_free(send_buffer);
//}

// TODO: content length optional

#define HTTP_RESPONSE_HEADER_FORMAT \
	"HTTP/1.0 %s\r\n"\
	"Server: lwIP/1.4.0\r\n"\
	"Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n"\
	"Pragma: no-cache\r\n"


static ICACHE_FLASH_ATTR
int httpserver_start_response(char *status, char **headers, 
		uint8_t headers_count) {
	int i;
	int cursor;
	char buffer[HTTP_RESPONSE_HEADER_BUFFER_SIZE];
	cursor += os_sprintf(buffer, HTTP_RESPONSE_HEADER_FORMAT, status);

	for (i = 0; i < headers_count; i++) {
		cursor += os_sprintf(buffer + cursor, "%s\r\n", headers[i]);
	}
	cursor += os_sprintf(buffer + cursor, "\r\n");
	
	os_printf("%d\r\n%s", cursor, buffer);
	espconn_send(server->request.conn, buffer, cursor);
	return OK;
}


static ICACHE_FLASH_ATTR
int httpserver_finalize_response(char *body) {
	char buffer[2] = {"\r\n"};
	espconn_send(server->request.conn, buffer, 2);
	return OK;
}


static ICACHE_FLASH_ATTR
int httpserver_send_response(char *status, char **headers, 
		uint8_t headers_count, char *body) {
	httpserver_start_response(HTTPSTATUS_NOTFOUND, headers, headers_count);
	httpserver_finalize_response(body);
	return OK;
}


static ICACHE_FLASH_ATTR
int httpserver_send_response_head(char *status) {
	char *headers[2] = {
		HTTPHEADER_CONTENTTYPE_TEXT,
		HTTPHEADER_CONTENTLENGTH_ZERO,
	};
	return httpserver_send_response(HTTPSTATUS_NOTFOUND, headers, 2, NULL);
}


static ICACHE_FLASH_ATTR
int _dispatch(char *body, uint32_t body_length) {
	Request *req = &server->request;
	HttpRoute *route = NULL;
	Handler handler;
	int16_t statuscode;
	int i;
	
	for (i = 0; i < server->routes_length; i++) {
		route = &server->routes[i];
		if (matchroute(route, req)) {
			break;
		}
	}
	
	if (route == NULL) {
		os_printf("Not found: %s\r\n", req->path);
		return httpserver_send_response_head(HTTPSTATUS_NOTFOUND);
	}

	uint32_t more = (req->content_length + 2) - req->body_cursor;
	bool last = (more - body_length) == 0;
	route->handler(
			req, 
			body, 
			last? body_length - 2: body_length, 
			more - 2
		);
//	if (req->content_length == 0) {
//		_cleanup_request();
//		return;
//	}
//
//	// Consuming body
//	uint32_t needed = (req->content_length + 2) - req->body_cursor;
//	os_printf("Reading body: %d\r\n", remaining);
//	req->body_cursor += remaining;
//	if (remaining >= needed) {
//		os_printf("Cleaning up: %d\r\n", req->body_cursor);
//		_cleanup_request();
//	}
}


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

	// Terminating strings
	if (req->content_type != NULL) {
		req->content_type[content_type_len] = 0;
	}
	return l;
}


static ICACHE_FLASH_ATTR
void _cleanup_request() {
	Request *req = &server->request;
	espconn_disconnect(req->conn);
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
	req->conn = (struct espconn*) arg; 

	if (server->status < HSS_REQ_BODY) {
		server->status = HSS_REQ_HEADER;
		readsize = _read_header(data, length);
		if (readsize < 0) {
			os_printf("Invalid Header: %d\r\n", readsize);
			httpserver_send_response_head(HTTPSTATUS_BADREQUEST);
			return;
		}

		if (readsize == 0) {
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
		server->status = HSS_REQ_BODY;
	}
	else {
		remaining = length;
	}
	
	_dispatch(data + (length-remaining), remaining);
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
    espconn_regist_reconcb(&s->connection, _client_recon);
	espconn_tcp_set_max_con_allow(&s->connection, 1);
	espconn_regist_time(&s->connection, HTTPSERVER_TIMEOUT, 1);
	espconn_set_opt(&s->connection, ESPCONN_NODELAY);
	espconn_accept(&s->connection);
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

