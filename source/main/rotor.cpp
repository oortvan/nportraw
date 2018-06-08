/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * rotor.cpp
 * Copyright (C) fedora 2017 <fedora@localhost.localdomain>
 * rotor.cpp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * rotor.cpp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rotor.h"

// constructor
rotor::rotor(){
	m_checksum = 0x0;
	m_byte_cnt = -1;
	ddd = -999;  // indicates no sample yet from rotor
	self = NULL;
	rotack = 0;
}	

// destructor
rotor::~rotor(){
}	

void rotor::get_mutex(float &rot_pos){  // called by var convention: &rot_pos
	rot_pos = ddd;
}

int rotor::scan_buf(struct tcp_server_info *tcp){
	int i;

	tcp->len = read( tcp->tcpfd , tcp->buff_tcp, 255);

	if (tcp->len < 1){
		//m_byte_cnt = -1;
		return(tcp->len);  // signal that remote host is closed
	}
	
	if (tcp->len == 7) {m_byte_cnt = 0; m_checksum = 0;}
		

	for (i=0; i<tcp->len; i++){
		
		//if (tcp->buff_tcp[i] == 0xff) {m_byte_cnt = 0; m_checksum = 0;} // gives problem with multiple ffh
		if ((m_byte_cnt >= 0) && (m_byte_cnt < n_bytes)){
			resp[m_byte_cnt] = tcp->buff_tcp[i];
			m_byte_cnt++;
			if ((m_byte_cnt == 7) && (resp[3] == 0x59))
			if (((resp[1]+resp[2]+resp[3]+resp[4]+resp[5]) & 0xff)  == resp[6]){
				pos_pt =  resp[4]; pos_pt <<= 8;	//MSB position count
				pos_pt |= resp[5];					//LSB position count  
				ddd = float(pos_pt)*max_resolution;
				ddd_rot = ddd;  // global var, keep the overlap in mind, this is the real rotor value
				
				if (ddd >= 360) ddd -= 360;

				//int retw = pthread_mutex_lock(&rot_cv);
				ddd_glb = ddd;  
				rotack = 2;
				//int retp = pthread_mutex_unlock(&rot_cv);

				dbgrot_printf("%f=0.033*(%u|%04xH): %02x %02x %02x %02x %02x %02x %02x\n",
	         	ddd_glb, pos_pt, pos_pt,
		        resp[0], resp[1], resp[2], resp[3], resp[4], resp[5], resp[6]);
			}	
		}
	}
	return 1;
}	

int rotor::_set(float ddd){ // tells the rotor to go (pan) to ddd position
	if (self->tcpfd == NULL) return -1;
	ddd += rotor_delta;
	if (ddd > 360) ddd -= 360;
	unsigned char cmd[7] = goto_position;
	unsigned short pan = static_cast<unsigned short>(ddd/max_resolution);
	// nu nog met schuiven naar MSB en LSB zetten
	cmd[4] = pan >> 8;
	cmd[5] = pan & 0xff;
	for (int i=2; i<6; i++) cmd[6] += cmd[i];
	dbgrot_printf("%f/0.033=(%u|%04xH): %02x %02x %02x %02x %02x %02x %02x *\n",
	             ddd, pan, pan, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6]);
	if (self->tcpfd > 0)
	if (write(self->tcpfd, &cmd, 7) < 0);
	else return 1;
}	

int rotor::_cmd(pt_command_type acmd){
	unsigned char cmd[7] = query_pan_pos;
/*	switch ( acmd ) {
		case pt_query_pan_pos: // tells the rotor to send position info over 485 interface
		  unsigned char cmd[7] = query_pan_pos; 
		  break;
		case pt_goto_preset_0: 
		  break;
		case pt_pan_left_maxs:
		  break;
		case pt_pan_rght_maxs:
		  break;
		case pt_stop_rotation:
		  break;
		default:
		  return -1;
		break;
	}*/
	if (write(self->tcpfd, &cmd, 7) < 0) return -1;
	else return 1;
}


