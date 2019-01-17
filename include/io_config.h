#ifndef __IO_CONFIG_H__
#define __IO_CONFIG_H__

#define USE_OPTIMIZE_PRINTF

#define EASYQ_RECV_BUFFER_SIZE  4096
#define EASYQ_SEND_BUFFER_SIZE  512 
#define EASYQ_PORT				1085
#define EASYQ_LOGIN				"lm"
#define DEVICE_NAME			    EASYQ_LOGIN	
#define FOTA_QUEUE				DEVICE_NAME":fota"
#define FOTA_STATUS_QUEUE		DEVICE_NAME":fota:status"

#define WIFI_ENABLE_SOFTAP  true
#endif

