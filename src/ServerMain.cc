#include "Server.h"
#include "threadpool.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    int port;
    //HTTPServer::init_daemon(argv[0],LOG_INFO);
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

    delete serv;
    return 0;
}
