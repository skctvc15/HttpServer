#ifndef _HTTPCONNECTION_H_
#define _HTTPCONNECTION_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <linux/tcp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "./Request/HTTPRequest.h"
#include "./Response/HTTPResponse.h"

#define SERV_ROOT "../www"
#define MAX_EVENTS 1024
#define READ_BUFFER_SIZE 2048
#define WRITE_BUFFER_SIZE 2048

class HttpConnection {
public:
    HttpConnection();
    HttpConnection(int);
    ~HttpConnection();

    void initEpollfd(int);
    void init(int, const sockaddr_in&);
    int setPort(size_t);
    int initSocket();

    int recvRequest();
    int handleRequest();
    int parseRequest();
    int processRequest();

    int prepareResponse();
    int sendResponse();

    void reset();
    void closeConn();
    void process();
    void unmap();
    void not_found();

    int  handleGET();
    int handlePUT();

    static int m_user_count;
    static int m_epollfd;

private:
    string getMimeType(string);

    int m_sockfd ;
    sockaddr_in m_addr;

    //for writev
    struct iovec iv[2];

    //mmap clien request file addr
    char *m_file_addr;
    struct stat m_file_stat;

    int  m_read_index;
    char m_read_buf[READ_BUFFER_SIZE];

    int  m_write_index;
    char m_write_buf[WRITE_BUFFER_SIZE];

    bool m_linger;

    string m_url;
    string m_mime_type;

    HttpRequest* m_http_request;
    HttpResponse* m_http_response;
};

#endif
