CC := clang++
CXXFLAGS := -g -Wall -std=c++11
CPP := HttpConnection.cpp Server.cpp ServerMain.cpp ./Request/HTTPRequest.cpp ./Response/HTTPResponse.cpp
SRCDIR := src
BINDIR := debug
TARGET := sv

#OBJECTS := $(shell find $(SRCDIR) -type f -name "*.o")
OBJECTS := Server.o ServerMain.o HTTPRequest.o HTTPResponse.o HttpConnection.o

$(TARGET):$(OBJECTS)
	ls ../$(BINDIR) || mkdir ../$(BINDIR); $(CC) $(OBJECTS) -o ../$(BINDIR)/$(TARGET) -lpthread
$(OBJECTS): $(CPP)
	$(CC) -c $(CPP) $(CXXFLAGS)

.PHONY:clean
clean:
	-rm *.o ../$(BINDIR)/$(TARGET)
