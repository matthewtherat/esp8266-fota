#ifndef _WEBADMIN_H__
#define _WEBADMIN_H__

#include "httpserver.h"


#define FB_HTTPSERVER_PORT 80
#define FB_URL_SIZE 10

typedef enum httpverb {
	GET,
	POST
} HTTPVerb;

typedef struct request {
	HTTPVerb verb;
	char *path;
	char *content_type;
	uint16_t body_length;
	char *body;
} Request;

typedef enum {
	IDLE,

	MULTIPART,


typedef struct {

typedef err_t Error;
#define OK 0



#endif
