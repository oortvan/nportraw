/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * 
 * Copyright (C) Cor van Oort 2015 <oortvan@knmi.nl>
 * 
 * this is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GILL_H_
#define _GILL_H_

#include	<sstream>
#include 	<ctime>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<string.h>
#include	"nport.h"

struct r3_frame{
	timeval tstamp, session_start;
	unsigned char status[2];
	short wc1, wc2, wc3, C, T, A1, A2, A3, A4, A5, A6;
	unsigned char checksum;
	bool isnan;
};

//using namespace std;

class r3{
	public:
		int R3_com,									// PORT1; 
		    R3_flow,							 	// HW_FLOW_CONTROL
		    R3mode, 								// 0 continuous, 1 command mode
			ser_fd,									// comport handle
			rincommand,								// is someone trying to reach the R3 setup
			lasti,									// keep track in message buffer
			R3_checksum_lim,						// depends on number of analog inputs
			r3_address,r3_status,old_c_limit,		// used in R3 binary message decoding
			r3phase,								// R3 message state counter
			lii,
			ringfull,								// all 5 r3 frames are filled
		    last_usec,								// used in detecting closest li interval
			r3_skip,								// skip so many frames
			us_cnt, sync_cnt, last_cindex,
			rotor_dcnt, rotor_bigturn_cnt;			// rotor data count, sort keeping track whats going on
		unsigned int 
			speed,   								// 19200;
		    R3_intf; 								// RS232_MODE;
		
		unsigned char chksum;
		char message[255], log_message[255], shrts[2];		// buffers for building message from serial port
		tm *local;									// realtime clock handling
		int *r3cnt, *li_av;							// pointers to sync variables
		r3_frame* r3_nan_frame;
		r3_frame	eddy;
		bool no_signal, go_push, enabled, go_check, debug;
		float ddd, ddd_nc, last_pt_ddd, horz_rotation;	// orientation of instrument: Uinst
		float u, v, w, T;
		signed short x, y, z, t;
		timeval tstamp;
		MovingAverageFilter *ddd_avg;

		std::string rotor;
		std::stringstream slog;
		struct tcp_server_info *rotdev;
		// constructor
		r3();
		// destructor
		~r3();										
		int r3Scan_buf(struct tcp_server_info *tcp);// scan buf for complete message
		void cleanup();                             // delete queue items when abort
		struct tcp_server_info *scan_for_rotor();
	protected:

	private:
		int scan_r3();
		int _set(float ddd); // tells the rotor to go (pan) to ddd position
		int _rot_query();
		float _process_set(float ddd); // controls the next pan position looking at the current and new
};

#endif // _GILL_H_
