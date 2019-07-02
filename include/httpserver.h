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


#define OK	0


typedef struct {
} Request;

typedef int (*Handler)(Request *req);

typedef struct {
	char *verb;
	char *pattern;
	Handler handler;
} HttpRoute;


typedef struct {
	struct espconn connection;
	esp_tcp esptcp;
	struct mdns_info mdns;
	char *hostname;
	HttpRoute routes[];
} HttpServer;


#endif
