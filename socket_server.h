#ifndef _SOCKET_SERVER_H_
#define _SOCKET_SERVER_H_

#define SOCKET_DATA 0
#define SOCKET_CLOSE 1
#define SOCKET_OPEN 2
#define SOCKET_ACCEPT 3
#define SOCKET_ERROR 4
#define SOCKET_EXIT 5

struct socket_server;

struct socket_message {
	int id;
	int ud;	// for accept, ud is listen id ; for data, ud is size of data 
	char* data;
};

struct socket_server* socket_server_create();
void socket_server_release(struct socket_server* ss);

int socket_server_listen(struct socket_server* ss, const char* host, int port, int backlog);

int socket_server_wait(struct socket_server* ss, struct socket_message* result);

#endif // _SOCKET_SERVER_H_
