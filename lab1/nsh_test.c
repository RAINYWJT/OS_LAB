#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

#define MAX_BG_TASKS 64
#define MAX_PATHS 64
#define MAX_PATH_LENGTH 256
#define HISTORY_LEN 20

char history[HISTORY_LEN][1024];
int history_count = 0;

char error_message[28] = "An error has occurred\n"; 
char PATH[MAX_PATHS][MAX_PATH_LENGTH] = {"/bin"}; 
int path_count = 1; 

typedef struct bg_task{
    pid_t pid;
    char cmd[1024];
}bg_task;

bg_task bg_tasks[MAX_BG_TASKS]; 
int bg_task_count = 0; 

////////////////////////////////////////////////////////////////// start bulit in pro
void add_to_history(char *cmd) {
    int index = history_count % HISTORY_LEN;
    strncpy(history[index], cmd, 1024);
    history_count++;
}

char* get_history(int history_index) {
    if (history_count == 0 || history_index < 0 || history_index >= history_count) {
        return NULL;
    }
    return history[(history_count - history_index - 1) % HISTORY_LEN];
}

void watching_history() {
    for (int i = 1; i <= 20; i++) { 
        char* cmd = get_history(i);
        if (cmd != NULL) {
            printf("History command: %s\n", cmd); 
        }
    }
}

void execute_exit() {
    exit(0);
}

void execute_cd(char* dir) {
    char original_dir[1024];
    if (getcwd(original_dir, sizeof(original_dir)) == NULL) {
        printf("cd_1:An error has occurred\n");
        fflush(stdout);
        return;
    }
    if (strcmp(dir, "~") == 0) {
        dir = getenv("HOME");
        if (dir == NULL) {
            printf("cd_2:An error has occurred\n");
            fflush(stdout);
            return;
        }
    }
    if (chdir(dir) != 0) {
        printf("cd_3:An error has occurred\n");
        fflush(stdout);
    } else {
        char current_dir[1024];
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            printf("cd_4:An error has occurred\n");
            fflush(stdout);
            return;
        }
        if (strcmp(current_dir, "/") == 0) {
            if (chdir(original_dir) != 0) {
                printf("cd_5:An error has occurred\n");
                fflush(stdout);
            }
        }
    }
}

void execute_paths(char** paths, int count) {
    if (count == 0) {
        for (int i = 0; i < path_count; i++) {
            printf("%d\t%s\n", i + 1 , PATH[i]);
        }
    } else {
        path_count = count; 
        for (int i = 0; i < count; i++) {
            strncpy(PATH[i], paths[i], MAX_PATH_LENGTH);
            PATH[i][MAX_PATH_LENGTH - 1] = '\0'; 
        }
    }
}
//paths ./b\nhello
////////////////////////////////////////////////////////////////// start bg
void add_bg_task(pid_t pid, char* cmd) {
    // printf("Adding background task: PID %d, Command %s\n", pid, cmd);
    if (bg_task_count < MAX_BG_TASKS) {
        bg_tasks[bg_task_count].pid = pid;
        strncpy(bg_tasks[bg_task_count].cmd, cmd, sizeof(bg_tasks[bg_task_count].cmd));
        bg_task_count++;
    } else {
        printf("add_task:An error has occurred\n");
        fflush(stdout);
    }
}

void remove_bg_task(pid_t pid) {
    for (int i = 0; i < bg_task_count; i++) {
        if (bg_tasks[i].pid == pid) {
            for (int j = i; j < bg_task_count - 1; j++) {
                bg_tasks[j] = bg_tasks[j + 1];
            }
            bg_task_count--;
            break;
        }
    }
}

void execute_bg() {
    for (int i = 0; i < bg_task_count; i++) {
        printf("%d\t%d\t%s\n", i + 1, bg_tasks[i].pid, bg_tasks[i].cmd);
    }
    // printf("yes\n");
}

void check_bg_tasks() {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_bg_task(pid);
    }
}

////////////////////////////////////////////////////////////////// end bulit in pro
void execute_external_command(char *arguments[], int arg_count, int background, char* input) {
    pid_t pid;
    pid = fork();
    if (pid == -1) {
        printf("An error has occurred\n");
        fflush(stdout);
    } else if (pid == 0) {
        if (execvp(arguments[0], arguments) == -1) {
            //printf("ext_2:An error has occurred\n");
            fflush(stdout);
            _exit(EXIT_FAILURE); 
        }
        //printf("yes!\n");
    } else {
        if (!background) {
            int status;
            waitpid(pid, &status, WUNTRACED);
            //printf("%d\n",WIFEXITED(status));
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    printf("An error has occurred\n");
                
            }
        } else {
            // printf("Started background job %d\n", pid);
            add_bg_task(pid , input);
        }
    }
}



////////////////////////////////////////////////////////////////// start pipeline command
void execute_pipeline_commands(char *pipeline_commands[], int num_pipeline_commands) {
    int i;
    int in_fd = 0;
    int fd[2]; // 0 read, 1 write
    for (i = 0; i < num_pipeline_commands; i++) {
        if (i < num_pipeline_commands - 1) {
            if (pipe(fd) == -1) {
                printf("pipe_1:An error has occurred\n");
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
        }
        pid_t pid = fork();
        if (pid == -1) {
            printf("pipe_2:An error has occurred\n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // child
            if (i < num_pipeline_commands - 1) {
                close(fd[0]); 
                dup2(fd[1], STDOUT_FILENO); 
                close(fd[1]);
            }
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO); 
                close(in_fd);
            }
            char *args[] = { "/bin/sh", "-c", pipeline_commands[i], NULL };
            execvp(args[0], args);
            printf("pipe_3:An error has occurred\n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        } else { // father
            int status;
            waitpid(pid, &status, WUNTRACED);
            // wait(NULL); 
            if (in_fd != 0) {
                close(in_fd);
            }
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                printf("pipe_4:An error has occurred\n");
            }
            if (i < num_pipeline_commands - 1) {
                close(fd[1]); 
                in_fd = fd[0];
            }
        }
    }
}

////////////////////////////////////////////////////////////////// two ways to move blanks
char* remove_spaces(const char* input) {
    char* output = malloc(strlen(input) + 1);
    if (output) {
        int j = 0;
        for (int i = 0; input[i]; ++i) {
            if (!isspace((unsigned char)input[i])) {
                output[j++] = input[i];
            }
        }
        output[j] = '\0';
    }
    return output;
}

char* remove_blanks(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0)  
        return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)){
        end--;
    }
    end[1] = '\0';
    return str;
}

////////////////////////////////////////////////////////////////// end pipeline command

void execute_command_with_redirection(char *command, char *filepath){
    // printf("%s\n",filepath);
    char *args[64];
    int arg_count = 0;
    args[arg_count++] = strtok(command, "\n");
    while ((args[arg_count] = strtok(NULL, " \n")) != NULL) {
        arg_count++;
    }
    args[arg_count] = NULL;
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644); // 见linux的打开文件方式
    // printf("%d\n",fd);
    if (fd == -1){
        printf("red_1:An error has occurred\n");
        fflush(stdout);
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        printf("red_2:An error has occurred\n");
        fflush(stdout);
    } 
    else if (pid == 0){ // child 
        if (dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1) {
            printf("red_3:An error has occurred\n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        close(fd); 
        execvp(args[0], args);
        printf("red_4:An error has occurred\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    else{ // father
        close(fd);
        wait(NULL);
    }
}

////////////////////////////////////////////////////////////////// end redirection

int pre_execute_commands(char* input){
    char *temp_ptr = input;
    char* no_spaces = remove_spaces(temp_ptr);
    
    while (*no_spaces) {
        if (*no_spaces == '<'){
            printf("pre_excm_1:An error has occurred\n");
            fflush(stdout);
            return 1;
        }
        if (*no_spaces == '|' || *no_spaces == ';' || *no_spaces == '>') {
            no_spaces++;
            if (*no_spaces == '|' || *no_spaces == ';' || *no_spaces == '>') {
                printf("pre_excm2:An error has occurred\n");
                fflush(stdout);
                return 1;
                }
        } else {
            no_spaces++;
        }
    }
    return 0;
}

int pre_execute_commands_for_and(char *input){
    char *temp_ptr = input;
    char* no_spaces = remove_spaces(temp_ptr);
    while (*no_spaces) {
        if (*no_spaces == '&') {
            no_spaces++;
            if (*no_spaces == '&' || *no_spaces == ';') {
                printf("pre_excm_3:An error has occurred\n");
                fflush(stdout);
                return 1;
                }
        } else {
            no_spaces++;
        }
    }
    return 0;
}

int pre_execute_commands_for_else(char *input){
    char *temp_ptr = input;
    char* no_spaces = remove_spaces(temp_ptr);
    //printf("%s",input);
    while (*no_spaces) {
        if (*no_spaces == '%' || *no_spaces == '!' || *no_spaces == '@' || *no_spaces == '(' || *no_spaces == ')' || *no_spaces == '^') {
            no_spaces++;
            printf("pre_excm_4:An error has occurred\n");
            fflush(stdout);
            return 1;
        } else {
            no_spaces++;
        }
    }

    // ||*no_spaces == ';' || *no_spaces == '&' || *no_spaces == '|' || *no_spaces == '>'
    
    if (strlen(temp_ptr) == 1 && (*temp_ptr == ';' || *temp_ptr == '&' || *temp_ptr == '|' || *temp_ptr == '>')) {
        printf("pre_excm5:An error has occurred\n");
        fflush(stdout);
        return 1;
    }
    //printf("%s\n",input);
    return 0;
}

//./1 &\nbg
void replace_newline_with_semicolon(char *input) {
    char *found = NULL;
    while ((found = strstr(input, "\\n")) != NULL) {  
        *found = ';';  
        memmove(found + 1, found + 2, strlen(found + 2) + 1);  
    }
}

void replace_newline_with_semicolon1(char *input) {
    char *found = NULL;
    while ((found = strstr(input, "\\n")) != NULL) {  
        if (found != input && *(found - 1) == '&') {
            found++;
        } else {
            *found = ';';  
            memmove(found + 1, found + 2, strlen(found + 2) + 1);  
        }
    }
}


////////////////////////////////////////////////////////////////// end pre execute

int main() {
    char input[1024];
    char *command;
    char *command_all[64];
    char *arguments[64];
    int arg_count;
    int background; 
    char *pipeline_commands[64];
    int num_pipeline_commands;
    int num_commands;

    while (1) {
        // printf("\n");
        // 检查后台进程
        check_bg_tasks();
        background = 0; 

        printf("nsh> "); 

        if (fgets(input, sizeof(input), stdin) != NULL) {
            if (input[0] == '\n') {
                continue; 
            }
            input[strcspn(input, "\n")] = 0;
            //printf("%s\n",input);
            replace_newline_with_semicolon1(input);

            //printf("Modified Input: %s\n", input);
        } else {
            exit(0);
        }

        add_to_history(input);

        char *input_copy_1 = strdup(input); 
        if (pre_execute_commands(input_copy_1) == 1 || pre_execute_commands_for_and(input_copy_1) == 1 || pre_execute_commands_for_else(input_copy_1) == 1){
            if (strstr(input_copy_1, "exit") != NULL){
                exit(0);
            }
            continue;
        }
        // free(input_copy_1);
        replace_newline_with_semicolon(input);
        
        char *input_copy_2 = strdup(input); 
        num_commands = 0;
        command_all[num_commands++] = strtok(input_copy_2, ";");
        while ((command_all[num_commands] = strtok(NULL, ";")) != NULL) {
            num_commands++;
        }
        //printf("num:%d\n",num_commands);
        // 处理多个任务
        for(int i=0;i < num_commands;i++){ 
            char *current_command = command_all[i];
            //printf("%s\n",command_all[i]);
            // 处理管道
            num_pipeline_commands = 0;
            char *input_copy_3 = strdup(current_command); 
            pipeline_commands[num_pipeline_commands++] = strtok(input_copy_3, "|");
            while ((pipeline_commands[num_pipeline_commands] = strtok(NULL, "|")) != NULL) {
                num_pipeline_commands++;
            }

            if (num_pipeline_commands > 1) {
                execute_pipeline_commands(pipeline_commands, num_pipeline_commands);
            }
            else{
                // redirection
                char *redirection_file = NULL;
                char *redirection_command = strtok(current_command, ">");
                if (redirection_command != NULL) {
                    redirection_file = strtok(NULL, " \n");
                    if (redirection_file != NULL) {
                        char *real_redirection_command = remove_blanks(redirection_command);
                        char *real_redirection_file = remove_blanks(redirection_file);
                        execute_command_with_redirection(real_redirection_command, real_redirection_file);
                        continue; 
                    }
                }

                // 解析当前命令
                command = strtok(current_command, " \n");
                arg_count = 0;
                background = 0;
                char full_cmd[128] = ""; 
                while (command != NULL && arg_count < 64) {
                    if (strcmp(command, "&") == 0) {
                        background = 1;
                        arguments[arg_count] = NULL;
                        execute_external_command(arguments, arg_count, background, full_cmd);
                        arg_count = 0;
                        background = 0;
                        full_cmd[0] = '\0'; 
                    } else {
                        arguments[arg_count++] = command;
                        strcat(full_cmd, command);
                        strcat(full_cmd, " ");
                    }
                    command = strtok(NULL, " \n");
                }
                if (arg_count > 0) {
                    arguments[arg_count] = NULL;
                    if (strcmp(arguments[0], "exit") == 0) {
                        execute_exit();
                    } 
                    else if (strcmp(arguments[0], "w") == 0) {
                        watching_history();
                    }
                    else if (strcmp(arguments[0], "cd") == 0) {
                        if (arg_count == 2) {
                            execute_cd(arguments[1]);
                        } else {
                            printf("cd:An error has occurred\n");
                            fflush(stdout);
                        }
                    } 
                    else if (strcmp(arguments[0], "paths") == 0) {
                        execute_paths(arguments + 1, arg_count - 1);
                    } 
                    else if (strcmp(arguments[0], "bg") == 0) {
                        execute_bg();
                    }
                    else {
                        execute_external_command(arguments, arg_count, background, full_cmd);
                    }
                }        
            }
        }
    }
    return 0;
}
