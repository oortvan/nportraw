/***************************************************************************
 *            queue_thread.h
 *
 *  Fri September 30 12:33:06 2016
 *  Copyright  2016  fedora
 *  <oortvan@knmi.nl>
 ****************************************************************************/
/*
 * queue_thread.cpp
 * http://codereview.stackexchange.com/questions/41604/thread-safe-concurrent-fifo-queue-in-c
 * Copyright (C) 2016 - fedora
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <queue>
#include <pthread.h>
#include "queue_thread.h"
#include "nportnc.h"

    //concurrent_queue::concurrent_queue(){
	void concurrent_queue::init(){	
        pthread_mutex_init(&push_mutex, NULL);
        pthread_mutex_init(&pop_mutex, NULL);
        pthread_cond_init(&cond, NULL);
    }

    void concurrent_queue::push(q_item* data)
    {
        pthread_mutex_lock(&push_mutex);

        _queue_.push(data);

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&push_mutex);
    }

    void concurrent_queue::pop(q_item** popped_data)
    {
        pthread_mutex_lock(&pop_mutex);

        while (_queue_.empty() == true)
        {
            pthread_cond_wait(&cond, &pop_mutex);
        }

        *popped_data = _queue_.front();
        _queue_.pop();

        pthread_mutex_unlock(&pop_mutex);
    }


void *consumer_thread(void *arguments)
{
	div_t days, inbtw, index;  // structure that holds quotient and remainder with div function
    concurrent_queue *cq = static_cast<concurrent_queue*>(arguments);
	int upl_rem, sav_rem, testval;
	//std::string wformatting;
    while (true)
    {
		q_item* data = NULL;
        cq->pop(&data);
        if (data != NULL)
        {

			// pointer received from the producer threads!!!
			days  = div(data->tstamp->tv_sec,86400);  // get the current day time in seconds as remainder
			inbtw = div(days.rem, proj.runlength);    // use as in between result to calculate index of < 1Hz
			inbtw = div(data->tstamp->tv_sec, proj.runlength);    // use as in between result to calculate index of < 1Hz
			index = div(inbtw.rem, data->tcp->dt);	  // current index in quotient: index.quot

			//dbg_printf("\nque indexing: %05d %03d",days.rem, index.quot);

			proj.linecnt++;
			if (proj.savedata == TRUE){
				upl_rem = (data->tstamp->tv_sec % proj.upload_interval); // next upload boundary reached?
				sav_rem = (data->tstamp->tv_sec % proj.runlength); // is runlength secs boundary reached, new file needed needed?
				// must it be ftp uploaded, depends on value of project.uploaded 
				// 0: waiting; 1: inhibit just after; 2: upload disabled
				if ((upl_rem == 0) && (proj.uploaded == 0)){
					//sprintf(cq->eas_todo,"echo \"%s/%s/raw/%d\" >> /tmp/eas_todo\n",
					//        proj.ncpath, proj.version.c_str(),proj.runlength); // create todo command ftp uploading
					sprintf(cq->eas_todo,"/home/moxa/bin/append_upload %s/%s/raw/%d /tmp/eas_todo\n",
					        proj.ncpath, proj.version.c_str(), proj.runlength); // create todo command ftp uploading
					system(cq->eas_todo);// fork a command
					proj.uploaded = proj.uploaded_cnt;   // inhibit command creation, avoids duplicates to shell script
				}
				else if (proj.uploaded > 0) proj.uploaded--; // give uploading todo free after proj.uploaded_cnt decrements
				//else if ((upl_rem != 0) && (proj.uploaded == 1)) proj.uploaded = 0; // give uploading todo free for next ten minutes

				// this call is responsible for redirecting data to its proper file
				// it does this by selecting the right file pointer and assigning
				// that pointer to the device structure
				// moet last_tstamp hier worden geupdate?
				if (data->tcp->no_write == 0){
					assign_dev_ncfptr(data->tstamp, data->tcp);
					if (data->tcp->nc_file != NULL){  // write data to netcdf file
						if (data->tcp->nc_file->open == TRUE){
							nc_write_signals(data, data->cindex); // cindex is calculated in message scan process
						}
					}
				}

				// handle websocket clients
				if (proj.wsock)
				if (proj.wsock->hasclients)	
				if (data->tcp->webclient){ // send to any open websocket
					//ws_printf("%s\n",data->samples->c_str());
					// extend message with device id, and unix data
					data->samples->insert(0, data->tcp->src+" "+itoa(data->tstamp->tv_sec)+" "+itoa(data->tstamp->tv_usec)+" ");
					if (data->tcp->src == "XLAS"){  // contains ';'
						while(data->samples->find(";") != std::string::npos) {
							data->samples->replace(data->samples->find(";"),1," ");
						}
					}	
					//std::cout << data->samples->c_str() << '\n';
					proj.wsock->put_to_websock(data->samples->c_str(), -1);	// index: -1 then scan all possible web clients
				}	

				// updating the status.xml file
				testval = int(1000000/data->tcp->freq);
				if ((data->tstamp->tv_usec < testval) || (data->tcp->fcnt == 0))
				if (((data->tstamp->tv_sec % 30) == 0) || (data->tcp->fcnt == 0)){
					// update the status xml file with gill data
					xml_status_update(data->tcp, "common", data->tstamp);
				}
				data->tcp->fcnt++;  // increment this devices frame count

			}
			// Delete all so memory keeps free, works only in combination with new
			// except the tcp_server_info structure
			delete (data->samples); 
			delete (data->tstamp);
			delete (data);
        }
    }
    return 0;
}
// Next are the variables used in the consumer approach, are externals for other
// modules
concurrent_queue cq;
pthread_t consumer;
