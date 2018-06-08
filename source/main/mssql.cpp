/***************************************************************************
 *            mssql.h
 *
 *  Fri november 16 10:08:01 2016
 *  Copyright  2016  KNMI, Cor van Oort
 *  
 ****************************************************************************/

#include	"mssql.h"

/*
 * MSSQL database interfacing
 * 
 */ 

static LOGINREC              *src_login;
static DBPROCESS             *src_dbproc;

RETCODE retcode;
int seqnbr_arg = 99;
char Name[500], Desc[500];
int Value;
char *src_serv = "knmi-cbsql-w01p"; /* as listed in freetds.config */

int opendb(){
	if (dbinit() == FAIL)
		return(-1);

	if((src_dbproc = dbopen(src_login, src_serv)) == FAIL) {
		fprintf(stderr, "Could not open server %s\n", src_serv);
		return(1);
	}
}

