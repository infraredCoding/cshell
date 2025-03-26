#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void parse_input_tokens(char* input, char** args, char** input_file, char** output_file, int* append){
  char* token = strtok(input, " \t\n");
  int i = 0;
  
  // reset for each command iteration
  *input_file = NULL;
  *output_file = NULL;
  *append = 0;

  while (token != NULL) {
    if (strncmp(token, "<", 1) == 0) {
      token = strtok(NULL, " \t\n");
      if (token) *input_file = token;
    }else if (strcmp(token, ">") == 0){
      token = strtok(NULL, " \t\n");
      if (token) *output_file = token;
    }else if(strncmp(token, ">>", 2) == 0){
      token = strtok(NULL, " \t\n");
      if (token) {
        *output_file = token;
        *append = 1;
      }
    }else {
      args[i++] = token;
    }
    token = strtok(NULL, " \t\n");
  }
  args[i] = NULL;
}


void execute_command(char** args, char* inp_file, char* out_file, int append){

  pid_t pid = fork();
  
  if(pid == -1) {
    perror("could not run command");
    exit(EXIT_FAILURE);
  } else if(pid == 0) {
    // child
    if (inp_file) {
      int fd = open(inp_file, O_RDONLY);
      dup2(fd, STDIN_FILENO);
      close(fd);
    }

    if (out_file) {
      int fd = open(out_file, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
      dup2(fd, STDOUT_FILENO);
      close(fd);
    }
    if (execvp(args[0], args) != -1) {
      perror("could not execute command");
      exit(EXIT_FAILURE);
    }
  }else {
    int status;
    waitpid(pid, &status, 0);
  }
}

int main() {
  // input buffer
  char buffer[1024];
  char* args[100];
  char* input_file, *output_file;
  int append;

  // emnei, shundor lage ig
  printf("Custom Shell Proj v0.1\n");
  

  // actual repl
  while(1) {
    printf("sh> ");
    fgets(buffer, 1024, stdin);

    // exit command
    if (strncmp(buffer, "exit", 4) == 0) {
      break;
    }

    parse_input_tokens(buffer, args, &input_file, &output_file, &append);
    execute_command(args, input_file, output_file, append);
    //printf("%s", buffer);
  }
  return 0;
}
