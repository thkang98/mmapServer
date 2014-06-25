#include "stdlib.h"
#include "string.h"
#include <errno.h>
#include "dlmmap.h"
#include "dlserver_cmd.h"
#include "mq_common.h"

#define MAX_MQRC_CLIENTS   8

/* { mq dlmalloc Reply Channel */
struct mqrc_t {
  long pid;
  char *name;
  mqd_t mq;
};
static struct mqrc_t mqrc_root[MAX_MQRC_CLIENTS];

static void mqrc_init(void) {
  memset(mqrc_root, 0, sizeof(mqrc_root));
}

static struct mqrc_t *mqrc_chk_pid(long pid) {
  int i;

  for (i=0; i<MAX_MQRC_CLIENTS; i++) {
    if (mqrc_root[i].pid == pid) return(&mqrc_root[i]);
  }
  return((struct mqrc_t *)NULL);
}

void mqrc_remove(long rpid) {
  int i;

  for (i=0; i<MAX_MQRC_CLIENTS; i++) {
    if (mqrc_root[i].pid == rpid) {
      CHECK((mqd_t)-1 != mq_close(mqrc_root[i].mq));
      mqrc_root[i].pid = 0;
      free(mqrc_root[i].name);
      mqrc_root[i].name = NULL;
      break;
    }
  }
}

static mqd_t mqrc_get(long rpid) {
  int i;

  for (i=0; i<MAX_MQRC_CLIENTS; i++) {
    if (mqrc_root[i].pid == rpid) {
      return(mqrc_root[i].mq);
    }
  }
  return(-1);
}

static void mqrc_cleanup(void) {
  int i;

  for (i=0; i<MAX_MQRC_CLIENTS; i++) {
    if (!mqrc_root[i].pid) continue;

    CHECK((mqd_t)-1 != mq_close(mqrc_root[i].mq));
    mqrc_root[i].pid = 0;
    free(mqrc_root[i].name);
    mqrc_root[i].name = NULL;
  }
}

void *mqrc_add(long pid) {
  char *mq_rcname;
  struct mqrc_t *mqrc;
  int i;
  int slen;

  if (mqrc = mqrc_chk_pid(pid)) { return((void *)mqrc); }

  for (i=0; i<MAX_MQRC_CLIENTS; i++) {
    if (!mqrc_root[i].pid) { mqrc = &mqrc_root[i]; break; }
  }

  if (mqrc == NULL) {
     printf("Overloaded: not able to acquire mqrc!\n");
     return(mqrc);
  }
  mqrc->pid = pid;
  slen = strlen(MQ_MMAPALLOC_RC) + PID_MAX_STR_SZ;
  mq_rcname = malloc(slen+1);
  if (!mq_rcname) {
     mqrc->pid = 0;
     return(NULL);
  }
  snprintf(mq_rcname, slen, MQ_MMAPALLOC_RC "%ld", pid);
  mqrc->name = mq_rcname;
  mqrc->mq = mq_open(mq_rcname, O_WRONLY);
  printf("Reply Channel name: %s created for pid:%ld\n",
         mq_rcname, pid);
  CHECK((mqd_t)-1 != mqrc->mq);
  return((void *)mqrc);
}

int mqrc_reply(char *sbuffer, long pid) {
  mqd_t mqrc;
  int  i;

  if (!pid) { return(-1); }

  mqrc = mqrc_get(pid);
  if(mqrc) {
    sbuffer[strlen(sbuffer)] = '\0';
    CHECK(0 <= mq_send(mqrc, sbuffer, strlen(sbuffer), 0));
  }
  return(0);
}

static mqd_t mqdl;
static int mqdl_init(void) {
  struct mq_attr attr;

  /* initialize the dlmalloc queue attributes */
  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MQ_MAXMSG;
  attr.mq_msgsize = MAX_MQ_MSGSIZE;
  attr.mq_curmsgs = 0;

  /* create the dlmalloc message queue */
  mqdl = mq_open(MQ_MMAPALLOC, O_CREAT|O_RDONLY, MQ_MODE, &attr);

  CHECK((mqd_t)-1 != mqdl);
  return errno;
}

extern volatile unsigned int sigint_rcvd;
int mqdl_server(void)
{
  char *buffer;
  int slen;
  int err;

  buffer = malloc(MAX_MQ_MSGSIZE+1);
  if (!buffer) return(-1);

  /* init. the dlmalloc cmd-channel */
  if (err = mqdl_init()) {
    return(-2);
  }

  /* init. the dlmalloc reply-channels */
  mqrc_init();

  do {
    size_t bytes_read;

    /* receive the message */
    bytes_read = mq_receive(mqdl, buffer, MAX_MQ_MSGSIZE, NULL);
    if (sigint_rcvd) { 
      break;
    }
    if (bytes_read <= 0) { continue ; }
    buffer[bytes_read] = '\0';
    slen = strlen(buffer);
    if (buffer[slen-1] == '\n') { buffer[slen-1] = '\0'; }

    mqServerCmdExec(buffer);

  } while (!sigint_rcvd);

  free(buffer);

  mqrc_cleanup();

  CHECK((mqd_t)-1 != mq_close(mqdl));
  CHECK((mqd_t)-1 != mq_unlink(MQ_MMAPALLOC));

  return 0;
}
