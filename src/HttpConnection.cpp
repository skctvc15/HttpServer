#include "HttpConnection.h"

using namespace std;

int HttpConnection::m_epollfd = -1;
int HttpConnection::m_user_count = 0;

void epollResetOneshot(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLERR | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int setNonBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void epollAddfd(int epollfd, int fd, bool enable_onshot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLERR;
    if (enable_onshot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

void epollModfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP | EPOLLERR;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void epollRemovefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void HttpConnection::unmap()
{
    if (m_file_addr)
    {
        munmap(m_file_addr, m_file_stat.st_size);
        m_file_addr = 0;
    }
}

HttpConnection::HttpConnection()
{
    bzero(&m_addr, sizeof(m_addr));
}

HttpConnection::~HttpConnection()
{
    delete m_http_request;
    delete m_http_response;
}

void HttpConnection::init(int sockfd, const sockaddr_in& addr)
{
    m_sockfd = sockfd;
    m_addr = addr;
    initEpollfd(m_sockfd);
    m_user_count++;
    reset();
}

void HttpConnection::reset()
{
    m_read_index = 0;
    m_write_index = 0;
    m_url.clear();
    m_mime_type.clear();

    if (m_http_request)
        m_http_request->reset();

    if (m_http_response)
        m_http_response->reset();

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
}

void HttpConnection::initEpollfd(int connectfd)
{
    m_sockfd = connectfd;
    epollAddfd(m_epollfd, connectfd, true);
    int nodelay = 1;
    setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
}

void HttpConnection::closeConn()
{
    if (m_sockfd != -1)
    {
        epollRemovefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

void HttpConnection::process()
{
    if (handleRequest() != HTTP_OK)
    {
        fprintf(stderr, "Failed handling request\n");
        syslog(LOG_ERR, "Can't handling request (%s)", strerror(errno));
        closeConn();
    }

    epollModfd(m_epollfd, m_sockfd, EPOLLOUT);
}

int HttpConnection::handleRequest()
{
    m_http_request->printRequest();

    if (parseRequest() != HTTP_OK)
    {
        fprintf(stderr, "Parsing HTTP Request failed \n");
        return HTTP_ERROR;
    }

    if (processRequest() != HTTP_OK)
    {
        fprintf(stderr, "Processing HTTP Request failed\n");
        return HTTP_ERROR;
    }

    if (prepareResponse() != HTTP_OK)
    {
        fprintf(stderr, "Preparing reply failed\n");
        return HTTP_ERROR;
    }

    printf("Response body:\n");
    m_http_response->printResponse();
    printf("Response body end\n");

    return HTTP_OK;
}

int HttpConnection::recvRequest()
{
    int recvlen = 0;
    m_http_request  = new HttpRequest();
    m_http_response = new HttpResponse();

    if (m_read_index >= READ_BUFFER_SIZE)
        return HTTP_ERROR;

    while (1)
    {
        recvlen = recv(m_sockfd, m_read_buf + m_read_index, READ_BUFFER_SIZE - m_read_index, 0);

        if (recvlen < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                printf(" read later \n");
                epollResetOneshot(m_epollfd, m_sockfd);
                break;
            }
            else
            {
                fprintf(stderr, "Receiving Request failed\n");
                return HTTP_ERROR;
            }
        }
        else if (recvlen == 0)
        {
            return HTTP_CLOSE;
        }
        else
        {
            printf("get %d bytes of content: %s\n", recvlen, m_write_buf);
            m_http_request->addData(m_read_buf, recvlen);
            m_read_index += recvlen;
        }
    }

    return HTTP_OK;
}

int HttpConnection::parseRequest()
{
    if (m_http_request->parseRequest() < 0)
    {
        fprintf(stderr,"failed parsing request\n");
        return HTTP_ERROR;
    }
    return HTTP_OK;
}

int HttpConnection::handleGET()
{
    ostringstream os;
    size_t content_length;

    printf("RequestUrl is : %s\n", m_http_request->getUrl().c_str());

    if (m_http_request->getUrl() == "/")
    {
        m_url = SERV_ROOT + string("/index.html");
    }
    else
    {
        m_url = SERV_ROOT + m_http_request->getUrl();
    }
    m_mime_type = getMimeType(m_url);

    int fd = open(m_url.c_str(), O_RDONLY);
    if (fd > 0)
    {
        if (fstat(fd, &m_file_stat) < 0)
            fprintf(stderr, "fstat\n");

        content_length = m_file_stat.st_size;
        os << content_length;

        m_file_addr = static_cast<char*>(mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (m_file_addr == MAP_FAILED)
        {
            fprintf(stderr,"Failed to copy file to Response Body\n");
            m_http_response->setStatusCode(500);       //Internal Server Error
            close(fd);
            return HTTP_OK;
        }

        close(fd);
        m_http_response->setHttpHeaders("Content-Length", os.str());
        m_http_response->setStatusCode(200);
    }
    else
    {
        string file404 = SERV_ROOT;
        file404 += "/404.html";

        fd = open(file404.c_str(), O_RDONLY);
        if (fd < 0)
        {
            fprintf(stderr,"Critical err . Shutting down\n");
            return HTTP_ERROR;
        }
        else
        {
            if (fstat(fd, &m_file_stat) < 0)
                fprintf(stderr,"fstat\n");

            content_length = m_file_stat.st_size;
            os << content_length;
            m_file_addr = static_cast<char*>(mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0));

            if (m_file_addr == MAP_FAILED)
            {
                fprintf(stderr,"Failed to copy file to Response Body\n");
                m_http_response->setStatusCode(500);       //Internal Server Error
                close(fd);
                return HTTP_OK;
            }

            close(fd);
            m_http_response->setHttpHeaders("Content-Length", os.str());
            m_http_response->setStatusCode(404);
            return HTTP_OK;
        }
        m_http_response->setStatusCode(404);
        return HTTP_OK;
    }

    return HTTP_OK;
}

int HttpConnection::handlePUT()
{
    ofstream ofs;
    m_url = SERV_ROOT + m_http_request->getUrl();
    m_mime_type = getMimeType(m_url);

    ofs.open(m_url.c_str(), ifstream::out | ofstream::trunc);
    if (ofs.is_open())
    {
        if (m_http_request->copy2File(ofs) < 0)
            m_http_response->setStatusCode(411);  //Length required
        else
            m_http_response->setStatusCode(201);  //Created
    }
    else
    {
        m_http_response->setStatusCode(403);      //Forbidden
        return HTTP_ERROR;
    }

    ofs.close();
    return HTTP_OK;
}

int HttpConnection::processRequest()
{
    int ret;
    Method method = m_http_request->getMethod();

    if (m_http_request->getVersion() == HTTP_UNKNOWN)
    {
        m_http_response->setStatusCode(505);         //HTTP Version Not Supported
        return 0;
    }

    string connection = m_http_request->getHttpHeaders("Connection");

    if (connection != "")
    {
        if (strcmp(connection.c_str(), "keep-alive") == 0)
            m_linger = true;
        else
            m_linger = false;
    }

    switch(method)
    {
        case GET:
            ret = handleGET();
            break;

        case PUT:
            ret = handlePUT();
            break;

        default:
            m_http_response->setStatusCode(501);          //Not Implemented
            ret = HTTP_ERROR;
            break;
    }

    return ret;
}

int HttpConnection::prepareResponse()
{
    time_t curtime;
    time(&curtime);
    string curtime_str = ctime(&curtime);
    std::replace(curtime_str.begin(), curtime_str.end(), '\n', '\0');

    m_http_response->setProtocol(m_http_request->getVersion());
    m_http_response->setReasonPhrase();

    m_http_response->setHttpHeaders("Date", curtime_str);
    m_http_response->setHttpHeaders("Server", "SK http-server");
    m_http_response->setHttpHeaders("Accept-Ranges", "bytes");
    m_http_response->setHttpHeaders("Content-Type", m_mime_type);

    if (!m_linger)
        m_http_response->setHttpHeaders("Connection", "close");
    else
        m_http_response->setHttpHeaders("Connection", "Keep-Alive");

    if (m_http_response->prepareResponse() < 0)
    {
        fprintf(stderr, "Failed to prepare response\n");
        return HTTP_ERROR;
    }

    const char* response_data = m_http_response->getResponseData()->c_str();
    size_t response_size = m_http_response->getResponseSize();

    memcpy(m_write_buf, response_data, response_size);
    m_write_index += response_size;

    return HTTP_OK;
}

int HttpConnection::sendResponse()
{
    if (m_write_index == 0)
    {
        epollModfd(m_epollfd, m_sockfd, EPOLLIN);
        reset();
        return HTTP_OK;
    }

    if (m_write_index >= WRITE_BUFFER_SIZE)
    {
        fprintf(stderr, "Response is too large to hold in send buffer\n");
        return HTTP_ERROR;
    }

    printf("Response Size:%lu\n", m_write_index + m_file_stat.st_size);

    while(1)
    {
        iv[0].iov_base = m_write_buf;
        iv[0].iov_len  = m_write_index;
        iv[1].iov_base = m_file_addr;
        iv[1].iov_len  = m_file_stat.st_size;
        int sentn = writev(m_sockfd, iv, 2);
        if (sentn < 0)
        {
    	    if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
    	        epollModfd(m_epollfd, m_sockfd, EPOLLOUT);
    	        break;
    	    }
            else
            {
                fprintf(stderr, "%s Sending response failed\n", strerror(errno));
                unmap();
                return HTTP_ERROR;
    	    }
        }

        if (sentn >= m_write_index)
        {
            unmap();
            if (m_linger)
            {
                epollModfd(m_epollfd, m_sockfd, EPOLLIN);
                reset();
                break;
            }
            else
            {
                epollModfd(m_epollfd, m_sockfd, EPOLLIN);
                return HTTP_CLOSE;
            }
        }
    }

    return HTTP_OK;
}

string HttpConnection::getMimeType(string filename)
{
   size_t extra_pos = filename.find_last_of(".");
   string extension;
   string mime_type = "text/html, charset=utf-8";

   if (extra_pos == string::npos)
   {
       extension = "";
   }
   else
   {
       extension = filename.substr(extra_pos + 1);
   }

   switch (extension[0])
   {
        case 'b':
            if (extension == "bmp")
                mime_type = "image/bmp";
            if (extension == "bin")
                mime_type = "application/octet-stream";
            break;
        case 'c':
            if (extension == "css")
               mime_type = "text/css";
            if (extension == "csh")
               mime_type = "application/csh";
            break;
        case 'd':
            if (extension == "doc")
                mime_type = "application/msword";
            if (extension == "dtd")
                mime_type = "application/xml-dtd";
            break;
        case 'e':
            if (extension == "exe")
                mime_type = "application/octet-stream";
            break;
        case 'g':
            if (extension == "gif")
                mime_type = "image/gif";
            break;
        case 'h':
            if (extension == "html" || extension == "htm")
                mime_type = "text/html";
            break;
        case 'i':
            if (extension == "ico")
                mime_type = "image/x-icon";
            break;
        case 'j':
            if (extension == "jpeg" || extension == "jpg")
                mime_type = "image/jpeg";
            break;
        case 'l':
            if (extension == "latex")
                mime_type = "application/x-latex";
            break;
        case 'p':
            if (extension == "png")
                mime_type = "image/png";
            break;
        case 'r':
            if (extension == "rtf")
                mime_type = "text/rtf";
            break;
        case 's':
            if (extension == "sh")
                mime_type = "application/x-sh";
            break;
        case 't':
            if (extension == "tar")
                mime_type = "application/x-tar";
            if (extension == "txt")
                mime_type = "text/plain";
            if (extension == "tif" || extension == "tiff")
                mime_type = "image/tiff";
            break;
        case 'x':
            if (extension == "xml")
                mime_type = "application/xml";
            break;
        default:
            break;
   }

   return mime_type;
}


