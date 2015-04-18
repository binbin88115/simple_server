#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "socket_t.h"

socket_t::socket_t()
: m_id(0)
{
	m_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(m_fd != -1);
}

socket_t::socket_t(int fd)
: m_fd(fd)
, m_id(0)
{
}

socket_t::~socket_t()
{
	if (m_fd > 3)
		::close(m_fd);
}

void socket_t::set_id(int id)
{
	m_id = id;
}

int socket_t::id()
{
	return m_id;
}

int socket_t::connect(const char* host, int port)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(port);
	return ::connect(m_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
}

int socket_t::listen(const char* host, int port, int backlog)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(port);

	int ret = bind(m_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
	if (ret == -1) {
		return ret;
	}
	return ::listen(m_fd, backlog);
}

socket_t* socket_t::accept()
{
	int cfd = ::accept(m_fd, NULL, NULL);
	if (cfd != -1) {
		return new socket_t(cfd);
	}
	return NULL;
}

int socket_t::write(const char* data, int len)
{
	return ::write(m_fd, data, len);
}

int socket_t::read(char* data, int len)
{
	return ::read(m_fd, data, len);
}

void socket_t::keepalive()
{
	int keepalive = 1;
	setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));
}

void socket_t::nonblocking()
{
	int flag = fcntl(m_fd, F_GETFL, 0);
	if (-1 == flag) {
		return;
	}
	fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
}

void socket_t::close()
{
	::close(m_fd);
	m_fd = -1;
}
