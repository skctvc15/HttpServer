#include "HTTPRequest.h"

using namespace std;
HttpRequest::HttpRequest() : m_request_body("") , m_data("")
{

}

HttpRequest::~HttpRequest()
{
    reset();
}

void HttpRequest::addData(const char *data, const int &len)
{
    m_data.append(data,len);
}

void HttpRequest::addRequestBody(const string & reqbody)
{
    m_request_body += reqbody;
}

void HttpRequest::printRequest()
{
    printf("------begin------\n");
    printf("%s\n", m_data.c_str());
    printf("------end------\n");
}

const Method HttpRequest::getMethod()
{
    return m_method;
}

void HttpRequest::setMethod(const Method& m)
{
    m_method = m;
}

const Protocol HttpRequest::getVersion()
{
    return m_version;
}

void HttpRequest::setVersion(const Protocol& p)
{
    m_version = p;
}

const string HttpRequest::getHostName()
{
    return m_hostname;
}

void HttpRequest::setHostName(const string& name)
{
    m_hostname = name;
}

const string HttpRequest::getUrl()
{
    return m_url;
}

void HttpRequest::setUrl(const string& url)
{
    m_url = url;
}

const string HttpRequest::getUserAgent()
{
    return m_user_agent;
}

void HttpRequest::setUserAgent(const string& usr_agent)
{
    m_user_agent = usr_agent;
}

const string& HttpRequest::getRequestBody()
{
    return m_request_body;
}

void HttpRequest::setRequestBody(const string& requestBody)
{
    m_request_body = requestBody;
}

const string& HttpRequest::getRequestData()
{
    return m_data;
}

const string HttpRequest::getHttpHeaders(const string& name)
{
    for (auto it = m_http_headers.begin(); it != m_http_headers.end(); ++it)
        if (it->first == name)
            return it->second;

    return "";
}

void HttpRequest::setHttpHeaders(const string& headerName, const string& headerContent)
{
    m_http_headers.push_back(make_pair(headerName,headerContent));
}

const vector<pair<string,string>>& HttpRequest::getHttpHeadersVec()
{
    return m_http_headers;
}

const size_t HttpRequest::getRequestSize()
{
    return  m_data.length();
}

void HttpRequest::reset()
{
    m_request_body.clear();
    m_data.clear();
    m_url.clear();
    m_user_agent.clear();
    m_hostname.clear();
    m_method = GET;
    m_version = HTTP1_1;
    m_http_headers.clear();
}

int HttpRequest::parseRequest()
{

    /* Request:
     * RequestLine(Method <space> RequestURI <space> HTTP1.1 CRLF)
     * RequestHead(HeaderName ":" <space> HeaderContent CRLF) *
     * CRLF
     * MessageBody *
     */
    size_t parse_cursor_checking = 0, parse_cursor_checked = 0;
    size_t header_parse_cursor_checking = 0, header_parse_cursor_checked = 0;
    string http_method, protocol_version, request_header;
    string request_header_name, request_header_content;

    //parse requestline
    //HTTP Method
    parse_cursor_checking = m_data.find_first_of(" ", parse_cursor_checked);
    http_method = m_data.substr(parse_cursor_checked, parse_cursor_checking - parse_cursor_checked);
    parse_cursor_checked = parse_cursor_checking + 1;

    if (http_method == "GET")
        m_method = GET;
    else if (http_method == "PUT")
        m_method = PUT;
    else {
        m_method = NOT_IMPLEMENTED;
        return HTTP_OK;
    }

    //URI
    parse_cursor_checking = m_data.find_first_of(" ", parse_cursor_checked);
    m_url = m_data.substr(parse_cursor_checked, parse_cursor_checking - parse_cursor_checked);
    parse_cursor_checked = parse_cursor_checking + 1;

    //HTTP Protocol
    parse_cursor_checking = m_data.find_first_of(CRLF, parse_cursor_checked);
    protocol_version = m_data.substr(parse_cursor_checked, parse_cursor_checking - parse_cursor_checked);
    parse_cursor_checked = parse_cursor_checking + 2;                   //\r\n skip it

    if (protocol_version == "HTTP/1.0")
        m_version = HTTP1_0;
    else if (protocol_version == "HTTP/1.1")
        m_version = HTTP1_1;
    else {
        m_version = HTTP_UNKNOWN;
        return HTTP_OK;
    }

    //Headers
    while(1) {
        parse_cursor_checking = m_data.find_first_of(CRLF, parse_cursor_checked);
        parse_cursor_checking += 2;                             //为了请求头中也包含CRLF,否则下面解析HeaderContent的时候header_parse_cursor_checking会找不到CRLF
        request_header = m_data.substr(parse_cursor_checked, parse_cursor_checking - parse_cursor_checked);
        parse_cursor_checked = parse_cursor_checking;             //skip CRLF

        header_parse_cursor_checking = header_parse_cursor_checked = 0;
        //Parse the Header
        //HeaderName
        header_parse_cursor_checking = request_header.find_first_of(":", header_parse_cursor_checked);
        request_header_name = request_header.substr(header_parse_cursor_checked, header_parse_cursor_checking - header_parse_cursor_checked);
        header_parse_cursor_checked = header_parse_cursor_checking + 2;//skip ":" and " "

        //HeaderContent
        header_parse_cursor_checking = request_header.find_first_of(CRLF, header_parse_cursor_checked);
        if (header_parse_cursor_checking == string::npos)
            fprintf(stderr, "nops!!\n");

        request_header_content = request_header.substr(header_parse_cursor_checked, header_parse_cursor_checking - header_parse_cursor_checked);
        header_parse_cursor_checked = header_parse_cursor_checking + 2; //skip CRLF

        setHttpHeaders(request_header_name, request_header_content);

        //another CRLF?
        if (m_data.substr(parse_cursor_checked, 2) == CRLF)
            break;
    }

    parse_cursor_checked += 2;
    m_request_body = m_data.substr(parse_cursor_checked);

    return HTTP_OK;
}

int HttpRequest::prepareRequest()
{
    string http_method,protocol;
    switch (m_method)
    {
        case GET:
            http_method = "GET";
            break;
        case PUT:
            http_method = "PUT";
            break;
        default:
            return HTTP_ERROR;
    }

    switch (m_version)
    {
        case HTTP1_0:
            protocol = "HTTP/1.0";
            break;
        case HTTP1_1:
            protocol = "HTTP/1.1";
            break;
        default:
            return HTTP_ERROR;
    }

    //组装请求体
    m_data += http_method + " " + m_url + " " + protocol + CRLF;

    for (auto head : m_http_headers)
        m_data += head.first + ": " + head.second + CRLF;
    m_data += CRLF;
    m_data += m_request_body;

    return HTTP_OK;
}

int HttpRequest::copy2File(ofstream& os)
{
    const char *cls = getHttpHeaders("Content-Length").c_str();
    if (!cls)
        return HTTP_ERROR;

    size_t content_length = atoi(cls);
    if (content_length && os.good())
        os.write(m_request_body.c_str(), content_length);

    if (os.bad())
        return HTTP_ERROR;

    return HTTP_OK;
}

int HttpRequest::copyFromFile(ifstream& is, size_t content_length)
{
    char buf[content_length];
    memset(buf, '\0', content_length);

    if (is.good())
    {
        is.read(buf, content_length);
    }
    m_request_body.append(buf, content_length);

    if (is.bad())
        return HTTP_ERROR;

    return HTTP_OK;
}




