#CC=g++
CC=g++-4.9
AR=ar rcs
STRIP=strip -s
INCLUDE=/usr/local/arm-linux/include/libxml2
FREETDSLIB=/usr/lib/arm-linux-gnueabihf/libsybdb.so.5.0.0
PUGILIB=/usr/lib/arm-linux-gnueabihf/libpugixml.so.1.4

ncdir=/usr/local/arm-linux/netcdf/
INCLUDES=-I $(ncdir)include -I/usr/local/arm-linux/freetds/include
CPPUNITS=nport.cpp nporttcp.cpp nportnc.cpp queue_thread.cpp mssql.cpp licor.cpp \
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


OBJS=tcpclient.o ../arm/example/serial/sio.o
EXEC=tcpclient
NAME=nportraw

LDFLAGS+=-lmoxalib -lpthread
FNAME=status

all: $(NAME)

nportraw: $(NAME).cpp
		$(CC) -I $(INCLUDE) $(CCFLAGS) $(CPPUNITS) $(INCLUDES) -o $(NAME) $(NAME).cpp -lpthread -lnetcdf $(PUGILIB) $(FREETDSLIB)
#		$(CC) -I $(INCLUDE) $(CCFLAGS) $(CPPUNITS) $(INCLUDES) -o $(NAME) $(NAME).cpp -lpthread -lmoxalib -lxml2 -lnetcdf $(PUGILIB) $(FREETDSLIB)
#		$(CC) -o $(FNAME) $(FNAME).cpp $(LDFLAGS) 

#	./upload.sh
	mv nportraw /home/moxa/bin/nportraw
#	mv nportraw /home/moxa/bin/nportws
#	chmod 755 /home/moxa/bin/nportws

clean:
#	rm -fr *.o *.a $(NAME)


