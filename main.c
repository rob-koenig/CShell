#include "sh.h"
#include <signal.h>
#include <stdio.h>

/**
 * sig_handler() function:
 * input: none
 * output: none
 * description: handels external signals 
 * special: interupts signals 
 */
void sig_handler(){
  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGTSTP, &sa, NULL);
} 

/**
 * main() function:
 * input: 1 int (argc), 2 char** (argv, envp)
 * output: 1 int (return status)
 * description: main
 * special: this function calls others that allocates memory
 */
int main( int argc, char **argv, char **envp )
{
  sig_handler();
  return sh(argc, argv, envp);
}
