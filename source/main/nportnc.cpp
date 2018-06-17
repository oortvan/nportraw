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

#include "nportnc.h"
#include <sstream>

struct nc_info nc;

double timetowin(time_t x){
	return (x/86400  + 25569);
}

void nc_derr(const char* funcn, int e, const char* xetra, int dimsize){
	char str[150];
	sprintf(str,"%s, %s size %d (%s)",nc_strerror(e), xetra, dimsize, funcn);
	tolog(str, LOG_ERR);
}

int nc_add_global_devices(){
	/*
	 * add all devices to global attributes of this file
	 */
	int i = 0;
	struct Tnc_attribtext dev_glb;
	struct tcp_server_info	*adev;
	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end()){
		adev = (*lt); // get a device structure from list
		if (adev->no_write == 0){ // when 0 write data from this device
			std::stringstream ss;
			ss << "device_" << i << "_" << adev->type.c_str();// << "_" << adev->src.c_str();
			dev_glb.name = ss.str();
			dev_glb.value = adev->comment.c_str();
			if (nc_put_att_text (nc.ncid, NC_GLOBAL, dev_glb.name.c_str(), strlen(dev_glb.value.c_str()), dev_glb.value.c_str())) ERR(nc.retval);
			i++;
		}
		lt++;
	}
}

int nc_set_global_attribs(){
	div_t days;

	days  = div(proj.tstamp,86400);  // get the current day time in seconds as remainder
	nc.date.value = static_cast<long int>(proj.tstamp) - days.rem;
	nc.stop.value = nc.date.value + 86400;

	nc.experiment.name = "EXPERIMENT";
	nc.experiment.value = proj.name;
	nc.source.name = "SOURCE";
	nc.source.value = "EAS";
	nc.location.name = "location";
	nc.location.value = proj.location.c_str();
	nc.affiliation.name = "affiliation";
	nc.affiliation.value = proj.affiliation.c_str();
	nc.date.name = "DATE";
	nc.stop.name = "STOP";
	nc.run.name = "RUN";
	nc.run.value = 1;
	nc.runlength.name = "runlength";
	nc.runlength.value = proj.runlength;
	nc.longitude.name = "longitude";
	nc.longitude.value = proj.longitude;
	nc.latitude.name = "latitude";
	nc.latitude.value = proj.latitude;
	nc.altitude.name = "altitude";
	nc.altitude.value = proj.altitude;

	nc.devs.name = "instruments";
	nc.devs.value = proj.devs.c_str();
	nc.measured_quantities.name = "measured_quantities";
	nc.measured_quantities.value = proj.measured_quantities.c_str();
	nc.url.name = "url";
	nc.url.value = proj.url.c_str();
	nc.pi.name = "pi";
	nc.pi.value = proj.pi.c_str();

	// now the attributes to the file, to make it useble for EAS runlook
	if (nc_put_att_text (nc.ncid, NC_GLOBAL, nc.experiment.name.c_str(), strlen(nc.experiment.value.c_str()), nc.experiment.value.c_str())) ERR(nc.retval);
	if (nc_put_att_text (nc.ncid, NC_GLOBAL, nc.source.name.c_str(), strlen(nc.source.value.c_str()), nc.source.value.c_str())) ERR(nc.retval);
	if (nc_put_att_long (nc.ncid, NC_GLOBAL, nc.date.name.c_str(), NC_LONG, 1, &nc.date.value)) ERR(nc.retval);
	if (nc_put_att_long (nc.ncid, NC_GLOBAL, nc.stop.name.c_str(), NC_LONG, 1, &nc.stop.value)) ERR(nc.retval);
	if (nc_put_att_int  (nc.ncid, NC_GLOBAL, nc.run.name.c_str(), NC_INT, 1, &nc.run.value)) ERR(nc.retval);

	// new global attributes compared to old version
	if (nc_put_att_text (nc.ncid, NC_GLOBAL, nc.measured_quantities.name.c_str(), strlen(nc.measured_quantities.value.c_str()), nc.measured_quantities.value.c_str())) ERR(nc.retval);
	if (nc_put_att_text (nc.ncid, NC_GLOBAL, nc.devs.name.c_str(), strlen(nc.devs.value.c_str()), nc.devs.value.c_str())) ERR(nc.retval);
	nc_add_global_devices();

	if (nc_put_att_text (nc.ncid, NC_GLOBAL, nc.location.name.c_str(), strlen(nc.location.value.c_str()), nc.location.value.c_str())) ERR(nc.retval);
	if (nc_put_att_float(nc.ncid, NC_GLOBAL, nc.longitude.name.c_str(), NC_FLOAT, 1, &nc.longitude.value)) ERR(nc.retval);
	if (nc_put_att_float(nc.ncid, NC_GLOBAL, nc.latitude.name.c_str(), NC_FLOAT, 1, &nc.latitude.value)) ERR(nc.retval);
	if (nc_put_att_float(nc.ncid, NC_GLOBAL, nc.altitude.name.c_str(), NC_FLOAT, 1, &nc.altitude.value)) ERR(nc.retval);
	if (nc_put_att_int  (nc.ncid, NC_GLOBAL, nc.runlength.name.c_str(), NC_INT, 1, &nc.runlength.value)) ERR(nc.retval);

	if (nc_put_att_text (nc.ncid, NC_GLOBAL, nc.affiliation.name.c_str(), strlen(nc.affiliation.value.c_str()), nc.affiliation.value.c_str())) ERR(nc.retval);
	if (nc_put_att_text (nc.ncid, NC_GLOBAL, nc.pi.name.c_str(), strlen(nc.pi.value.c_str()), nc.pi.value.c_str())) ERR(nc.retval);
	if (nc_put_att_text (nc.ncid, NC_GLOBAL, nc.url.name.c_str(), strlen(nc.url.value.c_str()), nc.url.value.c_str())) ERR(nc.retval);

	return 0;
}

// setup a dimensions as described in xml
// scan all devices
int nc_set_dimensions(){
	int adim;
	struct tcp_server_info	*adev;
	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end()){
		adev = (*lt); // get a device structure from list
		if (adev->no_write == 0); // when 0 write data from this device
		if (nc.retval = nc_inq_dimid(nc.ncid, adev->dim_name.c_str(), &adim)); // nc_derr("nc_inq_dimid", nc.retval, adev->dim_name.c_str(), adev->dim);
		if ( nc.retval != 0 ){  // dimension not present yet?
			// all relevant dimension parameters are produced in nport.cpp xml reading
			nc.retval = nc_def_dim(nc.ncid, adev->dim_name.c_str(),  adev->dim,  &adim);
			if (nc.retval) nc_derr("nc_def_dim", nc.retval, adev->dim_name.c_str(), adev->dim);
		}
		lt++;
	}
	return nc.retval;
}

int nc_set_variables(div_t *sday){
	struct tcp_server_info	*adev;
	int adim,curp=0;
	div_t inbtw, index, secs, usecs;
	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end()){
		adev = (*lt); // get a device structure from list
		if (adev->no_write == 1) lt++; // when 1 write no data from this device
		else{
			nc.retval = nc_inq_dimid(nc.ncid, adev->dim_name.c_str(), &adim);  //retrieve dim id for this device by name
			if (nc.retval == 0){ // found a valid dimension for this device

				// some processing for current index of this run
				secs = div(int(adev->tstamp.tv_sec),proj.runlength);
				inbtw = div(sday->rem, proj.runlength);     // use as in between result to calculate index of < 1Hz
				index = div(inbtw.rem, adev->dt);	  		// current index in quotient

				// calculate the start and stop times of this file
				// hier onderscheid maken tussen een lege volgende container maken en actuale container maken.
				long int astart = adev->tstamp.tv_sec - secs.rem, astop;
				if (proj.make_empty_container) astart = astart + proj.runlength;
				astop = astart + proj.runlength;
				dbg_printf("\nstart, stop: %d, %d, %d, %d", nc.date.value, proj.runlength,  astart, astop);
				//dbgnc_printf("start, stop: %d, %d, %d, %d\n", nc.date.value, proj.runlength,  astart, astop);

				// scan the variables of this device (siam)
				sigs_container::iterator avar = adev->signals.begin();
				while(avar != adev->signals.end()){
					// nc indexing
					avar->curpos = index.quot; // use index calculated from current time
					avar->index[0] = index.quot; // only one dimensional variable

					if (nc.retval = nc_def_var(nc.ncid, avar->name.c_str(), NC_FLOAT, 1, &(adim), &(avar->nc_id))) ERR(nc.retval);
					//nc.retval = nc_def_var(nc.ncid, avar->name.c_str(), NC_SHORT, 1, &(adim), &(avar->nc_id));
					if (nc.retval = nc_put_att_long(nc.ncid, avar->nc_id, "RDATE", NC_LONG,   1, &nc.date.value)) ERR(nc.retval);
					if (nc.retval = nc_put_att_long(nc.ncid, avar->nc_id, "START", NC_LONG, 1, &astart)) ERR(nc.retval); // unix time notation
					if (nc.retval = nc_put_att_long(nc.ncid, avar->nc_id, "STOP",  NC_LONG, 1, &astop)) ERR(nc.retval);

					if (nc.retval = nc_put_att_text(nc.ncid,  avar->nc_id, "instrum", strlen(avar->instrum.c_str()), avar->instrum.c_str())) ERR(nc.retval);
					if (nc.retval = nc_put_att_text(nc.ncid,  avar->nc_id, "sn",      strlen(avar->sn.c_str()),      avar->sn.c_str())) ERR(nc.retval);
					if (nc.retval = nc_put_att_text(nc.ncid,  avar->nc_id, "Unit",    strlen(avar->units.c_str()),   avar->units.c_str())) ERR(nc.retval);
					if (nc.retval = nc_put_att_text(nc.ncid,  avar->nc_id, "caldate", strlen(avar->caldate.c_str()), avar->caldate.c_str())) ERR(nc.retval);

					if (nc.retval = nc_put_att_int  (nc.ncid, avar->nc_id, "CURPOS", NC_INT,   1, &curp)) ERR(nc.retval);
					if (nc.retval = nc_put_att_float(nc.ncid, avar->nc_id, "A",      NC_FLOAT, 1, &avar->a)) ERR(nc.retval); // calculated from ranges
					if (nc.retval = nc_put_att_float(nc.ncid, avar->nc_id, "B",      NC_FLOAT, 1, &avar->b)) ERR(nc.retval); // calculated from ranges
					if (nc.retval = nc_put_att_float(nc.ncid, avar->nc_id, "C",      NC_FLOAT, 1, &avar->coef_3)) ERR(nc.retval);
					if (nc.retval = nc_put_att_float(nc.ncid, avar->nc_id, "D",      NC_FLOAT, 1, &avar->coef_4)) ERR(nc.retval); // mostly a calibration factor
					if (nc.retval = nc_put_att_float(nc.ncid, avar->nc_id, "k1",     NC_FLOAT, 1, &avar->k1)) ERR(nc.retval);  // pyrgeo cal k1
					if (nc.retval = nc_put_att_float(nc.ncid, avar->nc_id, "k2",     NC_FLOAT, 1, &avar->k2)) ERR(nc.retval);  // pyrgeo cal k2

					float adt = 1/static_cast<float>(adev->freq);  // actual must be freq
					if (nc.retval = nc_put_att_float(nc.ncid, avar->nc_id, "dt",         NC_FLOAT, 1, &adt)) ERR(nc.retval);
					if (nc.retval = nc_put_att_float(nc.ncid, avar->nc_id, "_FillValue", NC_FLOAT, 1, &adev->missing_value)) ERR(nc.retval);

					//dbgnc_printf("nc_setvars: name: %s, curpos: %d, rdate: %d, rstart: %d, rstop: %d\n",
					//			avar->name.c_str(), index.quot, nc.date.value, astart, astop);

					avar++;
				}
				lt++;
			}
		}
	}
	//dbgnc_printf("END OF PROJECT SET NC VARIABLES\n");
	return 0;
}

int nc_open_variables(div_t *sday){
	div_t inbtw, index;
	struct tcp_server_info	*adev;
	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end()){
		adev = (*lt); // get a device structure from list
		if (adev->no_write == 0){ // when 0 write data from this device
			// some processing for current index of this run
			inbtw = div(sday->rem, proj.runlength);    // use as in between result to calculate index < 1Hz
			index = div(inbtw.rem, adev->dt);	  		// current index in quotient

			// scan the variables of this device (siam)
			sigs_container::iterator avar = adev->signals.begin();
			while(avar != adev->signals.end()){
				avar->curpos = index.quot; // index calculated from current time
				avar->index[0] = index.quot; // nc var index of type size_t
				if (nc.retval = nc_inq_varid(nc.ncid,avar->name.c_str(),&(avar->nc_id))) ERR(nc.retval);
				dbg_printf("%s id:%d\n",avar->name.c_str(),avar->nc_id);
				avar++;
			}
		}
		lt++;
	} // end if select
	return 0;
}

// this function is called by the PRODUCER process when
// new records are available in   CONSUMER process
void nc_write_signals(struct q_item *qui, int index){
	std::string* acopy = qui->samples;
	char token[100], logme[100], phpmsg[300];
	int sp, fp=0, i=0, ti, vc=0, gonext, spike=0;
	short ashort;
	float afloat, dtsync=0;
	bool dosync = FALSE;
	FILE *fpt; // = fopen("/home/httpd/htdocs/nport/phpmsg", "w");

	// scan all signals in this device from the ascii line qui->samples->c_str()
	// write them to their own netcdf variable, is present when (sp==fp)
	//dbg_printf("\nrounded gill index: %d, %d, %s", index, qui->tcp->cindex, qui->samples->c_str());
	//return;

	sigs_container::iterator avar = qui->tcp->signals.begin();
	phpmsg[0] = 0;

	avar->index[0] = qui->cindex;
	avar->curpos = qui->cindex;

	while(avar != qui->tcp->signals.end() && (acopy->c_str()[i] != NULL)){
		sp = avar->ad_hard;
		gonext = TRUE;
		ti = 0;
		token[0] = 0;
		if (acopy->c_str()[i] != NULL) gonext = TRUE;
		while (gonext==TRUE){
			if ((acopy->c_str()[i] == ' ') || (acopy->c_str()[i] == 0x09) || (acopy->c_str()[i] == ';') || (acopy->c_str()[i] == NULL)){
				token[ti] = 0;
				if (sp==fp){  // is a position match found between token and adhard

					avar->index[0] = qui->cindex;
					avar->curpos = qui->cindex;

					// time handling for this device, includes synchronisation check
					if (sp == 0){
						afloat = (qui->tstamp->tv_sec % 86400) + qui->tstamp->tv_usec/1E6;
						afloat = ceilf(afloat * 100) / 100;
						qui->tcp->last_tstamp = afloat; // last current time in seconds remembered
						//std::cout << "\nindex: " << afloat;
						//dbg_printf("\ntime: %d, %d, %d, %f",qui->cindex, qui->tstamp->tv_sec % 86400, qui->tstamp->tv_usec, afloat);
					}
					else if (avar->is_status == 1){ 		   // test if its status and
						try{
							afloat = static_cast<float>(token[0]); // status is in ASCII
						}
						catch(...){
							afloat = 0;
							dbg_printf("\n%s status %f",avar->name.c_str(), afloat);
						}
						avar->c_status = token[0];  // remember last status
					}
					else if (avar->fie != NULL) afloat = avar->fie(token); // is some special function assigned to this signal
					else try{
						afloat = atof(token);
					}
					catch(...){
						afloat = 0;
					}
					if (avar->curpos < qui->tcp->dim){
						if (qui->tcp->nc_file != NULL)
						if (qui->tcp->nc_file->ncid != -1)
						if (avar->index[0] < (qui->tcp->dim) ){

							/*
							 * Despiking method that depends on delta_max and lastvalue
							 * delta_max from xml and lastvalue (after initial -999) from data stream.
							 * When afloat is the current sample value for a signal
							 * then when |afloat - lastvalue| > delta_max
							 * assign lastvalue to afloat else assign afloat to lastvalue
							 * A form of despiking the data is obtained
							 */
//#ifdef despike == 1
							if (avar->delta_max != -1){ // keep some data ok
								if (avar->lastvalue != -999){
									try{
										if ( abs(avar->lastvalue-afloat) > avar->delta_max){
											afloat = avar->lastvalue;
											spike = 1;
											//printf("DELTA_MAX ERROR %s: %f, %f\n", avar->sname.c_str(), afloat, avar->lastvalue);
										}
										else avar->lastvalue = afloat;
									}
									catch(...){
										afloat = -999999; //std::numeric_limits<float>::quiet_NaN();
									}
								}
								else avar->lastvalue = afloat;  // assumes first value is OK
							}
//#endif
							if (nc.retval = nc_put_var1_float(qui->tcp->nc_file->ncid, avar->nc_id, avar->index, &afloat)) ERR(nc.retval);

							// mark device activity
							gettimeofday(&qui->tcp->nc_file->last_used, NULL);
						}
#ifdef phpline == 1		// php approach to show device data
						//if (phpmsg[0] != 0)	sprintf(phpmsg,"%s,%f",phpmsg,afloat); // append float value to one-liner
						//else {
						//	sprintf(phpmsg,"%s%f",phpmsg,afloat);
						//	fpt = fopen(avar->envname.c_str(),"w");
						//}
#endif
					}
					avar->curpos++;
					gonext = FALSE;
					token[0] = 0;
				}
				fp++;	// increment signal to find position (must be equal to ad_hard)
				vc++;	// nth token found
				ti=0;	// token string index, restart from zero
				if (acopy->c_str()[i] == NULL) gonext = FALSE;  // leave the get next token loop
			}
			else{
				token[ti] = acopy->c_str()[i];
				ti++;
				if (ti > 99) gonext = FALSE;  // max size of token string
			}
			i++;
		}
		avar++;
	}
	if (spike == 1){
		spike = 0;
		tolog(qui->samples->c_str(), LOG_ERR);
	}

#ifdef phpline == 1
	//if (phpmsg[0] != 0) { // create one liner, for php ajax
	//	if (fpt != NULL){
	//		fputs(phpmsg, fpt);
	//		fclose(fpt);
	//	}
	//}
#endif
}

void show_devs_pinfos(){
	struct tcp_server_info	*tcp;
	dbgnc_printf("DEVS PINFO ");
	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end()){
		tcp = (*lt); // get device structure from list
		dbgnc_printf("%s:%p:%d ", tcp->src.c_str(), tcp->nc_file, tcp->tstamp.tv_sec);
		++lt;
	}
	dbgnc_printf("\n");
}

void nc_path_fname(char *fname, char *path, struct tm *curr){
	sprintf(fname,"%s_%4d%02d%02d_%02d%02d",
	        proj.shortname.c_str(),
	        curr->tm_year+1900,curr->tm_mon+1,curr->tm_mday,
	        curr->tm_hour,curr->tm_min);

	sprintf(path,"%s/%s/%4d/%02d/%02d",
	        proj.localpath.c_str(),proj.name.c_str(),
		    curr->tm_year+1900,curr->tm_mon+1,curr->tm_mday);

	sprintf(proj.ncpath,"%s/%s.%s",path,fname,proj.format.c_str()); // complete path to netcdf file

}

int init_nc_info(){
	nc.finfo.clear();

	nc.ninfo[0].ncid = -1;
	nc.ninfo[0].dayindex = -1;
	nc.ninfo[0].fname = "";
	nc.ninfo[0].open = FALSE;
	nc.ninfo[0].devcnt = 0;
	nc.ninfo[1].ncid = -1;
	nc.ninfo[1].dayindex = -1;
	nc.ninfo[1].fname = "";
	nc.ninfo[1].open = FALSE;
	nc.ninfo[1].devcnt = 0;

	return 1;
}

int nc_new_pfile(struct tcp_server_info *dev, div_t *indexing, pfile_info new_finfo, struct timeval *latest){
	char path[100],fname[50];
	struct tm *curr;
	time_t now = dev->tstamp.tv_sec - indexing->rem;
	if (now == 0) return -1;  // 1970 data is not possible
	if (dev->julday != dev->curr->tm_yday)
		dev->julday = dev->curr->tm_yday;
	curr = gmtime(&now);
	new_finfo->julday = curr->tm_yday;
	new_finfo->dayindex = indexing->quot;
	dev->dayindex = indexing->quot;
	new_finfo->devcnt = 1; // always first device comes here
	new_finfo->last_used.tv_sec = latest->tv_sec;
	nc_path_fname(fname, path, curr);
	new_finfo->fname = proj.ncpath;
	mkpath(path, 0777); // look for and create if needed all dirs in path 777 decimal and 0777 octal
	// code: open here the netcdf file
	new_finfo->ncid = create_open_nc_file(indexing, dev);
	if (new_finfo->ncid == -1) {return -1;}
	if (dev->gill != NULL) dev->gill->last_cindex = -1;
	new_finfo->open = TRUE;
	dev->nc_file = new_finfo;
	//dbgnc_printf("VECTOR HAS %d ELEMENTS\n",nc.finfo.size());
	dbgnc_printf("HANDLE %s:%d OPEN FOR %s\n",dev->src.c_str(), new_finfo->ncid,new_finfo->fname.c_str());
	dbgnc_printf("CREATE %s TO HANDLE %d:%d:%d:%p\n",
	             dev->src.c_str(), new_finfo->ncid, new_finfo->dayindex, new_finfo->devcnt, dev->nc_file);
	show_devs_pinfos();
	return new_finfo->ncid;
}

void nc_dangling(struct timeval *latest){
// moet dit aan de sample snelheid van het device gekoppeld zijn?
	for (int i=0; i>2; i++)
	if (nc.ninfo[i].open){
		if ((latest->tv_sec - nc.ninfo[i].last_used.tv_sec) > 5){ // means atleast 5 seconds before starting a new file data from the device stopped
				nc.ninfo[i].ncid = -1;
				nc.ninfo[i].dayindex = -1;
				nc.ninfo[i].open = FALSE;
				nc.ninfo[i].devcnt = 0;
		}
	}
}

int assign_dev_ncfptr(struct timeval *tstamp, struct tcp_server_info *dev){
	// new approach: give every device a netcdf file handle pointer
	if (dev == NULL) return -1;
	if (dev->tstamp.tv_sec == 0) return -1;
	if (tstamp->tv_sec == -1) return -1;

	div_t indexing = div(tstamp->tv_sec, 86400);  // the remainder delivers the second of the day
	indexing = div(indexing.rem, proj.runlength); // quot the nth file of the day, remainder index in current netcdf file for this device

	time_t now = dev->tstamp.tv_sec - indexing.rem;
	dev->curr = gmtime(&now); // get the julian day, day crossing detectection
	//dbgnc_printf("%s:%s:%d\n", dev->type.c_str(), dev->src.c_str(), dev->curr->tm_yday);
	if (dev->julday == -1) dev->julday = dev->curr->tm_yday; // init julday for day crossing

	// in case of day file (runlength=86400) indexing.quot will be 0 or 1,
	// this is only true for devs with sfreq >= 1Hz.
	// Slower devs will always show indexing.quot = 0
	if (dev->dayindex == indexing.quot){
		if (dev->julday == dev->curr->tm_yday)
			return 1;
	}

	dev->dayindex = indexing.quot;

	dbgnc_printf("\nENTRY %s:%s:%d:%d:%d\n", dev->type.c_str(), dev->src.c_str(), indexing.quot, tstamp->tv_sec, dev->last_used.tv_sec);
	// entry point testing for file handle
	timeval latest;
	latest.tv_sec = tstamp->tv_sec;
	latest.tv_usec = tstamp->tv_usec;
	//nc_dangling(&latest);

	if (dev->nc_file == NULL){
		// no netcdf file handle assigned to this device
		for (int i=0; i<2; i++){
			if ((nc.ninfo[i].dayindex == indexing.quot)){
				dev->dayindex = indexing.quot;
				// the proper file handle already opened
				nc.ninfo[i].devcnt++;
				nc.ninfo[i].last_used.tv_sec = latest.tv_sec;
				dev->nc_file = &nc.ninfo[i];
				dbgnc_printf("DEVNULL_ADDED %s TO HANDLE %d:%d:%d:%p\n",
				     dev->src.c_str(), nc.ninfo[i].ncid, nc.ninfo[i].dayindex, nc.ninfo[i].devcnt, dev->nc_file );
				return nc.ninfo[i].ncid;
			}
		}
		if (!nc.ninfo[0].open)
			return nc_new_pfile(dev, &indexing, &nc.ninfo[0], &latest);
		if (!nc.ninfo[1].open)
			return nc_new_pfile(dev, &indexing, &nc.ninfo[1], &latest);
	}

	else {
		// device has a pointer to a netcdf file
		// and has it reached end of runlen interval?

		if ((indexing.quot != dev->nc_file->dayindex) || (dev->julday != dev->curr->tm_yday)){ // indexing.quot is 0 when day crossing!!!!!!!
			dev->nc_file->devcnt--;
			dbgnc_printf("DECREMENT %s:%p:%d\n",dev->src.c_str(), dev->nc_file, dev->nc_file->devcnt);
			show_devs_pinfos();
			if (dev->nc_file->devcnt == 0){
				// all devices are done in this interval
				if (nc.retval = nc_close(dev->nc_file->ncid)){ // close netcdf
					ERR(nc.retval);
					return -1;
				}
				dbgnc_printf("HANDLE %s:%d CLOSED AND VECTOR[0] DELETED, ALL DEVS TO NEXT INTERVAL\n",
				             dev->src.c_str(), dev->nc_file->ncid);
				dev->nc_file->ncid = -1;
				dev->nc_file->dayindex = -1;
				dev->nc_file->open = FALSE;
				dev->nc_file = NULL;
			}

			for (int i=0; i<2; i++)
			if ((nc.ninfo[i].dayindex == indexing.quot) && (nc.ninfo[i].julday == dev->curr->tm_yday)){
				// the proper file handle already opened
				nc.ninfo[i].devcnt++;
				nc.ninfo[i].last_used.tv_sec = latest.tv_sec;
				dev->nc_file = &nc.ninfo[i];
				dev->dayindex = indexing.quot;
				dev->julday = dev->curr->tm_yday;  // handle the day crossing
				dbgnc_printf("FOUNDADDED %s TO HANDLE %d:%d:%d:%p\n",
						     dev->src.c_str(), nc.ninfo[i].ncid, nc.ninfo[i].dayindex, nc.ninfo[i].devcnt, dev->nc_file);
				show_devs_pinfos();
				return nc.ninfo[i].ncid;
			}
			// when the difference between dayindexing >= 2: atleast 1 device has stopped
			// thus failing to reach the devcnt 0 boundary
			if (!nc.ninfo[0].open || ((indexing.quot - nc.ninfo[0].dayindex) >= 2)){
				return nc_new_pfile(dev, &indexing, &nc.ninfo[0], &latest);
			}
			if (!nc.ninfo[1].open || ((indexing.quot - nc.ninfo[1].dayindex) >= 2)){
				return nc_new_pfile(dev, &indexing, &nc.ninfo[1], &latest);
			}
		}
	}
	return -1;
}

// create or open the nc file
int create_open_nc_file(div_t *index, struct tcp_server_info *device){
	nc.netcdf_name = proj.name+"."+proj.format;

	// close this nc file
	//nc.retval = nc_close(nc.ncid);
	proj.nc_is_open = FALSE;

	struct tm *curr;
	// calculate the current index
	// curr = gmtime(&tcp->tstamp.tv_sec);

	if (fileexists(proj.ncpath) && (device != NULL)){
		if (nc.retval = nc_open(proj.ncpath, NC_WRITE|NC_SHARE, &nc.ncid)){
			ERR(nc.retval)
			return -1;
		}
		else{
			proj.nc_is_open = TRUE;
			nc_open_variables(index); // use index to start registration at current time index
			dbg_printf("%s opened\n",proj.ncpath);
			//nc.finfo[0].open = TRUE;
			//device->nc_file = &nc.finfo[0]; // assign the current file to individual device
			return nc.ncid;
		}
	}

	else{  // Create the file.
		if ((nc.retval = nc_create(proj.ncpath, NC_SHARE, &nc.ncid)))
			ERR(nc.retval);
		proj.nc_is_open = TRUE;
		dbgnc_printf("\nproject_init: %s\n",proj.ncpath);

		// global attributes
		if (nc.retval = nc_set_global_attribs())
			ERR(nc.retval);
			dbgnc_printf("project_init_global_attributes: %s\n",proj.ncpath);
		// dimensions
		if (nc.retval = nc_set_dimensions())
			ERR(nc.retval);
		dbgnc_printf("project_init_set_dimensions: %s\n",proj.ncpath);
		if (nc.retval = nc_set_variables(index)) // use index to start registration at current time index
			ERR(nc.retval);
		dbgnc_printf("project_init_set_variables: %s\n",proj.ncpath);

		// End define mode.
		if (nc.retval = nc_enddef(nc.ncid)){
			ERR(nc.retval);
			return -1;
		}

		if (device == NULL){
			if (nc.retval = nc_close(nc.ncid)) ERR(nc.retval); // precreation of next container
			return -1;
		}
		return nc.ncid;
	}
}
