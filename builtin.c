#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  MAXLINE  128

int
main(void)
{
	char	buf[MAXLINE];
	pid_t	pid;
	int		status;
	char *ptr;
	int   background;

	background = 1;

	printf(">> ");
	while (fgets(buf, MAXLINE, stdin) != NULL) {
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0; /* replace newline with null */

             if (strcmp(buf, "pwd") == 0) {   /* built-in command pwd */
	       ptr = getcwd(NULL, 0);
               printf("CWD = [%s]\n", ptr);
               free(ptr);
	     }
	     else {                           /* external command */
		if ((pid = fork()) < 0) {
			printf("fork error\n");
			exit(1);
		} else if (pid == 0) {		/* child */
			execlp(buf, buf, (char *)0);
			printf("couldn't execute: %s\n", buf);
			exit(127);
		}

		/* parent */
		if (! background) {
		  if ((pid = waitpid(pid, &status, 0)) < 0)
			printf("waitpid error\n");
                }
		else {
		  // save pid somewhere for later
                }
             }
	     pid = waitpid(pid, &status, WNOHANG);
	     printf(">> ");
	}
	exit(0);
}
