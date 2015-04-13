CC=clang++
CXXFLAGS=-g -Wall -std=c++11 
CPP=Server.cc ServerMain.cc ./Request/HTTPRequest.cc ./Response/HTTPResponse.cc
objects:=Server.o ServerMain.o HTTPRequest.o HTTPResponse.o
sv:$(objects) 
	$(CC) $(objects) -o ../debug/sv -lpthread
$(objects):$(CPP)
	$(CC) -c $(CPP) $(CXXFLAGS)

.PHONY:clean
clean:
	-rm ../debug/sv $(objects)
