#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sh.h"

/**
 * sh() function:
 * input: 1 int (argc), 2 char** (argv, envp)
 * output: 1 int (return status)
 * description: main implimentation of the shell
 * special: this function allocates memory
 */
int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir, *projdir;
  struct pathelement *pathlist;

  uid = getuid();
  password_entry = getpwuid(uid); // get passwd info
  homedir = password_entry->pw_dir; // Home directory to startout with
     
  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  projdir = calloc(strlen(pwd) + 1, sizeof(char)); 
  memcpy(projdir, pwd, strlen(pwd)); // saves project directory for later
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; 
  prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
    /* print your prompt */
    printf(" %s [%s]> ", prompt, pwd);
    
    /* get command line and process */
    if (fgets(commandline, MAX_CANON, stdin) == NULL) {
      if (feof(stdin)) { // handle ctrl-d
        clearerr(stdin);
        printf("\n");
        continue;
      } else {
        go = 0;
      }
    }
  
    /* remove newline character if present */
    if (commandline[strlen(commandline) - 1] == '\n') {
      commandline[strlen(commandline) - 1] = '\0';
    }

    /* parse the command line */
    command = strtok(commandline, " ");
    if (command == NULL){
      continue;
    }

    /* copy command into args[0] */
    args[0] = command;

    /* parse arguments */
    argsct = 1;
    while ((arg = strtok(NULL, " ")) != NULL && argsct < MAXARGS) {
      args[argsct++] = arg;
    }
    args[argsct] = NULL; // null terminate the argument list

    cmd_print(args, argsct); 

    /* check for each built in command and implement */
    if (strcmp(args[0], "exit") == 0) {
      if (args[1] == NULL) { // exits
        printf("EC: 0\n");
        go = 0;
      } else { // exits with specified code
        printf("EC: %s\n", args[1]);
        go = 0;
      }
    } else if (strcmp(args[0], "which") == 0) {
      if (args[1] == NULL) { // error if no specified command
        perror("no specified command");
      } else { // executes which function 
        for (int i = 1; i < argsct; i++) {
          commandpath = which(args[i], pathlist);
          printf("%s\n", commandpath);
          free(commandpath);
        }
      }
    } else if (strcmp(args[0], "list") == 0) {
      if (args[1] == NULL) { // list current directory
        list(pwd);
      } else { // list specified directory
        for (int i = 1; i < argsct; i++) {
          list(args[i]);
        }
      }
    } else if (strcmp(args[0], "pwd") == 0) {
      printf("%s\n", pwd);
    } else if (strcmp(args[0], "cd") == 0) {
      if (args[1] == NULL) { // go to home directory
        if (chdir(homedir) != 0) {
          perror("cd to home failed");
        } else { // update current and old working directories
          free(owd);
          owd = pwd;
          pwd = getcwd(NULL, PATH_MAX+1);
        }
      } else if (strcmp(args[1], "-") == 0) { // goes back to old working directory
        if (chdir(projdir) != 0) {
          perror("cd to project failed");
        } else { // swap old and current working directories
          free(owd);
          owd = pwd;
          pwd = getcwd(NULL, PATH_MAX+1);
        }
      } else { // go to the specified directory
        if (chdir(args[1]) != 0) {
          perror("cd failed");
        } else { // update current and old working directories
          free(owd);
          owd = pwd;
          pwd = getcwd(NULL, PATH_MAX+1);
        }
      }
    } else if (strcmp(args[0], "pid") == 0) {
      pid_t pid = getpid();
      printf("%d\n", pid);
    } else if (strcmp(args[0], "prompt") == 0) {
      if (args[1] == NULL){ // no arguments prompts user for input
        printf("Enter new prompt prefix: ");
        if (fgets(prompt, PROMPTMAX, stdin) != NULL) { // uses user input
          int len = strlen(prompt);
          prompt[len - 1] = '\0';
        } else { // resets to blank
          prompt[0] = ' '; 
          prompt[1] = '\0';
        }
      } else { // uses argument in command 
        prompt = args[1];
      }
    } else if (strcmp(args[0], "printenv") == 0) {
      if (args[1] == NULL) { // no arguments prints whole enviorment
        printenv(envp);
      } else { // prints value of specified enviormental variable 
        for (int i = 1; i < argsct; i++) {
          char *value = getenv(args[i]);
          if (value) { // variable found
            printf("%s=%s\n", args[i], value);
          } else { // variable not found
            printf("No variable: %s\n", args[i]);
          }
        }
      }
    } else if (strcmp(args[0], "setenv") == 0) {
      if (argsct == 1) { // no arguments print the whole environment
      printenv(envp);
      } else if (argsct == 2) { // 1 argument sets empty variable
        if (setenv(args[1], "", 1) != 0) { // sets and returns non zero when error
          perror("setenv failed");
        } else { // set successful 
          printf("Set %s=%s\n", args[1], "");
          if (strcmp(args[1], "PATH") == 0) { // update pathlist as nececary
            pathlist = get_path();
          }
          if (strcmp(args[1], "HOME") == 0) { // update home directory as nececary
            homedir = getenv("HOME");
          }
        }
      } else if (argsct == 3) { // 2 arguments sets variable
        if (setenv(args[1], args[2], 1) != 0) { // sets and returns non zero when error
          perror("setenv failed");
        } else {  // set successful 
          printf("Set %s=%s\n", args[1], args[2]);
          if (strcmp(args[1], "PATH") == 0) { // update pathlist as nececary
            pathlist = get_path();
          }
          if (strcmp(args[1], "HOME") == 0) { // update home directory as nececary
            homedir = getenv("HOME");
          }
        }
      } else { // More than 2 args
        perror("Too many arguments");
      }
    } else {
      /* program to exec */
      commandpath = which(args[0], pathlist);
      /* find it */
      if (commandpath != NULL) {
        /* do fork(), execve() and waitpid() */
        pid_t pid = fork();
        if (pid == 0) { // child
          execve(commandpath, args, envp);
          perror("execve failed");
        } else if (pid > 0) { // parent
          waitpid(pid, &status, 0);
        } else { // error
          perror("fork failed");
        }
        free(commandpath);
      } else { // didnt find it :(
        fprintf(stderr, "%s: Command not found.\n", args[0]);
      }
    }
  }
  return 0;
} /* sh() */

/**
 * *which() function:
 * input: 1 char* (command), 1 pathelement* (pathlist)
 * output: 1 char*
 * description: finds and returns command in pathlist
 * special: this function allocates memory
 */
char *which(char *command, struct pathelement *pathlist )
{
  /* loop through pathlist until finding command and return it.  Return
  NULL when not found. */
  char *path = malloc(PATH_MAX); // allocate memory for the full path

  while (pathlist) {
    sprintf(path, "%s/%s", pathlist->element, command);
    if (access(path, X_OK) == 0) {
      return path;
    }
    pathlist = pathlist->next;
  }
   
  free(path); // free memory if command noy found
  return NULL;
} /* which() */

/**
 * list() function:
 * input: 1 char* (dir)
 * output: none
 * description: lists entries of given directory
 * special: none
 */
void list ( char *dir )
{
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
  DIR *dp;
  struct dirent *entry;
    
  if ((dp = opendir(dir)) == NULL) { // opens directory and catches possible error
    perror("opendir");
    return;
  }
    
  while ((entry = readdir(dp)) != NULL) { // prints directory entries
    printf("%s\n", entry->d_name);
  }
    
  closedir(dp); // closes directory 
} /* list() */

/**
 * cmd_print() function:
 * input: 1 int (argsct), 1 char** (args)
 * output: none
 * description: determines if command is builtin or not and prints commmand
 * special: none
 */
void cmd_print(char **args, int argsct) {
  const char *builtins[] = {"exit", "which", "list", "pwd", "cd", "pid", "prompt", "printenv", "setenv"}; // list of builtins 
  int temp = 0;  

  for (int i = 0; i < 9; i++) { // sees if input command it builtin
    if (strcmp(args[0], builtins[i]) == 0) {
      temp = 1;
    }
  }

  if (temp) { // message if builtin
    printf("Executing built-in command: %s", args[0]);
  } else { // message if not builtin
    printf("Executing command: %s", args[0]);
  }

  for (int i = 1; i < argsct; i++) { // prints all other arguments and next line
    printf(" %s", args[i]);
  }
  printf("\n");
}

/**
 * printenv() function:
 * input: 1 char** (envp)
 * output: none
 * description: prints enviorment variables
 * special: none
 */
void printenv(char **envp) {
  for (char **env = envp; *env != NULL; env++) {
    printf("%s\n", *env);
  }
}

