#include <ctypes.h>
#include <user_interface.h>
#include "debug.h"

#define PARAMS_SECTOR SYSTEM_PARTITION_PARAMS_ADDR / 4096 

typedef struct {
	char wifi_ssid[32];
	char wifi_psk[32];
	char easyq_host[32];
	char device_name[16];
} Params;


void ICACHE_FLASH_ATTR 
params_save(Params* params) {
	bool ok = system_param_save_with_protect(PARAMS_SECTOR, params, 
			sizeof(Params));
	if (!ok) {
		ERROR("Cannot save Params\r\n");
	}
}


void ICACHE_FLASH_ATTR 
params_load(Params* params) {
	bool ok = system_param_load(PARAMS_SECTOR, 0,
			params, sizeof(Params));
	if (!ok) {
		ERROR("Cannot load Params\r\n");
	}
}


