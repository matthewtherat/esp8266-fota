
// Internal 
#include "io_config.h"
#include "partition.h"
#include "wifi.h"
#include "fota.h"
#include "params.h" 
// SDK
#include <ets_sys.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h>
#include <upgrade.h>

// LIB: EasyQ
#include "easyq.h" 
#include "debug.h"


static Params params;
static EasyQSession eq;
static ETSTimer status_timer;


void ICACHE_FLASH_ATTR
fota_report_status() {
	char str[50];
	float vdd = system_get_vdd33() / 1024.0;

	uint32_t userbin_addr = system_get_userbin_addr();
	uint8_t image = system_upgrade_userbin_check();
	os_sprintf(str, "Image: %s BinAddress: 0x%05X, VDD: %d.%03d", 
			(UPGRADE_FW_BIN1 == image)? "FOTA": "APP",
			userbin_addr,
			(int)vdd, 
			(int)(vdd*1000)%1000);
	easyq_push(&eq, FOTA_STATUS_QUEUE, str);
}


void ICACHE_FLASH_ATTR
easyq_message_cb(void *arg, const char *queue, const char *msg, 
		uint16_t message_len) {
	//INFO("EASYQ: Message: %s From: %s\r\n", msg, queue);

	if (strcmp(queue, FOTA_QUEUE) == 0) {
		if (msg[0] == 'S') {
			char *server = (char *)(&msg[0]+1);
			char *colon = strrchr(server, ':');;
			uint8_t hostname_len = (uint8_t)(colon - server);
			uint16_t port = atoi(colon+1);
			colon[0] = 0;	
			
			INFO("INIT FOTA: %s %d\r\n", server, port);
			os_timer_disarm(&status_timer);
			easyq_delete(&eq);
			fota_init(server, hostname_len, port);
			fota_start();
			// TODO: decide about delete easyq ?
			easyq_disconnect(&eq);
		}
		else if (msg[0] == 'R') {
			os_timer_disarm(&status_timer);
			system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
			system_upgrade_reboot();
		}
		else if (msg[0] == 'I') {
			fota_report_status();
		}
	}
}


void ICACHE_FLASH_ATTR
easyq_connect_cb(void *arg) {
	INFO("EASYQ: Connected to %s:%d\r\n", eq.hostname, eq.port);
	INFO("\r\n***** OTA ****\r\n");
	INFO("Device: %s\r\n", DEVICE_NAME);
	
	const char * queues[] = {FOTA_QUEUE};
	easyq_pull_all(&eq, queues, 1);
    os_timer_disarm(&status_timer);
    os_timer_setfn(&status_timer, (os_timer_func_t *)fota_report_status, NULL);
    os_timer_arm(&status_timer, 3000, 1);
}


void ICACHE_FLASH_ATTR
easyq_connection_error_cb(void *arg) {
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Connection error: %s:%d\r\n", e->hostname, e->port);
	INFO("EASYQ: Reconnecting to %s:%d\r\n", e->hostname, e->port);
}


void easyq_disconnect_cb(void *arg)
{
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Disconnected from %s:%d\r\n", e->hostname, e->port);
}


void setup_easyq() {
	EasyQError err = \
			easyq_init(&eq, params.easyq_host, EASYQ_PORT, EASYQ_LOGIN);
	if (err != EASYQ_OK) {
		ERROR("EASYQ INIT ERROR: %d\r\n", err);
		return;
	}
	eq.onconnect = easyq_connect_cb;
	eq.ondisconnect = easyq_disconnect_cb;
	eq.onconnectionerror = easyq_connection_error_cb;
	eq.onmessage = easyq_message_cb;
}


void wifi_connect_cb(uint8_t status) {
    if(status == STATION_GOT_IP) {
        easyq_connect(&eq);
    } else {
        easyq_disconnect(&eq);
    }
}



void user_init(void) {
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
	bool ok = params_load(&params);
	if (!ok) {
		ERROR("Cannot load Params\r\n");
		fb_start();
		fb_webserver_init(80);
		return;
	}
	INFO("Params loaded sucessfully: ssid: %s psk: %s easyq: %s\r\n",
			params.wifi_ssid, 
			params.wifi_psk,
			params.easyq_host
		);
	setup_easyq();
    wifi_connect(params.wifi_ssid, params.wifi_psk, wifi_connect_cb);
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

