#ifndef FOTA_H_
#define FOTA_H_

#include <c_types.h>

#define FOTA_SECTORSIZE		4096
#define FOTA_BUFFERSIZE		FOTA_SECTORSIZE * 2


typedef struct {
	uint32_t sector;
} Fota;

#endif

