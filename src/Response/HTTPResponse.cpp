#include "HTTPResponse.h"

HttpResponse::HttpResponse() : m_response_body("") , m_data("")
{
}

HttpResponse::~HttpResponse()
{
}

const Protocol HttpResponse::getProtocol()
{
    return m_protocol;
}

void HttpResponse::setProtocol(const Protocol& p)
{
    m_protocol = p;
}

const size_t HttpResponse::getStatusCode()
{
    return m_status_code;
}

void HttpResponse::setStatusCode(const size_t code)
{
    m_status_code = code;
}

const string HttpResponse::getReasonPhrase()
{
    return m_reason_phrase;
}

int HttpResponse::setReasonPhrase()
{
    switch (m_status_code)
    {
        case 200:
            m_reason_phrase = "OK";
            break;
        case 201:
            m_reason_phrase = "Created";
            break;
        case 400:
            m_reason_phrase = "Bad Request";
            break;
        case 401:
            m_reason_phrase = "Unauthorized";
            break;
        case 403:
            m_reason_phrase = "Forbidden";
            break;
        case 404:
            m_reason_phrase = "Not Found";
            break;
        case 411:
            m_reason_phrase = "Length Required";
            break;
        case 500:
            m_reason_phrase = "Internal Server Error";
            break;
        case 501:
            m_reason_phrase = "Not Implemented";
            break;
        case 502:
            m_reason_phrase = "Bad Gateway";
            break;
        case 505:
            m_reason_phrase = "HTTP Version Not Supported";
            break;
        default:
            return HTTP_ERROR;
    }

    return HTTP_OK;
}

const string HttpResponse::getHttpHeaders(const string& name)
{
    for (auto head : m_http_headers)
    {
        if (head.first == name)
            return head.second;
    }

    return "";
}

const vector<std::pair<string,string>>& HttpResponse::getHttpHeadersVec()
{
    return m_http_headers;
}

void HttpResponse::setHttpHeaders(const string& name, const string& content)
{
    m_http_headers.push_back(std::make_pair(name, content));
}

const string* HttpResponse::getResponseBody()
{
    return &m_response_body;
}

void HttpResponse::setResponseBody(const string& res)
{
    m_response_body = res;
}

const string* HttpResponse::getResponseData()
{
    return &m_data;
}

void HttpResponse::addData(const char* buf, const int& len)
{
    m_data.append(buf,len);
}

const size_t HttpResponse::getResponseSize()
{
    return m_data.length();
}

void HttpResponse::printResponse()
{
    printf("------begin------\n");
    printf("%s\n", m_data.c_str());
    printf("-------end-------\n");
}

/* This function
 * for client
 */
int HttpResponse::copyToFile(std::ofstream& os)
{
    size_t content_length = atoi(getHttpHeaders("Content-Length").c_str());

    if (os.good()) {
        if (content_length)
            os.write(m_response_body.c_str(), content_length);
        else {
            fprintf(stderr, "WARNING:Content-Length Header not found.Written file might not be accurate\n");
            os.write(m_response_body.c_str(),m_response_body.length());
        }
    }

    if (os.bad())
        return HTTP_ERROR;

    return HTTP_OK;
}

/* This function
 * for client
 */
int HttpResponse::parseResponse()
{
    /* Response :
    * Status-Line(HTTP/1.0 <space> Status-Code <space> Reason-Phrase CRLF)
    * (Response-Header CRLF)
    * CRLF
    * [message-body]
    */

    size_t parse_checked = 0, parse_checking = 0;
    size_t header_parse_checked = 0, header_parse_checking = 0;
    string http_protocol, status_code, reason_phrase, response_header;
    string response_header_name, response_header_content;

    //HTTP Protocol
    parse_checking = m_data.find_first_of(" ", parse_checked);
    http_protocol = m_data.substr(parse_checked, parse_checking - parse_checked);
    parse_checked = parse_checking + 1;

    if (http_protocol == "HTTP/1.0")
    {
        m_protocol = Protocol::HTTP1_0;
    }
    else if (http_protocol == "HTTP/1.1")
    {
        m_protocol = Protocol::HTTP1_1;
    }
    else
    {
        m_protocol = Protocol::HTTP_UNKNOWN;
        return HTTP_OK;
    }

    //Status-Code
    parse_checking = m_data.find_first_of(" ", parse_checked);
    status_code = m_data.substr(parse_checked , parse_checking - parse_checked);
    m_status_code = atoi(status_code.c_str());
    parse_checked = parse_checking + 1;

    //Reason-Phrase
    parse_checking = m_data.find_first_of(CRLF, parse_checked);
    m_reason_phrase = m_data.substr(parse_checked, parse_checking - parse_checked);
    parse_checked = parse_checking + 2;

    //Response Headers
    while (1)
    {
        //Get whole header
        parse_checking = m_data.find_first_of(CRLF, parse_checked);
        parse_checking += 2;
        response_header = m_data.substr(parse_checked, parse_checking - parse_checked);
        parse_checked = parse_checking;

        header_parse_checked = header_parse_checking = 0;
        //Header Name
        header_parse_checking = response_header.find_first_of(":", header_parse_checked);
        response_header_name = response_header.substr(header_parse_checked, header_parse_checking - header_parse_checked);
        header_parse_checked = header_parse_checking + 2; //skip ":" and " "

        //Header Content
        header_parse_checking = response_header.find_first_of(CRLF, header_parse_checked);
        if (header_parse_checking == string::npos)
            fprintf(stderr, "nops!!\n");

        response_header_content = response_header.substr(header_parse_checked, header_parse_checking - header_parse_checked);
        header_parse_checked = header_parse_checking + 2;

        //Push the header
        setHttpHeaders(response_header_name, response_header_content);

        //Another CRLF
        if (m_data.substr(parse_checked,2) == CRLF)
            break;
    }

    parse_checked += 2;
    m_response_body = m_data.substr(parse_checked);

    return HTTP_OK;
}

int HttpResponse::prepareResponse()
{
    string protocol;

    switch (m_protocol)
    {
        case HTTP1_0:
            protocol = "HTTP/1.0";
            break;
        case HTTP1_1:
            protocol = "HTTP/1.1";
            break;
        default:
            fprintf(stderr, "UNKONW protocol\n");
            return HTTP_ERROR;
    }

    string stacode;
    std::stringstream is;
    is << m_status_code;
    is >> stacode;
    m_data += protocol + " " + stacode + " " + m_reason_phrase + CRLF;

    for (auto head : m_http_headers)
    {
        m_data += head.first + ": " + head.second;
        m_data += CRLF;
    }

    m_data += CRLF;
    m_data += m_response_body;

    return HTTP_OK;
}

void HttpResponse::reset()
{
    m_protocol = HTTP1_1;
    m_status_code = 0;
    m_reason_phrase.clear();
    m_http_headers.clear();
    m_response_body.clear();
    m_data.clear();
}

