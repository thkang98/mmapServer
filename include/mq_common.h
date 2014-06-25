#ifndef _MQ_COMMON_H_
#define _MQ_COMMON_H_

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define MQ_MMAPALLOC    "/mqdlcmd"
#define MQ_MMAPALLOC_RC "/mqrcdl"    /* + "pid_number" */
#define MAX_MQ_MSGSIZE  (128)
#define MAX_MQ_MAXMSG   (10)
#define MAX_MQRC        (MAX_MQ_NUM_OF_MESSAGE)
#define MQ_MODE         (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define PID_MAX_STR_SZ  (22)

#define CHECK(x) \
    do { \
        if (!(x)) { \
            perror(#x); \
            fprintf(stderr, "%s:%d: errno:%d %s\n", __func__, __LINE__, errno, sys_errlist[errno]); \
        } \
    } while (0)


#endif /* #ifndef _MQ_COMMON_H_ */
