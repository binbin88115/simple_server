#ifndef _SOCKET_SERVER_H_
#define _SOCKET_SERVER_H_

#define SOCKET_ERROR  -1
#define SOCKET_OK      0
#define SOCKET_ACCEPT  1
#define SOCKET_LISTEN  2

struct socket_server;
struct socket;

struct socket_message
{

};

struct socket_server* socket_server_create();
void socket_server_release(struct socket_server* ss);

int socket_server_start(struct socket_server* ss, const char* host, int port, int backlog);
int socket_server_wait(struct socket_server* ss, struct socket_message* message);

int socket_server_send(struct socket_server* ss, int fd, int sz, void* buf);

#endif // _SOCKET_SERVER_H_
