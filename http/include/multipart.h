#ifndef MULTIPART_H
#define MULTIPART_H

#include <c_types.h>

#define MP_FIELDNAME_MAXLEN	128


typedef unsigned short Size;

typedef enum {
	MP_FIELDHEADER,
	MP_FIELDBODY
} MultipartStatus;


typedef struct {
	char name[MP_FIELDNAME_MAXLEN];
	char type[MP_FIELDNAME_MAXLEN];
	char filename[MP_FIELDNAME_MAXLEN];
} MultipartField;


typedef void (*MultipartCallback)(MultipartField*, char* body, Size bodylen, 
		bool last);


typedef struct {
	MultipartStatus status;
	MultipartCallback callback;
	MultipartField field;
	char *boundary;
	unsigned char boundarylen;
} Multipart;


typedef enum {
	MP_OK				= 0,
	MP_MORE				= -1,
	MP_NOBOUNDARY		= -2,
	MP_INVALIDBOUNDARY	= -3,
	MP_INVALIDHEADER	= -4,
	MP_DONE				= -6
} MultipartError;


int mp_init(Multipart *mp, char *contenttype, MultipartCallback callback);
int mp_feed(Multipart *mp, char *data, Size datalen, Size* used);
void mp_close(Multipart *mp);

#endif
