#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <errno.h>
#include <sys/mman.h>

#include <fcntl.h>
#include "dlclient.h"

/* from myspace.h */
#define MMAP_SHARED_NAME       "/tmp/mmap_dlmalloc"

const char *rd_n = "rd";
const char *wr_n = "wr";
const char *pid_n = "pid";
const char *mymspace_n = "mymspace";
const char *mmalloc_n = "mmalloc";
const char *mfree_n = "mfree";
const char *exit_n = "exit";

static void *mymspace = NULL;
static size_t mymspace_sz = 0;

static int mqCmdParser(char *istr, char **cmd, char **arg1, char **arg2);

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

static char *mqreq_rd(char *sbuffer, int bsz, char *haddr, char *rdsz) {
  long ptr;
  unsigned long *mptr;
  long i, msz;

  if (!haddr) { return(NULL); }
  if (rdsz) {
    msz = atol(rdsz);
  } else { msz = 1; }
  if (msz > 128) { msz = 128; };
  if (hexstring_to_long(haddr, &ptr)) {
    mptr = (unsigned long *)ptr;
    if ((void *)mptr < mymspace) {
      printf("address must be higher or equal to \'mymspace\': %p\n", mymspace);
      return(NULL);
    }
    for(i=0; i<msz; i++, mptr++) {
      if ((i&3) == 0) { printf("\n%p: ", mptr); }
      // printf("%08ld ", *mptr);
      printf("%16p", (void *)*mptr);
    }
    printf("\n");
  }
}

static char *mqreq_wr(char *sbuffer, int bz, char *haddr, char *data) {
  long *mptr;
  long mda;
  if (!haddr) { return(NULL); }
  if (!data) { return(NULL); }
  mda = atol(data);
  if (hexstring_to_long(haddr, (long *)&mptr)) {
    printf("write address: %p <- %ld/%16p\n", mptr, mda, (void *)mda);
    *mptr = mda;
    printf("read  address: %p -> %ld/%16p\n", mptr, *mptr, (void *)*mptr);
  }
  return(NULL);
}

static char *mqreq_exit(char *sbuffer, int bsz, char *arg1, char *arg2) {
  snprintf(sbuffer, bsz, "%s:%d", exit_n, getpid());
  mqreq_send(sbuffer, 0);
}

static char *mqreq_mqrc_create(char *sbuffer, int bsz, char *arg1, char *arg2) {
  if (mqrc_open() == 0) {
    snprintf(sbuffer, bsz, "%s:%d", pid_n, getpid());
    mqreq_send(sbuffer, 0);
  }
  return(NULL);
}

static char *mqreq_mfree(char *sbuffer, int bsz, char *haddr, char *arg2) {
  long ptr;
  if (haddr) {
    hexstring_to_long(haddr, &ptr);
    snprintf(sbuffer, bsz, "%s:%d:%p", mfree_n, getpid(), (void *)ptr);
    mqreq_send(sbuffer, 0);
  }
  return(NULL);
}

static char *mqreq_mmalloc(char *sbuffer, int bsz, char *req_sz, char *arg)  {
  char *rbuffer = NULL;
  char *haddr, *arg1, *arg2;
  snprintf(sbuffer, bsz, "%s:%d:%s", mmalloc_n, getpid(), req_sz);
  rbuffer = mqreq_send(sbuffer, 1);
  mqCmdParser(rbuffer, &haddr, &arg1, &arg2);
  printf("%s = mmalloc(%s);\n", haddr, req_sz);
}

char *mount_mymspace(char *haddr, char *msz) {
static int dl_fd = -1;
static char *mm = MAP_FAILED;

  long val;

  if (!haddr) { return(NULL); }

  hexstring_to_long(haddr, &val);
  mymspace = (void *) val;

  if (!msz) { return(NULL); }
  mymspace_sz = (size_t) atol(msz);

  if (dl_fd <= 0) {
    printf("open %s shared file\n", MMAP_SHARED_NAME);
    if ((dl_fd = open(MMAP_SHARED_NAME, O_RDWR, 0666)) == -1) {
      printf("Fail to OPEN " MMAP_SHARED_NAME ", errno=%x\n", errno);
      return(NULL);
    }
  }
  if (mm == MAP_FAILED) {
    printf("kick of mmapping mymsapce:%p with size:%lu\n",
            mymspace, mymspace_sz);
    mm = (char *) mmap(mymspace, mymspace_sz, (PROT_READ|PROT_WRITE),
                       (MAP_SHARED), dl_fd, 0);
    if (mm == MAP_FAILED) {
        printf("%p = mmap(%p, %lu, ...) FAIL!\n", mm, mymspace, mymspace_sz);
    }
    printf("%p = mmap(mymspace:%p mymspace_sz: %lu)\n",
            mm, mymspace, mymspace_sz);
  }
}

static char *mqreq_mymspace(char *sbuffer, int bsz, char *arg1, char *arg2) {
  char *rbuffer;
  char *haddr, *msz, *arg;

  snprintf(sbuffer, bsz, "%s:%d", mymspace_n, getpid());
  rbuffer = mqreq_send(sbuffer, 1);
  if (rbuffer) {
    mqCmdParser(rbuffer, &haddr, &msz, &arg);
    return(mount_mymspace(haddr, msz));
  }
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

  if (istr == 0) { return 0; }
  if (strlen(istr) == 0) { return 0; }
  if (!cmd || !arg1 || !arg2) { return 0; }

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
  printf("cmd:%s arg1:%s arg2:%s\n", *cmd, *arg1, *arg2);
  return(i);
}

char *mqClientCmdExec(char *rbuffer) {
#define SBUFFER_SZ   128

  static char *sbuffer = NULL;

  char *cmd, *arg1, *arg2;
  int rcmd;
  int  i;

  typedef char *cmd_func_(char *sbuffer, int bsz, char *arg1, char *arg2);
  struct mqclient_ {
    const char *cmd;
    cmd_func_ *cmdfunc;
  };
  struct mqclient_ cmd_callback[] = {
    { rd_n,       mqreq_rd },
    { wr_n,       mqreq_wr },
    { mmalloc_n,  mqreq_mmalloc },
    { mfree_n,    mqreq_mfree },
    { pid_n,      mqreq_mqrc_create },
    { mymspace_n, mqreq_mymspace },
    { exit_n,     mqreq_exit }
  };
  rcmd = mqCmdParser(rbuffer, &cmd, &arg1, &arg2);
  if (!rcmd) { return(NULL); };

  if (!sbuffer) {
    sbuffer = malloc(SBUFFER_SZ+1);
  }
  memset(sbuffer, 0, SBUFFER_SZ+1);

  for (i=0; i < sizeof(cmd_callback)/sizeof(struct mqclient_); i++) {
    if (!strcmp(cmd, cmd_callback[i].cmd)) {
      if(cmd_callback[i].cmdfunc) {
        return(cmd_callback[i].cmdfunc(sbuffer, SBUFFER_SZ, arg1, arg2));
      }
    }
  }
  return(NULL);
}
