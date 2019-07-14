#ifndef FOTA_H_
#define FOTA_H_

#include <c_types.h>

#define FOTA_SECTORSIZE		4096


typedef struct {
	uint32_t sector;
} Fota;

#endif

