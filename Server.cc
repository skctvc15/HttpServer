#include "Server.h"
#include <syslog.h>

using namespace std;

HTTPServer::HTTPServer():servPort(80)
{
    bzero(&servaddr,sizeof(servaddr));
    bzero(&cliaddr,sizeof(cliaddr));
}

HTTPServer::HTTPServer(int port)
{
    if (setPort(port)) {
        cerr << __FUNCTION__ << "Failed to set port" << endl;
    }
    bzero(&servaddr,sizeof(servaddr));
    bzero(&cliaddr,sizeof(cliaddr));
}

HTTPServer::~HTTPServer()
{
    close(listenfd);
    close(newsockfd);
}

int HTTPServer::setPort( size_t port )
{
    if (port <= 1024 || port >= 65535)    
    {
        cerr << __FUNCTION__ << strerror(errno) << endl;
        return -1;
    }

    servPort = port;
    return 0;
}

int HTTPServer::initSocket()
{

    if (( listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0 )
    {
        cerr << __FUNCTION__ << " Failed to create socket! " << endl;
        return -1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(servPort);

    socklen_t addrlen = sizeof(servaddr);
    if (( bind(listenfd,(struct sockaddr*)&servaddr,addrlen)) < 0 )
    {
        cerr << __FUNCTION__ << " bind socket failed! " << endl;
        return -1;
    }

    if (( listen(listenfd,5)) < 0 )
    {
        cerr << __FUNCTION__ << " listen failed " << endl;
        return -1;
    }

    return 0;
}

int HTTPServer::run()
{

    if ( initSocket() < 0 )
    {
        cerr << __FUNCTION__ << "initSocket failed " << endl;
        return -1;
    }

    while (1) {

        //accept
        clilen = sizeof(cliaddr);
        if ((newsockfd = accept(listenfd,( struct sockaddr* )&cliaddr,&clilen)) < 0)
        {
            cerr << __FUNCTION__ << "accept failed !" << endl;
            return -1;
        }

        //multi-process for now
        switch(fork()) {
            case 0:
                close(listenfd);              //child close listenfd
                if (handleRequest()) {
                    cerr << __FUNCTION__ << "Failed handling request " << endl;
                    syslog(LOG_ERR,"Can't handling request (%s)",strerror(errno));
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            case -1:
                syslog(LOG_ERR, "Can't create child (%s)", strerror(errno));
                cerr << "fork failed! " << endl;
                close(listenfd);
                break;
            default:
                close(newsockfd);             //father close process-socket
                break;
        }
    }

    return 0;
}

int HTTPServer::handleRequest()
{
    m_httpRequest = new HttpRequest();
    m_httpResponse = new HttpResponse();
     
    if (recvRequest() < 0) {
        cerr << __FUNCTION__ << "Receiving request failed " << endl;
        return -1;
    }

    m_httpRequest->printRequest();

    if (parseRequest() < 0) {
        cerr << __FUNCTION__ << "Parsing HTTP Request failed " << endl;
        return -1;
    }

    if (processRequest() < 0) { 
        cerr << __FUNCTION__ <<  "Processing HTTP Request failed " << endl;
        return -1;
    }
    if (prepareResponse() < 0) { 
        cerr << __FUNCTION__ <<  "Preparing reply failed " << endl;
        return -1;
    }
    m_httpResponse->printResponse();

    if (sendResponse() < 0) {
        cerr << __FUNCTION__ << "Sending reply failed " << endl;
        return -1;
    }

    delete m_httpRequest;
    delete m_httpResponse;
    return 0;
}

int setNonBlocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

int HTTPServer::recvRequest()
{
    int recvlen;
    char* buf = new char[buf_sz];
    memset(buf,'\0',buf_sz);
    if ( !(recvlen = recv(newsockfd,buf,buf_sz,0)) ) {
        cerr << __FUNCTION__ << "Failed to receive request" << endl;
        return -1;
    }
    m_httpRequest->addData(buf, recvlen);

    while (1) {
        memset(buf,'\0',buf_sz);
        setNonBlocking(newsockfd);     //set newsockfd  nonblocking
        //recvlen = recv(newsockfd,buf,buf_sz,MSG_DONTWAIT);    //nonblocking
        recvlen = recv(newsockfd,buf,buf_sz,0);    

        if (recvlen < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) 
            break;
            else {
                cerr << __FUNCTION__ << "Failed receiving request (nonblocking)" << endl;
                return -1;
            }
        }
        m_httpRequest->addData(buf, recvlen);

        if (recvlen < buf_sz)
            break;
    }
    
    return 0;
}

int HTTPServer::parseRequest()
{
    if (m_httpRequest->parseRequest() < 0) {
        cerr <<__FUNCTION__ << "Failed parsing request" << endl;
        return -1;
    }

    return 0;
}

int HTTPServer::processRequest()
{
    return 0;
}

int HTTPServer::prepareResponse()
{
    return 0;
}

int HTTPServer::sendResponse()
{
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
               mimeType == "text/css";
           if (extension == "csh")
               mimeType == "application/csh";
           break; 
        case 'd':
           if (extension == "doc")
               mimeType == "application/msword";
           if (extension == "dtd")
               mimeType == "application/xml-dtd";
           break;
        case 'e':
           if (extension == "exe")
               mimeType = "application/octet-stream";
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
