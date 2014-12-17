#include "Server.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    int port;
    HTTPServer *serv; 
    if (argc == 2) {
        port = atoi(argv[1]);
        serv = new HTTPServer(port);
    } else {
        serv = new HTTPServer();
    }

    if (serv->run()) {
        cerr << "starting HTTPServer failed" << endl;
    }
    free(serv);
    return 0;
}
