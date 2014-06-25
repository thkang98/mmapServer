#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sysexits.h>
#include "dlclient.h"

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE (!FALSE)
#endif

/* { signal handlers */
volatile unsigned int sigint_rcvd = FALSE;
volatile unsigned int sighup_rcvd = FALSE;
volatile unsigned int sigterm_rcvd = FALSE;
volatile unsigned int sigusr1_rcvd = FALSE;

typedef void signal_func(int);
static signal_func *set_signale_handler(int signo, signal_func *safunc);

signal_func *set_signal_handler(int signo, signal_func *safunc) {
  struct sigaction act, oact;

  act.sa_handler = safunc;
  sigemptyset(&act.sa_mask);
  if (signo == SIGALRM) {
    act.sa_flags |= SA_RESTART;
  }
  if (sigaction(signo, &act, &oact) < 0)
    return SIG_ERR;
  return oact.sa_handler;
}

void signal_handler(int sig) {

  pid_t pid;
  int status;

  switch(sig) {
  case SIGINT:
    sigint_rcvd = TRUE;
    printf("Rreceived SIGINT\n");
    break;
  case SIGHUP:
    sighup_rcvd = TRUE;
    printf("Rreceived HUP\n");
    break;
  case SIGTERM:
    sigterm_rcvd = TRUE;
    printf("Rreceived TERM\n");
    break;
  case SIGCHLD:
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0);
    break;
  default:
    break;
  }
  return;
}


int set_signal_handlers(void) {
  if (set_signal_handler(SIGPIPE, SIG_IGN) == SIG_ERR) {
    printf("Could not set \"SIGPIPE\" signal.\n");
    return(EX_OSERR);
  }
  if (set_signal_handler(SIGCHLD, SIG_IGN) == SIG_ERR) {
    printf("Could not set \"SIGCHLD\" signal.\n");
    return(EX_OSERR);
  }
  if (set_signal_handler(SIGINT, signal_handler) == SIG_ERR) {
    printf("Could not set \"SIGINT\" signal.\n");
    return(EX_OSERR);
  }
  if (set_signal_handler(SIGHUP, signal_handler) == SIG_ERR) {
    printf("Could not set \"SIGHUP\" signal.\n");
    return(EX_OSERR);
  }
  if (set_signal_handler(SIGTERM, signal_handler) == SIG_ERR) {
    printf("Could not set \"SIGTERM\" signal.\n");
    return(EX_OSERR);
  }
}
/* } signal handlers */

int main(int argc, char **argv) {

  set_signal_handlers();

  mqdl_client();

  return(0);
}
