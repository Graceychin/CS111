// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <stdio.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/** Function Declaration **/
bool isSpecial(char c);
bool isInvalid(char c);
int isWord(char c);
command_stream_t init_command_stream();
command_t create_init_command(enum command_type type);
void skip_comment(char *s, int *p, const int size);
char* read_next_token(char *s, int *p, const int size);
command_t create_simple_command(char *s, int *p, const int size);
command_t create_special_command(char *s, int *p, const int size, enum command_type type, command_t left_c);
command_t create_general_command(char *s, int *p, const int size, bool sub_shell);


/** Global Variable **/
int line_num; //keep track of line number for error reports
static bool DEBUG = false;
extern int subp_num;

/** Structure Declaration **/


struct command_stream
{
  //pointer to command pointer
  struct cc_node* root;
  struct cc_node* last;
};

/** Auxilary Function **/
bool isSpecial(char c){
  return (c == '|' || c == '&' || c ==';'|| c ==')' || c =='(');
}

bool isInvalid(char c){
  return !isWord(c) && !isSpecial(c) && c!='\n' && c!=' ' && c!='#';

}


int isWord(char c){
        return isalpha(c) || isdigit(c) || (c == '!') || (c == '%') || (c == '+') || (c == ',')
                        || (c == '-') || (c == '.') || (c == '/') || (c == ':') || (c == '@') || (c == '^')
                        || (c == '_');
}

/** Function Definition **/
//return an initialized command stream
command_stream_t init_command_stream(){
  command_stream_t cst = (command_stream_t) checked_malloc(sizeof(struct command_stream));
  cst->root = NULL;
  cst->last = NULL;
  return cst;
}

//returns an initialized command
command_t create_init_command(enum command_type type){
  command_t cmd = (command_t)checked_malloc(sizeof(struct command));
  cmd->type = type;
  cmd->status = -1;
  cmd->input = NULL;
  cmd->output = NULL;
  return cmd;
}

//skip comment line
void skip_comment(char *s, int *p, const int size){
  if(DEBUG){
    printf("Skipping comment...\n");
 }
  for(;*p<size; (*p)++){
    if(s[*p] == '\n'){
      line_num++;
      if(DEBUG){
        printf("Comment skipped: %c\n", s[*p+1]);
      }
      return;
    }
  }
}

//retrieve the next word
//a word is defined as a string of characters before the next terminating character (space,tab)
char* read_next_token(char *s, int *p, const int size){
  //printf("%d: [%c]\n", *p, s[*p]);
  while((s[*p] == ' ' || s[*p] == '\t' || s[*p] == '#') && *p<size){
    if(s[*p] == '#'){
      skip_comment(s, p, size);
    }
    (*p)++;
  }
  if(DEBUG){
    printf("%d: [%c]\n", *p, s[*p]);
  }
  int e = *p;

  while(isWord(s[*p]) && *p<size){
    //incorrect words
    
    *p+=1;
  }
  
  /*if(!isWord(s[*p])){
      fprintf(stderr,"%d: invalid characters '%c'.\n", s[*p],line_num);
      exit(1);
    
    }*/
  int word_size = (*p)-e;
 
  //if there is no word then return null
  if(word_size == 0)
    return NULL;
  char *word = (char *) malloc(sizeof(char) * (word_size+1));
  int i = 0;
  for(; i< word_size; i++){
    word[i] = s[e+i]; 
  }
  word[word_size] = '\0';

  //printf("word: %s, size: %d\n", word, word_size);
  if(s[*p] == '\n' || isSpecial(s[*p]) || s[*p] == '<' || s[*p] == '>' || s[*p] == '#'){
    (*p)--;
  }

  return word;
}

//creates and returns a simple command
//NOTE: set p to the very last character of the simple command (just before the next special character and stuff)
command_t create_simple_command(char *s, int *p, const int size){
  if(DEBUG){
    printf("Creating simple command...\n");
  }
  command_t cmd = create_init_command(SIMPLE_COMMAND);

  int words_count = 0;
  int words_size = 1;
  cmd->u.word =  (char**) malloc( sizeof(char*) * words_size );
 
  for(;*p<size; (*p)++){
    char c = s[*p];
    
    if(DEBUG){
      printf("start, %d: [%c]\n", *p,c);
    }
    if(c == ' ' || c == '\t'){
      //printf("we hit spaces \n");
      continue;
    }else if(c == '\n'){
      //printf("WE HIT ANOTHER NEW LINE :)\n");
      line_num++;
      (*p)++;
      break;
    }else if( c == '<'){
      if(cmd->input){
        fprintf(stderr,"%d: More than one input IO redirection.\n", line_num);
        exit(1);
      }
      (*p)++; 
      cmd->input = read_next_token(s, p, size);
      if(cmd->input == NULL){
        fprintf(stderr,"%d: No file name for input redirect.\n", line_num);
        exit(1);
      }

      //maybe add error here check if input is NULL
    }else if(c  == '>'){
      if(cmd->output){
        fprintf(stderr,"%d: More than one output IO redirection.\n", line_num);
        exit(1);
      }
      (*p)++;
      cmd->output = read_next_token(s, p, size);
      //maybe add error here check if output is NULL
      if(cmd->output == NULL){
        fprintf(stderr,"%d: No file name for output redirect.\n", line_num);
        exit(1);
      }
 

    }else if(isWord(c)){
      char* w = read_next_token(s, p, size);
     
      //more words, reallocate the size
      if(words_count == words_size){
        words_size*=2;
        cmd->u.word = realloc( cmd->u.word, sizeof(char*) *  words_size );
      }
     
      cmd->u.word[words_count] = w;

      words_count++;
      cmd->u.word[words_count] = NULL;
      //printf("return from is word, %d: [%c][%c]\n", *p, s[*p], s[*p +1]);
     
    }else if(isInvalid(c)){
      fprintf(stderr,"%d: invalid characters '%c'.\n", line_num, c);
      exit(1);
    
    }else if(isSpecial(c)){
      (*p)++;
      break;
    //added skip comment
    }else if(c == '#'){
      skip_comment(s, p, size);
    }
  }

  //NOTE: something could be wrong here
  //TESTING
  if(DEBUG){
    printf("input: %s, output: %s, ", cmd->input, cmd->output);
      char **ww = cmd->u.word;
        printf ("[%s", *ww);
        while (*++ww)
          printf (", %s", *ww);
       printf("]\n");
  }
 
      

  //subtract one to match the next space (or special) character
  (*p)-=2;
  //printf("p value: %d, [%c][%c]\n",*p, s[*p],s[*p +1]);
  return cmd;
}

//creates and returns a special command
command_t create_special_command(char *s, int *p, const int size, enum command_type type, command_t left_c){
  if(DEBUG){
    printf("Creating special command...%d\n", type);
  }
  if(!left_c){
    fprintf(stderr,"%d: Tried creating special command without left command.\n", line_num);
    exit(1);
  }
  command_t cmd = create_init_command(type);
  
  subp_num++;
  //this gives pipes more precedence than && and ||
  if(type == PIPE_COMMAND && (left_c->type == AND_COMMAND || left_c->type == OR_COMMAND)){

    cmd->u.command[0] = left_c->u.command[1];
    left_c->u.command[1] = cmd;
  }else{
    //assign the simple command as left child to the special command
    cmd->u.command[0] = left_c;
  }
 
  for(; *p<size; (*p)++){
    char c = s[*p];
    if(c == ' ' || c == '\t'){
      continue;
   
    }else if(c =='\n' && (*p) != size-1){
      //printf("Line num: %d\n", line_num);
      line_num++;
      continue;
   
    }else if(isWord(c)){
      cmd->u.command[1] = create_simple_command(s, p, size);
      //check
      if(type == PIPE_COMMAND && (left_c->type == AND_COMMAND || left_c->type == OR_COMMAND)){
        return left_c;
      }
      return cmd;
    }else if(isInvalid(c)){
      fprintf(stderr,"%d: invalid characters '%c'.\n", line_num, c);
      exit(1);
    
    }else if(c == '('){
      //check
      (*p)+=1;
      command_t right_command = create_init_command(SUBSHELL_COMMAND);
      right_command->u.subshell_command = create_general_command(s, p, size, true);     
      cmd->u.command[1] = right_command;
      return cmd;
    }else if(c == ';' || c == '|' || c =='&' || c ==')'){
      fprintf(stderr,"%d: invalid characters following the special command.\n", line_num);
      exit(1);
    //added skip comment
    }else if(c == '#'){
      skip_comment(s, p, size);   
    }
  }
  //ERROR
  fprintf(stderr,"%d: No command following the special command.\n", line_num);
  exit(1);
}

command_t create_general_command(char *s, int *p, const int size, bool sub_shell){
  
  if(DEBUG){
   if(sub_shell)
     printf("Creating Subshell command..\n");
   else
     printf("Creating general command...\n");
  }
  command_t left_command = NULL;

  //enum command_type type;
  for(; *p<size; (*p)++){
    char c = s[*p];
    if(DEBUG){
      printf("%d: [%c][%c]\n", *p, c,s[*p +1]);
    }
    if(c == ' ' || c == '\t'){
      continue;
   
   
    }else if(isWord(c)){
      left_command = create_simple_command(s, p, size);
      
    }else if(isInvalid(c)){
      fprintf(stderr,"%d: invalid characters '%c'.\n", line_num, c);
      exit(1);
    
    }else if(c =='\n' && (*p) != size-1){
      line_num++;
      if(left_command){
        break;
     
      }else{
        continue;
      }     
      
    }else if(c == ';'){
      //A semicolon has to come after some command
      if(!left_command){
        fprintf(stderr,"%d: There has to be some commands before the token ';'.\n", line_num);
        exit(1);
      
      }
      //NOTE: This should handle the ; correctly within subshell
      if(sub_shell){
        (*p)+=1;
        left_command = create_special_command(s, p, size, SEQUENCE_COMMAND, left_command);
        
      }else{
        break;
      
      }

    }else if(c == '|'){
      //OR_COMMAND
      if(*p+1<size && s[*p+1] == '|'){
        *p+=2;
        left_command = create_special_command(s, p, size, OR_COMMAND, left_command);
     
      //PIPE COMMAND
      }else{
        *p+=1;
        left_command = create_special_command(s, p, size, PIPE_COMMAND, left_command);
      }
   
    }else if(c =='&'){
      //AND_COMMAND <- problem when theres a # after &
      if(*p+1<size && s[*p+1] == '&'){
        *p+=2;
        left_command = create_special_command(s, p, size, AND_COMMAND, left_command);
       
      }else{
        fprintf(stderr,"%d: invalid & character.\n", line_num);
        exit(1);
      }
   
    //check
    }else if(c == '('){
      *p+=1;
      left_command = create_init_command(SUBSHELL_COMMAND);
      left_command->u.subshell_command = create_general_command(s, p, size, true);
     
   
    }else if(c == ')'){
      if(!sub_shell){
        fprintf(stderr,"%d: an unmatched ) character for closing subshell command'.\n", line_num);
        exit(1);
       
      }else{
        return left_command;
      }
    //added skip comment
    }else if(c == '#'){
      skip_comment(s, p, size);   
      }      
  }
 
  if(sub_shell){
      fprintf(stderr,"%d: The subshell command is not closed with ')'.\n", line_num);
      exit(1);
 
  }
  
  return left_command;
}


command_stream_t
make_command_stream ( char* command_buffer)
{
  //get command size
 
  command_stream_t stream= init_command_stream();
  //parse the script
  int p;
  int size =0;
  while(command_buffer[size]!='\0'){
    size++;
    if(DEBUG)
      printf("[%d]: [%c]\n", size, command_buffer[size]);
  }
  printf("size: %d\n", size);
  line_num = 1;
  //start pos of current command
  for(p = 0; p < size; p++){
    //printf("Parsing commands..\n");
    command_t cmd = create_general_command(command_buffer, &p, size, false);
    
    if(cmd){
       subp_num++;
      cc_node_t cnode = (cc_node_t) checked_malloc(sizeof(struct cc_node));
      cnode->next = NULL;
      cnode->c = cmd;
      
      //first node
      if(!stream->root){
        stream->root = cnode;
        stream->last = cnode; 
     
      }else{
        stream->last->next = cnode;
        stream->last = cnode;
      }
    }
  }
 
  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
  if(s->root){
    //delete the node
    command_t cmd = s->root->c;
    cc_node_t temp = s->root;
    s->root = s->root->next;
    free(temp);
    return cmd;
  }
  return NULL;
}
