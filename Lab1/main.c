// UCLA CS 111 Lab 1 main program
#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h> //
#include <sys/wait.h>//
#include <sys/types.h>//
#include <unistd.h>//


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

  printf("TOTAL: %d\n", total_cmd);
  
  if(time_travel){
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
    
    int i;
    //list for child process id. Each index represent the corresponding complete command
    // -1: executed & one
    //  0: hasn't been executed
    // >0: child executing
    pid_t *cpid_list = (pid_t*) checked_malloc(sizeof(pid_t) * total_cmd);
    for(i=0; i<total_cmd; i++){
      cpid_list[i] = 0;
    }
    bool done = false;
    //while there are still commands waiting to be run
    while(!done){
      cc_node_t temp_cmd = get_root(command_stream);
      int c_count = 0;
      while(temp_cmd){
        pid_t cpid;
          
        //fork and execute commands with no dependency problem and that the command hasn't been run
        if(cpid_list[c_count] ==0 && no_dependency(temp_cmd)){
           cpid = fork();
           //child execute the command
           if(cpid == 0){
            execute_command (temp_cmd->c, time_travel);
            exit(0); //child exit
           }else if(cpid >0){
            //parent add child pid to the global array
            cpid_list[c_count] = cpid;

           }else{
            error(1, 0, "Forking process failed");  
           }
        
        }
        temp_cmd = temp_cmd->next;
        c_count++;
      
      }
      //wait for child
      for(i=0; i<total_cmd; i++){
        //if the child process is runnign the ith command
        if(cpid_list[i] >0){
          int status;
          waitpid(cpid_list[i], &status, 0);
          //update the dependency list (remove the ith row or something)
          update_dependency(i);
          //set the cpid to -1
          cpid_list[i] = -1;
        }
      
      }
      
      //if there are no more command
      done =  true;
      for(i=0; i<total_cmd; i++){
        if(cpid_list[i]==0){
          done = false;
        }
      
      }
    }
  
  
  
  }
 /* }else{
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
  }*/

  return print_tree || !last_command ? 0 : command_status (last_command);
}
