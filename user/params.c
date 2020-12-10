#include <c_types.h>
#include <user_interface.h>
#include "debug.h"
#include "params.h"


bool ICACHE_FLASH_ATTR 
params_print(Params* params) {
	INFO("\r\nParams: name: %s, ssid: %s psk: %s ap-psk: %s\r\n",
			params->device_name,
			params->station_ssid, 
			params->station_psk,
			params->ap_psk
		);
}


bool ICACHE_FLASH_ATTR 
params_save(Params* params) {
	params->magic = MAGIC;
	return system_param_save_with_protect(PARAMS_SECTOR, params, 
			sizeof(Params));
}


bool ICACHE_FLASH_ATTR 
params_load(Params* params) {
	bool ok = system_param_load(PARAMS_SECTOR, 0,
			params, sizeof(Params));
	return ok && params->magic == MAGIC;
}


bool ICACHE_FLASH_ATTR 
params_defaults(Params* params) {
	os_sprintf(params->device_name, NEWDEVICE_NAME);
	params->ap_psk[0] = 0;
	params->station_ssid[0] = 0;
	params->station_psk[0] = 0;
	return params_save(params);
}

