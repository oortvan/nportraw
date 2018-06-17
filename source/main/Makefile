#CC=g++
CC=g++
AR=ar rcs
STRIP=strip -s
INCLUDE=/usr/local/arm-linux/include/libxml2
PUGILIB=/usr/lib64/libpugixml.so.1.8

INCLUDES=/usr/include
CPPUNITS=nport.cpp nporttcp.cpp nportnc.cpp queue_thread.cpp licor.cpp \
	 	gill.cpp serial.cpp rotor.cpp movavg.cpp \
		../websocket/base64.cpp ../websocket/WebSocket.cpp ../websocket/sha1.cpp
#		 statusthread.cpp

CCFLAGS=\
	-w \
	-lz \
	-fPIC \
	-D r3f_size=5 -D numR3inp=0 \
	-D GDEBUG=1 \
	-D LDEBUG=0 \
	-D li_socket_silent=0 \
	-D INC_FROM_LICOR=1

NAME=nportraw

LDFLAGS+=-lmoxalib -lpthread
FNAME=status

all: $(NAME)

nportraw: $(NAME).cpp
		$(CC) -I $(INCLUDE) $(CCFLAGS) $(CPPUNITS) -o $(NAME) $(NAME).cpp -lpthread -lnetcdf $(PUGILIB)

#	./upload.sh
#	mv nportraw /home/moxa/bin/nportraw
#	mv nportraw /home/moxa/bin/nportws
#	chmod 755 /home/moxa/bin/nportws

clean:
#	rm -fr *.o *.a $(NAME)
