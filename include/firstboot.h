#ifndef _FIRSTBOOT_H__
#define _FIRSTBOOT_H__

#define FB_HTTPSERVER_PORT 80
#define FB_URL_SIZE 10

typedef enum httpverb {
	GET,
	POST
} HTTPVerb;

typedef struct request {
	HTTPVerb verb;
	char *body;
	uint16_t body_length;
} Request;

typedef err_t Error;
#define OK 0



#endif
