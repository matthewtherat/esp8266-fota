#ifndef _PARAMS_H__
#define _PARAMS_H__

#include "c_types.h"
#include "partition.h"

#define PARAMS_SECTOR SYSTEM_PARTITION_PARAMS_ADDR / 4096 


typedef struct {
	 char magic;
	 char device_name[16];
	 char wifi_ssid[32];
	 char wifi_psk[32];
} Params;


bool ICACHE_FLASH_ATTR 
params_save(Params* params);

bool ICACHE_FLASH_ATTR 
params_load(Params* params);

#endif

