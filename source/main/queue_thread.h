/***************************************************************************
 *            queue_thread.h
 *
 *  Fri September 30 12:33:06 2016
 *  Copyright  2016  fedora
 *  <user@host>
 ****************************************************************************/
/*
 * queue_thread.h
 *
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
#ifndef QUEUE_THREAD_H
#define QUEUE_THREAD_H

#include <queue>
#include "nport.h"
//#include "../websocket/WebSocket.h"


class concurrent_queue{

private:

    std::queue<q_item*> _queue_;
    pthread_mutex_t push_mutex;
    pthread_mutex_t pop_mutex;
    pthread_cond_t cond;
public:
	char eas_todo[100];
	//concurrent_queue();
	void init();
    void push(q_item* data);
    void pop(q_item** popped_data);
};

void *consumer_thread(void *arguments);

extern concurrent_queue cq;
extern pthread_t consumer;

#endif
