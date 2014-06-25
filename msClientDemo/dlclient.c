#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "mymspace.h"
#include "mq_common.h"
#include "dlclient_cmd.h"

static mqd_t mqdl;  /* to dlserver */
static mqd_t mqrc;  /* from dlserver */
static char *mq_rcname;

int mqrc_open(void) {
  struct mq_attr attr;

  int slen = strlen(MQ_MMAPALLOC_RC) + PID_MAX_STR_SZ;

  mq_rcname = malloc(slen+1);
  if (!mq_rcname) { return(-1); };
  snprintf(mq_rcname, slen, MQ_MMAPALLOC_RC "%d", getpid());

  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = MAX_MQ_MSGSIZE;
  attr.mq_curmsgs = 0;
  mqrc = mq_open(mq_rcname, O_CREAT|O_RDONLY, 0666, &attr);
  CHECK((mqd_t)-1 != mqrc);
  return(errno);
}

/* not thread-safe */
char *mqreq_send(char *sbuffer, int waitreply) {
static char *rbuffer = 0;

  size_t bytes_read;

  if (!rbuffer) {
    rbuffer = malloc(MAX_MQ_MSGSIZE+1);
    if (!rbuffer) {
      printf("%s()L%d: Failed to alloc %d bytes memory\n",
             __func__, __LINE__, MAX_MQ_MSGSIZE);
      return(NULL);
    }
  }

  if (mq_send(mqdl, sbuffer, strlen(sbuffer), 0) >= 0) {
    if(waitreply) {
      memset(rbuffer, 0, MAX_MQ_MSGSIZE);
      bytes_read = mq_receive(mqrc, rbuffer, MAX_MQ_MSGSIZE, NULL);
      if (bytes_read) { return(rbuffer); }
    }
  }
  return(NULL);
}

extern volatile unsigned int sigint_rcvd;
int mqdl_client(void) {
  char *buffer;
  int slen;
  size_t bytes_read;
  int must_stop = 0;

  buffer = malloc(MAX_MQ_MSGSIZE);

  /* open the dlmalloc mail queue */
  mqdl = mq_open(MQ_MMAPALLOC, O_WRONLY);
  CHECK((mqd_t)-1 != mqdl);

  printf("Send to server (enter \"exit\" to stop it):\n");

  do {
    printf("> ");
    fflush(stdout);

    memset(buffer, 0, MAX_MQ_MSGSIZE+1);
    fgets(buffer, MAX_MQ_MSGSIZE, stdin);
    if (sigint_rcvd) { 
      break;
    }
    slen = strlen(buffer);
    if (slen == 0) { continue ; }
    if (buffer[slen-1] == '\n') { buffer[slen-1] = 0; }
   
    mqClientCmdExec(buffer);

  } while (!sigint_rcvd);

  CHECK((mqd_t)-1 != mq_close(mqdl));

  CHECK((mqd_t)-1 != mq_close(mqrc));
  CHECK((mqd_t)-1 != mq_unlink(mq_rcname));

  return 0;
}
