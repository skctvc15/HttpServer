#include <iostream>
#include "HTTPResponse.h"

int main(void)
{
    HttpResponse htr;
    string headerName1 = "Content-type";
    string headerName2 = "Content-Length";
    string headerContent1 = "img/gif";
    string headerContent2 = "text/html";
    size_t statusCode = 404;
    string body = "Hello SK!";
    Protocol p = Protocol::HTTP1_1;
    htr.setHttpHeaders(headerName1,headerContent1);
    htr.setHttpHeaders(headerName2, headerContent2);
    htr.setHttpHeaders(headerName2, headerContent2);
    htr.setProtocol(p);
    htr.setStatusCode(statusCode);
    htr.setReasonPhrase();
    htr.setResponseBody(body);
    htr.prepareResponse();
    htr.printResponse();
    std::cout << "after parse " << std::endl;
    htr.parseResponse();

    htr.printResponse();

    return 0;
}
