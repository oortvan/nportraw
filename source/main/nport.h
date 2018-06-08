/***************************************************************************
 *            nport.h
 *
 *  Fri September 23 08:20:42 2016
 *  Copyright  2016  KNMI
 *  
 ****************************************************************************/
/*
 * Copyright (C) 2016  KNMI
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#ifndef NPORT_H
#define NPORT_H

//#define LICBUG  // time in licor scan msg
//#define NCWBUG  // licor time in netcdf writing
#define phpline 0 // write to /home/httpd/htdocs/nport/....
#define despike 1 // write spiked frame to /home/httpd/htdocs/nport/....

#define		rotorstatus 0  // only when a rotor is present in EB field app

//#define DEBUGWS_ON
#ifdef DEBUGWS_ON
#define ws_printf(x...)	printf(x)
#else
#define ws_printf(x...)
#endif

//#define DEBUG_ON
#ifdef DEBUG_ON
#define dbg_printf(x...)	printf(x)
#else
#define dbg_printf(x...)
#endif

#define DEBUGNC_ON
#ifdef DEBUGNC_ON
#define dbgnc_printf(x...)	printf(x)
#else
#define dbgnc_printf(x...)
#endif

//#define DEBUGROT_ON
#ifdef DEBUGROT_ON
#define dbgrot_printf(x...)	printf(x)
#else
#define dbgrot_printf(x...)
#endif

//#define DEBUGNCW_ON
#ifdef DEBUGNCW_ON
#define dbgncw_printf(x...)	printf(x)
#else
#define dbgncw_printf(x...)
#endif

//#define DEBUGSTAT_ON
#ifdef DEBUGSTAT_ON
#define dbgstat_printf(x...)	printf(x)
#else
#define dbgstat_printf(x...)
#endif

//#define DEBUGCRASH_ON
#ifdef DEBUGCRASH_ON
#define dbgcrash_printf(x...)	printf(x)
#else
#define dbgcrash_printf(x...)
#endif

// next define used to do configuration on LICOR
//#define LICOR_BREAK

#define BUF_LEN			 512//1024
#define BUF_QUEUE(buf)		((buf)->wptr - (buf)->rptr)
#define BUF_FREE(buf)		(BUF_LEN - (buf)->wptr)
#define STX 02
#define ETX 03
#define ENQ 05
#define ACK 06
#define TRUE   1
#define FALSE  0
#define UnixStartDate 25569.0 // Sets UnixStartDate to TDateTime of 01/01/1970
#define UnixtoDateTime(x) (x/86400.0)+UnixStartDate
#define max_clients 4
#define rotor_delta 300

#include	<stdio.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/select.h>
#include	<linux/if.h>

#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<netinet/tcp.h>

#include	<string.h>
#include	<pthread.h>
#include	<iostream>
#include	<sys/time.h>
#include	<list>
#include	<vector>
#include	<sys/resource.h>
#include	<syslog.h>
#include	<sys/stat.h>
#include	<string.h>
#include	<math.h>
#include	<stdbool.h>
#include	<linux/watchdog.h>
#include	<semaphore.h>

// application modules
#include	"/usr/include/pugixml.hpp"
#include	"movavg.h"
//#include	"statusthread.h"
#include	"gill.h"
#include	"licor.h"
#include	"rotor.h"
#include	"nporttcp.h"
#include	"../websocket/WebSocket.h"

typedef class r3  *r3ptr;
typedef class tlicor_7500  *liptr;
typedef class WebSocket *wsptr;
typedef class rotor *rotorptr;
typedef float sfie(char * x);
typedef void put_sock(const char *message, int index);  // writes data to a websocket

// definition function that scans a device message with raw data
typedef int scan_msg(struct tcp_server_info *tcp);

// a signal information structure
typedef struct signal_info{
	std::string name, sname, envname, units, sn, instrum, caldate, afie;
	int p, ad_hard;
	float ad_min, ad_max, range_min, range_max;
	int degree;
	int curpos;
	size_t index[1];
	float sa, sb, a, b, k1, k2, coef_3, coef_4;
	float delta_max, lastvalue;  // used in keeping data clean
	int nc_id;
	int is_status, stat_mail;
	char c_status;
	sfie *fie;
};	

typedef std::vector<signal_info> sigs_container;

//struct project_info;
struct project_info{
	std::string	name, longname, shortname, localpath, location, format, 
				affiliation, ftp, version, smtp, receipients, pi, url, 
				measured_quantities, devs, xml_status_file; 
	float longitude, latitude, altitude;
	int runlength, linecnt;
	long curr_flenmin;
	int flen;
	int si;     // will hold number of devices after initialization
	int savedata;
	int savedge;
	int nc_is_open;
	//FILE *ftup;
	char toMessages[200];
	char ncpath[100], fncpath[100], afork[100];
	time_t tstamp; //current date and time in UTC
	struct tm *local;
	std::string fname;  // current filename
	int uploaded, 
		uploaded_cnt, 
		upload_interval;  // every upload_interval seconds 
	int nodb_stat;
	bool make_empty_container;
	// pointer to a WebSocket object, accessible by all threads
	// init NULL will be created when needed by *.xml
	wsptr	wsock; 
};	

struct buffer_struct {
	char		buffer[BUF_LEN];
	int			rptr, wptr;
};

struct comintf {
	int cport;
	int baud;
	int sb;
	int dbits;
	std::string control; 
	std::string intf;
	std::string parity;
};	

struct tcp_server_info {
	// TCP information
	int				tcpport;	// TCP port number
	int				tcpfd;		// TCP client file handle
	int				tx;			// when 1 writefs is used in socket
	int 			phase;		// siam message scan phase
	int				len;		// received to process chars buffer
	int				uslp;		// sleep for a while
	int				dayindex;   //
	int				delay;		// wait delay seconds before connecting socket
	int				no_write;	// when one inhibit netcdf creation/writing for this device
	int				restart;
	char			buff_tcp[256];
	char			stype;		// M or X
	std::string		type,
					comment;	// more explainable text
	
	unsigned char	lc;			// location code
	
	// buffer 
	struct buffer_struct	*txbuf, *rxbuf, *msgbuf;
	struct sockaddr_in	des;
	bool ready;
	// remote server IP address
	std::string		ip;
	std::string		src;
	std::string		dim_name;
	std::string		server;  // indicates the source type: IP | COM
	struct comintf	*comport;// when comm interface creates this structure
	struct timeval 	tstamp;
	struct timeval  last_used;
	struct tm 		*curr;	// used in day crossing during dayfile acquisition
	int julday;		        // also for day crossing
	struct file_info	*nc_file;// pointer to the current netcdf file
	float			last_tstamp; // calculate interval between actual samples
	float freq;  // sample frequency
	int dt;      // interval between samples
	int usec_div;// divider to round usecs in timeval, used with indexing
	
	int sync_cnt, last_cindex; // used in sync with time to index
	int dim;	  				// dimension length
	int webclient;				// when 1 send data to websocket(s)
	int sepcnt;					// number of seperators in complete ASCII message
	unsigned long fcnt;			// number of frames sofar
	float ad_min, ad_max, missing_value;
	// signals used here
	sigs_container signals;  // a vector with all the signals used by this device
	// pthread file handle
	pthread_t		pthd;
	scan_msg		*fscan_msg;
	r3ptr			gill; // pointer to gill-R3 object, xml driven creation 
	liptr			licor;// pointer to LI7500RS object, xml driven creation
	rotorptr		rotor;// pointer to rotor object, xml driven creation
};

// structure to create elements that must be pushed to a queue
typedef struct q_item{
	struct timeval*	tstamp; // pointer to sample string timestamp
	std::string* samples;	// pointer to a space seperated string containing signal values
	tcp_server_info* tcp;	// pointer to device info structure
	int cindex;				// current index from tstamp
};	

extern struct project_info proj;
extern std::list<tcp_server_info*> listofdevs;
extern struct signal_info asig;
extern float ddd_glb;
extern float ddd_rot;
extern int rotor_maintenance;
extern int rotack;
extern pthread_mutex_t rot_cv;
extern int lipipe;
//extern WebSocket wsock;

void tolog(const char* amsg, int PRIORITY);
//void tolog(std::string amsg, int PRIORITY){
bool fileexists( const char* Filename );
static int do_mkdir(const char *path, mode_t mode);
int mkpath(const char *path, mode_t mode);
void tolog(const char* amsg, int PRIORITY);
const char * timestr(timeval tstamp);
int Getdfp(time_t tcurr, div_t *index, struct tcp_server_info *device);
int parse_xml(const char* filename);
int create_next_nc_file(const char* _arg);
int read_nc_xml(const char* filename);
void del_tcp_list();
int scan_siam_msg(struct tcp_server_info *tcp);
int scan_xlas_msg(struct tcp_server_info *tcp);
int scan_gill_msg(struct tcp_server_info *tcp);
int scan_licor_msg(struct tcp_server_info *tcp);
int scan_rotor_msg(struct tcp_server_info *tcp);
float sf(char * x);
std::string itoa(int value);
std::string ftoa(float value);
int nport_json(std::string *webformat);
int status_xml(const char* filename);
int xml_status_update(struct tcp_server_info *tcp, const char* node_name, struct timeval *tstamp);

#endif /*TROPN _H */
