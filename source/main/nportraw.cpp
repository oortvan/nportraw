
/*
 * This is a TCP threading application that scans NPORT COMM ports,
 * a consumer will process complete device strings,
 * optional saving to netcdf file (auto node in xml),
 * all app parameters are loaded from a project xml source (see xml example)
 *
 * done:
 * app argument to create the next nc file, enabling quick file switch
 * prevents data loss when a large runlength (> 43200s) is used
 *
 * History:
 * Date		    Author			    Comment
 * 21-09-2016	Cor van Oort		KNMI
 */

#define		rotorstatus 0  // only when a rotor is present in EB field app
#define		withmysql 0

#include	"nport.h"
#include	"nporttcp.h"
#include	"nportnc.h"
#include	"queue_thread.h"

#ifdef rotorstatus == 1
//#include	"statusthread.h"
#endif

#ifdef withmysql == 1
//#include	"mssql.h"
#endif

int priority=-19; // -20 is highest priority and 19 lowest, higher value nice for other processes
id_t pid;

int	main(int argc, char *argv[])
{
	pthread_t       tid2;
	//if (opendb() == 1);

	pid = getpid();
/*	int rc = setpriority(PRIO_PROCESS, pid, priority);
	if (rc != 0){
		tolog("ERROR setpriority\0", LOG_ERR);
		return -1;
	}
*/	
	if (argc == 1){
		dbg_printf("a project xml argument is needed!\n");
		return -1;
	}

	// get all the project parameters from xml file
	if (read_nc_xml(argv[1]) == 0){
		dbg_printf("could not open project %s\n",argv[1]);
		return -1;
	}

	//if (proj.xml_status_file != "")
	//	status_xml(proj.xml_status_file.c_str());

	init_nc_info();  // initialize the netcdf file pointing

	// create an empty skeleton for the next netcdf file,
	// this saves startup time during next run
	// must be executed in a crontask some time before the next run is started
	// assumes xml project parameters are already loaded
	if (argc > 2){
		if (strstr(argv[2],"-n") != NULL)
		  return create_next_nc_file(argv[2]);
	}

	dbg_printf("found %d devices\n",proj.si);
	sleep(1);

	if (argc > 2)
	if (strstr(argv[2],"-r") != NULL)
		daemon(0,0);

/*
	if (argc > 2)
	if (strstr(argv[2],"-r") != NULL){
		// daemonize this process, temporarely out commented
		pid_t pid, sid;
		// Fork the Parent Process
		pid = fork();
		if (pid < 0) { exit(EXIT_FAILURE); }
		if (pid > 0) { exit(EXIT_SUCCESS); }
		umask(0);
		// Create a new Signature Id for our child
		sid = setsid();
		if (sid < 0) { exit(EXIT_FAILURE); }
		if ((chdir("/")) < 0) {
			    exit(EXIT_FAILURE);
		}
		// Close Standard File Descriptors
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		// end of daemonizing
	}
*/
	// Create the consumer qeue thread cq
	cq.init();
	// Create a semaphore for rotor global vars
	pthread_mutex_init(&rot_cv, 0);

//	if (pthread_create(&tid2,NULL,pwriter,NULL) < 0)
//		tolog("ERROR CREATING PIPE THREAD\n",LOG_ERR);

	if ( pthread_create(&consumer, NULL, consumer_thread,  &cq) < 0 ) {
			dbg_printf("consumer Pthread create fail !\n");
			tolog("Error starting consumer",LOG_ERR);
			return -1;
	}

#ifdef rotorstatus == 1
//	if ( pthread_create(&status_test,NULL,dio_status_test, NULL) < 0 ) {
//			dbg_printf("status Pthread create fail !\n");
//			tolog("Error starting status thread",LOG_ERR);
//			return -1;
//	}
#endif

	void		*ret;
	struct tcp_server_info	*tcp;
	tolog("Starting the program !",LOG_INFO);
	if ( tcp_init_list() < 0 ){
		tolog("Error starting the program",LOG_ERR);
		return -1;
	}

	tolog("TCP initialize OK",LOG_INFO);

//	pthread_join(tid2, &ret);
	pthread_join(consumer, &ret);
#ifdef rotorstatus == 1
//	pthread_join(status_test, &ret);
#endif

	std::list<tcp_server_info*>::iterator lt = listofdevs.begin();
	while( lt != listofdevs.end()){
		tcp = (*lt); // get device structure from list
		pthread_join(tcp->pthd, &ret);
		lt++;
	} // end if select

	return 0;
}
