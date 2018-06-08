/***************************************************************************
 *            nporttcp.h
 *
 *  Fri September 23 12:02:35 2016
 *  Copyright  2016  fedora
 *  <user@host>
 ****************************************************************************/
/*
 * Copyright (C) 2016  Cor van Oort, KNMI
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
#ifndef NPORTTCP_H
#define NPORTTCP_H


void data_process(struct tcp_server_info *tcp);
void *tcp_thread(void *ptr);
int	tcp_init_list(void);
void *pwriter(void *arguments);  // used for piping data to sensor
#endif
