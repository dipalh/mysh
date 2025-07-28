#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <signal.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <stdarg.h>
#include <assert.h>

#include "builtins.h"
#include "io_helpers.h"
#include "processes.h"
#include "variables.h"
#include "socket.h"
#include "chat_helpers.h"

#define MAX_BUFFER_SIZE 4096

struct server_sock {
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf;
};

int sigint_received = 0;
int client_count = 0;
pid_t server_pid = -1;
int server_fd = -1;
int running = 1;

int in_client_mode = 0;

struct listen_sock s;
struct client_sock *clients = NULL;

void handle_sigterm(__attribute__((unused)) int sig) {
    running = 0;
}

// ====== Command execution =====

/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd) {
    ssize_t cmd_num = 0;
    while (cmd_num < BUILTINS_COUNT &&
           strncmp(BUILTINS[cmd_num], cmd, MAX_STR_LEN) != 0) {
        cmd_num += 1;
    }
    return BUILTINS_FN[cmd_num];
}


// ===== Builtins =====

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */
ssize_t bn_echo(char **tokens, size_t token_count) {
    ssize_t index = 1;

    if (index < (ssize_t)token_count && tokens[index] != NULL) {
	    display_message(tokens[index]);
	    index += 1;
    }
    while (index < (ssize_t)token_count && tokens[index] != NULL) {
	display_message(" ");
        display_message(tokens[index]);
        index += 1;
    }
    display_message("\n");

    return 0;
}

/*
 * Change the current working directory.
 *
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_cd(char **tokens, size_t token_count) {
    char path[256];
    path[0] = '\0';
    int abs = 0;

    if (token_count != 2) {
        display_message_format("ERROR: Too many arguments: %s takes a single path", tokens[0]);
        return -1;
    }
    
    if (tokens[1][0] == '/') {
        abs = 1;
    }
    
    char *token = strtok(tokens[1], "/");
    while (token != NULL) {
        if (strcmp(token, "...") == 0) {
            strncat(path, "../..", sizeof(path) - strlen(path) - 1);
        } else if (strcmp(token, "....") == 0) {
            strncat(path, "../../..", sizeof(path) - strlen(path) - 1);
        } else {
            strncat(path, token, sizeof(path) - strlen(path) - 1);
        }
        strncat(path, "/", sizeof(path) - strlen(path) - 1);
        token = strtok(NULL, "/");
    }
    char new_path[256];
    if (abs) {
        snprintf(new_path, sizeof(path) + 1, "/%s", path);
    }
    else{
        snprintf(new_path, sizeof(path) + 1, "%s", path);
    }

    if (chdir(new_path) != 0) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }

    return 0;
}

/*
 * List directory contents.
 *
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_ls(char **tokens, size_t token_count) {
    int rec = 0;
    int depth = -1;

    char search[128];
    search[0] = '\0';

    char path[128];
    path[0] = '\0';
    char *endptr;

    for (size_t i = 1; i < token_count; i++) {
        if (strncmp(tokens[i], "--rec", 5) == 0) {
            rec = 1;
        } else if (strncmp(tokens[i], "--d", 3) == 0) {
            i += 1;
            if (i >= token_count) {
                display_error("ERROR: No depth provided", "");
                return -1;
            }
            if (depth != -1 && strtol(tokens[i], NULL, 10) != depth) {
                display_error("ERROR: Depth already provided", "");
                return -1;
            }
            depth = strtol(tokens[i], &endptr, 10);
            if (*endptr != '\0' || depth < 0) {
                display_error("ERROR: Invalid depth value", "");
                return -1;
            }
        } else if (strncmp(tokens[i], "--f", 3) == 0) {
            i += 1;
            if (i >= token_count) {
                display_error("ERROR: No search string provided", "");
                return -1;
            }
            strncpy(search, tokens[i], 128);
        } else {
            if (path[0] != '\0') {
                display_message_format("ERROR: Too many arguments: %s takes a single path", tokens[0]);
                return -1;
            }
            else{
                strncpy(path, tokens[i], 128);
            }
        }
    }

    if (depth != -1 && !rec) {
        display_error("ERROR: Cannot specify depth without recursion", "");
        return -1;
    }

    if (path[0] == '\0') {
        strncat(path, ".", 2);
    }

    if (rec && depth == -1) {
        depth = 4096;
    }

    return list_directory(path, search, rec, depth, 0);
}

/*
 * List directory contents recursively.
 *
 * Returns 0 on success, -1 on error.
 */
int list_directory(char *path, char *search, int recursive, int max_depth, int current_depth) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        display_message(path);
        display_message("\n");
        display_error("ERROR: Invalid path", "");
        return -1;
    }

    struct dirent *folder;
    while ((folder = readdir(dir)) != NULL) {
        if (search[0] == '\0' || strstr(folder->d_name, search) != NULL) {
            display_message(folder->d_name);
            display_message("\n");
        }

        if (recursive && folder->d_type == 4 && current_depth < max_depth - 1) {
            if (strcmp(folder->d_name, ".") != 0 && strcmp(folder->d_name, "..") != 0) {
                char new_path[4096];
                snprintf(new_path, sizeof(new_path), "%s/%s", path, folder->d_name);
                list_directory(new_path, search, recursive, max_depth, current_depth + 1);
            }
        }
    }

    closedir(dir);
    return 0;
}

/*
 * Concatenate and display the contents of a file or stdin.
 *
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_cat(char **tokens, size_t token_count){
    FILE *file = NULL;

    if (token_count > 2) {
        display_message_format("ERROR: Too many arguments: %s takes a single file", tokens[0]);
        return -1;
    }

    if (token_count == 2) {
        file = fopen(tokens[1], "r");
        if (file == NULL) {
            display_error("ERROR: Cannot open file", "");
            return -1;
        }
    } else if (token_count == 1 && !isatty(fileno(stdin))) {
        file = stdin;
    } else {
        display_error("ERROR: No input source provided", "");
        return -1;
    }

    char line[MAX_STR_LEN];
    while (fgets(line, MAX_STR_LEN, file) != NULL) {
        display_message(line);
    }

    if (file != stdin) {
        fclose(file);
    }

    return 0;
}

/*
 * Count lines, words, and characters in a file or stdin.
 *
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_wc(char **tokens, size_t token_count) {
    FILE *file = NULL;

    if (token_count > 2) {
        display_message_format("ERROR: Too many arguments: %s takes a single file", tokens[0]);
        return -1;
    }

    if (token_count == 2) {
        file = fopen(tokens[1], "r");
        if (file == NULL) {
            display_error("ERROR: Cannot open file", "");
            return -1;
        }
    } else if (token_count == 1) {
        file = stdin;
    } else {
        display_error("ERROR: No input source provided", "");
        return -1;
    }

    int lines = 0;
    int words = 0;
    int chars = 0;
    int seen = 0;

    for (char c = fgetc(file); c != EOF; c = fgetc(file)) {
        chars += 1;
        if (c == ' ' || c == '\n' || c == '\t') {
            if (seen) {
                words += 1;
            }
            if (c == '\n') {
                lines += 1;
            }
            seen = 0;
        } else{
            seen = 1;
        }
    }

    if (seen) {
        words += 1;
    }
    
    display_message_format("word count %d\n", words);
    display_message_format("character count %d\n", chars);
    display_message_format("newline count %d\n", lines);

    if (file != stdin) {
        fclose(file);
    }

    return 0;
}

/*
 * Kill a process by its PID.
 * Optionally, specify a signal to send (default is SIGTERM).
 *
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_kill(char **tokens, size_t token_count) {
    if (token_count < 2) {
        display_error("ERROR: No process id provided", "");
        return -1;
    }

    pid_t pid = strtol(tokens[1], NULL, 10);
    if (pid < 1) {
        display_error("ERROR: Invalid process id", "");
        return -1;
    }

    int signum = SIGTERM;
    if (token_count >= 3) {
        signum = strtol(tokens[2], NULL, 10);
        if (signum < 1 || signum > 31) {
            display_error("ERROR: Invalid signal specified", "");
            return -1;
        }
    }

    if (kill(pid, signum) == -1) {
        display_error("ERROR: The process does not exist", "");
        return -1;
    }

    checkStatus(searchJob(pid));

    return 0;
}

ssize_t bn_ps(__attribute__((unused)) char **tokens, __attribute__((unused)) size_t token_count) {
    Node* curr = processes;
    while (curr != NULL) {
        if (curr->status == 0) {
            char *tokens[MAX_STR_LEN] = {NULL};
            ssize_t max_length = MAX_STR_LEN;
            size_t token_count = tokenize_input(curr->value, tokens, var_list, &max_length);
            if (token_count > 0) {
                display_message_format("%s %s\n", tokens[0], curr->key);
            }
            freeTokens(tokens, token_count);
        }
        curr = curr->next;
    }
    return 0;
}

/*
 * Accept a new client connection.
 */
int accept_connection(int fd) {
    struct sockaddr_in peer;
    unsigned int peer_len = sizeof(peer);
    peer.sin_family = AF_INET;

    int client_fd = accept(fd, (struct sockaddr *)&peer, &peer_len);
    if (client_fd < 0) {
        close(fd);
        exit(1);
    }

    struct client_sock *newclient = malloc(sizeof(struct client_sock));
    newclient->sock_fd = client_fd;
    newclient->inbuf = newclient->state = 0;
    newclient->next = NULL;
    newclient->username = malloc(32);
    snprintf(newclient->username, 32, "client%d", ++client_count);
    memset(newclient->buf, 0, BUF_SIZE);

    if (clients == NULL) {
        clients = newclient;
    } else {
        struct client_sock *curr = clients;
        while (curr->next != NULL) curr = curr->next;
        curr->next = newclient;
    }

    return client_fd;
}

/*
 * Main server loop that handles incoming connections and client messages.
 */
void *server_loop() {
    int max_fd;
    fd_set all_fds, listen_fds;

    FD_ZERO(&all_fds);
    FD_SET(s.sock_fd, &all_fds);
    max_fd = s.sock_fd;

    while (running) {
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (!running || nready == -1) {
            if (errno == EINTR) continue;
            break;
        }

        if (FD_ISSET(s.sock_fd, &listen_fds)) {
            int client_fd = accept_connection(s.sock_fd);
            if (client_fd > max_fd) max_fd = client_fd;
            FD_SET(client_fd, &all_fds);
        }

        struct client_sock *curr = clients;
        while (curr) {
            if (!FD_ISSET(curr->sock_fd, &listen_fds)) {
                curr = curr->next;
                continue;
            }

            int closed = read_from_client(curr);
            if (closed) {
                FD_CLR(curr->sock_fd, &all_fds);
                close(curr->sock_fd);
                assert(remove_client(&curr, &clients) == 0);
                continue;
            }

            char *msg;
            while (!get_message(&msg, curr->buf, &curr->inbuf)) {
                if (strcmp(msg, "\\connected") == 0) {
                    char response[128];
                    snprintf(response, sizeof(response), "Number of connected clients: %d", client_count);
                    write_buf_to_client(curr, response, strlen(response));
                    free(msg);
                    continue;
                }
                
                char out[BUF_SIZE];
                if (strncmp(msg, "[no-prefix]", 11) == 0) {
                    display_message_format("%s\n", msg + 11);
                    snprintf(out, sizeof(out), "%s\r\n", msg + 11);
                } else {
                    display_message_format("%s:%s\n", curr->username, msg);
                    snprintf(out, sizeof(out), "%s:%s\r\n", curr->username, msg);
                }
                
                struct client_sock *dest = clients;
                while (dest) {
                    if (write_buf_to_client(dest, out, strlen(out)) != 0) {
                        FD_CLR(dest->sock_fd, &all_fds);
                        close(dest->sock_fd);
                        assert(remove_client(&dest, &clients) == 0);
                        continue;
                    }
                    dest = dest->next;
                }
                free(msg);
            }
            curr = curr->next;
        }
    }
    return NULL;
}

/*
 * Start a server that listens on the specified port.
 * The server accepts incoming connections and handles client messages.
 *
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_start_server(char **tokens, size_t token_count) {
    if (token_count != 2) {
        display_error("ERROR: No port provided", "");
        return -1;
    }

    int port = strtol(tokens[1], NULL, 10);
    if (port < 1024 || port > 65535) {
        display_error("ERROR: Invalid port number", "");
        return -1;
    }

    if (setup_server_socket(&s, port) == -1){
        return -1;
    }
    server_fd = s.sock_fd;

    server_pid = fork();
    if (server_pid == 0) {
        signal(SIGTERM, handle_sigterm);
        signal(SIGINT, handle_sigterm);
        server_loop();
        exit(0);
    }
    return 0;
}

/*
 * Close the server and all connected clients and clean up resources.
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_close_server(__attribute__((unused)) char **tokens, __attribute__((unused)) size_t token_count) {
    if (server_fd <= 0) {
        display_error("ERROR: No server is running", "");
        return -1;
    }

    if (server_pid > 0) {
        running = 0;
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
        server_fd = -1;
        kill(server_pid, SIGTERM);
        waitpid(server_pid, NULL, 0);
        server_pid = -1;
    }

    struct client_sock *tmp;
    while (clients) {
        tmp = clients;
        close(tmp->sock_fd);
        clients = clients->next;
        free(tmp->username);
        free(tmp);
    }
    return 0;
}

/*
 * Send a message to the server at the specified port and hostname.
 * The message is sent as a single line with a network newline (CRLF) at the end.
 *
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_send(char **tokens, size_t token_count) {
    if (token_count < 4) {
        display_error("ERROR: Invalid number of arguments", "");
        return -1;
    }

    int port = strtol(tokens[1], NULL, 10);
    if (port < 1024 || port > 65535) {
        display_error("ERROR: Invalid port number", "");
        return -1;
    }

    char *hostname = tokens[2];
    if (hostname == NULL) {
        display_error("ERROR: No hostname provided", "");
        return -1;
    }

    char message[1024] = "[no-prefix]";
    for (size_t i = 3; i < token_count; i++) {
        if (i > 3) strncat(message, " ", sizeof(message) - strlen(message) - 1);
        strncat(message, tokens[i], sizeof(message) - strlen(message) - 1);
    }
    if (strlen(message) == strlen("[no-prefix]")) {
        display_error("ERROR: No message provided", "");
        return -1;
    }
    strcat(message, "\r\n");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        display_error("ERROR: Socket creation failed", "");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
        display_error("ERROR: Invalid hostname", "");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        display_error("ERROR: Connection failed", "");
        close(sock);
        return -1;
    }
    
    send(sock, message, strlen(message), 0);
    close(sock);
    return 0;
}

/*
 * Start a client that connects to a server at the specified port and hostname.
 * The client reads from stdin and sends messages to the server.
 * It also listens for messages from the server and displays them.
 *
 * Returns 0 on success, -1 on error.
 */
ssize_t bn_start_client(char **tokens, size_t token_count) {
    if (token_count != 3) {
        display_error("ERROR: No port or hostname provided", "");
        return -1;
    }

    int port = strtol(tokens[1], NULL, 10);
    if (port < 1024 || port > 65535) {
        display_error("ERROR: Invalid port number", "");
        return -1;
    }

    char *hostname = tokens[2];
    if (hostname == NULL) {
        display_error("ERROR: No hostname provided", "");
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        display_error("ERROR: Socket creation failed", "");
        return -1;
    }

    in_client_mode = 1;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
        display_error("ERROR: Invalid hostname", "");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        display_error("ERROR: Connection failed", "");
        close(sock);
        return -1;
    }

    char buffer[MAX_BUFFER_SIZE];
    char server_response[MAX_BUFFER_SIZE];
    fd_set readfds;
    int max_fd;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);
        max_fd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                shutdown(sock, SHUT_RDWR);  
                close(sock);                
                return 0;                   
            }
            buffer[strcspn(buffer, "\n")] = '\0';
            if (strlen(buffer) == 0)
                continue;
            strcat(buffer, "\r\n");
            if (send(sock, buffer, strlen(buffer), 0) == -1) {
                display_error("ERROR: Failed to send message", "");
                break;
            }
        }

        if (FD_ISSET(sock, &readfds)) {
            ssize_t bytes_received = recv(sock, server_response, sizeof(server_response) - 1, 0);
            if (bytes_received <= 0) {
                break;
            }
            server_response[bytes_received] = '\0';
            display_message_format("%s", server_response);
        }
        }

    close(sock);
    in_client_mode = 0;
    return 0;
}
