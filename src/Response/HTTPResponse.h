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
#include "../HTTP.h"

using std::string;
using std::vector;
using std::size_t;

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    const Protocol getProtocol( void );
    void setProtocol( const Protocol& );

    const size_t getStatusCode( void );
    void setStatusCode(const size_t );

    const string getReasonPhrase( void );
    int setReasonPhrase();

    const string getHttpHeaders( const string& name );
    const vector<std::pair<string,string>>& getHttpHeadersVec( void );
    void setHttpHeaders( const string& name , const string& content );

    const string* getResponseBody( void );
    void setResponseBody( const string& );

    const string* getResponseData( void );
    const size_t getResponseSize( void );

    void printResponse( void );
    void addData( const char* , const int& );

    int prepareResponse( void );
    int parseResponse( void );

    int copyToFile( std::ofstream& );
    int copyFromFile( std::ifstream& , size_t );
    void reset();

private:
    Protocol m_protocol;                            //协议版本
    size_t m_statusCode;                            //状态码
    string m_reasonPhrase;                          //原因

    vector<std::pair<string,string>> m_httpHeaders; //响应首部
    string m_responseBody;                          //响应体

    string m_data;                                  //整个响应
};

#endif //_HTTPRESPONSE_H
