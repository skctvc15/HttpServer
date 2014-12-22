/*************************************************************************
	> File Name:    Server.h
	> Author:       sk 
	> Mail:         skctvc15@gmail.com
	> Created Time: 2014年11月05日 星期三 19时08分56秒
 ************************************************************************/

#ifndef _SERVER_H
#define _SERVER_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>
#include <sstream>
#include "HTTPRequest.h"
#include "HTTPResponse.h"

#define SERV_ROOT "www"
#define MAX_EVENTS 1024

class HTTPServer {
public: 
    HTTPServer();
    HTTPServer( int );
    ~HTTPServer();

    int run( void );
    int setPort( size_t );
    int initSocket( void );

    int recvRequest( int );
    int handleRequest( int );
    int parseRequest( void );
    int processRequest( void );

    int prepareResponse( void );
    int sendResponse( void );
    void init_epfd( int );
    void init();

private:


    string getMimeType( string );
    static const int buf_size = 32;

    size_t servPort;
    int listenfd,m_sockfd ;
    
    socklen_t clilen;
    struct sockaddr_in servaddr , cliaddr;

    static int m_epollfd; 
    struct epoll_event ev;
    struct epoll_event evlist[MAX_EVENTS];
    
    string m_url;
    string m_mimeType;
    HttpRequest* m_httpRequest;
    HttpResponse* m_httpResponse;

};
#endif  /*_HTTP_SERVER_H_*/