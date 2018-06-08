/*
 * contains classes for reading LICOR 7500A over SERIAL OR IP socket:
 * 
 */

#ifndef licor_h
#define licor_h

#define time_sync_interval 900 // seconds to wait for next time sync

#include	<sstream>
#include 	<ctime>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<string.h>
#include	<set>
#include	"nport.h"


/*
 * structure that applies to a L7500A gas analyzer
 * not all possible parameters are used.
 * a licor message ends with a linefeed
 */
struct li_frame{
	timeval tstamp, session_startt, li_clock;
	int ndx, diagval, session_ndx;
	double co2raw,co2d,h2oraw,h2od,temp,pres,aux1,aux2,aux3,cooler;
	bool isnan;
};

struct t_li_socket{
	std::string ip;
	int port;
	int error;
};

void log_info(const char* amsg, int PRIORITY);

class tlicor_7500{
	public:
		int *li_av; 
		int nli_p, dt, fc, len, li_phase,
			li_socket_handle, 
			li_ser_fd, li_com,li_speed,li_flow,li_intf,
			li_sync_cnt,
			li_dcnt, d_sign, tab_cnt,
			afternsecs;
		unsigned int len_msg;
		char li_message[500], buf[500], log_message[400], aline[300];
		//std::strin li_message;
		float din, din_avg;
		int lidtmcnt, lidtmsct, has_bracket;
		double lidtm;
		int lidtmmin;
		std::string name, kind, litime;
		struct li_frame li7500;
		bool no_signal, go_push, clockupdate, debug;  // becomes true when serial interface is not streaming data
		timeval lioffset, tmscanf, lastclockupdate;
		div_t usecs, secs, days;

		int fd[2]; // pipe file descriptor
		int haspipe;

		tlicor_7500();  // constructor
		~tlicor_7500(); // destructor

		void lif_nan(li_frame &alif);				// all members set to NAN
		int LiScan_buf(struct tcp_server_info *tcp);// builds licor message as it comes in
		bool sensor_ready();						// sets no_signal bool to true on absent of serial streaming data
		void clocksync(int li_socket_handle);		// synchronize the linux system clock in in licor box
	private:
		int scan_li (li_frame &lif);				// takes a complete licor 7500 message, scans into structure
		int scan_li_safe (li_frame &lif);			// safer way of extracting data
		int scantime(timeval *astamp);
		bool is_valid(char a_c);
	protected:
};

#endif //
