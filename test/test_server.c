#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../socket_poll.h"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8888
#define MAX_EPOLL_SIZE 4096
#define MAXLINE 64 
#define BACKLOG 5

int main(int argc, char const *argv[])
{
	int fd, confd, sockfd, epfd, curfds;
	int i, n, ret;
	struct sockaddr_in addr;
	struct sockaddr_in cli_addr;
	socklen_t cli_addr_len;
	int buf[MAXLINE];
	struct event evt;

	memset(&addr, 0, sizeof(addr));
	memset(&cli_addr, 0, sizeof(cli_addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	addr.sin_port = htons(SERVER_PORT); 
	
	if( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("error to create the socket\n");
		exit(-1);
	} 

	if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK) == -1)
	{
		printf("error to set the listen fd to nonblock\n");
		exit(-2);
	}
	
	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("error to bind the socket\n");
		exit(-3);
	}
	
	if(listen(fd, BACKLOG) == -1) {
		printf("error to listen the socket\n");
		exit(-4);
	}

	epfd = sp_create();	
	sp_add(epfd, fd, (void*)fd);

	printf("start to epoll\n");
	while (1) {
		ret = sp_wait(epfd, &evt, 1);
		if(ret < 0 && errno == EINTR) {
			printf("error\n");
			continue;
		}
		else if(ret < 0)
		{
			printf("error to poll the socket \n");
			exit(-6);
		}
		else if(ret == 0) {
			printf("time expire\n");
			continue;
		}
		else if ((int)evt.s == fd) {
			int cfd = accept(fd, NULL, NULL);
			if (cfd != 0) {
				sp_nonblocking(cfd);
				sp_add(epfd, cfd, (void*)cfd);
			}
			printf("accept client socket\n");
		}
		else if (evt.read) {
			char buf[64];
			int num = 0;

			memset(buf, 0, 64);
			num = read((int)evt.s, buf, 63);
			if(num > 0)
			{
				printf("receive msg: %s", buf);
			}
			else if (num == 0) {
				sp_del(epfd, (int)evt.s);
				printf("socket close\n");
			}
		}
		else if (evt.write) {
			// TODO:
		}
		printf("request num = %d\n", ret);
	}

	close(fd);
	sp_release(epfd);

	return 0;
}