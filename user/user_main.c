// Internal 
#include "user_config.h"
#include "partition.h"
#include "wifi.h"
#include "params.h" 
#include "debug.h"
#include "status.h"
#include "uns.h"

// SDK
#include <ets_sys.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h>
#include <upgrade.h>
#include <c_types.h>
#include <ip_addr.h> 
#include <espconn.h>


#define __version__     "1.0.0"

static Params params;


static ICACHE_FLASH_ATTR 
void reboot_appmode() {
	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
	system_upgrade_reboot();
}


void wifi_connect_cb(uint8_t status) {
    if(status == STATION_GOT_IP) {
        uns_init(params.zone, params.name);
        INFO("WIFI Connected to: %s\r\n", params.station_ssid);
        if (params.apploaded) {
            INFO("Reboot in %d seconds\r\n", REBOOTDELAY);
            status_update(500, 500, REBOOTDELAY, reboot_appmode);
        }
    } 
    else {
        uns_deinit();    
        INFO("WIFI Disonnected from: %s\r\n", params.station_ssid);
    }
}


void boothello() {
    INFO("Fota image version: "__version__"\r\n");
    INFO("My full name is: %s.%s\r\n", params.zone, params.name);
    INFO(
        "Connect to WIFI Access point: %s, "
        "open http://192.168.43.1 to configure me.\r\n",
        params.name
        );
    status_update(700, 700, INFINITE, NULL);
}


void user_init(void) {
    //uart_init(BIT_RATE_115200, BIT_RATE_115200);
    uart_div_modify(UART0, UART_CLK_FREQ / BIT_RATE_115200);
    uart_rx_intr_disable(UART0);
    uart_rx_intr_disable(UART1);

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
    
    PARAMS_PRINT(params);
	
    // Disable wifi led before infrared
    wifi_status_led_uninstall();

    // Status LED
    status_init();

    /* Start WIFI */
    wifi_start(&params, wifi_connect_cb);
    
    /* Web UI */
	webadmin_start(&params);

    status_update(100, 500, 5, boothello);
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

