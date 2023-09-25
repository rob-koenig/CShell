#include <unistd.h>
#include <stdio.h>

int main(void)
{
  // char *argv[] = { "/bin/date", 0 };

  char *argv[] = { "/bin/ls", "*.c", 0 };

  // char *argv[] = { "/bin/ls", "11", "a.out", "exec", "mysh", 0 };

  execve(argv[0], &argv[0], NULL);

  // char *argv1[] = { "/usr/local/gnu/bin/gcc", "*.c", 0 };
  // execve(argv1[0], &argv1[0], NULL);
}
