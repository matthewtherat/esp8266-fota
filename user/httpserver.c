

typedef (int *)(Request *req) Handler;

typedef struct {
	char *pattern;
	Handler handler;
} HttpRoute;


typedef struct {
	HTTPRoute * routes[];
	int port;

} HttpServer;


int ICACHE_FLASH_ATTR httpserver_init(
