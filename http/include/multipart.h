#ifndef MULTIPART_H
#define MULTIPART_H




typedef enum {
	MP_IDLE,
	MP_FIELDHEADER,
	MP_BODY
} MultipartStatus;
	

typedef struct {
	void *callback;
	char *boundary;
	unsigned char boundarylen;
} Multipart;


typedef void (*MultipartCallback)(Multipart*);

typedef enum {
	MP_OK = 0,
	MP_MORE,
	MP_NOBOUNDARY
} MultipartError;


int mp_init(Multipart *mp, char *contenttype, MultipartCallback callback);

#endif
