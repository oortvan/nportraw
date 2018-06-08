// Copyright (C) 2016  KNMI
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
#include	"nportnc.h"
#include	"queue_thread.h"

struct project_info proj;
struct signal_info asig;
std::list<tcp_server_info*> listofdevs;

// next 2 global variables that updated in rotor and status thread
float ddd_glb = -999999, ddd_rot = -999999;
int rotor_maintenance = 0, rotack = 0;
pthread_mutex_t rot_cv;  // semaphore synchronizing with rotor thread, not used

// licor pipe for thread comm
int lipipe = -1;

void tolog(const char* amsg, int PRIORITY){
	dbg_printf("\n%s",amsg);
	std::string logmsg = proj.name + " " + amsg;
	openlog("NPORT", 0, LOG_USER);
	syslog(LOG_USER|PRIORITY, logmsg.c_str());
	closelog();
}	

bool fileexists( const char* Filename )
{
    return access( Filename, 0 ) == 0;
}

static int do_mkdir(const char *path, mode_t mode)
{
    struct stat  st;
    int status = 0;
    if (stat(path, &st) != 0){
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode)){
        errno = ENOTDIR;
        status = -1;
    }
    return(status);
}

int mkpath(const char *path, mode_t mode)
{
    char *pp;
    char *sp;
    int  status;
    char *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(path, mode);
	free (copypath);
    return (status);
}

const char * timestr(timeval tstamp){
	tm* nowtm;
	char tmbuf[64];
	
	nowtm = localtime(&tstamp.tv_sec);
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
	return tmbuf;
}	

int Getdfp(time_t tcurr, div_t *index, struct tcp_server_info *device){ // attempt to make more general regarding runlength
	char path[100],fname[50];
	struct tm *curr;
	ldiv_t indexing;
	time_t now; // fixed start time, begin of this runlength period

	// any reason to start file processing
	if ((((tcurr % proj.runlength) == 0) || proj.curr_flenmin == -1) && (proj.savedata == TRUE)){

		indexing  = ldiv(tcurr, proj.runlength);
		now = tcurr - indexing.rem;
		curr = gmtime(&now);
		sprintf(fname,"%s_%4d%02d%02d_%02d%02d",
		        proj.shortname.c_str(),
		        curr->tm_year+1900,curr->tm_mon+1,curr->tm_mday,
		        curr->tm_hour,curr->tm_min);//minits);

		if ((fname == proj.fname.c_str())){ // new file already exists and is open? inhibit doubles
			return -1;
		}	
		proj.tstamp = tcurr;
		proj.local = gmtime(&tcurr);  // make it the new start time
		
		sprintf(path,"%s/%s/%4d/%02d/%02d",
		       proj.localpath.c_str(),proj.name.c_str(), 
			   curr->tm_year+1900,curr->tm_mon+1,curr->tm_mday);

		mkpath(path, 0777); // look for and create if needed all dirs in path

		proj.curr_flenmin = now;//minits;
		// use project format entry as extension
		sprintf(proj.ncpath,"%s/%s.%s",path,fname,proj.format.c_str()); // complete path to netcdf file

		// give some file information to log messages
		sprintf(proj.toMessages,"FILE %s line-count: %d",fname, proj.linecnt);
		proj.linecnt = 1;
		tolog(proj.toMessages, LOG_INFO);

		// create a new or open file and return its handle
		// but only when no handle to the current index exists
		create_open_nc_file(index, device);
		proj.fname = fname;
	}	
}	

int parse_xml(const char* filename){
	dbg_printf("pugi parsing\n");
	
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename);

#ifdef DEBUG_ON
	std::cout 	<< "Load result: " << result.description()  
				<< "\nError offset: " << result.offset << "\n";
#endif
}	

int create_next_nc_file(const char* _arg){
	if (strstr(_arg,"-n") != NULL){
		div_t days;
		struct timeval tstamp;
		time_t nextstart;
		struct tm *curr;
		proj.make_empty_container = TRUE;
		gettimeofday(&tstamp, NULL); // record time stamp
		// set nextstart last start + runlength in the future
		nextstart = tstamp.tv_sec - (tstamp.tv_sec % proj.runlength) + proj.runlength; // zodat de volde container wordt aangewezen
		proj.tstamp = nextstart;  // be sure global attribure RDATE and START are OK
		days  = div(nextstart, proj.runlength);
		Getdfp (nextstart, &days, NULL);  // function creates the next empty file container
		curr = gmtime(&nextstart);
		dbg_printf("\n%s_%4d%02d%02d_%02d%02d\n",
	        proj.name.c_str(),
	        curr->tm_year+1900,curr->tm_mon+1,curr->tm_mday,
	        curr->tm_hour,curr->tm_min);//minits);
		return 0;
	}	
	return -1;
}

/*
 * Preprocessing of SIAM values, in some siam messages extended value notation
 * is applied. The SIAM type involved are: pressure, temperature, percipitation.
 * This preprocessing depends on the 'Function' element in a signal node of the
 * initialization xml project file, sofar 'ps', 'tx' and 'ex' are possible.
 * 
 * Pressure signal ranges from 9400..9999, 0000..50 -> 940 .. 1050 hPa, see ps
 * Air temp is given in units of 0.1 C, see tx function
 * Psychrometer temperatuur is in units 0.001 C, see py function
 * Percipitation NI and ND has exponential notation, ABCD -> 0.BCD 10^A see ex.
 */ 

float ps(char * x){
	float y = atof(x)*0.1;
	if (y < 50) y = y + 1000.0;
	//dbg_printf("\nps fie: %s %f",x,y);
	return y;
}

float tx(char * x){
	float y = 0.1*atof(x);
	return y;
}

float py(char * x){
	float y = 0.001*atof(x);
	return y;
}

float ex(char * x){
	float y;
	y = (((x[1]-48)*100) + ((x[2]-48)*10) + (x[3]-48))/1000;
	y = y*pow(10,(x[0]-48));
	//dbg_printf("\nex fie: %s %f",x,y);
	return y;
}

/*
 * Preprocessing of XLAS gps time
 * the time string hh:mm:ss must be converted to minits/day and seconds/minit
 * the string is first converted to unixtime: which is seconds since some date
 * 
 * Preprocessing CN values, which are very small, with cn function and make bigger
 */  

float cn(char * x){
	float y = 1.0E13*atof(x);
	return y;
}

float xs(char * x){
	struct tm mytm;
	strptime(x,"%H:%M:%S",&mytm);
	float y = mytm.tm_hour*3600+mytm.tm_min*60+mytm.tm_sec;
	//dbg_printf("\nxs fie: %s, %f",x, y);
	return y;
}	

/*
 * hex to float functions
 * mainly used in R3 processing
 * 
 */

float sf(char * x){
	short y=-999;
	sscanf(x,"%hx", &y);
	//dbg_printf("\nsf fie: %s, %d",x, y);
	return float(y);
}	

/*
 * implementation of itoa
 * conversion of integer to string
 */
std::string itoa(int value){
	char vbuf[12]; // max 10 digits
	sprintf(vbuf,"%d",value);
	return std::string(vbuf);  // implicit init of object
}
/*
 * implementation of ftoa
 * conversion of integer to string
 */
std::string ftoa(float value){
	char vbuf[12]; // max 10 digits
	sprintf(vbuf,"%f",value);
	return std::string(vbuf);  // implicit init of object
}
/*
 * implementation of lutoa
 * conversion of integer to string
 */
std::string lutoa(unsigned long value){
	char vbuf[12]; // max 10 digits
	sprintf(vbuf,"%lu",value);
	return std::string(vbuf);  // implicit init of object
}


/*
 * takes the initialization (project, devices with signals) of the xml as input
 * has as parameter a pointer to a string, in this string a json like presentation
 * is build
 */
int nport_json(std::string *webformat){
	std::string afie;
	*webformat = "{\"project\":\""+proj.name+", "+proj.longname+"\",\"devices\":[";
	struct tcp_server_info	*adev;
	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	int comma_cnt = 0;
	while( lt != listofdevs.end()){
		adev = (*lt); // get a device structure from list
		if (adev->webclient == 1){ // when possible client
			if (comma_cnt > 0) *webformat += ",";
			*webformat += "{\"dname\":\""+adev->src+"\","+
						  "\"sf\":\""+ftoa(adev->freq)+
						  "\",\"signals\":[";
			// scan the variables of this device (siam)
			sigs_container::iterator avar = adev->signals.begin();
			while(avar != adev->signals.end()){
					 if (avar->instrum == "PYRANOMETER") afie = "sw";
				else if (avar->instrum == "PYRHELIOMETER") afie = "sw";
				else if (avar->instrum == "PYRGEOMETER") afie = "lw";
				//else if (avar->instrum == "DIV1000") afie = "e3";
				else afie = avar->afie;
				*webformat += 	"{\"sname\":\""+avar->sname+
								"\",\"stype\":\""+afie+
								"\",\"units\":\""+avar->units+
								"\",\"coef_4\":\""+ftoa(avar->coef_4)+
								"\",\"index\":\""+itoa(avar->ad_hard)+
								"\"}";
				avar++;
				if (avar != adev->signals.end()) *webformat += ","; // still a next signal
			}
			*webformat += "]";
		}
		lt++;
		if (adev->webclient == 1){ // use only when indicated in xml
			*webformat += "}";
			comma_cnt++;
		}	
	}
	*webformat += "]}\n";
}	

/*
 * Load the status xml data static in the program
 * so it is maintainable for input sources
 * ensemble of several fuctions
 * 
 */ 

std::string xml_timestr(struct timeval *tstamp){
	char abuf[30];
	tm *local;
	local = gmtime(&tstamp->tv_sec);
	sprintf(abuf,"%4d-%02d-%02d %02d:%02d:%02d",
	       local->tm_year+1900,local->tm_mon+1,local->tm_mday,
	       local->tm_hour,local->tm_min,local->tm_sec);
	return std::string(abuf);
}
/*
char *datestr(char abuf[]){
	tm *local;
	if (startt.tv_sec == 0) gettimeofday(&startt, NULL);
	local = gmtime(&startt.tv_sec);
	sprintf(abuf,"%4d-%02d-%02d",
	       local->tm_year+1900,local->tm_mon+1,local->tm_mday);
	return abuf;
}	
*/
int edit_element(pugi::xml_node &node, std::string name, std::string value){
	node.remove_child(node.child(name.c_str()));
	pugi::xml_node running_since = node.append_child(name.c_str());
	return running_since.append_child(pugi::node_pcdata).set_value(value.c_str());
}	

int xml_status_update(struct tcp_server_info *tcp, const char* node_name, struct timeval *tstamp){
	if ( proj.xml_status_file == "" ) return 0;
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(proj.xml_status_file.c_str(), pugi::parse_default | pugi::parse_comments);  // load the xml project file
	if (result == 0){
		dbgstat_printf("FAILED PARSING STATUS.XML\n");
		return result;
	}
	pugi::xml_node turbulence = doc.document_element();  // get the root element, contains cdf main attributes
	pugi::xml_node node = turbulence.child(node_name);	 // get the node in which to update
	edit_element(node, std::string(tcp->src+"_seconds_curr"), lutoa(tstamp->tv_sec));
	edit_element(node, std::string(tcp->src+"_num_records"), lutoa(tcp->fcnt));
	if (tcp->fcnt == 0){
		edit_element(node, std::string(tcp->src+"_running_since"), xml_timestr(tstamp));
		edit_element(node, std::string(tcp->src+"_seconds_start"), lutoa(tstamp->tv_sec));
	}
	int sresult = doc.save_file(proj.xml_status_file.c_str());
	if (sresult == 0){
		dbgstat_printf("FAILED SAVING STATUS.XML\n");
	}
	return sresult;
}	

int status_xml(const char* filename){
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename, pugi::parse_default | pugi::parse_comments);  // load the xml project file
	if (result == 0){
		dbgstat_printf("FAILED PARSING STATUS.XML\n");
	}

	pugi::xml_node turbulence = doc.document_element();  // get the root element, contains cdf main attributes
	pugi::xml_node common = turbulence.child("common");
	pugi::xml_node R3 = turbulence.child("R3");
	pugi::xml_node LI = turbulence.child("LI7500");

	edit_element(R3, "r3_running_since", std::string("2018-02-09 07:07:07"));
	edit_element(R3, "r3_num_records", std::string("0"));
	edit_element(LI, "li_running_since", std::string("2018-02-09 07:07:07"));
	edit_element(LI, "li_num_records", std::string("0"));
	edit_element(common, "date", std::string("2018-02-09"));

	int sresult = doc.save_file(filename);
	if (sresult == 0){
		dbgstat_printf("FAILED SAVING STATUS.XML\n");
	}
}	

/* 
 * 
 * Load and initialize project parameters from a xml formatted ascii file.
 * Several structures all filled with values:
 * 
 * - Common project parameters
 * - Device parameters, devices are scanned using tcp socket (IP, PORT or Serial intf)
 * - Signals are part of a device and describe how to process a device message 
 * 
 * 
 */ 

int read_nc_xml(const char* filename){
	std::string afie;
	tcp_server_info* p_tcp_info;
	pugi::xml_document doc; // create the pugi xml class 
	pugi::xml_parse_result result = doc.load_file(filename);  // load the xml project file
	if (result == 0){
#ifdef DEBUG_ON		
		std::cout 	<< "Load result: " << result.description()  
					<< "\nError offset: " << result.offset << "\n";
#endif
		return result;
	}	
	pugi::xml_node project = doc.document_element();  // get the root element, contains cdf main attributes
	pugi::xml_node devices = project.child("devices");

	// project main properties
	proj.name = project.child_value("pname");
	proj.longname = project.child_value("longname");
	proj.shortname = project.child_value("shortname");  // one project name is embrella for more sub projects
	proj.localpath = project.child_value("localpath");
	proj.location = project.child_value("location");
	proj.affiliation = project.child_value("affiliation");
	proj.longitude = atof(project.child_value("longitude"));
	proj.latitude = atof(project.child_value("latitude"));
	proj.altitude = atof(project.child_value("altitude"));
	proj.version = project.child_value("version");
	proj.format = project.child_value("format");
	proj.ftp = project.child_value("ftp");
	proj.pi = project.child_value("PI");
	proj.url = project.child_value("url");
	proj.measured_quantities = project.child_value("measured_quantities");
	proj.devs = project.child_value("devs");
	proj.xml_status_file = project.child_value("xml_status_file");
	proj.smtp = project.child_value("smtp");
	proj.receipients = project.child_value("recp");
	proj.uploaded = atoi(project.child_value("upload"));
	proj.uploaded_cnt = 0;
	proj.nodb_stat = 0;
	proj.nodb_stat = atoi(project.child_value("nodb_stat"));  // when 1 status reporting to acquit.siamstatus is inhibited
	proj.runlength = atoi(project.child_value("RunLength")); // in seconds
	if (proj.runlength >= 86400){
		proj.upload_interval = atoi(project.child_value("upload_interval"));
		if (proj.upload_interval == 0) proj.upload_interval = 600; // default 600s = 10min interval
	}
	else proj.upload_interval = proj.runlength;
	proj.flen = proj.runlength/60;  // used in modulo operation when filing
	proj.savedata = atoi(project.child_value("auto")); // will data be saved or not
	proj.linecnt = 0;  // total number of lines received beginning from start
	proj.curr_flenmin = -1;
	//proj.ftup = NULL;
	proj.fname = "";
	proj.si = 0;
	proj.savedge = 0;
	proj.make_empty_container = FALSE;
	proj.wsock = NULL;

	dbg_printf("*** PROJECT ***\n%s, %s,  %s, %s, %s, %s, %s, %s, %s\n",
			project.child_value("pname"), 
			project.child_value("location"),
			project.child_value("version"),
			project.child_value("localpath"),
			project.child_value("RunLength"),
			project.child_value("format"),
			project.child_value("style"),
			project.child_value("ftp")
	);

	// project siam devices
	int delay = 0;
	for (pugi::xml_node siam = devices.first_child(); siam; siam = siam.next_sibling()){
		std::ostringstream dim_name;
		int moxaport = atoi(siam.child_value("port"));
		if (moxaport < 966 || moxaport == 7200 || moxaport == 8000){  // port is used when disabling SIAM
			p_tcp_info = new tcp_server_info();
			p_tcp_info->fcnt = 0; // frame count initial 0 frames
			p_tcp_info->julday = -1; // used in detecting day crossing moment
			p_tcp_info->delay = delay;
			delay++;
			p_tcp_info->dayindex = -1;
			p_tcp_info->no_write = 0;
			p_tcp_info->no_write = atoi(siam.child_value("no_write"));
			p_tcp_info->nc_file = NULL; // init netcdf pointer points to nothing after read xml
			p_tcp_info->gill = NULL;  // make the gill class pointer always NULL after creation
			p_tcp_info->licor = NULL; // make the licor class pointer default NULL after creation
			p_tcp_info->comport = NULL;
			p_tcp_info->server = siam.child_value("server");
			if (p_tcp_info->server == "COM"){  // using serial interface -> create its enviroment
				p_tcp_info->comport = new comintf;
				p_tcp_info->comport->intf = siam.child_value("intf");	
				p_tcp_info->comport->control = siam.child_value("control");
				p_tcp_info->comport->parity = siam.child_value("parity");
				p_tcp_info->comport->cport = atoi(siam.child_value("cport"));	
				p_tcp_info->comport->baud = atoi(siam.child_value("baud"));	
				p_tcp_info->comport->dbits = atoi(siam.child_value("dbits"));	
				p_tcp_info->comport->sb = atoi(siam.child_value("sb"));
				dbg_printf("\n%s, %s, %s, %d, %d, %d, %d\n",
           			p_tcp_info->comport->intf.c_str(),
           			p_tcp_info->comport->control.c_str(),
           			p_tcp_info->comport->parity.c_str(),
           			p_tcp_info->comport->baud, 
           			p_tcp_info->comport->cport, 
           			p_tcp_info->comport->dbits,
           			p_tcp_info->comport->sb);
			}	
			p_tcp_info->ready = false; // will be enabled in the process loop
			p_tcp_info->ip = siam.child_value("ip");
			p_tcp_info->tcpport = atoi(siam.child_value("port"));
			p_tcp_info->tx = atoi(siam.child_value("tx"));  // must socket writefs be used
			p_tcp_info->src = siam.child_value("name");
			
			p_tcp_info->stype = siam.child_value("TS")[0];
			p_tcp_info->type = siam.child_value("type");   // used for status event reporting and websock creation

				
			if (p_tcp_info->type == "WEBSOCK")
			if (!proj.wsock) // only one websocket permitted, proj.wsock set NULL for project during proj init
				proj.wsock = new WebSocket(); // create a websock object, that is visible from every thread
			ws_printf("WEBSOCKET %p\n", proj.wsock);
			
			p_tcp_info->comment = siam.child_value("comment");   // used for status event reporting
			sscanf(siam.child_value("LC"),"%hhx",&p_tcp_info->lc);
			p_tcp_info->tcpfd = 0;
			p_tcp_info->phase = 0;
			p_tcp_info->txbuf = NULL;
			p_tcp_info->rxbuf = NULL;
			p_tcp_info->msgbuf = NULL;
			p_tcp_info->last_tstamp = 0.0;
			p_tcp_info->sync_cnt = 0;

			// dimension related stuff
			p_tcp_info->freq = atof(siam.child_value("freq"));// get the sample frequency for this device
			proj.uploaded_cnt += int(p_tcp_info->freq)*2;
			p_tcp_info->dt = atoi(siam.child_value("dt"));    // get the sample interval for this device
			p_tcp_info->uslp = int(5E5/p_tcp_info->freq);	  // value to feed onto usleep in tcp_process
			p_tcp_info->usec_div = int(1E6/p_tcp_info->freq); // used in rounding tv_usecs from timeval
			dim_name.str("");
			dim_name << "dim_" << p_tcp_info->freq << "Hz";
			//dim_name << "dim_" << p_tcp_info->src.c_str() << "_" << p_tcp_info->freq << "Hz";
			p_tcp_info->dim_name = dim_name.str();

			//!!!!!!!! ooit /p_tcp_info->dt vervangen door *p_tcp_info->freq
			if (p_tcp_info->dt > 1) p_tcp_info->dim = proj.runlength/p_tcp_info->dt; // calculate the dimension for this device
			else p_tcp_info->dim = proj.runlength*p_tcp_info->freq; // calculate the dimension for this device
			//if (p_tcp_info->dt > 1) p_tcp_info->dim++; // toevoegen  p_tcp_info->dim + 1 voor bepaalde siams

			// has this device a ASCII seperator count
			p_tcp_info->sepcnt = atoi(siam.child_value("msg_seps"));

			// is this device a potential websocket client
			p_tcp_info->webclient = atoi(siam.child_value("webclient"));
			
			// next two items are depreciated
			p_tcp_info->ad_min = atof(siam.child_value("ADS_min"));
			p_tcp_info->ad_max = atof(siam.child_value("ADS_max"));

			if (siam.child_value("Missing_value") == std::string("nan"))
				 p_tcp_info->missing_value = 0.0/0.0; 
			else p_tcp_info->missing_value = atof(siam.child_value("Missing_value")); 

			// not all messages come from siams, read from xml what kind of source is out there
			p_tcp_info->last_cindex = -1;
			p_tcp_info->fscan_msg = &scan_siam_msg; // default use a siam message to scan
			afie = siam.child_value("scanfunc");
			     if (afie == "xlas_msg") p_tcp_info->fscan_msg = &scan_xlas_msg;  // use the xlas format to scan the image
			else if (afie == "gill_msg") {
				p_tcp_info->gill = new r3();			 // create a gill class object
				p_tcp_info->fscan_msg = &scan_gill_msg;  // use gill binary output format
				p_tcp_info->gill->horz_rotation = atof(siam.child_value("horz_rotation")); // orientation of the instrument
				p_tcp_info->gill->rotor = siam.child_value("rotor");
				if (p_tcp_info->gill->rotor != ""){
					// find the rotor component when it has a name
					p_tcp_info->gill->scan_for_rotor();
				}
				p_tcp_info->gill->debug = (siam.child_value("debug") == std::string("true"));
			}	
			else if (afie == "licor_msg") {
				dbg_printf("****************CHECKING FOR LICOR*****************\n");
				p_tcp_info->licor = new tlicor_7500();	 // create a licor class object
				p_tcp_info->fscan_msg = &scan_licor_msg; // use licor binary output format
				p_tcp_info->licor->debug = (siam.child_value("debug") == std::string("true"));

				if (p_tcp_info->licor->haspipe > -1) // use the pipe for intercommunication?
					lipipe = p_tcp_info->licor->fd[1];
				else lipipe = -1;
			}	
			else if (afie == "pt_msg"){   // is a pt rotor present in this project
				p_tcp_info->rotor = new rotor();
				p_tcp_info->rotor->self = p_tcp_info;  // device pointer inside the rotor class
				p_tcp_info->fscan_msg = &scan_rotor_msg;
			}	
			gettimeofday(&p_tcp_info->tstamp, NULL);

			dbg_printf("*** DEVICE ***\n%s, %s, %s, %s, %s, %s, %s\n", 
				siam.child_value("name"),
				siam.child_value("ip"),
				siam.child_value("port"),
				siam.child_value("LC"),
				siam.child_value("TS"),
				siam.child_value("freq"),
				siam.child_value("comment")
			);
			proj.si++;

			// signals for each device, use it when acquiring the signals to netcdf
			pugi::xml_node signals = siam.child("signals");
			dbg_printf("*** SIGNALS ***\n");

			// loop the signals of this device
			for (pugi::xml_node signal = signals.first_child(); signal; signal = signal.next_sibling()){
				// fill a signal structure first
				asig.sname = signal.child_value("name"); // short name without device id
				asig.name = p_tcp_info->src+"-"+signal.child_value("name"); // combine with device name to create unique names
				asig.envname = "/home/httpd/htdocs/nport/"+proj.name+"_"+p_tcp_info->src; // used to write comma delimeted one-liner
				//setenv(asig.envname.c_str(),"-999",1);
				asig.units = signal.child_value("units");
				asig.sn = signal.child_value("sn");
				asig.instrum = signal.child_value("Instrum");
				asig.p = atoi(signal.child_value("p"));
				asig.ad_hard = atoi(signal.child_value("ADHard"));  // position in string when seen as space separated

				// next 4 items are depreciated
				asig.ad_min = atof(signal.child_value("AD_min"));
				asig.ad_max = atof(signal.child_value("AD_max"));
				asig.range_min = atof(signal.child_value("Range_min"));
				asig.range_max = atof(signal.child_value("Range_max"));
				// keep out strange values
				asig.delta_max = atof(signal.child_value("delta_max"));
				if (asig.delta_max == 0) asig.delta_max = -1;
				asig.lastvalue = -999;

				asig.is_status = atoi(signal.child_value("status"));
				asig.stat_mail = atoi(signal.child_value("mail"));

				asig.a = 1; asig.b = 0;
				/*
				if ((p_tcp_info->ad_max-p_tcp_info->ad_min) != 0){
					asig.a = (asig.range_max-asig.range_min)/(p_tcp_info->ad_max-p_tcp_info->ad_min);
					asig.b = (asig.range_min - asig.a*p_tcp_info->ad_min);
				}*/

				asig.sa = 1; asig.sb = 0;
				if ((p_tcp_info->ad_max-p_tcp_info->ad_min) != 0){  //scale to 16bit integer
					// a from y=ax+b 
					asig.sa = (p_tcp_info->ad_max - p_tcp_info->ad_min) / (asig.ad_max - asig.ad_min);
					// b from y=ax+b
					asig.sb = p_tcp_info->ad_min - asig.sa*asig.ad_min;
				}

				asig.caldate = signal.child_value("caldate");  // get the calibration date if present
				asig.k1 = atof(signal.child_value("k1"));
				asig.k2 = atof(signal.child_value("k2"));
				asig.coef_3 = atof(signal.child_value("Coef_3"));
				asig.coef_4 = atof(signal.child_value("Coef_4"));
				asig.degree = atoi(signal.child_value("Degree"));
				asig.curpos = 0;
				asig.c_status = '?';  // last status from device, this gives initial start

				// use a direct function in converting siam float, PM, TA, NI?
				asig.fie = NULL;
				afie = signal.child_value("Function");
				asig.afie = afie;
					 if ( afie == "ps")	asig.fie = &ps;  // air pressure calculation
				else if ( afie == "tx")	asig.fie = &tx;  // air temperature calculation
				else if ( afie == "py")	asig.fie = &py;  // temperature psychrometer calculation
				else if ( afie == "ex")	asig.fie = &ex;  // exponential calculation
				else if ( afie == "xs")	asig.fie = &xs;  // seconds of the XLAS
				else if ( afie == "cn")	asig.fie = &cn;  // make scintillometer cn values bigger
				else if ( afie == "sf")	asig.fie = &sf;  // make short a float
				//else if ( afie == "bf")	asig.fie = &bf;  // make byte a float
				
				p_tcp_info->signals.push_back(asig);

				dbg_printf("%s %s %s %d %d %d %d %p %s\n",
					signal.child_value("name"),
					signal.child_value("p"),
					signal.child_value("ADHard"),
					(int)asig.ad_min, (int)asig.ad_max,
					(int)asig.range_min, (int)asig.range_max, asig.fie, signal.child_value("Function"));
			}

			listofdevs.push_back(p_tcp_info);
		}
	}	
	doc.reset();
	return result;
}

void del_tcp_list(){
	//delete each struct in list
	dbg_printf("\nDELETING SOCKETS\n");
	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end())
	{
		dbg_printf("socket %s:%d %s %c %x\n",(*lt)->ip.c_str(), (*lt)->tcpport, (*lt)->src.c_str(), (*lt)->stype, (*lt)->lc);
		delete (*lt); //how to delete the "Node"?
		lt++;
	}
	dbg_printf("deleted %d device sockets\n",listofdevs.size());
	listofdevs.clear();
}	

// calculates the cindex for variables with freq <= 1
// synchronizes for time jitter 
// verder veralgemeniseren 
int get_cindex(struct tcp_server_info *tcp){
	div_t index;
	index = div(tcp->tstamp.tv_sec, 86400);// current second of the day
	index = div(index.rem, proj.runlength);	// current #file of the day, remainder holds index when dt = 1
	index = div(index.rem, tcp->dt);		// quotient = index in netcdf: remainder could be used in rounding

	int lindex = index.quot;
		if (tcp->last_cindex != -1){
		if (tcp->sync_cnt > 2) tcp->sync_cnt = 0;
		else if ( lindex == tcp->last_cindex) {lindex++; tcp->sync_cnt++;}
		else if ( lindex == (tcp->last_cindex+2)) {lindex--; tcp->sync_cnt++;}
		else tcp->sync_cnt = 0;
	}
	tcp->last_cindex = lindex;

	return lindex;
}	

int scan_siam_msg(struct tcp_server_info *tcp){
	int i;
	char *split;
	//div_t index;
	// check for stx and etx to receive a message
	tcp->len = read( tcp->tcpfd , tcp->buff_tcp, 255);
	//if (tcp->len > 200) return -1;
	for (i=0; i<tcp->len; i++){
		switch (tcp->phase){
			case 0: // waiting for STX
				if (tcp->buff_tcp[i] == STX){
					gettimeofday(&tcp->tstamp, NULL); // record time stamp
				tcp->last_used.tv_sec = tcp->tstamp.tv_sec;
				tcp->last_used.tv_usec = tcp->tstamp.tv_usec;
					
					//index = div(tcp->tstamp.tv_sec, proj.runlength);  // calculate current index from time
					//dbg_printf("\nrounded index: %d, %d", tcp->tstamp.tv_usec, tcp->cindex);            
					tcp->phase = 1;
				}	
			break;
			case 1: // filter M for cabauw SIAM and X for oper SIAM
				if (tcp->buff_tcp[i] == 'M') tcp->phase = 2;
				else if (tcp->buff_tcp[i] == 'X'){
					tcp->msgbuf->buffer[tcp->rxbuf->rptr] = tcp->buff_tcp[i];
					tcp->rxbuf->rptr++;
					tcp->phase = 2;
				}	
			break;
			case 2: // storing until ETX detected
				if (tcp->buff_tcp[i] == ETX || tcp->buff_tcp[i] == 'X'){ // skip also the second and third message repetition for MUF X-SIAM's
					tcp->msgbuf->buffer[tcp->rxbuf->rptr+1] = 0;  // make a correct c string end
					//dbg_printf("\n%d.%06d %s %s", tcp->tstamp.tv_sec , tcp->tstamp.tv_usec, tcp->msgbuf->buffer, tcp->src.c_str());
					q_item* aqi = new q_item; // new cue item to be pushed
					aqi->samples = new std::string(tcp->msgbuf->buffer);
					aqi->tstamp = new timeval(tcp->tstamp);
					aqi->cindex = get_cindex(tcp); // current index calculated from time to device pointer, only for devs with dt >= 1
					aqi->tcp = tcp;  // tcp contains constant values at this stage
					//printf("%s %d\n",tcp->msgbuf->buffer, aqi->cindex);
					cq.push(aqi);
					
					tcp->rxbuf->rptr = 0;
					tcp->msgbuf->buffer[0] = 0;
					tcp->phase = 0;	
				}
				else{
					switch (tcp->buff_tcp[i]){
						case 10: break;  // no need for LF
						case 13: break;  // no need for CR
						default: {
							tcp->msgbuf->buffer[tcp->rxbuf->rptr] = tcp->buff_tcp[i];
							tcp->rxbuf->rptr++;
						}	
					}	
				}	
			break;	
		}
	}
	return(tcp->len);
}	

int scan_xlas_msg(struct tcp_server_info *tcp){
	int i;
	char *split;
	//div_t index;
	// check for stx and etx to receive a message
	tcp->len = read( tcp->tcpfd , tcp->buff_tcp, 255);
	for (i=0; i<tcp->len; i++){
		switch (tcp->phase){
			case 0: // waiting for STX
				if (tcp->buff_tcp[i] == '.'){  // xlas message starts with a period
					gettimeofday(&tcp->tstamp, NULL); // record time stamp
				tcp->last_used.tv_sec = tcp->tstamp.tv_sec;
				tcp->last_used.tv_usec = tcp->tstamp.tv_usec;
					//index = div(tcp->tstamp.tv_sec, proj.runlength); // calculate time index
					tcp->phase = 1;
				}	
			break;
			case 1: // storing until linefeed detected
				if (tcp->buff_tcp[i] == '\n' ){ 
					tcp->msgbuf->buffer[tcp->rxbuf->rptr+1] = 0;  // make a correct c string end
					//dbg_printf("\n%d.%06d %s %s", tcp->tstamp.tv_sec , tcp->tstamp.tv_usec, tcp->msgbuf->buffer, tcp->src.c_str());

					if (tcp->rxbuf->rptr > 100){  // is status=2 record
						q_item* aqi = new q_item; // new cue item to be pushed
						aqi->samples = new std::string(tcp->msgbuf->buffer);
						aqi->tstamp = new timeval(tcp->tstamp);
						aqi->cindex = get_cindex(tcp); // current index calculated from time to device pointer, only for devs with dt >= 1
						aqi->tcp = tcp;
						cq.push(aqi);
					}
					tcp->rxbuf->rptr = 0;
					tcp->msgbuf->buffer[0] = 0;
					tcp->phase = 0;	
				}
				else{
					switch (tcp->buff_tcp[i]){
						case 13: break;  // no need for CR
						case STX: break; // no start of text
						case ETX: break; // no end of text
						default: {
							tcp->msgbuf->buffer[tcp->rxbuf->rptr] = tcp->buff_tcp[i];
							tcp->rxbuf->rptr++;
						}	
					}	
				}	
			break;	
		}
	}
	return(tcp->len);
}	

int scan_gill_msg(struct tcp_server_info *tcp){
	// hier een call naar r3.scan_buf
	return(tcp->gill->r3Scan_buf(tcp));
}	

int scan_licor_msg(struct tcp_server_info *tcp){
	// hier een call naar r3.scan_buf
	return(tcp->licor->LiScan_buf(tcp));
}	

int scan_rotor_msg(struct tcp_server_info *tcp){
	// hier een call naar r3.scan_buf
	return(tcp->rotor->scan_buf(tcp));
}	
