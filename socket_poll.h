#ifndef _SOCKET_POLL_H_
#define _SOCKET_POLL_H_

class socket_t;
class socket_poll
{
public:
	struct event
	{
		socket_t* sock;
		bool read;
		bool write;
	};

public:
	socket_poll();
	virtual ~socket_poll();

	bool add(socket_t* sock);
	void del(socket_t* sock);
	void write(socket_t* sock, bool enable);
	int wait(struct socket_poll::event* e, int max);
	void close();

private:
	int m_pfd;
};

#endif // _SOCKET_POLL_H_