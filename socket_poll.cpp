#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include <stdio.h>

#include "socket_poll.h"
#include "socket_t.h"

socket_poll::socket_poll()
{
	m_pfd = kqueue();
	assert(m_pfd != -1);
}

socket_poll::~socket_poll()
{
	::close(m_pfd);
}

bool socket_poll::add(socket_t* sock)
{
	struct kevent ke;
	EV_SET(&ke, sock->m_fd, EVFILT_READ, EV_ADD, 0, 0, sock);
	if (kevent(m_pfd, &ke, 1, NULL, 0, NULL) == -1) {
		return false;
	}
	EV_SET(&ke, sock->m_fd, EVFILT_WRITE, EV_ADD, 0, 0, sock);
	if (kevent(m_pfd, &ke, 1, NULL, 0, NULL) == -1) {
		EV_SET(&ke, sock->m_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		kevent(m_pfd, &ke, 1, NULL, 0, NULL);
		return false;
	}
	EV_SET(&ke, sock->m_fd, EVFILT_WRITE, EV_DISABLE, 0, 0, sock);
	if (kevent(m_pfd, &ke, 1, NULL, 0, NULL) == -1) {
		del(sock);
		return false;
	}
	return true;
}

void socket_poll::del(socket_t* sock)
{	
	struct kevent ke;
	EV_SET(&ke, sock->m_fd, EVFILT_READ, EV_DELETE, 0, 0, sock);
	kevent(m_pfd, &ke, 1, NULL, 0, NULL);
	EV_SET(&ke, sock->m_fd, EVFILT_WRITE, EV_DELETE, 0, 0, sock);
	kevent(m_pfd, &ke, 1, NULL, 0, NULL);
}

void socket_poll::write(socket_t* sock, bool enable)
{
	struct kevent ke;
	EV_SET(&ke, sock->m_fd, EVFILT_WRITE, enable ? EV_ENABLE : EV_DISABLE, 0, 0, sock);
	if (kevent(m_pfd, &ke, 1, NULL, 0, NULL) == -1) {
		// todo: check error
	}
}

int socket_poll::wait(struct socket_poll::event* e, int max)
{
	struct kevent ev[max];
	int n = kevent(m_pfd, NULL, 0, ev, max, NULL);
	for (int i = 0; i < n; ++i) {
		e[i].sock = (socket_t*)ev[i].udata;
		unsigned filter = ev[i].filter;
		e[i].write = (filter == EVFILT_WRITE);
		e[i].read = (filter == EVFILT_READ);
	}

	return n;	
}

void socket_poll::close()
{
	::close(m_pfd);
}