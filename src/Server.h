/*************************************************************************
	> File Name:    Server.h
	> Author:       sk
	> Mail:         skctvc15@gmail.com
	> Created Time: 2014年11月05日 星期三 19时08分56秒
 ************************************************************************/

#ifndef _SERVER_H
#define _SERVER_H

#include "./Utils/threadpool.h"
#include "HttpConnection.h"

#define MAX_EVENTS 1024
#define MAX_CONN 1024

class HTTPServer {
public:
    HTTPServer();
    HTTPServer(int);
    ~HTTPServer();

    void init();
    static void initDaemon(const char*, int);

    int run();
    int setPort(size_t);
    int initSocket();
    int eventCycle();

private:
    int m_listenfd;
    int m_servport;

    sockaddr_in m_cliaddr;
    sockaddr_in m_servaddr;

    HttpConnection *m_user_conn;

    ThreadPool<HttpConnection> *m_pool;
};
#endif  /*_SERVER_H_*/
