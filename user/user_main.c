
// Internal 
#include "user_config.h"
#include "partition.h"
#include "wifi.h"
#include "fota.h"
#include "params.h" 
#include "debug.h"

// SDK
#include <ets_sys.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h>
#include <upgrade.h>


static Params params;


void wifi_connect_cb(uint8_t status) {
    if(status == STATION_GOT_IP) {
		fb_start();
    } else {
		fb_stop();
    }
}


void user_init(void) {
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
	bool ok = params_load(&params);
	if (!ok) {
		ERROR("Cannot load Params\r\n");
#if !WIFI_ENABLE_SOFTAP
		return;
#endif
		if(!params_defaults(&params)) {
			ERROR("Cannot save params\r\n");
			return;
		}
	
	}

	INFO("Params: name: %s, ssid: %s psk: %s\r\n",
			params.device_name,
			params.station_ssid, 
			params.station_psk
		);

#if WIFI_ENABLE_SOFTAP
    wifi_start(STATIONAP_MODE, &params, wifi_connect_cb);
#else
    wifi_start(STATION_MODE, &params, wifi_connect_cb);
#endif
    INFO("System started ...\r\n");
}


void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		FATAL("system_partition_table_regist fail\r\n");
		while(1);
	}
}

