#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#define BUFFER_SIZE_MAX   2000 


typedef void (*http_callback)(int status, char* body);


void http_send(
        const char *host, 
        const char *verb, 
        const char *path, 
        const char *headers, 
        const char *body, 
        http_callback user_callback
    );

#define http_nobody(host, verb, path, cb) \
    http_send((host), (verb), (path), "", "", (cb))

#endif
