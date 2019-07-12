#ifndef MULTIPART_H
#define MULTIPART_H



typedef void (*MultipartCallback)(void*);

typedef enum {
	MP_IDLE,
	MP_FIELDHEADER,
	MP_BODY
} MultipartStatus;
	

typedef struct {
	MultipartCallback callback;
	char *boundary;
	unsigned char boundarylen;
} Multipart;


typedef enum {
	MP_OK = 0,
	MP_MORE
} MultipartError;


#endif
