/*************************************************************************
	> File Name: HTTPResponse.h
	> Author: 
	> Mail: 
	> Created Time: 2014年11月04日 星期二 09时58分16秒
 ************************************************************************/

#ifndef _HTTPRESPONSE_H
#define _HTTPRESPONSE_H
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "../HTTP.h"

using std::string;
using std::vector;
using std::size_t;

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    const Protocol getProtocol();
    void setProtocol(const Protocol&);

    const size_t getStatusCode();
    void setStatusCode(const size_t);

    const string getReasonPhrase();
    int setReasonPhrase();

    const string getHttpHeaders(const string&);
    const vector<std::pair<string,string>>& getHttpHeadersVec();
    void setHttpHeaders(const string&, const string&);

    const string* getResponseBody();
    void setResponseBody(const string&);

    const string* getResponseData();
    const size_t getResponseSize();

    void printResponse();
    void reset();
    void addData(const char*, const int&);

    int prepareResponse();
    int parseResponse();

    int copyToFile(std::ofstream&);

private:
    Protocol m_protocol;                            //协议版本
    size_t m_status_code;                            //状态码
    string m_reason_phrase;                          //原因

    vector<std::pair<string,string>> m_http_headers; //响应首部
    string m_response_body;                          //响应体

    string m_data;                                  //整个响应
};

#endif //_HTTPRESPONSE_H
