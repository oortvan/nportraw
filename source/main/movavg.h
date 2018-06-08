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

#ifndef MovingAverageFilter_h
#define MovingAverageFilter_h
#define MAX_DATA_POINTS 6000
//#include <vector>

class MovingAverageFilter {
public:
  //construct without coefs
  int k, n; // k stores the index of the current array read to create a circular memory through the array
  float sum;	
  MovingAverageFilter(unsigned int newDataPointsCount);
  float ddd(float u, float v);	
  float lavg(float u, float v);	
private:
  int dataPointsCount;
  float out;
  int i; // just a loop counter
  bool ringfull;	
  float values[MAX_DATA_POINTS];
  //std::vector<float> vec;  // alternative to array
};
#endif
