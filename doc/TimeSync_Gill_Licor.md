# Time Synchronisation 
## Sonic anemometer Gill-R3 and Gas-Analuzer LICOR-7500RS

When calculating fluxes from turbulence measurements it is very important to know the time lag between samples from different instruments.  This time lag is considered constant, in order to use a constant time lag samples from instrument A and B must be time synchronized. In NPORTRAW 10Hz raw data from GILL-R3 and LI7500RS is synchronized using:
* The synchronized clock of the linux host system
* Oversampling of GILL-R3 data with 50Hz
* Extracting 10Hz GILL-R3 samples synchronized at host clock 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 moments 
* Synchronizing the internal clock of the LI7500RS (linux) with the host clock
