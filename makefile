all:Server.cc ServerMain.cc HTTPRequest.cc HTTPResponse.cc 
	clang++ Server.cc ServerMain.cc HTTPRequest.cc HTTPResponse.cc -o svUnit -std=c++11 -g
