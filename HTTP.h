/*************************************************************************
	> File Name: HTTP.h
	> Author: 
	> Mail: 
	> Created Time: 2014年11月03日 星期一 15时17分25秒
 ************************************************************************/

#ifndef _HTTP_H
#define _HTTP_H
using Method = enum method{GET,PUT,HEAD,POST,NOT_IMPLEMENTED};
using Protocol = enum protocol{HTTP1_0,HTTP1_1,HTTP_UNKOWN};
#define CR "\r"
#define LF "\n"
#define CRLF "\r\n"

#endif //_HTTP_H
