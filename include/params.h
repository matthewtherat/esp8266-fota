#ifndef _PARAMS_H__
#define _PARAMS_H__

#include "partition.h"

#define PARAMS_SECTOR SYSTEM_PARTITION_PARAMS_ADDR / 4096 


typedef struct {
	char wifi_ssid[32];
	char wifi_psk[32];
	char easyq_host[32];
	char device_name[16];
} Params;


int ICACHE_FLASH_ATTR 
params_save(Params* params);

int ICACHE_FLASH_ATTR 
params_load(Params* params);

#endif

