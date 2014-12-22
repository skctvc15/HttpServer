#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "HTTPResponse.h"

using std::cout;
using std::endl;

HttpResponse::HttpResponse():m_data(""),m_responseBody("")
{
}

HttpResponse::~HttpResponse()
{
}

Protocol HttpResponse::getProtocol()
{
    return m_protocol;
}

void HttpResponse::setProtocol( const Protocol& p )
{
    m_protocol = p;
}

size_t HttpResponse::getStatusCode()
{
    return m_statusCode;
}

void HttpResponse::setStatusCode( const size_t code )
{
    m_statusCode = code;
}

string HttpResponse::getReasonPhrase()
{
    return m_reasonPhrase;
}

int HttpResponse::setReasonPhrase()
{
    switch (m_statusCode) {
        case 200:
            m_reasonPhrase = "OK";
            break;
        case 201:
            m_reasonPhrase = "Created";
            break;
        case 400:
            m_reasonPhrase = "Bad Request";
            break;
        case 401:
            m_reasonPhrase = "Unauthorized";
            break;
        case 403:
            m_reasonPhrase = "Forbidden";
            break;
        case 404:
            m_reasonPhrase = "Not Found";
            break;
        case 411:
            m_reasonPhrase = "Length Required";
            break;
        case 500:
            m_reasonPhrase = "Internal Server Error";
            break;
        case 501:
            m_reasonPhrase = "Not Implemented";
            break;
        case 502:
            m_reasonPhrase = "Bad Gateway";
            break;
        case 505:
            m_reasonPhrase = "HTTP Version Not Supported";
            break;
        default:
            return -1;
            break;

    }
    return 0;
}

string HttpResponse::getHttpHeaders( const string& name )
{
    for (auto head : m_httpHeaders)
        if (head.first == name)
            return head.second;
    return nullptr;
}

vector<std::pair<string,string>>& HttpResponse::getHttpHeadersVec()
{
    return m_httpHeaders;
}

void HttpResponse::setHttpHeaders( const string& name , const string& content ) 
{
    m_httpHeaders.push_back(std::make_pair(name,content));
}

string* HttpResponse::getResponseBody()
{
    return &m_responseBody;
}

void HttpResponse::setResponseBody( const string& res )
{
    m_responseBody = res;
}

string* HttpResponse::getResponseData()
{
    return &m_data;
}

void HttpResponse::addData( const char* buf , const int& len )
{
    m_data.append(buf,len);
}

size_t HttpResponse::getResponseSize()
{
    return m_data.length();
}

void HttpResponse::printResponse()
{
    cout << "------begin------" << endl << m_data << endl
         << "-------end-------" << endl;
}


int HttpResponse::copyToFile( std::ofstream& os )
{
    size_t contentLength = atoi(getHttpHeaders("Content-Length").c_str());

    if (os.good()) {
        if (contentLength)
            os.write(m_responseBody.c_str(), contentLength);
        else {
            std::cerr << "WARNING:Content-Length Header not found.Written file might not be accurate" << endl;
            os.write(m_responseBody.c_str(),m_responseBody.length());
        }
    }

    if (os.bad())
        return -1;

    return 0;
}

int HttpResponse::copyFromFile( std::ifstream& is , size_t len)
{
    char* buf = new char[len];
    memset(buf,'\0',len);
    if (is.good())
        is.readsome(buf, len);
    m_responseBody.append(buf,len);
     
    if (is.bad())
        return -1;

    return 0;
}

int HttpResponse::parseResponse()
{
    /*
     * Response :
     * Status-Line(HTTP/1.0 <space> Status-Code <space> Reason-Phrase CRLF)
     * (Response-Header CRLF)*
     * CRLF
     * [message-body]
     */
    
    size_t parseChecked = 0,parseChecking = 0;
    size_t headerParseChecked = 0,headerParseChecking = 0;
    string httpProtocol,statusCode,reasonPhrase,responseHeader;
    string responseHeaderName,responseHeaderContent;

    //HTTP Protocol
    parseChecking = m_data.find_first_of(" ",parseChecked);
    httpProtocol = m_data.substr(parseChecked,parseChecking - parseChecked);
    parseChecked = parseChecking + 1;

    if (httpProtocol == "HTTP/1.0") {
        m_protocol = Protocol::HTTP1_0;
    } else if (httpProtocol == "HTTP/1.1"){
        m_protocol = Protocol::HTTP1_1;
    } else {
        m_protocol = Protocol::HTTP_UNKNOWN;
        return 0;
    }

    //Status-Code
    parseChecking = m_data.find_first_of(" ",parseChecked);
    statusCode = m_data.substr(parseChecked , parseChecking - parseChecked);
    m_statusCode = atoi(statusCode.c_str());
    parseChecked = parseChecking + 1;

    //Reason-Phrase
    parseChecking = m_data.find_first_of(CRLF,parseChecked);
    m_reasonPhrase = m_data.substr(parseChecked,parseChecking - parseChecked);
    parseChecked = parseChecking + 2;

    //Response Headers
    while (1) {
        //First step:Get whole header
        parseChecking = m_data.find_first_of(CRLF,parseChecked);
        parseChecking += 2;
        responseHeader = m_data.substr(parseChecked,parseChecking - parseChecked);
        parseChecked = parseChecking;

        headerParseChecked = headerParseChecking = 0;
        //Header Name
        headerParseChecking = responseHeader.find_first_of(":",headerParseChecked);
        responseHeaderName = responseHeader.substr(headerParseChecked,headerParseChecking - headerParseChecked);
        headerParseChecked = headerParseChecking + 2; //skip ":" and " "

        //Header Content
        headerParseChecking = responseHeader.find_first_of(CRLF,headerParseChecked);
        if (headerParseChecking == string::npos)
            std::cerr << "npos!!!!!" << std::endl;
        responseHeaderContent = responseHeader.substr(headerParseChecked,headerParseChecking - headerParseChecked);
        headerParseChecked = headerParseChecking + 2; 

        //push the header
        //m_httpHeaders.push_back(std::make_pair(responseHeaderName,responseHeaderContent));
          setHttpHeaders(responseHeaderName,responseHeaderContent);


        //another CRLF
        if (m_data.substr(parseChecked,2) == CRLF)
            break;
    }
    parseChecked += 2;
    m_responseBody = m_data.substr(parseChecked);

    return 0;
}

int HttpResponse::prepareResponse()
{
    string protocol;

    switch (m_protocol) {
        case Protocol::HTTP1_0:
            protocol = "HTTP/1.0";
            break;
        case Protocol::HTTP1_1:
            protocol = "HTTP/1.1";
            break;
        default:
            cout << "UNKONW protocol" << endl;
            return -1;
            break;

    }
    
    string staCode;
    std::stringstream is;
    is << m_statusCode;
    is >> staCode;
    m_data += protocol + " " + staCode + " " + m_reasonPhrase + CRLF;

    /*size_t h = 0,nh = 0;
    if ((nh = m_data.find_first_of(CRLF,h)))
        cout << "CRLF exist in : " << nh << endl;
    h = nh + 2;
    */

    for (auto head : m_httpHeaders)
    {
        m_data += head.first + ": " + head.second;
        m_data += CRLF;
    }
    //cout << "m_data Length  " << m_data.length() << endl;

    /*if ((nh = m_data.find_first_of(CRLF,h)))
        cout << "CRLF exist in : " << nh << endl;
    h = nh + 2;
    */

    m_data += CRLF;
    m_data += m_responseBody;
    //cout << "m_data Length  " << m_data.length() << endl;
    /*if ((nh = m_data.find_first_of(CRLF,h)))
        cout << "CRLF exist in : " << nh << endl;
        */

    return 0;
}

void HttpResponse::reset()
{
    m_protocol = HTTP1_1;
    m_statusCode = 0;
    m_reasonPhrase.clear();
    m_httpHeaders.clear();
    m_responseBody.clear();
    m_data.clear();
}
