/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * rotor.cpp
 * Copyright (C) fedora 2017 <fedora@localhost.localdomain>
 * 
rotor.cpp is free software: you can redistribute it and/or modify it
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

#ifndef _ROTOR_H_
#define _ROTOR_H_

#include	<pthread.h>
#include	"nport.h"

typedef enum {pt_query_pan_pos, pt_goto_preset_0, pt_pan_left_maxs, 
			  pt_pan_rght_maxs, pt_stop_rotation} pt_command_type;

#define max_count 10800
#define max_resolution 0.03333 // = 360/10800
#define n_bytes 7 // number of bytes in a rotor command
#define MSB 0x00
#define LSB 0x00
#define CHECKSUM 0x01  // taken from the address byte which is constant
#define query_pan_pos {0xFF, 0x01, 0x00, 0x51, 0x00, 0x00, 0x52}
#define goto_preset_0 {0xFF, 0x01, 0x00, 0x07, 0x00, 0x00, 0x08}
#define pan_left_maxs {0xFF, 0x01, 0x00, 0x07, 0x02, 0x3F, 0x42}
#define pan_rght_maxs {0xFF, 0x01, 0x00, 0x07, 0x04, 0x3F, 0x44}
#define stop_rotation {0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}
#define goto_position {0xFF, 0x01, 0x00, 0x4B, MSB, LSB, CHECKSUM}


class rotor
{
	public:
		int m_byte_cnt;
		unsigned char m_checksum;
		unsigned char resp[n_bytes];
		unsigned short pos_pt;
		float ddd;
		struct tcp_server_info *self;
		
		rotor();  // constructor
		~rotor(); // destructor
		
		int scan_buf(struct tcp_server_info *tcp);// scan buf for complete message
		// commands
		int _set(float ddd);
		int _cmd(pt_command_type acmd);
		void get_mutex(float &rot_pos);

	protected:

	private:
		//pthread_mutex_t mu_r, mu_w;
};

#endif // _ROTOR_H_

