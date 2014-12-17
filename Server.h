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
#include <iostream>
#include <sstream>
#include "HTTPRequest.h"
#include "HTTPResponse.h"

#define SERV_ROOT "www"

class HTTPServer {
public: 
    HTTPServer();
    HTTPServer( int );
    ~HTTPServer();

    int run( void );
    int setPort( size_t );
    int initSocket( void );

    int recvRequest( void );
    int handleRequest( void );
    int parseRequest( void );
    int processRequest( void );

    int prepareResponse( void );
    int sendResponse( void );

private:

    string getMimeType( string );
    static const int buf_size = 32;

    size_t servPort;
    int listenfd , newsockfd;
    socklen_t clilen;
    struct sockaddr_in servaddr , cliaddr;
    
    string m_url;
    string m_mimeType;
    HttpRequest* m_httpRequest;
    HttpResponse* m_httpResponse;

};
#endif  /*_HTTP_SERVER_H_*/
