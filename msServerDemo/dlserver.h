#ifndef _DLSERVER_H_
#define _DLSERVER_H_

extern int mqdl_server(void);
extern void *mqrc_add(long pid);
extern void mqrc_remove(long pid);
extern int mqrc_reply(char *sbuffer, long pid);

#endif
