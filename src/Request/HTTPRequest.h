/*************************************************************************
	> File Name: HTTPRequest.h
	> Author:skctvc15
	> Mail:
	> Created Time: 2014/06/25
 ************************************************************************/

#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H
#include <vector>
#include <string>
#include <iterator>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "../HTTP.h"

using std::string;
using std::vector;
class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    const Method getMethod();
    void setMethod(const Method&);

    const Protocol getVersion();
    void setVersion(const Protocol&);

    const string getHostName();
    void setHostName(const string&);

    const string getUrl();
    void setUrl(const string&);

    const string getUserAgent();
    void setUserAgent(const string&);

    const string& getRequestBody();
    void setRequestBody(const string&);

    const string& getRequestData();

    const string getHttpHeaders(const string& name);
    const vector<std::pair<string,string>>& getHttpHeadersVec();
    void setHttpHeaders(const string& name , const string& content);

    const std::size_t getRequestSize();
    int prepareRequest();
    int parseRequest();
    void printRequest();

    void reset();
    void addData(const char* , const int&);
    void addRequestBody(const string&);

    int copy2File(std::ofstream&);
    int copyFromFile(std::ifstream& , std::size_t);

private:
    Method m_method;           //请求方法
    Protocol m_version;        //HTTP协议版本
    string m_hostname;         //hostname
    string m_url;              //请求URL
    string m_user_agent;        //客户应用程序
    string m_request_body;
    string m_data;             //whole Request
    vector<std::pair<string,string>> m_http_headers; // name: header


};
#endif                         //_HTTPREQUEST_H
