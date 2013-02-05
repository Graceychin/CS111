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




void add_reuse(RESUSE *resp, RESUSE *temp){
  resp->start.tv_sec  += temp->start.tv_sec;	
  resp->start.tv_usec  += temp->start.tv_usec;	
  resp->elapsed.tv_sec  += temp->elapsed.tv_sec;	
  resp->elapsed.tv_usec  += temp->elapsed.tv_usec;	
  
  resp->ru.ru_utime.tv_sec  += temp->ru.ru_utime.tv_sec;	
  resp->ru.ru_utime.tv_usec += temp->ru.ru_utime.tv_usec;
  resp->ru.ru_stime.tv_sec  += temp->ru.ru_stime.tv_sec;	
  resp->ru.ru_stime.tv_usec += temp->ru.ru_stime.tv_usec;	
  resp->ru.ru_maxrss  +=temp->ru.ru_maxrss;
  resp->ru.ru_ixrss   +=temp->ru.ru_ixrss;
  resp->ru.ru_idrss   +=temp->ru.ru_idrss;
  resp->ru.ru_isrss   +=temp->ru.ru_isrss; 
  resp->ru.ru_minflt  +=temp->ru.ru_minflt;
  resp->ru.ru_majflt  +=temp->ru.ru_majflt;
  resp->ru.ru_nswap   +=temp->ru.ru_nswap;
  resp->ru.ru_inblock +=temp->ru.ru_inblock;
  resp->ru.ru_oublock +=temp->ru.ru_oublock;
  resp->ru.ru_msgsnd  +=temp->ru.ru_msgsnd;
  resp->ru.ru_msgrcv  +=temp->ru.ru_msgrcv; 
  resp->ru.ru_nsignals+=temp->ru.ru_nsignals;
  resp->ru.ru_nvcsw   +=temp->ru.ru_nvcsw;   
  resp->ru.ru_nivcsw  +=temp->ru.ru_nivcsw; 
}


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

void execute(command_t c, RESUSE *resp)
{
  RESUSE temp;
  resuse_start (&temp);
  
  pid_t cpid;
  cpid = fork();
 
    //child process executes the command
   if(cpid <0)
      error(1, 0, "Forking process failed");  
   else if(cpid == 0){
    //open up file descriptors
      if(c->input != NULL){
        freopen(c->input, "r", stdin);
      }
      if(c->output != NULL){
        freopen(c->output, "w", stdout);
      }

      if(execvp(c->u.word[0], c->u.word) < 0){
      	error(1, 0, "error executing command");
	  }	
      fclose(stdin);
      fclose(stdout);
  
    }
    
    //wait child
    int c_status;
    if (resuse_end (cpid, &temp) == 0)
      error (1, 0, "error waiting for child process");
    
    add_reuse(resp, &temp);
    c->status = c_status;
     return;
  
}

void execute_pipe(command_t c, RESUSE *resp)
{
  //calculate the number of pipes
  int n_pipes = 0;
  command_t temp = c;
  while(temp->type == PIPE_COMMAND){
    n_pipes++;
    temp = temp->u.command[0];
  } 
  
  pid_t *cpid_list = (pid_t*) checked_malloc(sizeof(pid_t) * (n_pipes+1));
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
      cpid_list[count] = cpid;

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
    int j=0;
    for (; j<count; j++){
      printf("child: %d\n", cpid_list[j]);
    
    }
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


int execute_command_type(command_t c, RESUSE *resp)
{
  switch(c->type){
    case AND_COMMAND:    
      //don't execute next command if first one fails
      if(execute_command_type(c->u.command[0], resp) > 0){
        c->status = command_status(c->u.command[0]);
      }else{
        c->status = execute_command_type(c->u.command[1], resp);
      }
      if(DEBUG){
        printf("AND:%d\n", c->status);
      } 
      break;

    case OR_COMMAND:
      //don't execute next command if first one already works
      if(execute_command_type(c->u.command[0], resp) == 0){
        c->status = 0;
      }else{
        c->status = execute_command_type(c->u.command[1], resp);
      }
      if(DEBUG){
        printf("OR:%d\n", c->status);
      }
      break;

    case PIPE_COMMAND:
      
      execute_pipe(c, resp);
      break;

    case SUBSHELL_COMMAND:
      c->status = execute_command_type(c->u.subshell_command, resp);
      break;

    case SIMPLE_COMMAND:
      execute(c, resp);
      break;

    case SEQUENCE_COMMAND:
      execute_command_type(c->u.command[0], resp);
      execute_command_type(c->u.command[1], resp);
      c->status = 0;
      break;
  }
  return c->status;
}

void
execute_command (command_t c, RESUSE *resp)
{
  execute_command_type(c, resp);
}
