/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * statusthread.cpp
 * Copyright (C) C. van Oort 2017 <oortvan@knmi.nl>
 * 
 */

#include "statusthread.h"

//int rotor_maintenance;

void mail_command(std::string *mailcmd, char *say_this){
	*mailcmd = "eventmail "+proj.smtp+" "+proj.receipients+" "+proj.name+say_this+"\"";
}	

void *dio_status_test(void *ptr){
	int statei[8], lstatei[8], i=0;
	std::string mail;
	for (i=0; i<8; i++)
	get_din_state(DINPORT,&lstatei[i]);
	//dbg_printf("start state D0I = %d\n", lstatei);
	while(1) {
		get_din_state(DINPORT,&statei[0]);
		if (statei[0] != lstatei[0]){
			rotor_maintenance = statei[0];
			//dbg_printf("thread D0I: %d -> %d\n", lstatei, statei);
			lstatei[0] = statei[0];
			if (rotor_maintenance == 1)
				mail_command(&mail, "-EB-ROTOR \"rotor maintenance started");
			else if (rotor_maintenance == 0)
				mail_command(&mail, "-EB-ROTOR \"rotor maintenance ended");
			system(mail.c_str()); // fork a command
			tolog(mail.c_str(), LOG_INFO);
		}	
		sleep(1); //check every second
	} 
	pthread_exit(NULL);
}

pthread_t	status_test;
