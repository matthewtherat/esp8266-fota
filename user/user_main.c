
// Internal 
#include "io_config.h"
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
		//fota_server_start()
    } else {
		//fota_server_stop()
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
	}
	else {
		INFO("Params loaded sucessfully: ssid: %s psk: %s\r\n",
				params.wifi_ssid, 
				params.wifi_psk
			);
	}

    wifi_start(STATIONAP_MODE, DEVICE_NAME, params.wifi_ssid, 
			params.wifi_psk, wifi_connect_cb);

#if WIFI_ENABLE_SOFTAP
	fb_start();
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

