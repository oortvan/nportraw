# Time Synchronisation
The basis of the time synchronization lies in
* Linux is a realtime operating system
* The system clock is used during acquisition
## Sonic anemometer Gill-R3 and Gas-Analyzer LICOR-7500RS
When calculating fluxes from turbulence measurements it is very important to know the time lack between samples from different instruments.  This time lack is considered constant, samples from instrument A and B must be time synchronized to get this constant time lack. In NPORTRAW 10Hz raw data from GILL-R3 and LI7500RS is synchronized using:
* The synchronized clock of the linux host system.
Linux is a realtime operating system and NPORTRAW runs with a high priority. The system clock is synced with either ntp or ptpd, both seem to work satisfactory
* Oversampling of GILL-R3 data with 50Hz.
The Gill-R3 runs in a non-polling mode, samples are streamed with a fixed time interval
* Extracting 10Hz GILL-R3 samples synchronized at host clock 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 moments.
From the GILL-R3 50Hz data stream samples are picked that best agree with the 0.1 .. 0.9 moments. The time info of the synced samples is used to index samples in the netcdf raw data file. 
* Synchronizing the internal clock of the LI7500RS (linux) with the host clock.
The LI7500RS runs in a non-polling mode at 10Hz, in its output message date-time information is present, where the 1/10 s info runs from 0.1 .. 0.9 equal to that where the GILL-R3 oversampled stream is synced at. The time info coming from the LI7500RS is used for indexing samples in the netcdf raw data file.
## SIAM data
A SIAM is not synchronized by an external clock and standard sample-rate intervals (SRI) are 1, 12, 30 and 60 seconds. It is assumed that the time lack between sending message by SIAM and receiving this message by NPORTRAW is small. On reception of a SIAM message a timestamp T from the system clock is generated. From SRI and T an index in the netcdf raw data file is calculated and the sample is written to this position.  
