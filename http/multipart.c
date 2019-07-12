#include <osapi.h>
#include <mem.h>

#include "multipart.h"


int mp_init(Multipart *mp, char *contenttype, MultipartCallback callback) {
	char *e;
	char *b = os_strstr(contenttype, "boundary=");
	if (b == NULL) {
		return MP_NOBOUNDARY;
	}
	b += 9;

	e = os_strstr(b, "\r\n");
	if (e == NULL) {
		return MP_NOBOUNDARY;
	}
	
	mp->boundarylen = e - b;
	mp->boundary = (char*)os_malloc(mp->boundarylen + 1);
	strncpy(mp->boundary, b, mp->boundarylen);
	mp->boundary[mp->boundarylen] = '\0';
	return MP_OK;
}


void mp_close(Multipart *mp) {
	free(mp->boundary);
}

