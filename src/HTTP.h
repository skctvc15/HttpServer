/*************************************************************************
	> File Name: HTTP.h
	> Author: 
	> Mail: 
	> Created Time: 2014年11月03日 星期一 15时17分25秒
 ************************************************************************/

#ifndef _HTTP_H
#define _HTTP_H
using Method = enum method{GET, PUT, HEAD, POST, NOT_IMPLEMENTED};
using Protocol = enum protocol{HTTP1_0, HTTP1_1, HTTP_UNKNOWN};
#define CR "\r"
#define LF "\n"
#define CRLF "\r\n"

#define SERV_ROOT "../www"

#define HTTP_OK 0
#define HTTP_ERROR -1
#define HTTP_CLOSE 1

#endif //_HTTP_H
