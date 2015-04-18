#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <mutex>

#include "socket_t.h"

std::mutex g_sock_mtx;

void poll(socket_t* sock)
{
    std::cout << "==========" << std::endl;
    char buffer[2048];
    for (;;) {
        memset(buffer, 0, 2048);

        int n = sock->read(buffer, 2048);
        if (n < 0) {
            break;
        }
        if (n > 0) {
            std::cout.write(buffer, n);
        }
        usleep(10000);
    }
    std::cout << "==========1111" << std::endl;
}

int main()
{
    socket_t* sock = new socket_t();
    sock->connect("127.0.0.1", 8888);
    // sock->nonblocking();

    std::thread tpoll(poll, sock);
    tpoll.detach();

    char buffer[2048];
    for (;;) {
        // memset(buffer, 0, strlen(buffer));

        std::string text = "this is a client";
        // fgets(buffer, 2048, stdin);
        sock->write(text.c_str(), text.size()); 

        usleep(1000000);
    }

    delete sock;

    return 0;
}