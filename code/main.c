#include <stdio.h>
#include <stdlib.h>
#include "core.h"

#if OS_LINUX
#include <signal.h>
#include <unistd.h>
void sigint_handler(int signo)
{
char error_msg[] = "Interruption signal received. Finishing execution\n";

  write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
  _exit(EXIT_SUCCESS); 
}
int main(int arg_count, char *args[])
{
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, sigint_handler);
  app_entry_point(arg_count, args);
	return EXIT_SUCCESS;
}
#else
int main(int argc, char **argv)
{
  fprintf(stderr, "Not implemented.\n");  
}
#endif
