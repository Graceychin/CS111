// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h> //
#include <sys/wait.h>//
#include <sys/types.h>//
#include <unistd.h>//

#include "alloc.h"//
#include "command-internals.h" //
#include "command.h"

static char const *program_name;
static char const *script_name;

//static variables
extern struct depend **depend_list;
extern int total_cmd;
extern char **file_list;
extern int total_file;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  total_cmd = 0;//
  total_file = 0;//
  int command_number = 1;
  bool print_tree = false;
  bool time_travel = false;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "pt"))
      {
      case 'p': print_tree = true; break;
      case 't': time_travel = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);

  command_t last_command = NULL;
  command_t command;

  if(!time_travel){
    while ((command = read_command_stream (command_stream)))
    {
      if (print_tree){
	    printf ("# %d\n", command_number++);
	    print_command (command);
	  }else{
	    last_command = command;
	    execute_command (command, time_travel);
        /*
        int i,j;
        for(i=0;i<total_cmd;i++){
          printf("row: %d\n", i);
          for(j=0;j<total_file;j++){
            printf("input: %d, ", depend_list[i][j].input);
            printf("output: %d, ", depend_list[i][j].output);
          }  
          printf("\n");
        }
        */
	  }
    }
  }else{
    int counter = 0;
    while((command = read_command_stream(command_stream))){
      counter++;
      last_command = command;
      pid_t cpid;
      cpid = fork();
      if(cpid == 0){
        continue;
      }else if(cpid > 0){
        execute_command(command, time_travel);
        int c_status;
	    waitpid(cpid, &c_status, 0);
        break;
      }else{
        error(1, 0, "Forking process failed");	
      }
    }
  }

  return print_tree || !last_command ? 0 : command_status (last_command);
}
