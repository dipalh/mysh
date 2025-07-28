#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socket.h"
#include "chat_helpers.h"

int write_buf_to_client(struct client_sock *c, char *buf, int len) {
    char *lf;
    if ((lf = strchr(buf, '\n')) != NULL) {
        len = strlen(buf) - strlen(lf);
    }
    
    buf[len] = '\r';
    buf[len+1] = '\n';
    return write_to_socket(c->sock_fd, buf, len+2);
}

int remove_client(struct client_sock **curr, struct client_sock **clients) {
    struct client_sock *curr_client = *clients;
    struct client_sock *prev_client = NULL;
    struct client_sock *remv_client = *curr;

    if (curr_client == NULL) return 1;

    if (curr_client->sock_fd == remv_client->sock_fd) {
        *clients = curr_client->next;
        *curr = curr_client->next;

        free(remv_client->username);
        free(remv_client);
        return 0;
    }

    while (curr_client != NULL) {
        if (curr_client->sock_fd == remv_client->sock_fd) {
            prev_client->next = curr_client->next;
            *curr = curr_client->next;

            free(remv_client->username);
            free(remv_client);
            return 0;
        }
        prev_client = curr_client;
        curr_client = curr_client->next;
    }

    return 1
}

int read_from_client(struct client_sock *curr) {
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}
