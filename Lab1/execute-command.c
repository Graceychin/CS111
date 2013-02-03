// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <stdio.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
static bool DEBUG = false;
extern char**environ;

int
command_status (command_t c)
{
  return c->status;
}

void free_pipe_list(cc_node_t* c){
  //cc_node_t temp = c;
  free(c);


}

//return a list of commands linked between pipes
//bottom left to top right
cc_node_t* get_pipe_list(command_t c, int num_pipes){
  cc_node_t* r = (cc_node_t*) checked_malloc(sizeof(cc_node_t) * (num_pipes+1));  
  int i = 0;
  //allocate
  for(; i<=num_pipes; i++){
    r[i] = (cc_node_t) checked_malloc(sizeof(struct cc_node));
  }
  
  i=0;
  while(c->type != SIMPLE_COMMAND){
    r[i]->c = c->u.command[1];
    c = c->u.command[0];
    i++;
  }
  //get left child 
  r[i]->c = c;
  
  //set pointer traversal
  for(; i>0; i--){
    r[i]->next = r[i-1];  
  }

  r[i]->next = NULL;
  
  return r;

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
      //printf("status:%d\n", c_status);
      return;
    }
  }else{
    error(1, 0, "Forking process failed");   
  }
}

void execute_pipe(command_t c)
{
  //calculate the number of pipes
  int n_pipes = 0;
  command_t temp = c;
  while(temp->type == PIPE_COMMAND){
    n_pipes++;
    temp = temp->u.command[0];
  } 

  //get a linked list of commands for piping
  cc_node_t* temp_list = get_pipe_list(c, n_pipes);
  cc_node_t pipe_c = temp_list[n_pipes];
  
  //0,1: read/write for 1st pipe, 2,3: read/write for 2nd pipe, and so on
  //start piping
  pid_t cpid;
  int count =0;
  int i;
  int pipe_fd[2];
  int new_fd[2];

  while(pipe_c){
    //if there's next command
    if(pipe_c->next){
      pipe(new_fd);
    }
    
    cpid = fork();
    
    if(cpid == 0){
      //if not first command (if there is a previous command)
      //redirect stdin to pipe in
      if(count !=0){
        dup2(pipe_fd[0], 0);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
      }
      
      //if not last commmand (there is a next command)
      //redirect stdout to pipe out
      if(pipe_c->next){
        close(new_fd[0]);
        dup2(new_fd[1], 1);
        close(new_fd[1]);
        
      }
      
      command_t cmd = pipe_c->c;
      if(DEBUG){
        printf("\n\nchild: %d\t", (int)getpid());
        printf("command: %s %s\n",cmd->u.word[0], cmd->u.word[1]);
      
      }
      if(cmd->input != NULL){
        freopen(cmd->input, "r", stdin);
      }

      if(cmd->output != NULL){
        freopen(cmd->output, "w", stdout);
      }
      
      if(execvp(cmd->u.word[0], cmd->u.word) < 0){
        error(1, 0, "error executing command");    
      }
      
      fclose(stdout);
      fclose(stdin);
    
    }else if(cpid >0){

      if(count !=0){
        close(pipe_fd[0]);
        close(pipe_fd[1]);
      }
      if(pipe_c->next){
        pipe_fd[0] = new_fd[0];
        pipe_fd[1] = new_fd[1];
      }
    }else{
       error(1, 0, "Forking process failed during piping");   
    }
    count++;
    pipe_c = pipe_c->next;
  
  }
  if(count>1){
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    int status;
    waitpid(cpid, &status, 0);
    c->status = status;
  }
  //fclose(stdin);
  //free memory
  free_pipe_list(temp_list);
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
      if(DEBUG){
        printf("AND:%d\n", c->status);
      } 
      break;

    case OR_COMMAND:
      //don't execute next command if first one already works
      if(execute_command_type(c->u.command[0]) == 0){
        c->status = 0;
      }else{
        c->status = execute_command_type(c->u.command[1]);
      }
      if(DEBUG){
        printf("OR:%d\n", c->status);
      }
      break;

    case PIPE_COMMAND:
      
      execute_pipe(c);
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
