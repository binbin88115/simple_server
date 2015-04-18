#ifndef _SOCKET_T_H_
#define _SOCKET_T_H_

class socket_t
{
	friend class socket_poll;
	friend class socket_server;

public:
	socket_t();
	~socket_t();

	void set_id(int id);
	int id();

	int connect(const char* host, int port);
	int listen(const char* host, int port, int backlog);
	socket_t* accept();

	int write(const char* data, int len);
	int read(char* data, int len);

	void keepalive();
	void nonblocking();
	void close();

protected:
	socket_t(int fd);

private:
	socket_t(const socket_t& sock);
	socket_t& operator=(const socket_t& sock);

private:
	int m_fd;
	int m_id;
};

#endif // _SOCKET_T_H_