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


#define OK	0

#define IP_FORMAT	"%d.%d.%d.%d:%d"

#define unpack_ip(ip) ip[0], ip[1], ip[2], ip[3]
#define unpack_tcp(tcp) \
	tcp->local_ip[0], tcp->local_ip[1], \
	tcp->local_ip[2], tcp->local_ip[3], \
	tcp->local_port


typedef struct {
	char *verb;
	char *path;
	char *content_type;
	uint32_t content_length;
	uint16_t body_length;

	uint16_t buff_header_length;
	uint16_t body_read_size;
} Request;

typedef int (*Handler)(Request *req);

typedef struct {
	char *verb;
	char *pattern;
	Handler handler;
} HttpRoute;


typedef enum {
	HSS_IDLE = 0,
	HSS_HEADER,
	HSS_BODY
} HttpServerStatus;


typedef struct {
	struct espconn connection;
	esp_tcp esptcp;
	char *hostname;
	Request request;
	HttpServerStatus status;
	HttpRoute routes[];
} HttpServer;


#endif
