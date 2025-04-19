
#include "get_path.h"

int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);
void cmd_print(char **args, int argsct);
char *var_sub(char *arg, char *argv0);
void addacc(char *arg);

#define PROMPTMAX 32
#define MAXARGS 10
