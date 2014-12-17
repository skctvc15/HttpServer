#include "Server.h"
#include <syslog.h>
#include <algorithm>

extern int errno;

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


const int HTTPServer::buf_size;
int HTTPServer::recvRequest()
{
    int recvlen;
    char* buf = new char[buf_size];
    memset(buf,'\0',buf_size);
    if ( !(recvlen = recv(newsockfd,buf,buf_size,0)) ) {
        cerr << __FUNCTION__ << "Failed to receive request" << endl;
        return -1;
    }
    m_httpRequest->addData(buf, recvlen);

    while (1) {
        memset(buf,'\0',buf_size);
        //setNonBlocking(newsockfd);     //set newsockfd  nonblocking
        recvlen = recv(newsockfd,buf,buf_size,MSG_DONTWAIT);    //nonblocking
        //recvlen = recv(newsockfd,buf,buf_size,0);    

        if (recvlen < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) 
            break;
            else {
                cerr << __FUNCTION__ << "Failed receiving request (nonblocking)" << endl;
                return -1;
            }
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
        cerr <<__FUNCTION__ << "Failed parsing request" << endl;
        return -1;
    }

    return 0;
}

int HTTPServer::processRequest()
{
    Method method = m_httpRequest->getMethod();

    ifstream ifs,errfs;
    ofstream ofs;
    size_t contentLength;
    ostringstream os;

    if (m_httpRequest->getVersion() == HTTP_UNKNOWN) {
        m_httpResponse->setStatusCode(505);    //HTTP Version Not Supported
        return 0;
    }
    switch(method) {
        case GET: 
            m_url = SERV_ROOT + m_httpRequest->getUrl();
            m_mimeType = getMimeType(m_url);
            ifs.open(m_url.c_str(),ifstream::in);
            if (ifs.is_open()) {
                ifs.seekg(0,ifstream::end);
                contentLength = ifs.tellg();
                ifs.seekg(0,ifstream::beg);
                os << contentLength;
                if (m_httpResponse->copyFromFile(ifs, contentLength)) {
                    cerr << __FUNCTION__ << "Failed to copy file to Response Body" << endl;
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

                        cerr << __FUNCTION__ << "Failed to copy file to  Response Body" << endl;
                        m_httpResponse->setStatusCode(500); //Internal Server Error
                        return 0;
                    }
                    m_httpResponse->setHttpHeaders("Content-Length", os.str());
                    m_httpResponse->setStatusCode(404);  //Not found
                    return 0;
                } else {
                    cerr << "Critical err . Shutting down" << endl;
                    return -1;
                }
                
            }
        ifs.close();
        m_httpResponse->setStatusCode(200);              //OK
        break;

        case PUT:
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
    m_httpResponse->setHttpHeaders("Server", "Sk epoll threadpool http server");
    m_httpResponse->setHttpHeaders("Accept-Ranges", "bytes");
    m_httpResponse->setHttpHeaders("Content-Type", m_mimeType);
    m_httpResponse->setHttpHeaders("Connection", "close");

    if (m_httpResponse->prepareResponse()) {
        cerr << __FUNCTION__ << "Failed to prepare response" << endl;
        return -1;
    }

    return 0;
}

int HTTPServer::sendResponse()
{
    size_t responseSize = m_httpResponse->getResponseSize();
    string* responseData = m_httpResponse->getResponseData();

    char * buf = new char[responseSize];
    memset(buf,'\0',responseSize);
    memcpy(buf,responseData->c_str(),responseSize);

    if ((send(newsockfd,buf,responseSize,0)) < 0) {
        cerr << __FUNCTION__ << "Sending response failed" << endl;
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
