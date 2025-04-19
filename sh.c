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
#include <ctype.h>
#include "sh.h"

// global variable to store last exit code
char ext_code[2] = "0";

/**
 * sh() function:
 * input: 1 int (argc), 2 char** (argv, envp)
 * output: 1 int (return status)
 * description: main implementation of the shell
 * special: this function allocates memory
 */
int sh(int argc, char **argv, char **envp)
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  char **sub_args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir, *projdir;
  struct pathelement *pathlist;
  int cond_ex = 1;
  FILE *script_file = NULL;

  uid = getuid();
  password_entry = getpwuid(uid);   // get passwd info
  homedir = password_entry->pw_dir; // Home directory to startout with
     
  if ((pwd = getcwd(NULL, PATH_MAX+1)) == NULL)
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

  char *shell_path = argv[0]; // save argv[0] for $0 substitution
  if (getenv("ACC") == NULL) { // initialize ACC
    setenv("ACC", "0", 1);
  }

  if (argc > 1) { // run commands from file
    script_file = fopen(argv[1], "r");
  }

  while (go)
  {
    if (script_file) { // get command from script or stdin
      if (fgets(commandline, MAX_CANON, script_file) == NULL) {
        fclose(script_file);
        return atoi(ext_code);
      }
    } else {
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
          continue;
        }
      }
    }
  
    /* remove newline character if present */
    if (commandline[strlen(commandline) - 1] == '\n') {
      commandline[strlen(commandline) - 1] = '\0';
    }

    /* check for conditional execution */
    cond_ex = 1;
    if (commandline[0] == '?') {
      if (strcmp(ext_code, "0") != 0) { // run if last exit code was 0
        cond_ex = 0;
      }
      memmove(commandline, commandline + 1, strlen(commandline)); // remove ?
    }
    
    if (!cond_ex) {
      continue;
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

    /* variable substitution */
    for (i = 0; i < argsct; i++) {
      sub_args[i] = var_sub(args[i], shell_path);
    }
    sub_args[argsct] = NULL;

    /* print command if NOECHO not set */
    if (getenv("NOECHO") == NULL || strlen(getenv("NOECHO")) == 0) {
      cmd_print(sub_args, argsct);
    }

    /* check for each built in command and implement */
    if (strcmp(sub_args[0], "exit") == 0) {
      if (sub_args[1] == NULL) { // exits
        printf("EC: 0\n");
        strcpy(ext_code, "0");
        if (script_file) fclose(script_file);
        go = 0;
      } else { // exits with specified code
        printf("EC: %s\n", sub_args[1]);
        strcpy(ext_code, sub_args[1]);
        if (script_file) fclose(script_file);
        go = 0;
      }
    } else if (strcmp(sub_args[0], "which") == 0) {
      if (sub_args[1] == NULL) { // error if no specified command
        perror("no specified command");
        strcpy(ext_code, "1");
      } else { // executes which function
        for (int i = 1; i < argsct; i++) {
          commandpath = which(sub_args[i], pathlist);
          printf("%s\n", commandpath);
          free(commandpath);
          strcpy(ext_code, "0");
        }
      }
    } else if (strcmp(sub_args[0], "list") == 0) {
      if (args[1] == NULL) { // list current directory
        list(pwd);
      } else { // list specified directory
        for (int i = 1; i < argsct; i++) {
          list(args[i]);
        }
      }
    } else if (strcmp(sub_args[0], "pwd") == 0) {
      printf("%s\n", pwd);
      strcpy(ext_code, "0");
    } else if (strcmp(sub_args[0], "cd") == 0) {
      if (sub_args[1] == NULL) { // go to home directory
        if (chdir(homedir) != 0) {
          perror("cd to home failed");
          strcpy(ext_code, "1");
        } else { // update current and old working directories
          free(owd);
          owd = pwd;
          pwd = getcwd(NULL, PATH_MAX+1);
          strcpy(ext_code, "0");
        }
      } else if (strcmp(sub_args[1], "-") == 0) { // goes back to old working directory
        if (chdir(owd) != 0) {
          perror("cd to previous dir failed");
          strcpy(ext_code, "1");
        } else { // swap old and current working directories
          free(owd);
          owd = pwd;
          pwd = getcwd(NULL, PATH_MAX+1);
          strcpy(ext_code, "0");
        }
      } else { // go to the specified directory
        if (chdir(sub_args[1]) != 0) {
          perror("cd failed");
          strcpy(ext_code, "1");
        } else { // update current and old working directories
          free(owd);
          owd = pwd;
          pwd = getcwd(NULL, PATH_MAX+1);
          strcpy(ext_code, "0");
        }
      }
    } else if (strcmp(sub_args[0], "pid") == 0) {
      pid_t pid = getpid();
      printf("%d\n", pid);
      strcpy(ext_code, "0");
    } else if (strcmp(sub_args[0], "prompt") == 0) {
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
    } else if (strcmp(sub_args[0], "printenv") == 0) {
      if (sub_args[1] == NULL) { // no arguments prints whole environment
        printenv(envp);
        strcpy(ext_code, "0");
      } else { // prints value of specified environmental variable
        for (int i = 1; i < argsct; i++) {
          char *value = getenv(sub_args[i]);
          if (value) { // variable found
            printf("%s=%s\n", sub_args[i], value);
            strcpy(ext_code, "0");
          } else { // variable not found
            printf("No variable: %s\n", sub_args[i]);
            strcpy(ext_code, "1");
          }
        }
      }
    } else if (strcmp(sub_args[0], "setenv") == 0) {
      if (argsct == 1) { // no arguments print the whole environment
        printenv(envp);
        strcpy(ext_code, "0");
      } else if (argsct == 2) { // 1 argument sets empty variable
        if (setenv(sub_args[1], "", 1) != 0) { // sets and returns non zero when error
          perror("setenv failed");
          strcpy(ext_code, "1");
        } else { // set successful
          printf("Set %s=%s\n", sub_args[1], "");
          strcpy(ext_code, "0");
          if (strcmp(sub_args[1], "PATH") == 0) { // update pathlist if PATH changed
            struct pathelement *tmp, *head = pathlist;
            while (head != NULL) {
              tmp = head;
              head = head->next;
              free(tmp);
            }
            pathlist = get_path(); // get updated PATH
          }
          if (strcmp(sub_args[1], "HOME") == 0) { // update homedir if HOME changed
            homedir = getenv("HOME");
          }
        }
      } else if (argsct == 3) { // 2 arguments sets variable
        if (setenv(sub_args[1], sub_args[2], 1) != 0) { // sets and returns non zero when error
          perror("setenv failed");
          strcpy(ext_code, "1");
        } else { // set successful
          printf("Set %s=%s\n", sub_args[1], sub_args[2]);
          strcpy(ext_code, "0");
          if (strcmp(sub_args[1], "PATH") == 0) { // update pathlist if PATH changed
            struct pathelement *tmp, *head = pathlist;
            while (head != NULL) {
              tmp = head;
              head = head->next;
              free(tmp);
            }
            pathlist = get_path(); // get updated PATH
          }
          if (strcmp(sub_args[1], "HOME") == 0) { // update homedir if HOME changed
            homedir = getenv("HOME");
          }
        }
      } else { // more than 2 args
        perror("setenv failed");
        strcpy(ext_code, "1");
      }
    } else if (strcmp(sub_args[0], "addacc") == 0) {
      addacc(sub_args[1]);
      strcpy(ext_code, "0");
    } else {
      /* program to exec */
      if (sub_args[0][0] == '.' && sub_args[0][1] == '/') {
        commandpath = strdup(sub_args[0]);
      } else {
        commandpath = which(sub_args[0], pathlist);
      }
      /* find it */
      if (commandpath != NULL) {
        /* do fork(), execve() and waitpid() */
        pid_t pid = fork();
        if (pid == 0) { // child
          execve(commandpath, sub_args, envp);
          perror("execve failed");
        } else if (pid > 0) { // parent
          waitpid(pid, &status, 0);
        } else { // error
          perror("fork failed");
          strcpy(ext_code, "1");
        }
        free(commandpath);
      } else { // didn't find it :(
        fprintf(stderr, "%s: Command not found.\n", sub_args[0]);
        strcpy(ext_code, "1");
      }
    }

    /* free substituted arguments */
    for (i = 0; i < argsct; i++) {
      if (sub_args[i]) free(sub_args[i]);
      sub_args[i] = NULL;
    }
  }
  
  free(prompt);
  free(commandline);
  free(projdir);
  free(owd);
  free(pwd);
  free(args);
  free(sub_args);
  
  struct pathelement *tmp, *head = pathlist;
  while (head != NULL) {
    tmp = head;
    head = head->next;
    free(tmp);
  }
  
  return atoi(ext_code);
} /* sh() */

/**
 * *which() function:
 * input: 1 char* (command), 1 pathelement* (pathlist)
 * output: 1 char*
 * description: finds and returns command in pathlist
 * special: this function allocates memory
 */
char *which(char *command, struct pathelement *pathlist)
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
   
  free(path); // free memory if command not found
  return NULL;
} /* which() */

/**
 * list() function:
 * input: 1 char* (dir)
 * output: none
 * description: lists entries of given directory
 * special: none
 */
void list(char *dir)
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
 * description: determines if command is builtin or not and prints command
 * special: none
 */
void cmd_print(char **args, int argsct) {
  if (args[0] == NULL) return;
  
  const char *builtins[] = {"exit", "which", "list", "pwd", "cd", "pid", "prompt", "printenv", "setenv", "addacc"}; // list of builtins
  int temp = 0;

  for (int i = 0; i < 10; i++) { // sees if input command is builtin
    if (strcmp(args[0], builtins[i]) == 0) {
      temp = 1;
      break;
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
 * description: prints environment variables
 * special: none
 */
void printenv(char **envp) {
  for (char **env = envp; *env != NULL; env++) {
    printf("%s\n", *env);
  }
}

/**
 * var_sub() function:
 * input: 1 char* (arg), 1 char* (argv0)
 * output: 1 char* (substituted string)
 * description: substitutes environment variables in argument
 * special:
 */
 char* var_sub(char* arg, char* argv0) {
  if (arg == NULL) {
    return NULL;
  }
  
  if (arg[0] == '$') { // if arg starts with $
    if (strcmp(arg, "$0") == 0) { // arg is $0
      return strdup(argv0);
    } else if (strcmp(arg, "$?") == 0) { // arg is $?
      return strdup(ext_code);
    }
    
    char* env_name = arg + 1;
    char* env_value = getenv(env_name);
    
    if (env_value != NULL) {
      return strdup(env_value);
    } else {
      return strdup(""); // var not found
    }
  }
  
  return strdup(arg);
}

/**
 * addacc() function:
 * input: 1 char* (arg)
 * output: none
 * description: adds given value to ACC environment variable
 * special: none
 */
void addacc(char* arg) {
  int inc = 1;
  if (arg != NULL) {
    inc = atoi(arg); // parse arg as int
  }
  
  char* acc_str = getenv("ACC");
  int acc_val = 0;
  
  if (acc_str != NULL) { // parse current ACC if exists
    acc_val = atoi(acc_str);
  }
  
  acc_val += inc;
  
  char new_acc[32];
  sprintf(new_acc, "%d", acc_val);
  setenv("ACC", new_acc, 1); // set var
}