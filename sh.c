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

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;

  uid = getuid();
  password_entry = getpwuid(uid); // get passwd info
  homedir = password_entry->pw_dir; // Home directory to startout with
     
  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; 
  prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
    /* print your prompt */

    /* get command line and process */

    /* check for each built in command and implement */

     /*  else  program to exec */
    //{
       /* find it */
       /* do fork(), execve() and waitpid() */

      /* else */
        /* fprintf(stderr, "%s: Command not found.\n", args[0]); */
    //}
    printf(" %s[%s]> ", prompt, pwd);
    
    /* get command line and process */
    if (fgets(commandline, MAX_CANON, stdin) == NULL) {
      go = 0;
      continue;
    }
  
    /* Remove newline character if present */
    if (commandline[strlen(commandline) - 1] == '\n')
        commandline[strlen(commandline) - 1] = '\0';

    /* If nothing was entered, continue */
    if (strlen(commandline) == 0)
      continue;

    /* Parse the command line */
    command = strtok(commandline, " ");
    if (command == NULL)
      continue;

    /* Copy command into args[0] */
    args[0] = command;

    /* Parse arguments */
    argsct = 1;
    while ((arg = strtok(NULL, " ")) != NULL && argsct < MAXARGS) {
      args[argsct++] = arg;
    }
    args[argsct] = NULL; /* Null-terminate the argument list */

    cmd_print(args, argsct); 

    /* check for each built in command and implement */
    if (strcmp(args[0], "exit") == 0) {
      if (args[1] == NULL) {
        go = 0;
      } else {
        exit(atoi(args[1]));
      }
    } else if (strcmp(args[0], "which") == 0) {
      commandpath = which(args[1], pathlist);
      printf("%s\n", commandpath);
    } else if (strcmp(args[0], "list") == 0) {
      /* Handle the list command */
      if (args[1] == NULL) {
        /* If no directory specified, list the current directory */
        list(pwd);
      } else {
        /* List the specified directory */
        list(args[1]);
      }
    } else if (strcmp(args[0], "pwd") == 0) {
      /* Print working directory */
      printf("%s\n", pwd);
    } else if (strcmp(args[0], "cd") == 0) {
      /* Change directory command */
      if (args[1] == NULL) {
        // cd with no arguments goes to home directory 
        if (chdir(homedir) != 0) {
          perror("cd to home failed");
        } else {
          // Update current and old working directories 
          free(owd);
          owd = pwd;
          pwd = getcwd(NULL, PATH_MAX+1);
        }
      } else {
        if (chdir(args[1]) != 0) {
          perror("cd failed");
        } else {
          // Update current and old working directories 
          free(owd);
          owd = pwd;
          pwd = getcwd(NULL, PATH_MAX+1);
        }
      }
    } else if (strcmp(args[0], "pid") == 0) {
      pid_t pid = getpid();
      printf("%d\n", pid);
    } else if (strcmp(args[0], "prompt") == 0) {
      if (args[1] == NULL){
        continue;
      } else {
        prompt = args[1];
        int temp = strlen(prompt);
        prompt[temp + 1] = '\n';
      }
      
    } else if (strcmp(args[0], "printevn") == 0) {

    } else if (strcmp(args[0], "setevn") == 0) {
      
    } else { // Add more built-in commands as needed 
      /* program to exec */
      commandpath = which(args[0], pathlist);
      if (commandpath != NULL) {
          /* do fork(), execve() and waitpid() */
          pid_t pid = fork();
          if (pid == 0) {
              /* Child process */
              execve(commandpath, args, envp);
              /* If execve returns, there was an error */
              perror("execve failed");
          } else if (pid > 0) {
              /* Parent process */
              waitpid(pid, &status, 0);
          } else {
              /* Error forking */
              perror("fork failed");
          }
          free(commandpath);
      } else {
          fprintf(stderr, "%s: Command not found.\n", args[0]);
      }
    }
  }
  return 0;
} /* sh() */

char *which(char *command, struct pathelement *pathlist )
{
  /* loop through pathlist until finding command and return it.  Return
  NULL when not found. */
  char *path = malloc(PATH_MAX); // Allocate memory for the full path
  
  while (pathlist) {
    sprintf(path, "%s/%s", pathlist->element, command);
    if (access(path, X_OK) == 0) {
      return path;
    }
    pathlist = pathlist->next;
  }
   
  free(path); // Free memory if command wasn't found
  return NULL;

} /* which() */

void list ( char *dir )
{
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
  DIR *dp;
  struct dirent *entry;
    
  /* Open the directory */
  if ((dp = opendir(dir)) == NULL) {
    perror("opendir");
    return;
  }
    
  /* Read directory entries */
  while ((entry = readdir(dp)) != NULL) {
    printf("%s\n", entry->d_name);
  }
    
  /* Close the directory */
  closedir(dp);
} /* list() */

void cmd_print(char **args, int argsct) {
  const char *builtins[] = {"exit", "which", "list", "pwd", "cd", "pid", "prompt", "printenv", "setenv"};
  int temp = 0;  

  for (int i = 0; i < 9; i++) {
    if (strcmp(args[0], builtins[i]) == 0) {
      temp = 1;
    }
  }

  if (temp) {
    printf("Executing built-in command: %s", args[0]);
  } else {
    printf("Executing command: %s", args[0]);
  }

  for (int i = 1; i < argsct; i++) {
    printf(" %s", args[i]);
  }
  printf("\n");
}
