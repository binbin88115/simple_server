#ifndef _SOCKET_SERVER_H_
#define _SOCKET_SERVER_H_

#include <set>
#include <queue>
#include <unordered_map>
#include <list>
#include <mutex>

#include "socket_poll.h"

class socket_t;
class socket_server 
{
protected:
    enum {
        MSG_TYPE_UNKNOWN = 0,
        MSG_TYPE_ACCEPT,
        MSG_TYPE_CLOSE,
        MSG_TYPE_ERROR,
        MSG_TYPE_READ,
        MSG_TYPE_WRITE,
        MSG_TYPE_CMD
    };

    class socket_message
    {
    public:
        socket_message() : sock_id(0), type(MSG_TYPE_UNKNOWN), buffer(nullptr), sz(0) {}
        ~socket_message() { if (buffer) { delete buffer; } }

    public:
        int sock_id;
        int type;
        char* buffer;
        int sz;
    };

public:
	socket_server();
	virtual ~socket_server();

    void listen(const char* host, int port, int backlog);
    int connect(const char* host, int port);
    void send(int sock_id, const char* buffer, int sz);
    void close_socket(int sock_id);

protected:
    virtual void onMessage(const socket_message* msg);
    virtual void onCommand(const socket_message* msg);

private:
    int get_sock_id();

    void socks_polling();
    void main_thread_polling();

    socket_message* pack_message(int sock_id, int type, char* buffer = nullptr, int sz = 0);
    void push_message(socket_message* msg);

    void add_sock(socket_t* sock);
    void del_sock(socket_t* sock);
    socket_t* get_sock(int sock_id);

    socket_message* read_sock(socket_t* sock);
    socket_message* write_sock(socket_t* sock);

    void check_buffers();
    void quit();

private:
    bool m_exit;

    std::string m_pcmd;

    socket_t* m_bindin_sock;
    socket_t* m_listen_sock;
    socket_poll m_poll;

    std::mutex m_socks_mtx;
	std::set<socket_t*> m_socks;

    std::mutex m_msgs_mtx;
    std::queue<socket_message*> m_msgs;

    std::mutex m_buffers_mtx;
    std::unordered_map<int, std::list<std::string> > m_buffers;

    static int s_sock_id;
};

#endif // _SOCKET_SERVER_H_