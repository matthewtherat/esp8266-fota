#ifndef WEBADMIN_H
#define WEBADMIN_H

#define HTTPD_MAXCONN   2

int webadmin_start(Params *_params);
void webadmin_stop();

#endif
