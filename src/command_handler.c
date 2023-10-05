#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/command_handler.h"
#include "../include/net.h"
#include "../include/string_array.h"
#include "../include/string_utils.h"

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1
#define COMMAND_LEN 128

bool validate_command(char* command)
{
    if (!string_has_prefix(command, "matinvpar") && !string_has_prefix(command, "kmeanspar")) {
        return false;
    }
    return true;
}

char* receive_command(int socket)
{
    char* command = calloc(COMMAND_LEN + 1, sizeof(char));
    ssize_t ret = recv(socket, command, COMMAND_LEN, 0);
    if (ret == 0) {
        fprintf(stdout, "Client closed connection to server.\n");
        free(command);
        return NULL;
    } else if (ret == -1) {
        perror("recv");
        free(command);
        return NULL;
    }
    return command;
}

int handle_command(int socket, char* buffer)
{
    char* program_name = get_program_name(buffer);
    printf("Recieved command: '%s'\n", buffer);
    struct string_array* args = split_string(buffer, ' ');

    if (strcmp(program_name, "kmeanspar") == 0) {

        size_t input_data_size;
        ssize_t recv_ret = recv(socket, &input_data_size, sizeof(input_data_size), 0);
        if (recv_ret <= 0) {
            free(program_name);
            string_array_free(args);
            fprintf(stderr, "Error: Client failed to send size of input data.\n");
            return EXIT_FAILURE;
        }

        char* input_data_buffer = read_full(socket, input_data_size);
        if (input_data_buffer == NULL) {
            free(program_name);
            string_array_free(args);
            fprintf(stderr, "Error: Client failed to send input data.\n");
            return EXIT_FAILURE;
        }

        // We create a temporary file that will be used as the input file for kmeanspar
        char *template = strdup("kmeans-data-XXXXXX");
        int temp_fd = mkstemp(template);
        if (temp_fd == -1) {
            free(program_name);
            string_array_free(args);
            perror("mkstemp");
            return EXIT_FAILURE;
        }

        int write_ret = write_full(temp_fd, input_data_buffer, input_data_size);
        if (write_ret == -1) {
            free(program_name);
            free(input_data_buffer);
            string_array_free(args);
            fprintf(stderr, "Error: Failed to write input data to temporary file.\n");
            return EXIT_FAILURE;
        }

        close(temp_fd);
        free(input_data_buffer);

        // Set the input file for kmeanspar to the temporary file
        for (size_t i = 0; i < args->size; i++) {
            if (strcmp(args->data[i], "-f") == 0 && args->size > i + 1) {
                free(args->data[i + 1]);
                args->data[i + 1] = template;
                break;
            }
        }
    }

    int pipefd[2];
    int status = pipe(pipefd);
    if (status == -1) {
        perror("pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    char *solution_buffer = NULL;
    size_t read_total = 0;
    if (pid == 0) {
        close(pipefd[PIPE_READ_END]);
        dup2(pipefd[PIPE_WRITE_END], STDOUT_FILENO);
        close(pipefd[PIPE_WRITE_END]);

        // TODO: move this into a separate function e.g. args->data[0] = prepend_string(args->data[0], "./");
        // void prepend_string(char *str, const char *prefix);
        // Append "./" to the program name, so that it can be found by execvp
        size_t program_name_length = strlen(args->data[0]);
        size_t dot_slash_length = 2;
        char *program_name_with_dot_slash = calloc(dot_slash_length + program_name_length + 1, sizeof(char));
        memcpy(program_name_with_dot_slash , "./", dot_slash_length);
        memcpy(program_name_with_dot_slash + dot_slash_length, args->data[0], program_name_length);
        free(args->data[0]);
        args->data[0] = program_name_with_dot_slash;

        int ret = execvp(args->data[0], args->data);
        if (ret == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        close(pipefd[PIPE_WRITE_END]);
        solution_buffer = read_all(pipefd[PIPE_READ_END], &read_total);
        close(pipefd[PIPE_READ_END]);
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            free(solution_buffer);
            exit(EXIT_FAILURE);
        }
        int exit_status;
        if (WIFEXITED(status) && (exit_status = WEXITSTATUS(status)) != 0) {
                fprintf(stderr, "Error: Child process returned with exit code %d.\n", exit_status);
                exit(EXIT_FAILURE);
        }
    }


    string_array_free(args);

    //size_t read_total = 0;
    //char* solution_buffer = read_all(pipefd[PIPE_READ_END], &read_total);

    // Send size of solution buffer to client
    ssize_t send_ret = send(socket, &read_total, sizeof(size_t), 0);
    if (send_ret == -1) {
        free(program_name);
        free(solution_buffer);
        string_array_free(args);
        fprintf(stderr, "Error: Failed to send size of solution data to client.\n");
        return EXIT_FAILURE;
    }

    // Send solution data to client
    int ret = write_full(socket, solution_buffer, read_total);
    if (ret == -1) {
        free(program_name);
        free(solution_buffer);
        string_array_free(args);
        fprintf(stderr, "Error: Failed to send solution data to client.\n");
        return EXIT_FAILURE;
    }

    free(solution_buffer);

    return 0;
}
