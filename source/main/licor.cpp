/*
 * contains classes for reading licor data from ip socket:
 * 
 */
#include <math.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <syslog.h>
#include "licor.h"
#include "serial.h"
#include "queue_thread.h"


using namespace std;
/*
 * A licor gas-analyzer can be configured over a connection
 * definitions that control how the analog inputs of the licor
 * are used, aux1: thermocouple; aux2: inc_x; aux3: inc_y.
 * when INC_FROM_SOCKET is defined, means data is comming from
 * a SIAM over a network socket, else it is assumed that aux2
 * and aux3 from the 7500A are used.
 * with the next 2 stings a particular setup is achieved
 */ 

//li7500A configuration string when inclinometer not connected to aux
string li_O_mode = 	"(Outputs(RS232"
					"(Ndx TRUE)"
					"(DiagVal TRUE)"
					"(CO2D TRUE)"
					"(H2OD TRUE)"
					"(CO2AW TRUE)"
					"(H2OAW TRUE)"
					"(Temp TRUE)"
					"(Pres TRUE)"
					"(Aux TRUE)"
					"(Aux2 FALSE)"
					"(Aux3 FALSE)"
					"(Cooler TRUE)"
					"(Labels FALSE)))\n";
// li7500A configuration string when inclinometer connected to aux
string li_A_mode = 	"(Outputs(RS232"
					"(Ndx TRUE)"
					"(DiagVal TRUE)"
					"(CO2D TRUE)"
					"(H2OD TRUE)"
					"(CO2AW FALSE)"
					"(H2OAW FALSE)"
					"(Temp TRUE)"
					"(Pres TRUE)"
					"(Aux TRUE)"
					"(Aux2 TRUE)"
					"(Aux3 TRUE)"
					"(Cooler TRUE)"
					"(Labels FALSE)))\n";

/*
 * licor 7500A code for own class
 * 
 */
 // constructor
	tlicor_7500::tlicor_7500(){
		char* pPath;
		//r3cnt = r3c; li_av = li_a;  // assign pointers of variables from the sync process
		gettimeofday(&li7500.tstamp, NULL); // initialize for is data available process, see r3_li_dt variable
		gettimeofday(&lastclockupdate,NULL);
		li_av = NULL; // assign pointers of variables from the sync process
		li_dcnt = fc = len_msg = len = li_phase = li_sync_cnt = 0;
		has_bracket = 0;
		li_message[0] = buf[0] = 0;
		nli_p = 10;			// number of parameters in a message record
		din=0.0065;
		din_avg=0.0065;
		no_signal = true;
		go_push = false;
		clockupdate = false;
		debug = false;
		afternsecs = 5; // 5 sec after start do clock update
		litime.clear();
		//const char *v_c = "0123456789.-+:\n\t";
		//const std::set<char> valid_c(v_c, v_c+strlen(v_c)); 
		dbg_printf("CREATED LICOR CLASS\n");
		haspipe = -1;
#ifdef LICOR_BREAK
		haspipe = pipe (fd); // initialize a pipe for intercommunication
		if (haspipe < 0){
		   perror("pipe ");
		}
#endif		
	}  

// destructor
	tlicor_7500::~tlicor_7500(){
	}

/*
 * this function checks if a character is legal
 * when baudrate or message format is wrong it will be detected
 */ 

bool tlicor_7500::is_valid(char a_c){
	switch (a_c){
		case '0': return true;
		case '1': return true;
		case '2': return true;
		case '3': return true;
		case '4': return true;
		case '5': return true;
		case '6': return true;
		case '7': return true;
		case '8': return true;
		case '9': return true;
		case '(': return true;
		case ')': return true;
		case '.': return true;
		case '-': return true;
		case '+': return true;
		case ':': return true;
		case '\n':return true;
		case '\t':return true;
	}
	return false;
}	

// is this licor sending data over its serial interface
// compare "class creation"|"last detected frame" time with current time
	bool tlicor_7500::sensor_ready(){
		timeval currentT;
		gettimeofday(&currentT, NULL);
		// calculate time difference between last licor sample and current time
		double li_dt = ((li7500.tstamp.tv_sec + li7500.tstamp.tv_usec*1e-6) - 
		                (currentT.tv_sec + currentT.tv_usec*1e-6));
		no_signal = (li_dt < -0.5);
#if LDEBUG == 1
		//if (no_signal) cout << li_dt << " diff, no data from licor\n";
#endif
		return !no_signal;
	}

	/*
	 *This function invalidates a licor frame with NAN and -1 values
 	 *It is used when the sysnchronisation process detects that
	 *no licor frame is ariving with a certain time frame.
	 */
	void tlicor_7500::lif_nan(li_frame &lif){
		lif.ndx = -1;
		lif.diagval = -1;
		lif.co2raw = nanf("/");
		lif.co2d = nanf("/");
		lif.h2oraw  = nanf("/");
		lif.h2od = nanf("/");
		lif.temp = nanf("/");
		lif.pres = nanf("/");
		lif.aux1 = nanf("/");
		lif.aux2 = nanf("/");
		lif.aux3 = nanf("/");
		lif.cooler = nanf("/");
	}
	/*
	 * This function scans a complete licor data line
	 * Depends also on compiler preprocessor INC_FROM_LICOR value
	 * which indicates how inclinometer data is acquired
	 * inclino x, y connected to aux2 and aux3
	 */
	int tlicor_7500::scan_li_safe (li_frame &lif){ // safer way of extracting data, see also scan_li
		char * pch;
		char * pend;
		int pcnt = 0;
		std::stringstream error_msg;
		pch = strtok (li_message," \t\n\r");  // init token extract, skipping tabs and newline chars 
		while (pch != NULL){
			error_msg << pcnt << ":" << pch << " ";
			switch (pcnt){
				case 0: lif.ndx 	= atoi(pch); break;
				case 1: lif.diagval = atoi(pch); break;
				case 2:  						 break; // ISO date
				case 3:  						 break; // ISO time
				case 4: lif.co2d 	= atof(pch); break;
				case 5: lif.h2od 	= atof(pch); break;
				case 6: lif.temp 	= atof(pch); break;
				case 7: lif.pres 	= atof(pch); break;
				case 8: lif.aux1 	= atof(pch); break;
				case 9: lif.aux2 	= atof(pch); break;
				case 10:lif.aux3 	= atof(pch); break;
				case 11:lif.cooler  = atof(pch); break;
			}
			pcnt++;
			pch = strtok (NULL,"\t\n"); //pch points to a value sub string when found else NULL
		}
		if (pcnt != 12){	// in principle there must be 10 values
			sprintf(log_message,"ERROR LI-7500A invalid record, %d values -> %s",pcnt, error_msg.str().c_str());
			tolog(log_message, LOG_ERR);
			return 0;
		}
		printf("limsg OK\n");
		return pcnt;
	}
	/*
	 * This function scans a complete licor data line
	 * Depends also on compiler preprocessor INC_FROM_LICOR value
	 * which indicates how inclinometer data is acquired
	 * 0: inclino x, y connected to aux2 and aux3
 	 * 1: inclino x, y connected to U-SIAM
	 */
	int tlicor_7500::scan_li (li_frame &lif){
		int ni = sscanf(li_message,"%d%d%lf%lf%lf%lf%lf%lf%lf%lf",
			        &lif.ndx,&lif.diagval,
		// control how the inclinometer data is sampled
		#if INC_FROM_LICOR == 1	// order of signals is different 
			&lif.co2d,&lif.h2od,&lif.temp,&lif.pres,&lif.aux1,&lif.aux2,&lif.aux3,&lif.cooler);
		#else
			&lif.co2raw,&lif.co2d,&lif.h2oraw,&lif.h2od,&lif.temp,&lif.pres,&lif.aux1,&lif.cooler);
		#endif
		if (ni != nli_p){
			sprintf(log_message,"ERROR LI-7500A invalid record -> %s",li_message);
			tolog(log_message, LOG_ERR);
			li_message[0] = 0;  		// empty message
			return 0;	// no complete message received
		}	
		*li_av = 1;
		li_message[0] = 0;  		// empty message
		no_signal = false;	
		li_dcnt++;	// valid sample, increment sample count
		return 1;
	}

	/*
	 * synchronizes the licor clock with the local system clock
	 * command syntax looks like: "(Clock(Time hh:mm:ss.zzz))"
	 * 
	 * This function must be executed every 30 minutes		 
	 */
	void tlicor_7500::clocksync(int li_socket_handle){
		char clkbuf[60];
		if (!clockupdate) return;  // do nothing

		struct timeval tv;
	    gettimeofday(&tv,NULL);
		if (tv.tv_usec > 900000){  // do nothing when not on the next sec time border
			//tv.tv_sec =- 1;
			/* get seconds since the Epoch */
			time_t secs = tv.tv_sec; //time(0);
			/* convert to localtime */
			struct tm *local = localtime(&secs);
			li_sync_cnt = 0;
			// prepare licor clock command
			sprintf(clkbuf,"(Clock(Date %04d%02d%02d))(Clock(Time %02d%02d%02d))\n",
				    local->tm_year+1900, local->tm_mon+1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec+1);
			// send command to licor must be placed here
			if (li_socket_handle != 0){
				int clen = strlen(clkbuf);
				if (write(li_socket_handle, &clkbuf, clen) < 0) tolog("LICOR clock-sync error", LOG_ERR);
				else tolog("LICOR clock-sync OK", LOG_INFO);
				tolog(clkbuf, LOG_INFO);
#ifdef LICBUG
				printf("handle:%d clen:%d %s\n", li_socket_handle, clen, clkbuf);
#endif				
			}
			lastclockupdate = tv;
			clockupdate = false;
			afternsecs = time_sync_interval; // wait 10 min for next update
		}	
	}	
	/*
	 * This function scans buf for a LF char
	 * when found it scannes the licor record to the eddies frame
	 * Updates sync variables li_av and r3cnt
	 */ 
	int tlicor_7500::LiScan_buf(struct tcp_server_info *tcp){
		int i;

		tcp->len = read( tcp->tcpfd , tcp->buff_tcp, 255);
		if (tcp->len == 0)
			return(0);

		tcp->buff_tcp[tcp->len] = 0;

		//for (i=0; i<tcp->len; i++){
		//	dbg_printf("%c", tcp->buff_tcp[i]);  // just for testing if all is well
		//}
		
		for (i=0; i<tcp->len; i++){
			
			if (!is_valid(tcp->buff_tcp[i])) // belongs this char to the legal set
				{li_phase = 0; li_message[0] = 0; tab_cnt = 0;}// printf("%c",tcp->buff_tcp[i]);}

			else {
				if ((tcp->buff_tcp[i] == 0x0A) && (strlen(li_message) > 0)){
					li_message[li_phase] = tcp->buff_tcp[i];
					li_message[li_phase+1] = 0;
					li_phase = 0;
					if (debug == true) 
						printf("%s-%d: %s",tcp->src.c_str(), tab_cnt, li_message);
					if (has_bracket == 1) ;

					else{ // no ACK message
						if (!scantime(&tcp->tstamp)) return i;
		#ifdef LICBUG
						if ((tcp->tstamp.tv_sec % 5) == 0)
							cout << tmscanf.tv_sec << "." << tmscanf.tv_usec << " " 
								 << tcp->tstamp.tv_sec << "." << tcp->tstamp.tv_usec << " " << litime << "\n";
		#endif
						if (tab_cnt == tcp->sepcnt){ //complete message received
							tcp->last_used.tv_sec = tmscanf.tv_sec;
							tcp->last_used.tv_usec = tmscanf.tv_usec;

							tcp->tstamp.tv_sec = tmscanf.tv_sec;
							tcp->tstamp.tv_usec = tmscanf.tv_usec;

							// prepare the this licor record to push it onto consumer
							secs = div(int(tcp->tstamp.tv_sec),proj.runlength);
							usecs = div(int(tcp->tstamp.tv_usec),tcp->usec_div); // USECS ROUNDING TO 0.0, 0.1, 0.2, ..., 0.9
							tcp->tstamp.tv_usec = long(usecs.quot)*tcp->usec_div;
							int cindex = int(tcp->freq) * secs.rem + usecs.quot; // and the index is, 10Hz means
							//printf("%d %s", tab_cnt, li_message);
						//if (tab_cnt == tcp->sepcnt){ //complete message received
							q_item* aqi = new q_item; // new cue item to be pushed
							aqi->samples = new std::string(li_message);
							aqi->tstamp = new timeval(tcp->tstamp);
							aqi->cindex = cindex;
							aqi->tcp = tcp;  // remember tcp contains only constant values at this stage
							cq.push(aqi);
						}
						//else{ tolog("LICOR garbled message", LOG_ERR); tolog(li_message, LOG_INFO); }
						//else printf("%d %s", tab_cnt, li_message);
		#ifdef LICBUG
						printf("%d > %d?\n",abs(tcp->tstamp.tv_sec-lastclockupdate.tv_sec), afternsecs);
		#endif				

						if ( tcp->tstamp.tv_sec != -1 )
						if (abs(tcp->tstamp.tv_sec - lastclockupdate.tv_sec) > afternsecs)  // every aftersecs secs an update
							clockupdate = true;
						clocksync(tcp->tcpfd);
						tab_cnt = 0;
						litime.clear();
					}	
				}
				else{
					if (li_phase == 0){	// new frame is ready
						gettimeofday(&tcp->tstamp, NULL);
						if (tcp->buff_tcp[i] == '(')
							has_bracket = 1; // start of ACK message
						else has_bracket = 0;
					}	
					else if (li_phase > 498) // keep room for the last 0x0A
					  li_phase = 498;
					if (tcp->buff_tcp[i] == 0x09){ tab_cnt++; }//tcp->buff_tcp[i] = ',';}
					if (tab_cnt == 2 || tab_cnt == 3)
						litime = litime + tcp->buff_tcp[i];
					li_message[li_phase] = tcp->buff_tcp[i];
					li_phase++;
				}
			} // invalid char found
		}
		return tcp->len;
	}

	/*
	 * SCANTIME 
	 * scan the date time message from the LI7500RS
	 * syntax: yyyy-mm-dd hh:mm:ss:zzz
	 * zzz are milli seconds
	 */
	int tlicor_7500::scantime(timeval *astamp){
		int month, ms, tmin;
		timeval ctime;
		struct tm tmv;
		time_t converted;

		tmv.tm_wday = 0;
		tmv.tm_yday = 0;
		tmv.tm_isdst = -1;
		if(sscanf(litime.c_str(), "%d-%d-%d %d:%d:%d:%d", &tmv.tm_year, &month, &tmv.tm_mday, 
		                                                  &tmv.tm_hour, &tmv.tm_min, &tmv.tm_sec, &ms) == 7);{
			tmv.tm_mon = month-1;
			tmv.tm_year = tmv.tm_year - 1900;
			converted = mktime(&tmv);
			tmscanf.tv_usec = ms*1000;
			tmscanf.tv_sec = converted;
			return 1;  // the rest of the code only needed when calculation the time offset

		}
		return 0;
	}	

