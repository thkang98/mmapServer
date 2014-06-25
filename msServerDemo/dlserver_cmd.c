#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <errno.h>

#include "mymspace.h"
#include "malloc.h"
#include "dlmmap.h"
#include "dlserver.h"

const char *pid_n = "pid";
const char *mymspace_n = "mymspace";
const char *mmalloc_n = "mmalloc";
const char *mfree_n = "mfree";
const char *exit_n = "exit";

static long hexstring_to_long(char *hexstring, long *val) {
  char *endptr;

  errno = 0;  /* distinguish success/failure after call */
  *val = strtoul(hexstring, &endptr, 16);

  /* Check for various possible errors */

  if ((errno == ERANGE && (*val == LONG_MAX || *val == LONG_MIN))
        || (errno != 0 && *val == 0)) {
    perror("strtol");
    return(-1);
  }   

  if (endptr == hexstring) {
    fprintf(stderr, "No digits were found\n");
    return(-1);
  }   

  return 1;
}

static char *mqreply_exit(char *sbuffer, int bsz, int pid, char *arg) {
  if (!pid) { return(NULL); }
  mqrc_remove(pid);
}

static char *mqreply_mqrc_create(char *sbuffer, int bsz, int pid, char *arg) {
  if (pid) { mqrc_add(pid); }
  return(NULL);
}

static char *mqreply_mfree(char *sbuffer, int bsz, int pid, char *haddr) {
  long ptr;
  if (haddr) {
    if (hexstring_to_long(haddr, &ptr)) {
      printf("mfree haddr:%s ptr:%ld 0x%08lx\n", haddr, ptr, (unsigned long)ptr);
      MFREE((void *)ptr);
    }
  }
  return(NULL);
}

static char *mqreply_mmalloc(char *sbuffer, int bsz, int pid, char *req_sz)  {
  size_t msz;
  size_t *ptr;

  if (req_sz) {
    msz = atol(req_sz);
    ptr = MMALLOC(msz);
  } else { ptr = (void *)NULL; }

  snprintf(sbuffer, bsz, "0x%lx", (long)ptr);
  printf("0x%lx = mmalloc(%lu)\n", (long)ptr, msz);
  mqrc_reply(sbuffer, pid);
  return(NULL);
}

static char * mqreply_mymspace(char *sbuffer, int bsz, int pid, char *arg) {
  long msz;

  if (!pid) { return(NULL); }

  msz= (long)msmalloc_stats_maxfp();
  snprintf(sbuffer, bsz, "0x%lx:%ld", (long)mymspace, msz);
  mqrc_reply(sbuffer, pid);
  return(NULL);
}

static int mqCmdParser(char *istr, char **cmd, char **arg1, char **arg2) {
  int i = 0;
  char *token = NULL;
  char *saveptr, *pS;
  char delims[] = " :\n";
  int  rcmd = 0;

  /* format: cmd:arg1:arg2 */
  *cmd = NULL;
  *arg1 = NULL;
  *arg2 = NULL;
  if (istr == 0) { return(0); }
  if (strlen(istr) == 0) { return(0); }

  for (i=0, pS=istr; ; i++, pS = NULL) {
    token = strtok_r(pS, delims, &saveptr);
    if (token == NULL) { break; }
    switch(i) {
    default:               break;
    case 0: *cmd  = token; break;
    case 1: *arg1 = token; break;
    case 2: *arg2 = token; break;
    }
  }
  return(i);
}

char *mqServerCmdExec(char *rbuffer) {
#define SBUFFER_SZ   128

  static char *sbuffer = NULL;

  char *cmd, *pid, *arg;
  int  rpid;
  int rcmd;
  int i;

  typedef char *cmd_func_(char *sbuffer, int bsz, int pid, char *arg);
  struct mqserver_ {
    const char *cmd;
    cmd_func_ *cmdfunc;
  };
  struct mqserver_ cmd_callback[] = {
    { mmalloc_n,  mqreply_mmalloc },
    { mfree_n,    mqreply_mfree },
    { pid_n,      mqreply_mqrc_create },
    { mymspace_n, mqreply_mymspace },
    { exit_n,     mqreply_exit }
  };
  rcmd = mqCmdParser(rbuffer, &cmd, &pid, &arg);
  if (!rcmd) { return(NULL); }

  if (!sbuffer) {
    sbuffer = malloc(SBUFFER_SZ+1);
  }
  memset(sbuffer, 0,SBUFFER_SZ+1);

  if (!pid) { return(NULL); }
  rpid = atol(pid);
  for (i=0; i < sizeof(cmd_callback)/sizeof(struct mqserver_); i++) {
    if (!strcmp(cmd, cmd_callback[i].cmd)) {
      if(cmd_callback[i].cmdfunc) {
        return(cmd_callback[i].cmdfunc(sbuffer,SBUFFER_SZ, rpid, arg));
      }
    }
  }
  return(NULL);
}
