
#include "get_path.h"

//int pid;
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);
//int is_builtin(char *input);
void cmd_print(char **args, int argsct);

#define PROMPTMAX 32
#define MAXARGS 10
