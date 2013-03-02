
# User defined
TARGET=vswitchd
SOURCES=$(wildcard *.cxx)
OBJECTS=$(SOURCES:%.cxx=%.o)
STDHEADERS=stdlib.h stdio.h string.h time.h signal.h pthread.h syslog.h errno.h vector list map deque fcntl.h sys/ioctl.h unistd.h poll.h sys/select.h sys/socket.h sys/types.h sys/uio.h netinet/in.h arpa/inet.h libconfig.h termios.h
PROCCOUNT=$(shell sysctl -n hw.ncpu)

# Standard
CXX=g++
CXXFLAGS:=-g -Wall -O0 -DWORKER_THREAD_COUNT=$(PROCCOUNT) -DDEBUG -D_DEBUG -Iinclude -I/usr/local/include -c
#CXXFLAGS:=-g -Wall -O0 -DWORKER_THREAD_COUNT=16 -DDEBUG -D_DEBUG -Iinclude -c
LD=g++
LIBS=-L/usr/local/lib -L/usr/local/lib
LDFLAGS=$(LIBS) -pthread

all: $(TARGET)
	@echo Done!

$(TARGET) : precomp.h.gch $(OBJECTS)
	@echo Linking $(TARGET)
	@$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.cxx
	@echo "Compiling module: $< -> $@"
	@$(CXX) $< $(CXXFLAGS) -include precomp.h -o $@

precomp.h.gch : precomp.h
	@echo Precompiled header
	@$(CXX) $< $(CXXFLAGS) -o $@

precomp.h:
	@for h in $(STDHEADERS); do echo "#include <$$h>">>precomp.h; done;
	@echo "// Application headers">>precomp.h;
	@for h in `ls ./include/`; do echo "#include \"$$h\"">>precomp.h; done;

clean:
	@echo Cleaning
	rm -f precomp.* *.o $(TARGET)

install: $(TARGET)
	@echo Installing
	cp -f ./$(TARGET) /usr/bin/$(TARGET)

remove:
	@echo Removing
	rm -f /usr/bin/$(TARGET)
