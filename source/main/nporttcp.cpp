// Copyright (C) 2016  KNMI, Cor van Oort
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with main.c; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA

#include	"nport.h"
#include	"serial.h"
//#include	"../websocket/WebSocket.h"

// extra declarations.
char buff[255] = {0},  cmd[255] = {0};//buff_tcp[255] = {0},

//WebSocketFrameType wframe;
//WebSocket wsock;
//c_socket web_client[max_clients];

static void	check_and_clear_buffer(struct buffer_struct *buf)
{
	if ( buf->rptr == BUF_LEN && buf->wptr == BUF_LEN )
		buf->rptr = buf->wptr = 0;
}

// open device over a serial connection
void open_serial(struct tcp_server_info *atcp){
	unsigned int intf;
	int flow;
	intf = RS_STRMODE(atcp->comport->intf.c_str());	//interface type
//	if (intf < 0) {slog << "Open "<< getenv("so_type") << " MODE error\n"; return;}
	if (intf < 0) dbgrot_printf("%s MODE ERROR: %s\n",atcp->src.c_str(), atcp->comport->intf.c_str());
	flow = RS_STRFLOW(atcp->comport->control.c_str()); 	//interface CONTROL type
//	if (flow < 0) {slog << "Open "<< getenv("so_type") << " FLOW error\n"; return;}
	atcp->tcpfd = SerialOpen( atcp->comport->cport );
	if (atcp->tcpfd < 0){
		if ( atcp->tcpfd == SERIAL_ERROR_OPEN) dbgrot_printf("SERIAL ERROR OPEN\n");
		exit(-1);
	}
//	if (ser_fd < 0) slog << "Open "<< getenv("so_type") << " COMPORT error\n";
	SerialSetParam(atcp->comport->cport, 0, 8, 1);
	SerialSetMode (atcp->comport->cport, intf);
	SerialSetSpeed( atcp->comport->cport, atcp->comport->baud);

	SerialFlowControl (atcp->comport->cport, flow);
	if (atcp->tcpfd){  // is connected
		sprintf(atcp->buff_tcp,"COMPORT:%d, dev:%s-%s, handle:%d, %d, %s, %s:%d INIT OK",
		           atcp->comport->cport,
		           atcp->src.c_str(),
		           atcp->type.c_str(),
		           atcp->tcpfd,
		           atcp->comport->baud,
		           atcp->comport->intf.c_str(),
		           atcp->comport->control.c_str(),
		           flow);
		tolog(atcp->buff_tcp, LOG_INFO);
	}
}	

int getSO_ERROR(int fd) {
   int err = 1;
   socklen_t len = sizeof err;
   if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
      dbgcrash_printf("CLOSE SOCKET: FATAL ERROR\n"); //FatalError("getSO_ERROR");
   if (err)
      errno = err;              // set errno to the socket SO_ERROR
   return err;
}

void closeSocket(int fd) {      // *not* the Windows closesocket()
	dbgcrash_printf("CLOSING SOCKET\n");	
   if (fd >= 0) {
      getSO_ERROR(fd); // first clear any errors, which can cause close to fail
      if (shutdown(fd, SHUT_RDWR) < 0) // secondly, terminate the 'reliable' delivery
         if (errno != ENOTCONN && errno != EINVAL) // SGI causes EINVAL
            dbgcrash_printf("CLOSE SOCKET:SHUTDOWN ERROR\n");
      if (close(fd) < 0) // finally call close()
         dbgcrash_printf("CLOSE SOCKET:CLOSE ERROR\n");
   }
}

void data_process(struct tcp_server_info *tcp)
{
	fd_set		readfds, writefds;
	fd_set*      pwritefds = NULL;   // init as NULL, use only when device.tx = 1
	struct timeval	time = {60, 0};  // timeout could also depend on tcp->dt
	int		j, maxfd = 0, tpipe = -1;
	long sec, usec;

#ifdef LICOR_BREAK	
	char cmd[100];
	int clen = 0, cmc = 0;
	char    ch;
	int     result;
#endif
	
	/*
		The timeout specifies the maximum time to wait. If you pass a null 
	 	pointer for this argument, it means to block indefinitely until one of 
	 	the file descriptors is ready. Otherwise, you should provide the time in 
	 	struct timeval format; see High-Resolution Calendar. Specify zero as the 
	 	time (a struct timeval containing all zeros) if you want to find out 
		which descriptors are ready without waiting if none are ready.
	 */ 


	// timeout fine tuning, process uses less cpu
	// this has a direct influence on the tstamp value, specially the slow
	// signals: exam dt=12 sec=9 then somewhere in the 9 sec timeout the sample
	// was taken, making it impossible to know where exact.
/*
	if (tcp->dt > 1){
		sec = 5; // 1 second resolution for tstamp
		usec = 0;
	}
	else {
		sec = 17;
		usec = 700000; // 0.7 second resolution for tstamp
	}
*/
#ifdef LICOR_BREAK
	if (tcp->licor != NULL) // add reader pipe in select loop for config communication?
	if (tcp->licor->haspipe > -1)
		tpipe = tcp->licor->fd[0];	
	if (tcp->licor != NULL) printf("LICOR PIPES: %d, %d\n",tcp->licor->fd[0],tcp->licor->fd[1]);
#endif
		
	while ( 1 ) {
		// initialize the file handle to select
		int rv;
		time.tv_usec = usec;
		time.tv_sec = sec;
		maxfd = 0;
		FD_ZERO(&readfds);
		// see timeout comment above
		//if (tcp->tx == 1){ FD_ZERO(&writefds); pwritefds = &writefds;}

		if ( tcp->tcpfd > maxfd )
			maxfd = tcp->tcpfd;
		if (tcp->tx == 1){
			if ( BUF_QUEUE(tcp->txbuf))
				FD_SET(tcp->tcpfd, &writefds);
		}	
		if ( BUF_FREE(tcp->rxbuf) )
			FD_SET(tcp->tcpfd, &readfds);

#ifdef LICOR_BREAK
		if (tpipe > 0) 
			FD_SET(tpipe, &readfds);
#endif
		
		tcp->ready = true;
		tcp->restart = 0;
		rv = select(maxfd+1, &readfds, pwritefds, NULL, 0);//&time);
		if (rv == 0) return;
	    else if (rv > 0){
			if ( FD_ISSET(tcp->tcpfd, &readfds) ) {  // look for char data from device
				if (tcp->fscan_msg(tcp) == 0){   	 // use the xml assigned message scan function
					tcp->ready = false;				 // use to signal other threads that this one is working OK or not
					tcp->tcpfd = -1;
					tcp->buff_tcp[0] = 0;
					tcp->len = 0;

					if (tcp->server == "COM") SerialClose(tcp->comport->cport);
					else close(tcp->tcpfd); //closeSocket(tcp->tcpfd); 
					sprintf(tcp->buff_tcp,"SOCKET FOR %s : %d : %s : %s DISCONECTED BY REMOTE SERVER",
					       tcp->ip.c_str(), tcp->tcpport, tcp->src.c_str(), tcp->type.c_str());
					dbgcrash_printf("%s\n",tcp->buff_tcp);
					tolog(tcp->buff_tcp, LOG_ERR);  // tolog has dbg_printf inside

					return;
				}	
			}

#ifdef LICOR_BREAK
			// pipe communication to send config data to sensor
			else if( tpipe > 0){
				if (FD_ISSET(tpipe, &readfds)){
					result = read (tpipe,&ch,1);
					if (result = 1){
						write(tcp->tcpfd, &ch, 1);
						if (ch == '\n'){ 
							//tcsendbreak(tcp->tcpfd,3);
							ch = 0x6;
							write(tcp->tcpfd, &ch, 1);
							printf("ACK\n");
						}	
/*
						if ( ch == '\n'){
							cmd[clen] = ch;
							clen++;
							cmd[clen] = 0;
							//write(tcp->tcpfd, &cmd, clen);
							printf("%4d:%s", cmc,cmd);
							cmc++;
							clen = 0;
						}
						else{
							cmd[clen] = ch;
							clen++;
						}
*/					
					}	
				}	
			}	
#endif
			else if (tcp->tx == 1){  // only when <tx>1</tx> in xml
				if ( FD_ISSET(tcp->tcpfd, &writefds) ) {
					j = write(tcp->tcpfd, &tcp->txbuf->buffer[tcp->txbuf->rptr], BUF_QUEUE(tcp->txbuf));
					if ( j <= 0 ) {
						dbg_printf("TCP port [%d] disconnect !\n", tcp->tcpport);
						return;
					}
					tcp->txbuf->rptr += j;
					check_and_clear_buffer(tcp->txbuf);
				}
			}
		}
		else if (rv < 0) dbg_printf("socket error\n"); 
		// end if select

	}
}


void *tcp_thread(void *ptr){
	int	f, j, yes=1,  interval=3, keep_cnt=10, idle=10;
	socklen_t optlen = sizeof(int);
	//struct sockaddr_in	des;
	struct tcp_server_info	*tcp=(struct tcp_server_info *)ptr;
	tcp->restart=0;

	
	while ( 1 ) {
		if (tcp->server == "COM") open_serial(tcp); // serial interface setting/opening

		else if (tcp->server == "IP"){				// network interface setting

			tcp->tcpfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if ( tcp->tcpfd <= 0 ) {
				dbgcrash_printf("Open socket fail !\n");
				sleep(1);
				continue;
			}

			fcntl(tcp->tcpfd, F_SETFL, O_NONBLOCK);

			if (setsockopt(tcp->tcpfd,SOL_SOCKET, SO_REUSEADDR, &yes, optlen) < 0) 
			         dbgcrash_printf("SO_REUSEADDR ERROR\n");
			if (setsockopt(tcp->tcpfd,SOL_SOCKET, SO_KEEPALIVE, &yes, optlen) < 0) 
			         dbgcrash_printf("SO_KEEPALIVE ERROR\n");
			if (setsockopt(tcp->tcpfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, optlen) < 0) 
			         dbgcrash_printf("TCP_KEEPIDLE ERROR\n");
			if (setsockopt(tcp->tcpfd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, optlen) < 0) 
			         dbgcrash_printf("TCP_KEEPINTVL ERROR\n");
			if (setsockopt(tcp->tcpfd, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, optlen) < 0) 
			         dbgcrash_printf("TCP_KEEPCNT ERROR\n");
			//set master socket to allow multiple connections , 
			if (tcp->type == "WEBSOCK"){  // websocket create listening bindet socket
				// setting options is just a good habit, it will work without this
				//printf("WEBSOCk SERVER ENTRYPOINT\n");
				//if( setsockopt(tcp->tcpfd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) < 0 ){
					// some error handling
				//	printf("sockoption error\n");
				//}
				tcp->des.sin_family = AF_INET;
				tcp->des.sin_port = htons(tcp->tcpport);
				tcp->des.sin_addr.s_addr = INADDR_ANY;
				if (bind(tcp->tcpfd, (struct sockaddr *)&tcp->des, sizeof(tcp->des))<0){
					// some socket handling
					printf("websock binding error\n");
				}
				if (listen(tcp->tcpfd, 5) < 0){
					// some error handling
					printf("listen error\n");
				}
				ws_printf("WEBSOCKET LISTEN PORT %d SET\n", tcp->tcpport);
			}
			else{  // source is active device sending data
				tcp->des.sin_family = AF_INET;
				tcp->des.sin_port = htons(tcp->tcpport);
				tcp->des.sin_addr.s_addr = inet_addr(tcp->ip.c_str());
				j = sizeof(tcp->des);
				f = 1;
			
				sleep(tcp->delay);

				if (tcp->restart == 1) dbgcrash_printf("RESTARTING SOCKET\n");
				while ( connect(tcp->tcpfd, (struct sockaddr *)&tcp->des, j) < 0 ) {
					dbgcrash_printf("TCP port [%s][%d] connect to server fail, socket:%d !\n", tcp->ip.c_str(), tcp->tcpport, tcp->tcpfd);
					f++;
					sleep(2);
				}
				sprintf(tcp->buff_tcp,"TCP SOCKET %s:%d (%s-%s)  after %d attempts: CONNECT OK\n", 
					       tcp->ip.c_str(), tcp->tcpport, tcp->src.c_str(), tcp->type.c_str(), f);
				if (tcp->restart == 1) {
					dbgcrash_printf("%s",tcp->buff_tcp);
				}	
				tolog(tcp->buff_tcp, LOG_INFO);
				//dbg_printf("\n%s\n",tcp->buff_tcp);
			}
		}
		
		if (tcp->type == "WEBSOCK"){  // do nothing for now
			if (proj.wsock)  // is websock object created
				proj.wsock->websock_process_0(tcp); // enter the websock select loop here
			pthread_exit(NULL);
		}	

		else data_process(tcp);// start to receive/transfer data
		dbg_printf("\nTCP port [%d] data process return !", tcp->tcpport);
		
		// stop data processing and try a restart
		if (tcp->tx == 1) tcp->txbuf->rptr = tcp->txbuf->wptr = 0;
		tcp->rxbuf->rptr = tcp->rxbuf->wptr = 0;

		sprintf(tcp->buff_tcp,"THREAD %s:%s DATA LOOP RESTART", proj.name.c_str(), tcp->src.c_str());
		tcp->restart = 1;
		sleep(1);  // wait a long time for server to come up after restart server

		dbgcrash_printf("%s\n",tcp->buff_tcp);
		tolog(tcp->buff_tcp, LOG_INFO);

	}
	pthread_exit(NULL);
}

/*
Creating a Thread With a Specified Priority
You can set the priority attribute before creating the thread. 
The child thread is created with the new priority that is specified in the 
sched_param structure (this structure also contains other scheduling information).

It is always a good idea to get the existing parameters, change the priority, 
xxx the thread, and then reset the priority.

Example 3-2 Creating a Prioritized Thread

#include <pthread.h>
#include <sched.h>

pthread_attr_t tattr;
pthread_t tid;
int ret;
int newprio = 20;
sched_param param;

// initialized with default attributes 
ret = pthread_attr_init (&tattr);

// safe to get existing scheduling param
ret = pthread_attr_getschedparam (&tattr, &param);

// set the priority; others are unchanged 
param.sched_priority = newprio;

// setting the new scheduling param 
ret = pthread_attr_setschedparam (&tattr, &param);

// with new priority specified 
ret = pthread_create (&tid, &tattr, func, arg); 

*/

int	tcp_init_list(void){  // from list of structs
	//int			i;
	struct tcp_server_info	*tcp;

	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end()){
		tcp = (*lt); // get device structure from list

		if (tcp->tx == 1){ // only needed when declared
			tcp->txbuf = (struct buffer_struct*) malloc(sizeof(struct buffer_struct));
			if ( tcp->txbuf == NULL ) {
				dbg_printf("Allocate transmit buffer fail !\n");
				return -1;
			}
			memset(tcp->txbuf, 0, sizeof(struct buffer_struct));
		}	

		tcp->rxbuf = (struct buffer_struct*) malloc(sizeof(struct buffer_struct));
		tcp->rxbuf->rptr = 0;
		if ( tcp->rxbuf == NULL ) {
			dbg_printf("Allocate transmit buffer fail !\n");
			return -1;
		}
		memset(tcp->rxbuf, 0, sizeof(struct buffer_struct));

		tcp->msgbuf = (struct buffer_struct*) malloc(sizeof(struct buffer_struct));
		if ( tcp->msgbuf == NULL ) {
			dbg_printf("Allocate message buffer fail !\n");
			return -1;
		}
		memset(tcp->msgbuf, 0, sizeof(struct buffer_struct));

		if ( pthread_create(&tcp->pthd, NULL, tcp_thread, (void *)tcp) < 0 ) {
			dbg_printf("Pthread create fail !\n");
			return -1;
		}

		//printf("%s: %d\n",tcp->src.c_str(), tcp->pthd);

		dbg_printf("TCP port [%d] initialize OK\n", tcp->tcpport);

		lt++;
	}
}

#ifdef LICOR_BREAK
void *pwriter(void *arguments){
	char* linitstr = "(Outputs(RS232(Freq 0.2)))\n(Outputs(RS232 ?))\n";
	//char* linitstr = "(Program(Reset TRUE))\n";
	int len = strlen(linitstr);
	while (1){
		sleep(3);
		if (lipipe > -1){
			write (lipipe, linitstr, len );
		}	
	}	
}
#endif
