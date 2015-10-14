#include <sys/epoll.h>
#include <linux/tcp.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>
#include "Server.h"

extern int errno;
extern void epollAddfd(int , int , bool);

void addsig(int sig, void (handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));

    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);

    if (sigaction(sig, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

void HTTPServer::initDaemon(const char *pname, int facility)
{
    int pid;

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGHUP , SIG_IGN);

    if ((pid = fork()))
    {
        exit(EXIT_SUCCESS);
    }
    else if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    setsid();

    if ((pid = fork()))
    {
        exit(EXIT_SUCCESS);
    }
    else if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NOFILE; ++i)
        close(i);

    umask(0);
    signal(SIGCHLD, SIG_IGN);
    openlog(pname, LOG_PID, facility);
}

HTTPServer::HTTPServer():m_servport(80)
{
    bzero(&m_servaddr,sizeof(m_servaddr));
}

int HTTPServer::setPort(size_t port)
{
    if (port <= 1024 || port >= 65535)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return HTTP_ERROR;
    }

    m_servport = port;
    return HTTP_OK;
}

HTTPServer::HTTPServer(int port)
{
    if (setPort(port))
    {
        fprintf(stderr, "Failed to set port\n");
    }

    m_user_conn = new HttpConnection[MAX_CONN];
    m_pool = new ThreadPool<HttpConnection>();

    bzero(&m_cliaddr, sizeof(m_cliaddr));
    bzero(&m_servaddr, sizeof(m_servaddr));
}

HTTPServer::~HTTPServer()
{
    delete []m_user_conn;
    delete m_pool;
}

int HTTPServer::initSocket()
{
    if ((m_listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
    {
        perror("socket");
        return HTTP_ERROR;
    }

    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_addr.s_addr = INADDR_ANY;
    m_servaddr.sin_port = htons(m_servport);

    int reuse = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    socklen_t addrlen = sizeof(m_servaddr);
    if ((bind(m_listenfd, reinterpret_cast<struct sockaddr*>(&m_servaddr), addrlen)) < 0)
    {
        perror("bind");
        return HTTP_ERROR;
    }

    if ((listen(m_listenfd,10)) < 0)
    {
        perror("listen");
        return HTTP_ERROR;
    }

    return HTTP_OK;
}

int HTTPServer::eventCycle()
{
    struct epoll_event evlist[MAX_EVENTS];
    int epfd = epoll_create(5);
    if (epfd == -1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    HttpConnection::m_epollfd = epfd;
    epollAddfd(epfd, m_listenfd, false);

    //we use ET model here
    while(1)
    {
        int ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
        if (ready < 0)
        {
            perror("epoll wait");
            break;
        }

        for (int i = 0; i < ready; i++)
        {
            int sockfd = evlist[i].data.fd;
            socklen_t clilen = sizeof(m_cliaddr);
            if (sockfd == m_listenfd)
            {
                clilen = sizeof(m_cliaddr);
                int connfd = accept(m_listenfd, reinterpret_cast<struct sockaddr *>(&m_cliaddr), &clilen);
                printf("New Connection\n");
                if (connfd < 0)
                {
                    perror("accept");
                    continue;
                }
                if (HttpConnection::m_user_count >= MAX_CONN)
                {
                    char info[] = "Internal server busy\n";
                    send(connfd,  info, strlen(info) + 1, 0);
                    close(connfd);
                    continue;
                }

                m_user_conn[connfd].init(connfd, m_cliaddr);
            }
            else if (evlist[i].events & EPOLLIN)
            {
                printf("Read event triggering! \n");
                int ret = m_user_conn[sockfd].recvRequest();
                if (ret != HTTP_OK)
                {
                    if (ret == HTTP_ERROR)
                        fprintf(stderr,"Receiving request failed\n");
                    else if (ret == HTTP_CLOSE)
                        fprintf(stderr, "Recv Fin, Closing....\n");

                    m_user_conn[sockfd].closeConn();
                    continue;
                }

                m_pool->pushToWorkQueue(m_user_conn + sockfd);
                //m_user_conn[sockfd].process();
            }
            else if (evlist[i].events & (EPOLLHUP | EPOLLERR))
            {
                printf("Closing fd %d\n",evlist[i].data.fd);
                if (close(evlist[i].data.fd) == -1)
                {
                    perror("Close evlist.data.fd");
                    exit(EXIT_FAILURE);
                }
            }
            else if (evlist[i].events & EPOLLOUT)
            {
                int ret = m_user_conn[sockfd].sendResponse();
                if (ret == HTTP_ERROR)
                {
                    fprintf(stderr, "Send response error\n");
                    m_user_conn[sockfd].closeConn();
                }
                else if (ret == HTTP_CLOSE)
                {
                    fprintf(stderr, "Send complete, shutdown connection\n");
                    shutdown(sockfd, SHUT_WR);
                }
            }
            else
            {
                fprintf(stderr, "Something else happened\n");
            }
        }
    }

    close(m_listenfd);
    return HTTP_OK;
}

int HTTPServer::run()
{
    addsig(SIGPIPE, SIG_IGN);

    if (initSocket() != HTTP_OK)
    {
        fprintf(stderr, "initSocket failed \n");
        return HTTP_ERROR;
    }

    return eventCycle();
}

