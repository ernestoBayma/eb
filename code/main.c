#include <stdio.h>
#include <stdlib.h>
#include "core.h"

#if OS_LINUX
#include <signal.h>
void sigint_handler(int signo)
{
  puts("Interruption signal received. Finishing execution");
  exit(EXIT_SUCCESS); 
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
