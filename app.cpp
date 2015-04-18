#include <assert.h>
#include <stdio.h>
#include <thread>

#include "app.h"

#define HELP    "help"

Application::Application()
: m_port(0)
{
}

Application::~Application()
{
}

void Application::run()
{
    onInitial();
    onInitHelps();
    listen(host(), port(), 10); 
}

void Application::stop()
{
    onRelease();
}

void Application::onInitial()
{
    set_host("127.0.0.1");
    set_port(8888);
}

void Application::onRelease()
{
}

void Application::onMessage(const socket_message* msg)
{
    switch (msg->type) {
    case MSG_TYPE_ACCEPT:
        printf("message(accept) [id=%d]\n", msg->sock_id);
        break;
    case MSG_TYPE_CLOSE:
        printf("message(close) [id=%d]\n", msg->sock_id);
        break;
    case MSG_TYPE_ERROR:
        printf("message(error) [id=%d]\n", msg->sock_id);
        break;
    case MSG_TYPE_READ:
        printf("message(read) [id=%d] size=%d\n", msg->sock_id, msg->sz);
        break;
    case MSG_TYPE_WRITE:
        printf("message(write) [id=%d] size=%d\n", msg->sock_id, msg->sz);
        break;
    case MSG_TYPE_UNKNOWN:
    default:
        break;
    }
}

void Application::onCommand(const socket_message* msg)
{
    // printf("message(cmd) [id=%d] size=%d\n", msg->sock_id, msg->sz);
    if (strcmp(msg->buffer, HELP) == 0) {
        printf("The server commands are:\n");
        for (auto iter : m_helps) {
            printf("  %-13s%s\n", iter.first.c_str(), iter.second.c_str());
        }
        printf("\n");
    }
}

void Application::onInitHelps()
{
    m_helps["quit"] = "Quit applicaton";
    m_helps["socknum"] = "Show number of sockets";
}