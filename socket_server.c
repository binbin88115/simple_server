#include "socket_server.h"
#include "socket_poll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENT       20

#define SOCKET_TYPE_INVALID    1
#define SOCKET_TYPE_LISTEN     2
#define SOCKET_TYPE_PACCEPT    3

struct write_buffer
{
    int sz;
    void* buffer;
    struct write_buffer* next;
};

struct socket
{
    int fd;
    int type;
    struct write_buffer* wb; 
    struct socket* prev;
    struct socket* next;
};

struct socket_server 
{
    poll_fd event_fd;
    int event_n;
    int event_index;
    struct event ev[MAX_EVENT];
    int socket_n;
    struct socket* slot;
};

static void socket_keepalive(int fd) 
{
    int keepalive = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));  
}

static struct socket* new_socket(struct socket_server *ss, int fd) 
{
    struct socket* s = NULL;
    struct socket* ns = (struct socket*)malloc(sizeof(struct socket));
    ns->fd = fd;
    ns->type = SOCKET_TYPE_INVALID;
    ns->prev = NULL;
    ns->next = NULL;

    if (ss->slot == NULL) {
        ss->slot = ns;
    }
    else {
        for (s = ss->slot; s != NULL; s = s->next) {
            if (s->next == NULL) {
                s->next = ns;
                ns->prev = s;
                break;
            }
        }
    }
    ++ss->socket_n;

    return ns;
}

static void del_socket(struct socket_server *ss, struct socket* ds) 
{
    struct socket* s = ss->slot;
    for (; s != NULL; s = s->next) {
        if (s == ds) {
            if (s->prev == NULL) {
                ss->slot = NULL;
            }
            else {
                s->prev->next = s->next;
            }
            free(s);
            --ss->socket_n;
            break;
        }
    }
}

static int accept_socket(struct socket_server *ss, struct socket *s) 
{
    int client_fd = accept(s->fd, NULL, NULL);
    if (client_fd < 0) {
        return 1;
    }
    
    socket_keepalive(client_fd);
    sp_nonblocking(client_fd);
    struct socket* ns = new_socket(ss, client_fd);
    if (ns == NULL) {
        close(client_fd);
        return 1;
    }
    if (sp_add(ss->event_fd, client_fd, ns)) {
        close(client_fd);
        del_socket(ss, ns);
        return 1;
    }
    ns->type = SOCKET_TYPE_PACCEPT;

    return 0;
};

struct socket_server* socket_server_create()
{
    poll_fd efd = sp_create();
    if (sp_invalid(efd)) {
        fprintf(stderr, "socket-server: create event pool failed.\n");
        return NULL;
    }

    struct socket_server* ss = (struct socket_server*)malloc(sizeof(struct socket_server));
    ss->event_fd = efd;
    ss->event_index = 0;
    ss->event_n = 0;
    ss->socket_n = 0;
    ss->slot = NULL;
    return ss;
}

void socket_server_release(struct socket_server* ss)
{
    struct socket* s = ss->slot;
    for (;;) {
        if (s == NULL) {
            break;
        }
        struct socket* ds = s;
        s = s->next;

        close(ds->fd);
        free(ds);
    }
    sp_release(ss->event_fd);
    free(ss);
}

int socket_server_start(struct socket_server* ss, const char* host, int port, int backlog)
{
    int reuse = 1;
    uint32_t addr = INADDR_ANY;
    if (host[0]) {
        addr = inet_addr(host);
    }
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        return SOCKET_ERROR;
    }
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1) {
        close(listen_fd);
        return SOCKET_ERROR;
    }

    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = addr;
    if (bind(listen_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        close(listen_fd);
        return SOCKET_ERROR;
    }
    if (listen(listen_fd, backlog) == -1) {
        close(listen_fd);
        return SOCKET_ERROR;
    }

    struct socket* s = new_socket(ss, listen_fd);
    s->type = SOCKET_TYPE_LISTEN;
    if (sp_add(ss->event_fd, listen_fd, s)) {
        close(listen_fd);
        del_socket(ss, s);
        return SOCKET_ERROR;
    }
    
    return SOCKET_OK;
}

int socket_server_wait(struct socket_server* ss, struct socket_message* message)
{
    for (;;) {
        if (ss->event_index == ss->event_n) {
            ss->event_n = sp_wait(ss->event_fd, ss->ev, MAX_EVENT);
            ss->event_index = 0;
            if (ss->event_n <= 0) {
                ss->event_n = 0;
                return -1;
            }
        }

        struct event* e = &ss->ev[ss->event_index++];
        struct socket* s = (struct socket*)e->s;
        if (s == NULL) {
            // dispatch pipe message at beginning
            continue;
        }
        switch (s->type) {
        // case SOCKET_TYPE_CONNECTING:
        //     break;
        case SOCKET_TYPE_LISTEN:
            if (!accept_socket(ss, s)) {
                return SOCKET_ACCEPT;
            }
            break;
        case SOCKET_TYPE_INVALID:
            fprintf(stderr, "socket-server: invalid socket\n");
            break;
        default:
            if (e->write) {
                // int type = send_buffer(ss, s, result);
                // if (type == -1)
                //  break;
                // return type;
                return 1;
            }
            if (e->read) {
                return 1;
                // int type = forward_message(ss, s, result);
                // if (type == -1)
                //  break;
                // return type;
            }
            break;
        }
    }
    return 0;
}

int socket_server_send(struct socket_server* ss, int fd, int sz, void* buf)
{
    return 0;
}
