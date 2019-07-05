#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include <ip_addr.h> 
#include <espconn.h>

#ifndef HTTPSERVER_PORT
#define HTTPSERVER_PORT	80
#endif

#ifndef HTTPSERVER_MDNS
#define HTTPSERVER_MDNS
#endif

#ifndef HTTPSERVER_NAME
#define HTTPSERVER_NAME	"ESPHTTPServer"
#endif

#ifndef HTTPSERVER_VERSION
#define HTTPSERVER_VERSION	"0.1.0"
#endif

#ifndef HTTPSERVER_TIMEOUT
#define HTTPSERVER_TIMEOUT	1
#endif


#ifndef HTTP_HEADER_BUFFER_SIZE
#define HTTP_HEADER_BUFFER_SIZE	8*1024
#endif

#define HTTPSTATUS_NOTFOUND		"404 Not Found"
#define HTTPSTATUS_BADREQUEST	"400 Bad Request"
#define HTTPSTATUS_OK			"200 Ok"

#define HTTPHEADERKEY_CONTENTTYPE		"Content-Type: "
#define HTTPHEADERKEY_CONTENTLENGTH		"Content-Length: "

#define HTTPHEADER_CONTENTTYPE_TEXT		HTTPHEADERKEY_CONTENTTYPE"text/plain"
#define HTTPHEADER_CONTENTLENGTH_ZERO	HTTPHEADERKEY_CONTENTLENGTH"0"
#define OK	0
#define HTTPVERB_ANY	NULL
#define HTTP_RESPONSE_HEADER_BUFFER_SIZE	1024


#define IP_FORMAT	"%d.%d.%d.%d:%d"


#define unpack_ip(ip) ip[0], ip[1], ip[2], ip[3]
#define unpack_tcp(tcp) \
	tcp->local_ip[0], tcp->local_ip[1], \
	tcp->local_ip[2], tcp->local_ip[3], \
	tcp->local_port


#define startswith(str, searchfor) \
	(strncmp(searchfor, str, strlen(searchfor)) == 0)


#define matchroute(route, req) (\
	(route->verb == HTTPVERB_ANY || strcmp(route->verb, req->verb) == 0) \
	&& startswith(req->path, route->pattern) \
)

typedef struct {
	char *verb;
	char *path;
	char *content_type;
	uint32_t content_length;
	uint16_t body_length;
	
	struct espconn *conn;
	uint16_t buff_header_length;
	uint16_t body_cursor;
} Request;


typedef void (*Handler)(Request *req, char *body, uint32_t body_length, 
		uint32_t more);


typedef struct {
	char *verb;
	char *pattern;
	Handler handler;
} HttpRoute;


typedef enum {
	HSS_IDLE = 0,
	HSS_REQ_HEADER,
	HSS_REQ_BODY,
	HSS_RESP_HEADER,
	HSS_RESP_BODY
} HttpServerStatus;


typedef struct {
	struct espconn connection;
	esp_tcp esptcp;
	char *hostname;
	Request request;
	HttpServerStatus status;
	uint8_t routes_length;
	HttpRoute routes[];
} HttpServer;


#endif
