// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

extern char**environ;

int
command_status (command_t c)
{
  return c->status;
}

void execute(command_t c)
{
  pid_t cpid;
  cpid = fork();

  if(cpid >= 0){
    //child process executes the command
    if(cpid == 0){
    //open up file descriptors
      if(c->input != NULL){
        freopen(c->input, "r", stdin);
      }
      if(c->output != NULL){
        freopen(c->output, "w", stdout);
      }
      execvp(c->u.word[0], c->u.word);
      error(1, 0, "error executing command");
      fclose(stdin);
      fclose(stdout);
    }else{
    //parent process closes file descriptors after child executes and exits
      int c_status;
      waitpid(cpid, &c_status, 0);
      c->status = c_status;
      printf("status:%d\n", c_status);
      return;
    }
  }else{
    error(1, 0, "Forking process failed");   
  }
}

void execute_pipe(command_t c1, command_t c2)
{
  int fd[2];
  pid_t cpid;
 
  if(pipe(fd) == -1){
    error(1, 0, "error piping command");
  }

  cpid = fork();
 
  if(cpid >= 0){
    //child process executes the command
    if(cpid == 0){
    //open up file descriptors
      close(fd[0]);
      dup2(fd[1], 1);
      close(fd[1]);

      if(c1->input != NULL){
        freopen(c1->input, "r", stdin);
      }

      if(c1->output != NULL){
        freopen(c1->output, "w", stdout);
      }

      execvp(c1->u.word[0], c1->u.word);
      error(1, 0, "error executing command");
      fclose(stdout);
      fclose(stdin);
    }else{
    //parent process closes file descriptors after child executes and exits
      close(fd[1]);
      dup2(fd[0], 0);
      close(fd[0]);
     
      if(c2->input != NULL){
        freopen(c2->input, "r", stdin);
      }

      if(c2->output != NULL){
        freopen(c2->output, "w", stdout);
      }
     
      execvp(c2->u.word[0], c2->u.word);
      error(1, 0, "error executing command");
      fclose(stdout);
      fclose(stdin);
    }
  }else{
    error(1, 0, "Forking process failed");   
  }
  return;
}


int execute_command_type(command_t c)
{
  switch(c->type){
    case AND_COMMAND:    
      //don't execute next command if first one fails
      if(execute_command_type(c->u.command[0]) > 0){
        c->status = command_status(c->u.command[0]);
      }else{
        c->status = execute_command_type(c->u.command[1]);
      }
      printf("AND:%d\n", c->status);
      break;

    case OR_COMMAND:
      //don't execute next command if first one already works
      if(execute_command_type(c->u.command[0]) == 0){
        c->status = 0;
      }else{
        c->status = execute_command_type(c->u.command[1]);
      }
      printf("OR:%d\n", c->status);
      break;

    case PIPE_COMMAND:
      execute_pipe(c->u.command[0], c->u.command[1]);
      c->status = 0;
      break;

    case SUBSHELL_COMMAND:
      c->status = execute_command_type(c->u.subshell_command);
      break;

    case SIMPLE_COMMAND:
      execute(c);
      break;

    case SEQUENCE_COMMAND:
      execute_command_type(c->u.command[0]);
      execute_command_type(c->u.command[1]);
      c->status = 0;
      break;
  }
  return c->status;
}

void
execute_command (command_t c, bool time_travel)
{
  execute_command_type(c);
  //error (1, 0, "command execution not yet implemented");
}
