#include <c_types.h>
#include <user_interface.h>
#include "debug.h"
#include "params.h"


int ICACHE_FLASH_ATTR 
params_save(Params* params) {
	bool ok = system_param_save_with_protect(PARAMS_SECTOR, params, 
			sizeof(Params));
	if (!ok) {
		ERROR("Cannot save Params\r\n");
	}
}


int ICACHE_FLASH_ATTR 
params_load(Params* params) {
	bool ok = system_param_load(PARAMS_SECTOR, 0,
			params, sizeof(Params));
	if (!ok) {
		ERROR("Cannot load Params\r\n");
	}
}


