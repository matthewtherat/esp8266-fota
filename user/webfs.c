#include "httpd.h"
#include "fs.h"


static ICACHE_FLASH_ATTR
httpd_err_t webfs_format(struct httpd_session *s) {
    fs_err_t err = fs_format();
    if (err) {
        return err;
    }
    return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, "Format Successfull"CR, 20);
}


static ICACHE_FLASH_ATTR
httpd_err_t webfs_post(struct httpd_session *s) {
    httpd_err_t err;
    struct file *f;
    size16_t avail = HTTPD_REQ_LEN(s);
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    size32_t chunklen;
    

    CHK("Handler: avail: %d, more: %d", avail, more);
    if (s->reverse == NULL) {
        CHK("Initialize");
        f = os_zalloc(sizeof(struct file));
        os_strcpy(f->name, s->request.path + 3);
        err = fs_new(f);
        if (err) {
            return err;
        }
        s->reverse = f;
    }
    else {
        f = (struct file*)s->reverse;
    }

    do {
        if (!(avail && more)) {
            os_free(f);
            return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, "Done."CR, 7);
        }

        /* Read from session */
        chunklen = MIN(FS_SECTOR_SIZE, avail);
        INFO("W: %04d More: %07d ", chunklen, more);
        f->bufflen += HTTPD_RECV(s, f->buff, chunklen); 
        
        /* Write */
        //err = fs_write(f, tmp, chunklen);
        //if (err) {
        //    return err;
        //}
        
        avail = HTTPD_REQ_LEN(s);
    } while(avail || (!more));
    
    if (more) {
        CHK("Unhold");
        if(!HTTPD_SCHEDULE(HTTPD_SIG_RECVUNHOLD, s)) {
            return HTTPD_ERR_TASKQ_FULL;
        }
    }
    return HTTPD_MORE;
   
}
