#include "sh.h"
#include <signal.h>
#include <stdio.h>

int main( int argc, char **argv, char **envp )
{
  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;  // Restart system calls (like fgets) instead of interrupting

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGTSTP, &sa, NULL);  // Ignore Ctrl-Z

  return sh(argc, argv, envp);
}

void sig_handler(int signal); 
