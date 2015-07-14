CC := clang++
CXXFLAGS := -g -Wall -std=c++11
CPP := Server.cc ServerMain.cc ./Request/HTTPRequest.cc ./Response/HTTPResponse.cc
SRCDIR := src
BINDIR := debug
TARGET := sv

#OBJECTS := $(shell find $(SRCDIR) -type f -name "*.o")
OBJECTS := Server.o ServerMain.o HTTPRequest.o HTTPResponse.o

$(TARGET):$(OBJECTS)
	$(CC) $(OBJECTS) -o ../$(BINDIR)/$(TARGET) -lpthread
$(OBJECTS): $(CPP)
	$(CC) -c $(CPP) $(CXXFLAGS)

.PHONY:clean
clean:
	-rm ../$(BINDIR)/$(TARGET)
