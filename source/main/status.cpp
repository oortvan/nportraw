//#include	"nport.h"
#include	<stdio.h>
#include	<stdlib.h>
//#include	<signal.h>	// keep
#include	<sys/time.h>
#include	<fcntl.h>
#include	<unistd.h>
//#include	<pthread.h>
#include	<moxadevice.h> // keep
//#include  	"statusthread.h"

#define DOUTPORT 0

//#define DEBUG
#ifdef DEBUG
#define dbg_printf(x...)	printf(x)
#else
#define dbg_printf(x...)
#endif

int main(int argc, char * argv[])
{
	int key;

	//pthread_t	dio_test;
	set_dout_state(DOUTPORT, 0);  // set the DOUT0 as high
	//pthread_create(&dio_test,NULL,dio_status_test, NULL);
	
	while( 1 ){
		usleep(300000);
		printf("input 0 of 1> ");
		scanf("%d",&key);
		if (key == 0) set_dout_state(DOUTPORT, 0);
		if (key == 1) set_dout_state(DOUTPORT, 1);
	}	
}
