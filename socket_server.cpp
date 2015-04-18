#include <string.h>
#include <unistd.h>
#include <functional>
#include <thread>
#include <string>

#include "socket_t.h"
#include "socket_server.h"

#define MAX_EVENT   5
#define QUIT        "quit"
#define QUIT_OK     "y"

int socket_server::s_sock_id = 0;

socket_server::socket_server()
: m_exit(false)
, m_bindin_sock(nullptr)
, m_listen_sock(nullptr)
{
}

socket_server::~socket_server()
{
    for (auto sock : m_socks) {
       m_poll.del(sock); 
       delete sock;
    }
    m_socks.clear();

    while (!m_msgs.empty()) {
        delete m_msgs.front();
        m_msgs.pop();
    }

    m_buffers.clear();
    m_poll.close();

    printf("exit socket server...\n");
}

void socket_server::listen(const char* host, int port, int backlog)
{
    m_bindin_sock = new socket_t(1);
    if (!m_poll.add(m_bindin_sock)) {
        delete m_bindin_sock;
        return;
    }
    m_bindin_sock->set_id(get_sock_id());
    add_sock(m_bindin_sock);

    m_listen_sock = new socket_t();
    if (m_listen_sock->listen(host, port, backlog) == -1) {
        delete m_listen_sock;
        return;
    }

    if (!m_poll.add(m_listen_sock)) {
        delete m_listen_sock;
        return;
    }
    m_listen_sock->set_id(get_sock_id());
    add_sock(m_listen_sock);

    std::thread tpoll(std::bind(&socket_server::socks_polling, this));
    tpoll.detach();

    main_thread_polling();
}

int socket_server::connect(const char* host, int port)
{
	return -1;
}

void socket_server::close_socket(int sock_id)
{
}

void socket_server::send(int sock_id, const char* buffer, int sz)
{
	if (sock_id == m_listen_sock->id()) 
		return;

    m_buffers_mtx.lock();
    auto iter = m_buffers.find(sock_id);
    if (iter != m_buffers.end()) {
        iter->second.push_back(std::string(buffer, sz));
    }
    else {
        m_buffers[sock_id] = { std::string(buffer, sz) };
    }
    m_buffers_mtx.unlock();
}

void socket_server::onMessage(const socket_message* msg)
{
}

void socket_server::onCommand(const socket_message* msg)
{
}

int socket_server::get_sock_id()
{
    return ++s_sock_id;
}

void socket_server::socks_polling()
{
    struct socket_poll::event ev[MAX_EVENT];
    for (;;) {
        int n = m_poll.wait(ev, MAX_EVENT);
        for (int i = 0; i < n; ++i) {
            if (ev[i].sock == m_listen_sock) {
                socket_t* client_sock = m_listen_sock->accept();
                if (!m_poll.add(client_sock)) {
                    client_sock->close();
                    continue;
                }
                client_sock->set_id(get_sock_id());
                add_sock(client_sock);

                push_message(pack_message(client_sock->id(), MSG_TYPE_ACCEPT));
            }
            else {
                if (ev[i].read) {
                    socket_message* msg = read_sock(ev[i].sock);                   
                    push_message(msg);

                    if (msg->type == MSG_TYPE_CMD) {
                        if ((strcmp(msg->buffer, QUIT_OK) == 0) 
                            && m_pcmd == QUIT) {
                            printf("exit socks polling...\n");
                            return;
                        }
                        m_pcmd = msg->buffer;
                    }
                }
                else if (ev[i].write) {
                    write_sock(ev[i].sock);
                }
            }
        }
        check_buffers();
    }
}

void socket_server::main_thread_polling()
{
    for (;;) {
        bool quit = false;

        m_msgs_mtx.lock();
        while (!m_msgs.empty()) {
            socket_message* msg = m_msgs.front();
            if (msg->type == MSG_TYPE_CMD) {
                onCommand(msg);
                if (strcmp(msg->buffer, QUIT) == 0) {
                    printf("Quit anyway? (y or n): ");
                    fflush(stdout);
                }
                else if ((strcmp(msg->buffer, QUIT_OK) == 0) 
                    && m_pcmd == QUIT) {
                    quit = true;
                }
            }
            else {
                onMessage(msg);
            }
            m_msgs.pop();
            delete msg;
        }
        m_msgs_mtx.unlock(); 

        if (quit) {
            printf("exit main while...\n");
            break;
        }

        usleep(1000);
    }
}

socket_server::socket_message* socket_server::pack_message(int sock_id, int type, char* buffer, int sz)
{
    socket_message* msg = new socket_message();
    msg->sock_id = sock_id;
    msg->type = type;
    msg->sz = sz;
    if (buffer && sz != 0) {
        msg->buffer = new char[sz];
        if (type == MSG_TYPE_CMD) {
            memcpy(msg->buffer, buffer, sz - 1);
        }
        else {
            memcpy(msg->buffer, buffer, sz);
        }
    }
    return msg;
}

void socket_server::push_message(socket_message* msg)
{
    if (msg) {
        m_msgs_mtx.lock();
        m_msgs.push(msg);
        m_msgs_mtx.unlock();
    }
}

void socket_server::add_sock(socket_t* sock)
{
    m_socks_mtx.lock();
    m_socks.insert(sock);
    m_socks_mtx.unlock();
}

void socket_server::del_sock(socket_t* sock)
{
    m_socks_mtx.lock();
    m_socks.erase(m_socks.find(sock));
    delete sock;
    m_socks_mtx.unlock();
}

socket_t* socket_server::get_sock(int sock_id)
{
    socket_t* ret = nullptr;
    m_socks_mtx.lock();
    for (auto sock : m_socks) {
        if (sock->id() == sock_id) {
            ret = sock;
            break;
        }
    }
    m_socks_mtx.unlock();
    return ret;
}

socket_server::socket_message* socket_server::read_sock(socket_t* sock)
{
    socket_message* ret_msg = nullptr;

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    int n = sock->read(buffer, sizeof(buffer));
    if (n < 0) {
        switch(errno) {
        case EINTR:
            break;
        case EAGAIN:
            fprintf(stderr, "socket-server: EAGAIN capture.\n");
            break;
        default: {
                m_poll.del(sock);
                del_sock(sock);
                ret_msg = pack_message(sock->id(), MSG_TYPE_ERROR);
            }
        }
    }
    else if (n == 0) {
        m_poll.del(sock);
        del_sock(sock);
        ret_msg = pack_message(sock->id(), MSG_TYPE_CLOSE);
    }
    else {
        int msg_type = sock == m_bindin_sock ? MSG_TYPE_CMD : MSG_TYPE_READ;
        ret_msg = pack_message(sock->id(), msg_type, buffer, n);
    }
    return ret_msg;
}

socket_server::socket_message* socket_server::write_sock(socket_t* sock)
{
    std::list<std::string> buffers;
    m_buffers_mtx.lock();
    auto iter = m_buffers.find(sock->id());
    if (iter != m_buffers.end()) {
        buffers = std::move(iter->second);
        iter->second.clear();
    }
    m_buffers_mtx.unlock();

    bool close_error = false;
    socket_message* ret_msg = nullptr;
    for (const std::string& buff : buffers) {
        int n = sock->write(buff.c_str(), buff.size());
        if (n < 0) {
            switch(errno) {
            case EINTR:
                break;
            case EAGAIN:
                fprintf(stderr, "socket-server: EAGAIN capture.\n");
                break;
            default: {
                    m_poll.del(sock);
                    del_sock(sock);
                    ret_msg = pack_message(sock->id(), MSG_TYPE_ERROR);
                    close_error = true;
                    break;
                }
            }
        }
        else if (n == 0) {
            m_poll.del(sock);
            del_sock(sock);
            ret_msg = pack_message(sock->id(), MSG_TYPE_CLOSE);
            close_error = true;
            break;
        }
        else {
            ret_msg = pack_message(sock->id(), MSG_TYPE_WRITE, nullptr, n);
        }
    }

    if (!close_error)
        m_poll.write(sock, false);

    return ret_msg;
}

void socket_server::check_buffers()
{
    m_buffers_mtx.lock();
    auto iter = m_buffers.begin();
    for (; iter != m_buffers.end(); ++iter) {
        if (!iter->second.empty()) {
            socket_t* sock = get_sock(iter->first);
            if (sock) {
                m_poll.write(sock, true);
            }
            else {
                iter->second.clear();
            }
        }
    }
    m_buffers_mtx.unlock();
}