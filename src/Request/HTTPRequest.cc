#include <iostream>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include "HTTPRequest.h"

using namespace std;
HttpRequest::HttpRequest():m_requestBody(""),m_data("") 
{
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::addData( const char *con,const int &len )
{
    m_data.append(con,len);
}

void HttpRequest::addRequestBody(const string & RequestBody)
{
    m_requestBody += RequestBody;
}

void HttpRequest::printRequest()
{
    cout << "------begin------" << endl <<
        m_data << "------end------" << endl;
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
    return m_hostName;
}

void HttpRequest::setHostName(const string& name)
{
    m_hostName = name;
}

const string HttpRequest::getUrl()  
{
    return m_url;
}

void HttpRequest::setUrl(const string& Url)
{
    m_url = Url;
}

const string HttpRequest::getUserAgent()
{
    return m_userAgent;
}

void HttpRequest::setUserAgent(const string& usrAgent)
{
    m_userAgent = usrAgent;
}

const string& HttpRequest::getRequestBody() 
{
    return m_requestBody;
}

void HttpRequest::setRequestBody(const string& requestBody) 
{
    m_requestBody = requestBody;
}

const string& HttpRequest::getRequestData()
{
    return m_data;
}

const string HttpRequest::getHttpHeaders(const string& name)
{
    for (auto it = m_httpHeaders.begin(); it != m_httpHeaders.end();++it)
        if (it->first == name)
            return it->second;

    return nullptr;
}

void HttpRequest::setHttpHeaders(const string& headerName, const string& headerContent)
{
    m_httpHeaders.push_back(make_pair(headerName,headerContent));
}

const vector<pair<string,string>>& HttpRequest::getHttpHeadersVec()
{
    return m_httpHeaders;
}

const size_t HttpRequest::getRequestSize()
{
    return  m_data.length();
}

void HttpRequest::reset()
{
    m_requestBody.clear();
    m_data.clear();
    m_url.clear();
    m_userAgent.clear();
    m_hostName.clear();
    m_version = HTTP1_1;
    m_method = GET;
    m_httpHeaders.clear();
}

int HttpRequest::parseRequest()
{

    /* Request:
     * RequestLine(Method <space> RequestURI <space> HTTP1.1 CRLF)
     * RequestHead(HeaderName ":" <space> HeaderContent CRLF) *
     * CRLF
     * MessageBody *
     */
    size_t parseCursorChecking = 0,parseCursorChecked = 0;
    size_t HeaderParserCursorChecking = 0,HeaderParserCursorChecked = 0;
    string httpMethod,protocolVersion,requestHeader;
    string requestHeaderName,requestHeaderContent;

    //parse requestline
    //HTTP Method
    parseCursorChecking = m_data.find_first_of(" ",parseCursorChecked);
    httpMethod = m_data.substr(parseCursorChecked,parseCursorChecking - parseCursorChecked);
    parseCursorChecked = parseCursorChecking + 1;

    if (httpMethod == "GET") 
        m_method = GET;
    else if (httpMethod == "PUT")
        m_method = PUT;
    else {
        m_method = NOT_IMPLEMENTED;
        return 0;
    }

    //URI
    parseCursorChecking = m_data.find_first_of(" ",parseCursorChecked);
    m_url = m_data.substr(parseCursorChecked,parseCursorChecking - parseCursorChecked); 
    parseCursorChecked = parseCursorChecking + 1;

    //HTTP Protocol
    parseCursorChecking = m_data.find_first_of(CRLF,parseCursorChecked);
    protocolVersion = m_data.substr(parseCursorChecked,parseCursorChecking - parseCursorChecked);
    parseCursorChecked = parseCursorChecking + 2;                   //\r\n skip it

    if (protocolVersion == "HTTP/1.0") 
        m_version = HTTP1_0;
    else if (protocolVersion == "HTTP/1.1")
        m_version = HTTP1_1;
    else {
        m_version = HTTP_UNKNOWN;
        return 0;
    }
   
    //Headers
    while(1) {
        parseCursorChecking = m_data.find_first_of(CRLF,parseCursorChecked);
        parseCursorChecking += 2;                             //为了请求头中也包含CRLF,否则下面解析HeaderContent的时候HeaderParserCursorChecking会找不到CRLF
        requestHeader = m_data.substr(parseCursorChecked,parseCursorChecking - parseCursorChecked);
        parseCursorChecked = parseCursorChecking;             //skip CRLF

    HeaderParserCursorChecking = HeaderParserCursorChecked = 0;
    //Parse the Header
    //HeaderName
    HeaderParserCursorChecking = requestHeader.find_first_of(":",HeaderParserCursorChecked);
    requestHeaderName = requestHeader.substr(HeaderParserCursorChecked,HeaderParserCursorChecking - HeaderParserCursorChecked);
    HeaderParserCursorChecked = HeaderParserCursorChecking + 2;//skip ":" and " "

    //HeaderContent
    HeaderParserCursorChecking = requestHeader.find_first_of(CRLF,HeaderParserCursorChecked);
    if (HeaderParserCursorChecking == string::npos)
        cerr << "nops!!!!" << std::endl;
    requestHeaderContent = requestHeader.substr(HeaderParserCursorChecked,HeaderParserCursorChecking - HeaderParserCursorChecked);
    HeaderParserCursorChecked = HeaderParserCursorChecking + 2; //skip CRLF

    setHttpHeaders(requestHeaderName,requestHeaderContent);

    //another CRLF?
    if (m_data.substr(parseCursorChecked,2) == CRLF)
        break;

    }
    parseCursorChecked += 2;
    m_requestBody = m_data.substr(parseCursorChecked);

    return 0;
}

int HttpRequest::prepareRequest()
{

    //Method Protocol 为enum类型，需要定义两个string来做请求体拼接操作
    string httpMethod,protocol;
    switch (m_method) {
        case GET:
            httpMethod = "GET";
            break;
        case PUT:
            httpMethod = "PUT";
            break;
        default:
            return -1;
            break;
    }

    switch (m_version) { 
        case HTTP1_0:
            protocol = "HTTP/1.0";
            break;
        case HTTP1_1:
            protocol = "HTTP/1.1";
            break;
        default:
            return -1;
            break;
    }

    //组装请求体
    m_data += httpMethod + " " + m_url + " " + protocol + CRLF;

    for (auto head:m_httpHeaders)
        m_data += head.first + ": " + head.second + CRLF;
    m_data += CRLF;
    m_data += m_requestBody;
    return 0;
}

int HttpRequest::copy2File(ofstream& os)
{
    size_t contentLength = atoi(getHttpHeaders("Content-Length").c_str());
    if (os.good())
        os.write(m_requestBody.c_str(),contentLength);
    
    if (os.bad())
        return -1;

    return 0;
}

int HttpRequest::copyFromFile(ifstream& is,size_t contentLength)
{
    char buf[contentLength]; 
    memset(buf,'\0',contentLength);

    if (is.good())
    {
        is.read(buf, contentLength);
    }
    m_requestBody.append(buf,contentLength);

    if (is.bad())
        return -1;

    return 0;
}




