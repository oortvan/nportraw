/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * statusthread.cpp
 * Copyright (C) C. van Oort 2017 <oortvan@knmi.nl>
 * 
 */

#ifndef _STATUSTHREAD_H_
#define _STATUSTHREAD_H_

#define DINPORT 0
#define DOUTPORT 0

#include	"nport.h"
#include	<moxadevice.h>

void *dio_status_test(void *ptr);  // threaded function
//extern int rotor_maintenance;
extern pthread_t status_test;
#endif // _STATUSTHREAD_H_
