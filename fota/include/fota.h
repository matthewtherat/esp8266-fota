#ifndef FOTA_H_
#define FOTA_H_

#include <c_types.h>

#define FOTA_SECTORSIZE		4096
#define FOTA_BUFFERSIZE		FOTA_SECTORSIZE * 2


typedef struct {
	uint32_t sector;
} Fota;


int fota_feed(char * data, uint32_t datalen);
void fota_init();
void fota_finalize();

#endif

