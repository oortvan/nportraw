/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * movavg.cpp
 * Copyright (C) fedora 2017 <fedora@localhost.localdomain>
 * 
movavg.cpp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * movavg.cpp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "movavg.h"
#include <math.h>

MovingAverageFilter::MovingAverageFilter(unsigned int newDataPointsCount) {
  k = 0; //initialize so that we start to write at index 0
  sum = 0;
  n = 0;
	ringfull = 0;
  if (newDataPointsCount < MAX_DATA_POINTS)
	dataPointsCount = newDataPointsCount;
  else
	dataPointsCount = MAX_DATA_POINTS;
  for (i=0; i<dataPointsCount; i++) {
    values[i] = 0; // fill the array with 0's
  }
  //vec.resize(10);
}

float MovingAverageFilter::lavg(float u, float v) {
	if (ringfull == 1) sum -= values[k];
	values[k] = ddd(u, v);
	sum += values[k];
	if (!ringfull){	
		n = k+1;
		if (k == dataPointsCount-1) ringfull = 1;
	}		
	k = (k+1) % dataPointsCount; // next data point
	return sum/n;
}

float MovingAverageFilter::ddd(float u, float v){
	return (atan2(u,v) * 57.29578);
}



