#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>

void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void resolve_command_path(char *cmd, char *full_path) {
    char *path_env = getenv("PATH");
    if (!path_env) {
        fprintf(stderr, "PATH environment variable not set.\n");
        exit(EXIT_FAILURE);
    }

    char *path = strtok(path_env, ":");
    while (path != NULL) {
        snprintf(full_path, PATH_MAX, "%s/%s", path, cmd);
        if (access(full_path, X_OK) == 0) {
            return;
        }
        path = strtok(NULL, ":");
    }

    fprintf(stderr, "Command not found: %s\n", cmd);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <inp> <cmd> <out>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *inp = argv[1];
    char *cmd = argv[2];
    char *out = argv[3];

    char *cmd_args[256];
    int arg_index = 0;

    char *token = strtok(cmd, " ");
    while (token != NULL && arg_index < 255) {
        cmd_args[arg_index++] = token;
        token = strtok(NULL, " ");
    }
    cmd_args[arg_index] = NULL;

    char full_path[PATH_MAX];
    resolve_command_path(cmd_args[0], full_path);

    pid_t pid = fork();
    if (pid == -1) {
        error_exit("fork");
    } else if (pid == 0) {
        if (strcmp(inp, "-") != 0) {
            int fd_in = open(inp, O_RDONLY);
            if (fd_in == -1) error_exit("open input file");
            if (dup2(fd_in, STDIN_FILENO) == -1) error_exit("dup2 input");
            close(fd_in);
        }

        if (strcmp(out, "-") != 0) {
            int fd_out = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) error_exit("open output file");
            if (dup2(fd_out, STDOUT_FILENO) == -1) error_exit("dup2 output");
            close(fd_out);
        }

        execv(full_path, cmd_args);
        error_exit("execv");
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return EXIT_FAILURE;
        }
    }
}
