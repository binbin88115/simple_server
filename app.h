#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <string>
#
#include "socket_server.h"

class Application : public socket_server
{
public:
	Application();
	virtual ~Application();

    void run();
    void stop();

protected:
    void set_host(const char* host) { m_host = host; }
    const char* host() { return m_host.c_str(); }

    void set_port(int port) { m_port = port; }
    int port() { return m_port; }

protected:
    /* will be launch before start server app,
       use for init app resource and config **/
    virtual void onInitial();

    /* will be launch before stop server app, 
       use for release app resource **/
    virtual void onRelease();

    /* every msg will be received by this function **/
    virtual void onMessage(const socket_message* msg) override;
    /* cmd msg will be received by this function **/
    virtual void onCommand(const socket_message* msg) override;

    // init help commands
    virtual void onInitHelps();

private:
    std::string m_host;
    int m_port;

    std::unordered_map<std::string, std::string> m_helps;
};

#endif // _APPLICATION_H_