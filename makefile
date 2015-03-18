all:Server.cc ServerMain.cc HTTPRequest.cc HTTPResponse.cc 
	clang++ Server.cc ServerMain.cc HTTPRequest.cc HTTPResponse.cc -o sv -std=c++11 -I/usr/include/i386-linux-gnu/c++/4.8 -g -O2 
