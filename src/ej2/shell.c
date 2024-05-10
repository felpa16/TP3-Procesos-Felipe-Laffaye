#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200

char** parse_command(char* command, int* len_result) {
    int len = strlen(command);
    int q_counter = 0;
    int token_counter = 1;
    for (int i = 0; i < len; i++) {
        if (command[i] == '\"') {
            q_counter++;
        }
        if ((command[i] == ' ') && (q_counter % 2 == 0)) {
            token_counter++;
        }
    }

    if (token_counter == 1) {
        *len_result = 1;
        char** result = (char**)malloc(sizeof(char*));
        result[0] = strdup(command);
        return result;
    }

    char** result = (char**)malloc(token_counter * sizeof(char*));
    int token_idx = 0;
    int src = 0;
    for (int i = 0; i < len; i++) {
        if (command[i] == '\"') {
            q_counter++;
        }
        if (command[i] == ' ' && (q_counter % 2 == 0)) {
            result[token_idx] = (char*)malloc(i - src + 1);
            strncpy(result[token_idx], command + src, i - src);
            result[token_idx][i - src] = '\0';

            if (result[token_idx][0] == '\"' && result[token_idx][i - src - 1] == '\"') {
                memmove(result[token_idx], result[token_idx] + 1, i - src - 1);
                result[token_idx][i - src - 2] = '\0';
            }
            src = i + 1;
            token_idx++;
        }
    }

    result[token_idx] = (char*)malloc(len - src + 1);
    strncpy(result[token_idx], command + src, len - src);
    result[token_idx][len - src] = '\0';

    if (result[token_idx][0] == '\"' && result[token_idx][len - src - 1] == '\"') {
        memmove(result[token_idx], result[token_idx] + 1, len - src - 2);
        result[token_idx][len - src - 2] = '\0';
    }

    *len_result = token_counter;
    return result;
}

void add_null(char*** array, int* size) {
    (*size)++;
    *array = (char **) realloc(*array, (*size) * sizeof(char*));
    (*array)[(*size) - 1] = NULL;
}

void trim_spaces(char* str) {
    int start = 0, end = strlen(str) - 1;
    while (str[start] == ' ') {
        start++;
    }
    while (str[end] == ' ') {
        end--;
    }
    for (int i = 0; i <= end - start; i++) {
        str[i] = str[start + i];
    }
    str[end - start + 1] = '\0';
}

int main() {
    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) {
        printf("Shell> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        char *token = strtok(command, "|");
        while (token != NULL) {
            trim_spaces(token);
            commands[command_count++] = strdup(token);
            token = strtok(NULL, "|");
        }

        char** parsed_commands[command_count];
        int lens[command_count];
        for (int i = 0; i < command_count; i++) {
            parsed_commands[i] = parse_command(commands[i], &lens[i]);
            add_null(&parsed_commands[i], &lens[i]);
        }

        int pipes[command_count - 1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipes[i]) == -1) {
                printf("Error al generar el pipe %i", i);
                return -1;
            }
        }

        int pids[command_count];
        for (int i = 0; i < command_count; i++) {
            pids[i] = fork();

            if (pids[i] == -1) {
                printf("Error en el fork del proceso %i", i);
                return -1;
            }

            if (pids[i] == 0) { // algún proceso hijo
                if (i != 0) {
                    close(pipes[i - 1][1]);
                    dup2(pipes[i - 1][0], 0);
                    close(pipes[i - 1][0]);
                }

                if (i != command_count - 1) {
                    close(pipes[i][0]);
                    dup2(pipes[i][1], 1);
                    close(pipes[i][1]);
                }

                execvp(parsed_commands[i][0], parsed_commands[i]);
                printf("Falló la función execvp");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < command_count - 1; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        for (int i = 0; i < command_count; i++) {
            wait(NULL);
            free(commands[i]);
        }

        command_count = 0;
    }

    return 0;
}