#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void parse_input_tokens(char* input, char** args){
  char* token = strtok(input, " \t\n");
  int i = 0;
  while (token != NULL) {
    args[i++] = token;
    token = strtok(NULL, " \t\n");
  }
  args[i] = NULL;
}


void execute_command(char** args){

  pid_t pid = fork();
  
  if(pid == -1) {
    perror("could not run command");
    exit(EXIT_FAILURE);
  } else if(pid == 0) {

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

    parse_input_tokens(buffer, args);
    execute_command(args);
    //printf("%s", buffer);
  }
  return 0;
}
