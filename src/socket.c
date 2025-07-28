#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include "io_helpers.h"

#include "socket.h"

/*
* Setup a server socket to listen on the specified port.
*
* Returns 0 on success, -1 on error.
*/
int setup_server_socket(struct listen_sock *s, int port) {
    if(!(s->addr = malloc(sizeof(struct sockaddr_in)))) {
        display_error("ERROR: malloc", "");
        return -1;
    }

    s->addr->sin_family = AF_INET;

    s->addr->sin_port = htons(port);

    memset(&(s->addr->sin_zero), 0, 8);
    s->addr->sin_addr.s_addr = INADDR_ANY;

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0) {
        display_error("ERROR: server socket", "");
        return -1;
    }
    int on = 1;
    int status = setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on));
    if (status < 0) {
        display_error("ERROR: setsockopt", "");
        return -1;
    }

    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0) {
        display_error("ERROR: server bind", "");
        close(s->sock_fd);
        return -1;
    }

    if (listen(s->sock_fd, MAX_BACKLOG) < 0) {
        display_error("ERROR: server listen", "");
        close(s->sock_fd);
        return -1;
    }
    return 0;
}

/*
 * Search the first inbuf characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 */
int find_network_newline(const char *buf, int inbuf) {
    for (int i = 1; i<inbuf; i++) {
        if (buf[i] == '\n' && buf[i-1] == '\r') {
            return i + 1;
        }
    }

    return -1;
}

/* 
 * Reads from socket sock_fd into buffer *buf containing *inbuf bytes
 * of data. Updates *inbuf after reading from socket.
 *
 * Return -1 if read error or maximum message size is exceeded.
 * Return 0 upon receipt of CRLF-terminated message.
 * Return 1 if socket has been closed.
 * Return 2 upon receipt of partial (non-CRLF-terminated) message.
 */
int read_from_socket(int sock_fd, char *buf, int *inbuf) {
    if (*inbuf >= BUF_SIZE) {
        return -1;
    }

    int ret = read(sock_fd, buf + *inbuf, BUF_SIZE-*inbuf);
    if (ret < 0) {
        return -1;
    }

    *inbuf += ret;

    if (ret == 0) {
        return 1;
    }

    if (find_network_newline(buf, *inbuf) >= 0) {
        return 0;
    }

    return 2;
}

/*
 * Search src for a network newline, and copy complete message
 * into a newly-allocated NULL-terminated string **dst.
 * Remove the complete message from the *src buffer by moving
 * the remaining content of the buffer to the front.
 *
 * Return 0 on success, 1 on error.
 */
int get_message(char **dst, char *src, int *inbuf) {
    int ret = find_network_newline(src, *inbuf);
    if (ret < 0) {
        return 1;
    }

    *dst = malloc(ret-1);
    if (*dst == NULL) {
        display_error("ERROR: malloc", "");
        return 1;
    }

    memcpy(*dst, src, ret - 2);
    (*dst)[ret-2] = '\0';

    int remaining = *inbuf - ret;
    memmove(src, src+ret, remaining);
    *inbuf = remaining;

    return 0;
}


/* 
 * Write len bytes from buf to the socket sock_fd.
 * Returns 0 on success, 1 on error, 2 if the socket is closed.
 */

int write_to_socket(int sock_fd, char *buf, int len) {
    int bytes_written = 0;
    int remaining = len;

    while (remaining > 0) {
        int r = write(sock_fd, buf + bytes_written, remaining);
        if (r == -1) {
            return 1;
        }

        if (r == 0) {
            return 2;
        }

        bytes_written += r;
        remaining -= r;
    }

    return 0;
}
