/*************************************************************************
	> File Name: HTTPRequest.h
	> Author: 
	> Mail: 
	> Created Time: 2014年11月03日 星期一 15时26分04秒
 ************************************************************************/

#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H
#include <vector>
#include <string>
#include <fstream>
#include "../HTTP.h"

using std::string;
using std::vector;
class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    const Method getMethod( void );
    void setMethod ( const Method& );

    const Protocol getVersion( void );
    void setVersion( const Protocol& );

    const string getHostName( void );
    void setHostName( const string& );

    const string getUrl( void );
    void setUrl( const string& );

    const string getUserAgent( void );
    void setUserAgent( const string& );

    const string& getRequestBody( void );
    void setRequestBody( const string& );

    const string& getRequestData( void );

    const string getHttpHeaders( const string& name );
    const vector<std::pair<string,string>>& getHttpHeadersVec();
    void setHttpHeaders( const string& name , const string& content );

    const std::size_t getRequestSize( void );
    int prepareRequest( void );
    int parseRequest( void );

    void printRequest( void );
    void addData( const char* , const int& );
    void addRequestBody( const string& );

    int copy2File( std::ofstream& );
    int copyFromFile( std::ifstream& , std::size_t );

    void reset();



private:
    Method m_method;           //请求方法
    Protocol m_version;        //HTTP协议版本
    string m_hostName;         //hostname
    string m_url;              //请求URL
    string m_userAgent;        //客户应用程序
    string m_requestBody;      //请求体
    string m_data;             //整个请求
    vector<std::pair<string,string>> m_httpHeaders;
                               //首部名，首部值 组成一个pair


};
#endif                         //_HTTPREQUEST_H
