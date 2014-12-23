/*************************************************************************
	> File Name: unit.cpp
	> Author: 
	> Mail: 
	> Created Time: 2014年11月03日 星期一 21时21分08秒
 ************************************************************************/

#include<iostream>
#include "HTTPRequest.h"
using namespace std;

int main()
{
    HttpRequest ht;
    //Protocol pp = Protocol::HTTP1_1;
    Protocol pp = HTTP1_1;
    //Method m = Method::GET;
    Method m = GET;
    string headName1 = "Accept",headcontent1 = "text/html";
    string headName2 = "Accept",headcontent2 = "text/html";
    string url = "localhost";
    string UserAgent = "Chrome";
    string hostname = "sk";
    ht.setHttpHeaders(headName1,headcontent1);
    ht.setHttpHeaders(headName2,headcontent2);

    ht.setVersion(pp);
    ht.setMethod(m);
    ht.setUrl(url);
    ht.setUserAgent(UserAgent);
    ht.setHostName(hostname);
    ht.prepareRequest();
    ht.printRequest();
    
    ht.parseRequest();
    cout << "after parse " << endl;
    ht.printRequest();
    return 0;
}

