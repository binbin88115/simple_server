#include "socket_server.h"

#include <stdio.h>

int main()
{
    struct socket_server* ss = socket_server_create();
    int ret = socket_server_start(ss, "127.0.0.1", 8888, 10);
    printf("============start server: %d\n", ret);
    struct socket_message message;
    for (;;) {
        int code = socket_server_wait(ss, &message);
        switch (code) {
        case SOCKET_LISTEN:
            break;
        case SOCKET_ACCEPT:
            break;
        default:
            break;
        }
    }
    socket_server_release(ss);
    /*
        struct socket_server* ss = socker_server_create();
        socket_server_listen(ss, "localhost", 8888, 100);
        for (;;) {
            struct socket_message message;
            int ret = socket_server_wait(ss, &message);
            switch (ret) {
                case SOCKET_DATA:
                    message.data, message.sz
                    break;
                case SOCKET_ACCEPT:
                    message.fd
                    break;
                case SOCKET_CLOSE:
                    break;
            }           
        }
    */
    
    return 0;
}