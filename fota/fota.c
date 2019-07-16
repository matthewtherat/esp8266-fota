#include "fota.h"
#include "partition.h"
#include "common.h"
#include "ringbuffer.h"

#include <mem.h>
#include <osapi.h>
#include <upgrade.h>


static Fota fs;
static RingBuffer rb = {FOTA_BUFFERSIZE, 0, 0, NULL};


static _write_sector() {
	//SpiFlashOpResult err;

	system_soft_wdt_feed();
	system_upgrade_erase_flash(0xFFF);
	//os_printf("E: 0x%05X\r\n", fs.sector * FOTA_SECTORSIZE);
	//os_delay_us(100);
	//if (fs.sector % 16 == 0) {
	//}
	//err = spi_flash_erase_sector((uint16_t)fs.sector);
	//if (err != SPI_FLASH_RESULT_OK) {
	//	ERROR("Canot erase flash: %d\r\n", err);
	//}

	system_soft_wdt_feed();
	char sector[FOTA_SECTORSIZE];
	rb_safepop(&rb, sector, FOTA_SECTORSIZE);
	system_upgrade(sector, FOTA_SECTORSIZE);
	//os_delay_us(100);
	os_printf("W: 0x%05X\r\n", fs.sector * FOTA_SECTORSIZE);
	fs.sector++;
	//err = spi_flash_write(fs.sector * FOTA_SECTORSIZE, 
	//		(uint32_t *)fs.recv_buffer, 
	//		FOTA_SECTORSIZE);
	//if (err != SPI_FLASH_RESULT_OK) {
	//	ERROR("Canot write flash: %d\r\n", err);
	//	return;
	//}

}


int fota_feed(char * data, Size datalen) {
	RingBuffer *b = &rb;
	int err = rb_safepush(b, data, datalen);
	if (err != RB_OK) {
		return err;
	}

	if (rb_used(b) >= FOTA_SECTORSIZE) {
		return _write_sector();
	}
	return RB_OK;
}


void fota_init() {
	// Buffer
	rb.blob = (char*) os_malloc(FOTA_BUFFERSIZE + 1);
	rb_reset((RingBuffer*)&rb);
	fs.sector = system_upgrade_userbin_check() == UPGRADE_FW_BIN1 ?
		SYSTEM_PARTITION_OTA2_ADDR / FOTA_SECTORSIZE: 1;

	//system_soft_wdt_stop();
	//wifi_fpm_close();
	//bool fp = spi_flash_erase_protect_disable();
	//if (!fp) {
	//	os_printf("Cannot disable the flash protection\r\n");
	//	return;
	//}

	system_upgrade_init();
	system_upgrade_flag_set(UPGRADE_FLAG_START);
	os_printf("FOTA: Init Sector: %X\r\n", fs.sector);
}


void fota_finalize() {
	os_free(rb.blob);
	os_printf("REBOOTING\r\n");
	system_soft_wdt_feed();
	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);

	//system_upgrade_deinit();
	system_upgrade_reboot();
}

