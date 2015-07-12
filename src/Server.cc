#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>

#include "Server.h"
//#include "threadpool.h"
//#include "lock.h"

extern int errno;

using namespace std;

int HTTPServer::m_epollfd;

void reset_oneshot( int epollfd,int fd )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

void addsig(int sig,void ( handler )(int),bool restart = true)
{
    struct sigaction sa;
    memset( &sa,'\0',sizeof(sa) );
    sa.sa_handler = handler;
    if(restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset( &sa.sa_mask );
    if (sigaction( sig,&sa,NULL ) == -1 )
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

int setNonBlocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

/* 把文件描述符fd上的EPOLLIN注册到epollfd指示的epoll内核事件表中，
 * enable_oneshot*/
void addfd( int epollfd,int fd, bool enable_onshot ) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if ( enable_onshot ) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl( epollfd,EPOLL_CTL_ADD,fd,&event );
    setNonBlocking(fd);
}

void modfd( int epollfd,int fd,int ev )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd, &event);
}

void removefd( int epollfd, int fd )
{
    epoll_ctl( epollfd,EPOLL_CTL_DEL,fd,0 );
    close(fd);
}

void HTTPServer::init_daemon(const char *pname, int facility)
{
	int pid;
	int i;
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP ,SIG_IGN);
	if((pid = fork()))
		exit(EXIT_SUCCESS);
	else if(pid< 0)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	setsid();
	if((pid=fork()))
		exit(EXIT_SUCCESS);
	else if(pid< 0)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}

	for(i=0;i< NOFILE;++i)
		close(i);
	umask(0);
	signal(SIGCHLD,SIG_IGN);
	openlog(pname, LOG_PID, facility);
	return;
}

HTTPServer::HTTPServer():servPort(80)
{
    bzero(&servaddr,sizeof(servaddr));
    bzero(&cliaddr,sizeof(cliaddr));
}

HTTPServer::HTTPServer(int port)
{
    if (setPort(port)) {
        fprintf(stderr,"Failed to set port\n");
    }
    bzero(&servaddr,sizeof(servaddr));
    bzero(&cliaddr,sizeof(cliaddr));
}

HTTPServer::~HTTPServer()
{
    close(listenfd);
    close(m_sockfd);
}

int HTTPServer::setPort( size_t port )
{
    if (port <= 1024 || port >= 65535)
    {
        fprintf(stderr,"%s\n",strerror(errno));
        return -1;
    }

    servPort = port;
    return 0;
}

int HTTPServer::initSocket()
{

    if (( listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0 )
    {
        fprintf(stderr,"Failed to create socket!\n");
        return -1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(servPort);

    socklen_t addrlen = sizeof(servaddr);
    if (( bind(listenfd,reinterpret_cast<struct sockaddr*>(&servaddr),addrlen)) < 0 )
    {
        fprintf(stderr," bind socket failed!\n");
        return -1;
    }

    if (( listen(listenfd,5)) < 0 )
    {
        fprintf(stderr,"listen failed\n");
        return -1;
    }

    return 0;
}

void HTTPServer::init()
{
    m_url.clear();
    m_mimeType.clear();
    m_httpRequest->reset();
    m_httpResponse->reset();
}

void HTTPServer::init_epfd(int connectfd)
{
    m_sockfd = connectfd;
    addfd(m_epollfd,connectfd,true);                        //对connfd开启oneshot 模式
}

int HTTPServer::run()
{

    addsig( SIGPIPE,SIG_IGN );                              //忽略sigpipe信号

    if ( initSocket() < 0 )
    {
        fprintf(stderr,"initSocket failed \n");
        return -1;
    }


    while (1) {
        int epfd = epoll_create(5);
        if (epfd == -1) {
            perror("epoll_create");
            exit(EXIT_FAILURE);
        }
        addfd( epfd, listenfd, false );       //listenfd cannot be oneshot
        HTTPServer::m_epollfd = epfd;

        //we use ET model here
        while(1) {
            int ready = epoll_wait( epfd,evlist,MAX_EVENTS,-1 );
            if (ready < 0) {
                fprintf(stderr,"epoll failure\n");
                break;
            }

            for (int i = 0;i < ready;i++) {
                int sockfd = evlist[i].data.fd;
                if ( sockfd == listenfd ) {
                    clilen = sizeof( cliaddr );
                    int connfd = accept( listenfd,reinterpret_cast<struct sockaddr *>(&cliaddr),&clilen );
                    if (connfd < 0)
                    {
                        perror("Accept");
                        continue;
                    }
                    init_epfd(connfd);
                } else if (evlist[i].events & EPOLLIN) {
                    printf(" event trigger \n");
                    m_sockfd = sockfd;
                if ( handleRequest() < 0 ) {
                        fprintf(stderr,"Failed handling request\n");
                        syslog(LOG_ERR,"Can't handling request (%s)",strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                } else if( evlist[i].events & (EPOLLHUP | EPOLLERR) ) {
                    printf("closing fd %d\n",evlist[i].data.fd);
                    if (close(evlist[i].data.fd) == -1) {
                        perror("close");
                        exit(EXIT_FAILURE);
                }
                } else {
                    fprintf(stderr," something else happened\n");
                }
            }
        }
    }

    close(listenfd);
    return 0;
}

int HTTPServer::handleRequest()
{
    m_httpRequest  = make_shared<HttpRequest>();
    m_httpResponse = make_shared<HttpResponse>();
    if (recvRequest() < 0) {
        fprintf(stderr,"Receiving request failed\n");
        return -1;
    }

    m_httpRequest->printRequest();

    if (parseRequest() < 0) {
        fprintf(stderr,"Parsing HTTP Request failed \n");
        return -1;
    }

    if (processRequest() < 0) {
        fprintf(stderr,"Processing HTTP Request failed\n");
        return -1;
    }

    if (prepareResponse() < 0) {
        fprintf(stderr,"Preparing reply failed\n");
        return -1;
    }
    printf("Response body:\n");
    m_httpResponse->printResponse();
    printf("Response body end\n");

    if (sendResponse() < 0) {
        fprintf(stderr,"Sending reply failed \n");
        return -1;
    }

    return 0;
}


const int HTTPServer::buf_size;
int HTTPServer::recvRequest()
{
    int recvlen;
    char* buf = new char[buf_size];

    while (1) {
        memset(buf,'\0',buf_size);
        setNonBlocking(m_sockfd);                            //set m_sockfd  nonblocking
        //recvlen = recv(m_sockfd,buf,buf_size,MSG_DONTWAIT);//nonblocking
        recvlen = recv(m_sockfd,buf,buf_size,0);

        if (recvlen < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                printf(" read later \n");
                reset_oneshot(m_epollfd,m_sockfd);           //一开始忘了重置oneshot，导致有些后续EPOLLIN事件无法被触发,害我调了大半天
                break;
            }
            else
            {
                close(m_sockfd);
                fprintf(stderr,"Receiving Request failed\n");
                return -1;
            }
        } else if ( recvlen == 0 ) {
            printf("recvlen = 0\n");
            close(m_sockfd);
        } else {
            printf("get %d bytes of content: %s\n",recvlen,buf);
        }
        m_httpRequest->addData(buf, recvlen);

        if (recvlen < buf_size )
            break;
    }

    return 0;
}

int HTTPServer::parseRequest()
{
    if (m_httpRequest->parseRequest() < 0) {
        fprintf(stderr,"failed parsing request\n");
        return -1;
    }
    return 0;
}

int HTTPServer::handleGET()
{
    ifstream ifs,errfs;
    ofstream ofs;
    ostringstream os;
    size_t contentLength;

    printf("RequestUrl is : %s\n",m_httpRequest->getUrl().c_str());
    if (m_httpRequest->getUrl() == "/")
    {
        m_url = SERV_ROOT + string("/index.html");
    } else {
        m_url = SERV_ROOT + m_httpRequest->getUrl();
    }
    m_mimeType = getMimeType(m_url);
    ifs.open(m_url.c_str(),ifstream::in);
    if (ifs.is_open()) {
        ifs.seekg(0,ifstream::end);
        contentLength = ifs.tellg();
        ifs.seekg(0,ifstream::beg);
        os << contentLength;
        if (m_httpResponse->copyFromFile(ifs, contentLength)) {
            fprintf(stderr,"Failed to copy file to Response Body\n");
            m_httpResponse->setStatusCode(500);   //Internal Server Error
        }
        m_httpResponse->setHttpHeaders("Content-Length", os.str());
    } else {
        ofs.close();
        string file404 = SERV_ROOT;
        file404 += "/404.html";
        errfs.open(file404.c_str(),ifstream::in);
        if (errfs.is_open()) {
            errfs.seekg(0,ifstream::end);
            contentLength = errfs.tellg();
            errfs.seekg(0,ifstream::beg);
            os << contentLength;

            if (m_httpResponse->copyFromFile(errfs, contentLength)) {
                fprintf(stderr,"failed to copy file to response body\n");
                m_httpResponse->setStatusCode(500); //Internal Server Error
                return 0;
            }
            m_httpResponse->setHttpHeaders("Content-Length", os.str());
            m_httpResponse->setStatusCode(404);     //Not found
            return 0;
        } else {
            fprintf(stderr,"Critical err . Shutting down\n");
            return -1;
        } //end if(errfs.is_open())
    } //end if (ifs.is_open())
    ifs.close();
    m_httpResponse->setStatusCode(200);              //OK
    return 0;
}

void HTTPServer::handlePUT()
{
    ofstream ofs;
    m_url = SERV_ROOT + m_httpRequest->getUrl();
    m_mimeType = getMimeType(m_url);

    ofs.open(m_url.c_str(),ifstream::out | ofstream::trunc);
    if (ofs.is_open()) {
        if (m_httpRequest->copy2File(ofs))
            m_httpResponse->setStatusCode(411);  //Length required
        else
            m_httpResponse->setStatusCode(201);  //Created
    } else {
        m_httpResponse->setStatusCode(403);      //Forbidden
    }
    ofs.close();
}

int HTTPServer::processRequest()
{
    Method method = m_httpRequest->getMethod();


    if (m_httpRequest->getVersion() == HTTP_UNKNOWN) {
        m_httpResponse->setStatusCode(505);         //HTTP Version Not Supported
        return 0;
    }
    switch(method) {
        case GET:
            return handleGET();

        case PUT:
            handlePUT();
            break;

        default:
            m_httpResponse->setStatusCode(501);          //Not Implemented
            break;
    }
    return 0;
}

int HTTPServer::prepareResponse()
{
    time_t curTime;
    time(&curTime);
    string curTimeStr = ctime(&curTime);
    replace(curTimeStr.begin(),curTimeStr.end(),'\n','\0');

    m_httpResponse->setProtocol(m_httpRequest->getVersion());
    m_httpResponse->setReasonPhrase();

    m_httpResponse->setHttpHeaders("Date", curTimeStr);
    m_httpResponse->setHttpHeaders("Server", "Sk NB http server");
    m_httpResponse->setHttpHeaders("Accept-Ranges", "bytes");
    m_httpResponse->setHttpHeaders("Content-Type", m_mimeType);
    m_httpResponse->setHttpHeaders("Connection", "close");

    if (m_httpResponse->prepareResponse() < 0) {
        fprintf(stderr,"Failed to prepare response\n");
        return -1;
    }

    return 0;
}

int HTTPServer::sendResponse()
{
    size_t responseSize = m_httpResponse->getResponseSize();
    const string* responseData = m_httpResponse->getResponseData();

    char * buf = new char[responseSize];
    printf("响应长度为%zu\n",responseSize);
    memset(buf,'\0',responseSize);
    memcpy(buf,responseData->c_str(),responseSize);

    if ((send(m_sockfd,buf,responseSize,0)) < 0) {
        fprintf(stderr,"Sending response failed\n");
        return -1;
    }

    delete buf;
    return 0;
}

string HTTPServer::getMimeType(string fileName)
{
   size_t extraPos = fileName.find_last_of(".");
   string extension;
   string mimeType = "text/plain, charset=utf-8";

   if (extraPos == string::npos) {
       extension = "";
   } else {
       extension = fileName.substr(extraPos + 1);
   }

   switch (extension[0]) {
        case 'b':
            if (extension == "bmp")
                mimeType = "image/bmp";
            if (extension == "bin")
                mimeType = "application/octet-stream";
            break;
        case 'c':
            if (extension == "css")
               mimeType = "text/css";
            if (extension == "csh")
               mimeType = "application/csh";
            break;
        case 'd':
            if (extension == "doc")
                mimeType = "application/msword";
            if (extension == "dtd")
                mimeType = "application/xml-dtd";
            break;
        case 'e':
            if (extension == "exe")
                mimeType = "application/octet-stream";
            break;
        case 'g':
            if (extension == "gif")
                mimeType = "image/gif";
            break;
        case 'h':
            if (extension == "html" || extension == "htm")
                mimeType = "text/html";
            break;
        case 'i':
            if (extension == "ico")
                mimeType = "image/x-icon";
            break;
        case 'j':
            if (extension == "jpeg" || extension == "jpg")
                mimeType = "image/jpeg";
            break;
        case 'l':
            if (extension == "latex")
                mimeType = "application/x-latex";
            break;
        case 'p':
            if (extension == "png")
                mimeType = "image/png";
            break;
        case 'r':
            if (extension == "rtf")
                mimeType = "text/rtf";
            break;
        case 's':
            if (extension == "sh")
                mimeType = "application/x-sh";
            break;
        case 't':
            if (extension == "tar")
                mimeType = "application/x-tar";
            if (extension == "txt")
                mimeType = "text/plain";
            if (extension == "tif" || extension == "tiff")
                mimeType = "image/tiff";
            break;
        case 'x':
            if (extension == "xml")
                mimeType = "application/xml";
            break;
        default:
            break;
   }

   return mimeType;
}
