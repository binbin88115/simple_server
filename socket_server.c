#include "socket_server.h"
#include "socket_poll.h"

#define MAX_EVENT	10
#define MAX_SOCKET  10
#define MIN_READ_BUFFER 64

#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_RESERVE 1
#define SOCKET_TYPE_PLISTEN 2
#define SOCKET_TYPE_LISTEN 3
#define SOCKET_TYPE_CONNECTING 4
#define SOCKET_TYPE_CONNECTED 5
#define SOCKET_TYPE_HALFCLOSE 6
#define SOCKET_TYPE_PACCEPT 7
#define SOCKET_TYPE_BIND 8

struct socket {
	int fd;
	int type;
	int size;
	struct socket* prev;
	struct socket* next;
};

struct socket_server {
	poll_fd event_fd;
	int event_n;
	int event_index;
	struct event ev[MAX_EVENT];
	int socket_n;
	struct socket* slot;
}

static void socket_keepalive(int fd) 
{
	int keepalive = 1;
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));  
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

static struct socket* new_socket(struct socket_server *ss, int fd) 
{
	struct socket* s = NULL;
	struct socket* ns = malloc(sizeof(struct socket));
	ns->fd = fd;
	ns->type = SOCKET_TYPE_INVALID;
	ns->head = NULL;
	ns->tail = NULL;

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

static int accept_socket(struct socket_server *ss, struct socket *s) 
{
	int client_fd = accept(s->fd, NULL, NULL);
	if (client_fd < 0) {
		return 1;
	}
	
	do_keepalive(client_fd);
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

static void close_socket(struct socket_server *ss, struct socket *s) 
{
	sp_del(ss->event_fd, s->fd);
	del_socket(ss, s);
};

static int send_buffer(struct socket_server* ss, struct socket* s) {
	// while (s->head) {
	// 	struct write_buffer * tmp = s->head;
	// 	for (;;) {
	// 		int sz = write(s->fd, tmp->ptr, tmp->sz);
	// 		if (sz < 0) {
	// 			switch (errno) {
	// 			case EINTR:
	// 				continue;
	// 			case EAGAIN:
	// 				return -1;
	// 			}
	// 			force_close(ss, s, result);
	// 			return SOCKET_CLOSE;
	// 		}
	// 		s->wb_size -= sz;
	// 		if (sz != tmp->sz) {
	// 			tmp->ptr += sz;
	// 			tmp->sz -= sz;
	// 			return -1;
	// 		}
	// 		break;
	// 	}
	// 	s->head = tmp->next;
	// 	FREE(tmp->buffer);
	// 	FREE(tmp);
	// }
	// s->tail = NULL;
	// sp_write(ss->event_fd, s->fd, s, false);

	// if (s->type == SOCKET_TYPE_HALFCLOSE) {
	// 	force_close(ss, s, result);
	// 	return SOCKET_CLOSE;
	// }

	// return -1;
}

static int read_buffer(struct socket_server *ss, struct socket *s, struct socket_message* result) {
	int sz = s->size;
	char* buffer = malloc(sz);
	int n = (int)read(s->fd, buffer, sz);
	if (n < 0) {
		free(buffer);
		switch (errno) {
		case EINTR:
			break;
		case EAGAIN:
			fprintf(stderr, "socket-server: EAGAIN capture.\n");
			break;
		default:
			// close when error
			//force_close(ss, s, result);
			return SOCKET_ERROR;
		}
		return -1;
	}
	if (n == 0) {
		free(buffer);
		// force_close(ss, s, result);
		return SOCKET_CLOSE;
	}

	if (s->type == SOCKET_TYPE_HALFCLOSE) {
		// discard recv data
		free(buffer);
		return 1;
	}

	if (n == sz) {
		s->size *= 2;
	} else if (sz > MIN_READ_BUFFER && n*2 < sz) {
		s->size /= 2;
	}

	result->opaque = s->opaque;
	result->id = s->id;
	result->ud = n;
	result->data = buffer;
	return SOCKET_DATA;
}

struct socket_server* socket_server_create() 
{
	int i = 0;
	poll_fd efd = sp_create();
	if (sp_invalid(efd)) {
		fprintf(stderr, "socket-server: create event pool failed.\n");
		return NULL;
	}

	struct socket_server* ss = malloc(sizeof(struct socket_server));
	ss->event_fd = efd;
	ss->event_index = 0;
	ss->event_n = 0;
	ss->socket_n = 0;
	ss->socket_queue = NULL;

	return ss;
}

void socket_server_release(struct socket_server* ss)
{
	int i;
	struct socket_message dummy;
	for (i = 0; i < MAX_SOCKET; ++i) {
		struct socket *s = &ss->slot[i];
		if (s->type != SOCKET_TYPE_RESERVE) {
			close(s->fd);
		}
	}
	sp_release(ss->event_fd);
	free(ss);
}

int socket_server_listen(struct socket_server* ss, const char* host, int port, int backlog)
{
	// only support ipv4
	uint32_t addr = INADDR_ANY;
	if (host[0]) {
		addr = inet_addr(host);
	}
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		return 1;
	}

	int reuse = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int)) == -1) {
		close(listen_fd);
		return 1;
	}

	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = addr;
	if (bind(listen_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		close(listen_fd);
		return 1;
	}
	if (listen(listen_fd, backlog) == -1) {
		close(listen_fd);
		return 1;
	}
	return 0;
}

int socket_server_wait(struct socket_server* ss, struct socket_message* result)
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

		struct event *e = &ss->ev[ss->event_index++];
		struct socket *s = (struct socket*)e->s;
		if (s == NULL) {
			// dispatch pipe message at beginning
			continue;
		}
		switch (s->type) {
		case SOCKET_TYPE_CONNECTING:
			break;
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
				// 	break;
				// return type;
			}
			if (e->read) {
				// int type = forward_message(ss, s, result);
				// if (type == -1)
				// 	break;
				// return type;
			}
			break;
		}
	}
	return 0;
}