#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_HISTORY 50

void signal_handler(int sig){
  printf("\nsh> ");
  fflush(stdout);
}

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

int execute_piped_commands(char* input_line){
  char* commands[20];
  int num_cmds = 0;
  char* token = strtok(input_line, "|");

  while(token != NULL && num_cmds < 20){
    commands[num_cmds++] = token;
    token = strtok(NULL, "|");
  }

  int pipe_fds[2* (num_cmds - 1)];

  for (int i=0; i< num_cmds - 1; i++){
    if(pipe(pipe_fds + i*2) < 0){
      perror("pipe error");
      exit(EXIT_FAILURE);
    }
  }

  for (int i=0; i< num_cmds; i++) {
    pid_t pid = fork();
    if (pid == 0){
      char* args[100], *input_file = NULL, *output_file = NULL;
      int append = 0;

      parse_input_tokens(commands[i], args, &input_file, &output_file, &append);

      if (i > 0){
        // not the first command. pipe can be redirected
        dup2(pipe_fds[(i-1)*2], STDIN_FILENO);
      }else if (input_file) {
        int fd = open(input_file, O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);
      }

      if (i < num_cmds - 1) {
        // not the last one. pass the output to next
        dup2(pipe_fds[i*2 + 1], STDOUT_FILENO);
      }else if(output_file) {
        int fd = open(output_file, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }

      for (int j=0; j < 2 * (num_cmds -1); j++) {
        close(pipe_fds[j]);
      }

      execvp(args[0], args);
      exit(EXIT_FAILURE);
    }
  }

  for (int i=0; i< 2 * (num_cmds - 1); i++) {
    close(pipe_fds[i]);
  }

  int last_status = 0;
  for (int i = 0; i < num_cmds; i++) {
    int status;
    wait(&status);
    if(i == num_cmds - 1){
      last_status = status;
    }
  }

  return last_status;
}

int main() {
  // input buffer
  char buffer[1024];
  char* args[100];
  char* input_file, *output_file;
  int append;

  // history
  char *cmd_history[MAX_HISTORY];
  int history_count = 0;

  // emnei, shundor lage ig
  printf("Custom Shell Proj v0.1\n");

  signal(SIGINT, signal_handler);
  

  // actual repl
  while(1) {
    printf("sh> ");
    fflush(stdout);

    fgets(buffer, 1024, stdin);

    // exit command
    if (strncmp(buffer, "exit", 4) == 0) {
      break;
    }
    
    // history command
    if (strncmp(buffer, "history", 7) == 0) {
      for (int i=0; i<history_count; i++) {
        printf("%d. %s", i+1, cmd_history[i]);
      }
      continue;
    }

    if (buffer[0] == '\0') continue;

    // if valid input, update history
    // history is not persistent
    if (strnlen(buffer, 1024) > 0){
      // if the max lenght is reached, shift the array using memmove
      if(history_count == MAX_HISTORY) {
        free(cmd_history[0]);
        memmove(&cmd_history[0], &cmd_history[1], sizeof(char*) * (MAX_HISTORY - 1));
        history_count--;
      }
      cmd_history[history_count++] = strndup(buffer, 1024);
    }

    char* sc_command = strtok(buffer, ";");
    while(sc_command != NULL) {
      int run_next = 1;
      char* and_ptr;
      char* rest = sc_command;

      while((and_ptr = strstr(rest, "&&")) != NULL){
        *and_ptr = '\0';
        char* cmd = rest;
        while(*cmd == ' ' || *cmd == '\t') cmd++;

        if(*cmd != '\0' && run_next){
          pid_t pid = fork();
          if(pid == 0){
            int status = execute_piped_commands(cmd);
            exit(WEXITSTATUS(status));
          }else{
            int status;
            waitpid(pid, &status, 0);
            run_next = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
          }
        }

        rest = and_ptr + 2;
      }

      char* cmd = rest;
      while(*cmd == ' ' || *cmd == '\t') cmd++;

      if (*cmd != '\0' && run_next) {
        pid_t pid = fork();
        if (pid == 0) {
          execute_piped_commands(cmd);
          exit(0);
        }else {
          int status;
          waitpid(pid, &status, 0);
        }
      }

      sc_command = strtok(NULL, ";");

    }
    
  }
  return 0;
}
