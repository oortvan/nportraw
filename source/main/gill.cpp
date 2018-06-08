/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * KTS_7500A
 * Copyright (C) fedora 2015 <oortvan@knmi.nl>
 * 
KTS_7500A is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * KTS_7500A is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <sys/socket.h>
#include <iostream>
#include <syslog.h>
#include "gill.h"
#include "queue_thread.h"

using namespace std;
/*
 * implementation of the r3 class
 * 
 */
	// constructor
	r3::r3(){
		rotack = rotor_bigturn_cnt = rotor_dcnt = chksum = lii = R3mode = ser_fd = message[0] = rincommand = lasti = sync_cnt = 0;
		last_usec = us_cnt = -1; // used in detecting nearest licor time intervals
		R3_checksum_lim = 11; // save limit to look for the final checksum
		r3_address = r3_status = -1;
		r3_skip = 0;
		gettimeofday(&eddy.tstamp, NULL); // get a first time stamp for sensor available checking
		r3_nan_frame = NULL;
		no_signal = true;
		go_push = false;
		enabled = true;
		last_cindex = -1;
		ddd_avg = new MovingAverageFilter(MAX_DATA_POINTS);
		rotdev = NULL;

		//open_serial(slog);  // tijdelijk outcommented
#if GDEBUG == 1
		//cout << "CREATED GILL CLASS" << "\n";
#endif
	}

	// destructor
	r3::~r3(){ 
		delete ddd_avg;
	}	


	// this function assumes ddd to be the current direction angle of R3:
	// ddd = atan2(-v/u) is clockwise oriented and has range 0 .. 360
	// ddd_rot is the current rotor position and has range 0 .. 400.5
	float r3::_process_set(float ddd){
		float pddd;
		if (ddd_rot >= 220){			// last rotor position from query has value in range 0 .. 400.5
			if (ddd > 0 && ddd < 41)	// 41 makes rotor wait to cross border secure 
				pddd = ddd+360;			// pan to 360 .. 400 overlap interval
			else pddd = ddd;			// 40 < ddd < 360  
		}
		else if (ddd_rot < 220){		// last rotor position from query has value in range 0 .. 400.5
			if (ddd < 5)				// 5 makes rotor wait to cross border secure
				pddd = ddd+360;			// pan left into 360 .. 400 overlap
			else pddd = ddd;			// 7 < ddd < 360
		}
		else pddd = ddd;

		// gard borders
		if (pddd > 400) pddd = 400;
		if (pddd < 7) pddd = 7;

		// set the wait for pan duration
		// speedup rotor by inhibiting panning
		// when it has to turn more than 2 degrees
		if (fabs(pddd - ddd_rot) > 2){
		if (rotor_bigturn_cnt == 0)
			rotor_bigturn_cnt = int(fabs(ddd_rot-pddd)*0.28+20);  // +20: rekening houden met versnelling en vertraging	
			printf("BIGPAN DET: 20 + |%f - %f| * 0.28 = %d, %d\n",ddd_rot, pddd,int(fabs(ddd_rot-pddd)*0.28+20), rotor_bigturn_cnt);
		}	
		//else printf("OLDPAN:%f NEWPAN: %f\n",ddd_rot, pddd);

		return pddd;
	}	

	int r3::_rot_query(){  // polls the rotor to send its current position
		if (rotdev->tcpfd < 1) return -1;
		unsigned char cmd[7] = {0xFF, 0x01, 0x00, 0x51, 0x00, 0x00, 0x52};
		return write(rotdev->tcpfd, &cmd, 7);
	}

	int r3::_set(float ddd){ // tells the rotor to go (pan) to ddd position
		if (rotdev->tcpfd < 1) return -1;
		ddd += rotor_delta;

		if (ddd > 360)
			ddd -= 360;

		ddd = _process_set(ddd);
		//if (ddd > 400){
		//	ddd -= 360;
		//	rotor_bigturn_cnt = 100;
		//}	

		
		unsigned char cmd[7] = {0xFF, 0x01, 0x00, 0x4B, MSB, LSB, CHECKSUM};
		unsigned short pan = static_cast<unsigned short>(ddd/max_resolution);
		// nu nog met schuiven naar MSB en LSB zetten
		cmd[4] = pan >> 8;
		cmd[5] = pan & 0xff;
		for (int i=2; i<6; i++) cmd[6] += cmd[i];  // calc checksum
		dbgrot_printf("%f/0.033=(%u|%04xH): %02x %02x %02x %02x %02x %02x %02x *\n",
			         ddd, pan, pan, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6]);
		return write(rotdev->tcpfd, &cmd, 7);
	}	

	int r3::r3Scan_buf(struct tcp_server_info *tcp){
		int i, s, index;
		char aline[255];
		div_t usecs, secs, days;

		tcp->len = read( tcp->tcpfd , tcp->buff_tcp, 255);
		if (tcp->len == 0)
			return(0);
		
		for (i=0; i<tcp->len; i++){
			if (r3phase == 0) {
				if (tcp->buff_tcp[i] == 0xBA){
					r3phase = 1;
					lasti = i;
					message[0] = 0;  // start of message
				} 
			}
			else if (r3phase == 1){		
				if (tcp->buff_tcp[i] == 0xBA && (lasti == (i-1) || i==0)){
					r3phase = 2;
					chksum = 0;
					gettimeofday(&tstamp, NULL);  // used later on in 10Hz phase
					sprintf(aline,"%d %06d", tstamp.tv_sec, tstamp.tv_usec);
					strcat(message, aline);
				}
				else r3phase = 0;
			}
			else if (r3phase >= 2 && r3phase <= R3_checksum_lim){
				chksum = chksum ^ tcp->buff_tcp[i];
				/*
				 * Rotor_maintenance is refreshed in a thread,
				 * in this thread the condition of a digital input
				 * is refreshed, this input is controlled by a remote switch.
				 * Extra code to add rotor maintenance switch 
				 * info to GILL status byte 6, where bits 3 upto 7
				 * are not used see manual. Bit 3 wil indicate roter status
				 */ 
				if (r3phase == 2) r3_address = tcp->buff_tcp[i];	// address status gill in tcp->buf
				if ((r3phase == 3) && (r3_address == 0x6)){			// status gill in tcp->buff
					if (rotor_maintenance == 1) tcp->buff_tcp[i] |= 0x8; // set bit 3 to 1
					if (rotor_bigturn_cnt > 0)  tcp->buff_tcp[i] |= 0x10; // set bit 4 to 1, rotor is turning big angle
				}
				/*
				 * modulo test op even buffer waarde om short ints
				 * te kunnen detecteren met scanf
				 * wat ook mogelijk is, addres en status in 16 bits combi
				 * dan vervalt || (r3phase < 4) in de volgende if
				 */
				if (((r3phase % 2) == 0) || (r3phase < 4))
					sprintf(aline, " %02x", tcp->buff_tcp[i]);
				else
					sprintf(aline, "%02x", tcp->buff_tcp[i]);
				// u,v,w,t -> ddd phase detection
					 if (r3phase == 4) {x = tcp->buff_tcp[i]; x <<= 8;}			//MSB u
				else if (r3phase == 5) {x |= tcp->buff_tcp[i]; u = float(x);}   //LSB u
				else if (r3phase == 6) {y = tcp->buff_tcp[i]; y <<= 8;}			//MSB v
				else if (r3phase == 7) {y |= tcp->buff_tcp[i]; v = float(y);}   //LSB v  
				else if (r3phase == 8) {z = tcp->buff_tcp[i]; z <<= 8;}			//MSB w
				else if (r3phase == 9) {z |= tcp->buff_tcp[i]; w = float(z);}   //LSB w 
				else if (r3phase == 10) {t = tcp->buff_tcp[i]; t <<= 8;}		//MSB T
				else if (r3phase == 11) {t |= tcp->buff_tcp[i]; T = float(t);}  //LSB T  

				strcat(message, aline);

#if GDEBUG == 1
				//cout << message << "\n";
#endif
				/*
				 * if r3phase = 2 betekent staA byte is available
				 * if r3phase = 3 betekent staD byte is available
				 * 
				 */
				old_c_limit = R3_checksum_lim;
				if (r3phase == 2) r3_address = int(tcp->buff_tcp[i]);
				if (r3phase == 3) r3_status = int(tcp->buff_tcp[i]);
				r3phase++;
			}
			else if (r3phase > R3_checksum_lim && r3phase < 24){
#if GDEBUG == 1
				//cout << int(chksum) << " " << int(tcp->buff_tcp[i]) << " " << int(r3phase) << "\n";
#endif
				if (chksum == tcp->buff_tcp[i] && ((r3phase) % 2 == 0)){
					sprintf(aline, " %02x", tcp->buff_tcp[i]);
					strcat(message, aline); // message is complete
					r3phase = 0; // fresh start next frame

					if (us_cnt > -1){
						us_cnt++;
						if (us_cnt > 49) us_cnt = 0; // freq=50Hz: important to use > 49, avoids system hang
					}
					// testing block for starting point of new 1 second interval
					// looks for transition of 0.9 to 0.0
					if (last_usec == -1); // do nothing at init, get the last_usec first
					else if (tstamp.tv_usec < last_usec){
						us_cnt = 0; // means transition from 0.9 to 0.0 secs
					}	

					// gill indexing in netcdf
					if ((us_cnt % 5) == 0){  // modulo 5 results in 10 Hz sampling rate
						// assign here the timestamp, no interference of 50HZ in 10Hz, threading background
						tcp->tstamp.tv_sec = tstamp.tv_sec;
						tcp->tstamp.tv_usec = tstamp.tv_usec;
						tcp->last_used.tv_sec = tstamp.tv_sec;
						tcp->last_used.tv_usec = tstamp.tv_usec;
						secs = div(int(tcp->tstamp.tv_sec),proj.runlength);
						usecs = div(int(tcp->tstamp.tv_usec),tcp->usec_div); // USECS ROUNDING TO 0.0, 0.1, 0.2, ..., 0.9
						tcp->tstamp.tv_usec = long(usecs.quot)*tcp->usec_div;
						int cindex = int(tcp->freq) * secs.rem + usecs.quot; // and the index is, 10Hz means

						// syncing the running index
						if (last_cindex != -1){
							if (sync_cnt > 3) sync_cnt = 0;
							else if ( cindex == last_cindex) {cindex++; sync_cnt++;}
							else if (cindex == (last_cindex+2)) {cindex--; sync_cnt++; tcp->fcnt--;}
							else sync_cnt = 0;
						}
						last_cindex = cindex;

						ddd = ddd_avg->lavg(-v, u); // update rotor criteria here, use rotor commands for new position
						ddd_nc = rotor_delta+ddd;
						if (ddd_nc > 360) ddd_nc -= 360;

						sprintf(aline," %f",ddd_nc);   // add ddd to message
						strcat(message, aline);

						// is a external rotor present
						if (rotdev != NULL)
						if (rotdev->ready) { // can this rotor receive data
					
							//pthread_mutex_lock(&rot_cv);  // protect with mutex lock, inhibit rotor

							switch (rotack){
								case 0:// query position rotor 
									if (_rot_query() < 1) {
										ddd_glb = ddd_nc; 
										rotack = 0;
									}
									else { rotack = 1; rotor_dcnt = 0; }
								break;

								case 1:// give rotor time to respond before next command
									rotor_dcnt++;  
									if (rotor_dcnt >= 40) {rotor_dcnt = 0; rotack = 0;}
								break;

								case 2:// try to pan to new position, 
									   //rotack is set to 2 by rotor thread when received valid frame from rotor
									if (fabs(ddd_glb-ddd_nc) > 0.033){	// ddd_glb is terug koppeling van rotor
										if (rotor_bigturn_cnt == 0){
											if (_set(ddd) < 1)  // pan command OK?
												ddd_glb = ddd_nc; 
										}
										else{  // is new pan position reached
											if (fabs(ddd_glb-last_pt_ddd) < 1) rotor_bigturn_cnt = 0;
											else rotor_bigturn_cnt--;
										}	
									}
									rotack = 0;
								break;
							}	
							// use rotor data in net cdf message?
							last_pt_ddd = ddd_glb;
							sprintf(aline," %f", ddd_glb);
							strcat(message, aline);

							//pthread_mutex_unlock(&rot_cv);  // unlock rotor thread
						}
						if (debug)
							printf("%s: %s\n",tcp->src.c_str(), message);
						// push the this gill record onto consumer
						q_item* aqi = new q_item; // new cue item to be pushed
						aqi->samples = new std::string(message);
						aqi->tstamp = new timeval(tcp->tstamp);
						aqi->cindex = cindex;
						aqi->tcp = tcp;  // remember tcp contains only constant values at this stage
						cq.push(aqi);
					}

					last_usec = tstamp.tv_usec;

				}
				else {
					r3phase = 0;
				}	
			}
			else r3phase = 0;	
		}
		return(tcp->len);
	}

void r3::cleanup(){
	r3_frame* qr3_frame;
	//sprintf(log_message,"sonic queue cleaned: %d\n", eddy->r3_queue.size());
	//log_info(log_message, LOG_INFO);
}

int r3::scan_r3(){  // place the r3 values in its frame
	int ni, unixtime, usecs;
	ni = sscanf(message,"%d %d %hhx %hhx %hx %hx %hx %hx %hhx",
			   &unixtime,&usecs,
			   &eddy.status[0],&eddy.status[1],
			   &eddy.wc1,&eddy.wc2,&eddy.wc3,&eddy.T,
		       &eddy.checksum);

	if (ni != 9){  // something went wrong for this record
		return 0;
	}
	u = (float)eddy.wc1;
	v = (float)eddy.wc2;
	w = (float)eddy.wc3;
}

struct tcp_server_info *r3::scan_for_rotor(){
	if (rotor == "") return NULL;
	struct tcp_server_info	*adev;
	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end()){
		adev = (*lt); // get a device structure from list
		if (adev->src == rotor){
			dbgrot_printf("found rotor %s - %s\n",rotor.c_str(), adev->comment.c_str());
			rotdev = adev;
			return adev;
		}	
		lt++;
	}	
}	


