#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "multipart.h"


char *sample = 
	"-----------------------------9051914041544843365972754266\r\n"
	"Content-Disposition: form-data; name=\"text\"\r\n"
	"\r\n"
	"text default\r\n"
	"-----------------------------9051914041544843365972754266\r\n"
	"Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
	"Content-Type: text/plain\r\n"
	"\r\n"
	"Content of a.txt.\r\n"
	"\r\n"
	"-----------------------------9051914041544843365972754266\r\n"
	"Content-Disposition: form-data; name=\"file2\"; filename=\"a.html\"\r\n"
	"Content-Type: text/html\r\n"
	"\r\n"
	"<!DOCTYPE html><title>Content of a.html.</title>\r\n"
	"\r\n"
	"-----------------------------9051914041544843365972754266--\r\n";


#define CONTENTTYPE	"Content-Type: multipart/form-data; boundary=" \
	"---------------------------9051914041544843365972754266\r\n"

#define CONTENTLEN	554


void cb(Multipart *mp) {
}


int main() {
	int err;
	Multipart mp;
	if((err = mp_init(&mp, CONTENTTYPE, cb)) != MP_OK) {
		printf("Cannot init: %d\r\n", err);
		goto failed;
	}
	printf("Boundary: %d:%s\r\n", mp.boundarylen, mp.boundary);
	
	char temp[1024];
	strncpy(temp, sample, 100);
	printf("%s\r\n", temp);
	return 0;
//	if (err = mp_feed(&mp, sample, 100) != MP_MORE) {
//		goto failed;
//	}
//	
//	if (err = mp_feed(&mp, sample, strlen(sample)) != MP_OK) {
//		goto failed;
//	}

failed:
	printf("Fail\r\n");
	return 1;
}
