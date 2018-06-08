/***************************************************************************
 *            nportnc.h
 *
 *  Fri September 23 12:44:01 2016
 *  Copyright  2016  KNMI, Cor van Oort
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
#ifndef _NPORTNC_H_
#define _NPORTNC_H_

#include	"nport.h"
#include "/usr/include/netcdf.h"

//#define ERR(e) {printf("Error: %s\n", nc_strerror(e));}
//use tolog("ERROR setpriority\0", LOG_ERR); in a MACRO
#define ERR(e) {tolog(nc_strerror(e), LOG_ERR);}
//#define ERR(e, str) {nc_xerr(e, str);}


// strategy of having 2 netcdf files open to deal with lagging device data
// a file can be closed when all devices have entered the next file interval
// the next file must be opened as soon as a device has entered the next file interval  
typedef struct file_info{
	int ncid;				// file handle from netcdf
	bool open;          	// is file open
	std::string fname;  	// name of file = projectname_yyyymmdd_hhMM.cdf
	int dayindex;      	 	// a day can be divided in n=86400/runlen files, given a run length in seconds
	int devcnt;				// counts up when more devices use it and counts down when device switch to next file
	struct timeval last_used;// how long ago was the last use of this netcdf file handle
	int julday;				// used in day crossing during dayfile acquisition
};
typedef file_info *pfile_info;
typedef std::vector<pfile_info> nc_finfo;

/* structure to hold a netcdf text attribute */
typedef struct Tnc_attribtext{
	std::string name,	value;
}Tnc_attribtext;

/* structure to hold a netcdf double attribute */
typedef struct Tnc_attribdouble{
	std::string name;
	double value;
}Tnc_attribdouble;

/* structure to hold a netcdf float attribute */
typedef struct Tnc_attribfloat{
	std::string name;
	float value;
}Tnc_attribfloat;

/* structure to hold a netcdf int attribute */
typedef struct Tnc_attribint{
	std::string name;
	int value;
}Tnc_attribint;

/* structure to hold a netcdf double attribute */
typedef struct Tnc_attribtime{
	std::string name;
	long int value;
}Tnc_attribtime;

/* structure to hold a netcdf int attribute */
typedef struct Tnc_dim{
	std::string name;
	int value;
	int id;
}Tnc_dim;

struct nc_info{
	std::string netcdf_name;  // the file name

	// global attributes
	//std::list<Tnc_attributes> global_attribs; // will contain the netcdf global attributes
	struct Tnc_attribtext experiment, source, location, affiliation, pi, url, measured_quantities, devs;
	struct Tnc_attribfloat longitude, latitude, altitude;
	struct Tnc_attribtime date, stop;
	struct Tnc_attribint run, runlength;
	// dimensions
	struct Tnc_dim dim_1s, dim_12s, dim_30s, dim_60s, dim_01s;
	// local variables	
	int ncid;			// netcdf file handle
	int retval;
	struct file_info ninfo[2];
	nc_finfo finfo;
};

int init_nc_info();
int create_open_nc_file(div_t *index, struct tcp_server_info *device);
int nc_set_global_attribs();
void nc_write_signals(struct q_item *qui, int index);
int assign_dev_ncfptr(struct timeval *tstamp, struct tcp_server_info *dev);
int pfile_info_cleanup_dangling(struct timeval *pstamp);

#endif