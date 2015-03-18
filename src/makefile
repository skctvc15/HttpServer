CC=clang++
CXXFLAGS=-g -Wall -std=c++11
CPP=Server.cc ServerMain.cc ./Request/HTTPRequest.cc ./Response/HTTPResponse.cc
objects:=Server.o ServerMain.o HTTPRequest.o HTTPResponse.o
sv:$(objects) 
	$(CC) $(objects) -o ../bin/sv -lpthread
$(objects):$(CPP)
	$(CC) -c $(CPP) $(CXXFLAGS)

.PHONY:clean
clean:
	-rm ../bin/sv $(objects)
