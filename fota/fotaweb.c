#include "httpd.h"
#include "multipart.h"
#include "params.h"
#include "status.h"
#include "fota.h"


#define BUFFSIZE	2048


static ETSTimer ff;
static Multipart mp;
static char buff[BUFFSIZE];
static RingBuffer rb = {BUFFSIZE, 0, 0, buff};


static
void ff_func(void *arg) {
    Params params;
	if (!params_load(&params)) {
        ERROR("Cannot load paramters.\r\n"); 
        return;
    }
    params.apploaded = 1;
	if (!params_save(&params)) {
        ERROR("Cannot save apploaded flag paramter.\r\n"); 
        return;
    }
    os_delay_us(1000);
	fota_finalize();
}


static
void _mp_callback(MultipartField *f, char *body, Size bodylen, bool last) {
	if (os_strncmp(f->name, "firmware", 8) != 0) {
		return;
	}
	fota_feed(body, bodylen, last);
	//os_printf("total: %d, Chunk len: %d last: %d\r\n", ll, bodylen, last);
	//if (last) {
	//	//espconn_recv_unhold(request->conn);
	//	httpd_response_text(request, HTTPSTATUS_OK, "Done", 4);
	//	fota_finalize();
	//}

}


ICACHE_FLASH_ATTR
void fotaweb_upgrade_firmware(struct httprequest *req, char *body, 
        uint32_t body_length, uint32_t more) {

	int err;
	if (body_length <= 0) {
		return;
	}
	
	if (mp.status == MP_IDLE) {
		err = mp_init(&mp, req->contenttype, _mp_callback);
		if (err != MP_OK) {
			os_printf("Cannot init multipart: %d\r\n", err);
			goto badrequest;
		}
		rb_reset(&rb);
		fota_init();
        status_update(1000, 1000, INFINITE, NULL);
	}
	
	espconn_recv_hold(req->conn);
	if ((err = rb_safepush(&rb, body, body_length)) == RB_FULL) {
		goto badrequest;
	}
    
    err = mp_feedbybuffer(&mp, &rb);
	espconn_recv_unhold(req->conn);
	switch (err) {
		case MP_DONE:
			goto done;

		case MP_MORE:
			return;

		default:
			goto badrequest;
	}

done:
	mp_close(&mp);
	httpd_response_text(req, HTTPSTATUS_OK, "Rebooting...", 12);
	os_timer_disarm(&ff);
	os_timer_setfn(&ff, (os_timer_func_t *)ff_func, NULL);
	os_timer_arm(&ff, 2000, 0);
	return;

badrequest:
	mp_close(&mp);
    status_update(100, 100, 3, NULL);
	httpd_response_notok(req, HTTPSTATUS_BADREQUEST);
}


