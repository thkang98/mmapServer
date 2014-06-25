#ifndef _DLCLIENT_H_
#define _DLCLIENT_H_

extern int mqrc_open(void);
extern void *mqreq_send(char *sbuffer, int action);
extern int mqdl_client(void);

#endif
