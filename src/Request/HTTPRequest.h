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

    Method getMethod( void );
    void setMethod ( const Method& );

    Protocol getVersion( void );
    void setVersion( const Protocol& );

    string getHostName( void );
    void setHostName( const string& );

    string getUrl( void );
    void setUrl( const string& );

    string getUserAgent( void );
    void setUserAgent( const string& );

    string& getRequestBody( void );
    void setRequestBody( const string& );

    string& getRequestData( void );

    string getHttpHeaders( const string& name );
    vector<std::pair<string,string>>& getHttpHeadersVec();
    void setHttpHeaders( const string& name , const string& content );

    std::size_t getRequestSize( void );
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
    vector<std::pair<string,string>> m_httpHeaders;   //首部名，首部值 组成一个pair 
   
    bool m_linger;             //HTTP Request是否保持链接

};
#endif
