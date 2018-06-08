# NETCDF USE
A device (sensor) has a sample frequency, this frequency determines the dimension needed to store its raw data for a certain runlength. Durng the initialization cycle the sample freq of each sensor is stored in a device structure. When sensor rawdata becomes available the system checks if the current storage runfile is present, if not it will create an empty container and opens it for writing. In an empty container all the variable data arrays are present for the complete runlength of all variables and contain a NAN. PE sample every 12s and runlength is 600 gives 600/12 = 50 elements for the array. This means that the file size is fixed. After starting acquisition at a certain moment in time the samples of variables will be written to a fixed index determined by the variable sample frequency and the system clock.   


## example netcdf content

### dimensions
```
dim_0.08333Hz	length = 7200; (index)
dim_0.083333Hz	length = 7200; (index)
dim_1Hz	length = 86400; (index)
dim_0.0166667Hz	length = 1440; (index)
```
### global attributes
```
EXPERIMENT	CABSURF
SOURCE	EAS
DATE	1528416000
STOP	1528502400
RUN	1
measured_quantities	datetime,air-soil-temperature,groundwaterlevel,soilfluxes,scintillo
instruments	psychrometer, pyrometer, fluxmeter, temp, precipitation
device_0_NX_SIAM	engelse put
device_1_CAB-P	psychrometer at 1, 2 and 3 meter above ground
device_2_CAB-P	bodem temperatuur
device_3_CAB-P	bodem temperatuur
device_4_CAB-P	bodem temperatuur
device_5_CAB-P	bodem temperatuur
device_6_CAB-F	bodemwarmteflux
device_7_CAB-F	bodemwarmteflux
device_8_CAB-F	bodemwarmteflux
device_9_CAB-H	infrared heiman
device_10_CAB-H	infrared heiman
device_11_CAB-H	infrared heiman
device_12_CAB-M	bodemvocht
device_13_CAB-M	bodemvocht
device_14_CAB-S	grondwater
device_15_MKII	scintillometer
location	CABAUW - CESAR observatory, the Netherlands
longitude	4.92619
latitude	51.97027
altitude	1.8
runlength	86400
affiliation	KNMI - Royal Netherlands Meteorological Institute
pi	John Doe(john.doe@doe.do)
url	http://projects.knmi.nl/cabauw/insitu/index2.htm
```
### variables
```
RAINE-TIME_S
RAINE-NI_LCST
RAINE-NI
RAINE-NI_60S
   float RAINE-NI_60S(dim_0.083333Hz=7200);
     :RDATE = 1528416000; // int
     :START = 1528416000; // int
     :STOP = 1528502400; // int
     :instrum = "0";
     :sn = "";
     :Unit = "Enot/1000";
     :caldate = "";
     :CURPOS = 0; // int
     :A = 1.0f; // float
     :B = 0.0f; // float
     :C = 0.0f; // float
     :D = 0.0f; // float
     :k1 = 0.0f; // float
     :k2 = 0.0f; // float
     :dt = 12.000048f; // float
     :_FillValue = -99999.0f; // float
RAINE-NI_MAX
RAINE-NI_MIN
RAINE-NI_600S
RAINE-NI_STD
RAINE-ND_LCST
RAINE-ND_12S
RAINE-ND_60S
RAINE-ND_600S
PSY_EB-TIME_S
PSY_EB-LCST
PSY_EB-D1m
   float PSY_EB-D1m(dim_1Hz=86400);
     :RDATE = 1528416000; // int
     :START = 1528416000; // int
     :STOP = 1528502400; // int
     :instrum = "0";
     :sn = "";
     :Unit = "°C";
     :caldate = "";
     :CURPOS = 0; // int
     :A = 1.0f; // float
     :B = 0.0f; // float
     :C = 0.0f; // float
     :D = 0.0f; // float
     :k1 = 0.0f; // float
     :k2 = 0.0f; // float
     :dt = 1.0f; // float
     :_FillValue = -99999.0f; // float
PSY_EB-N1m
PSY_EB-D2m
PSY_EB-N2m
PSY_EB-D4m
PSY_EB-N4m
CABT1-TIME_S
CABT1-LCST
CABT1-TSA00
CABT1-TSA02
CABT1-TSA04
CABT1-TSA06
CABT1-TSA08
CABT1-TSA12
CABT2-TIME_S
CABT2-LCST
CABT2-TSA20
CABT2-TSA30
CABT2-TSA50
CABT3-TIME_S
CABT3-LCST
CABT3-TSB00
CABT3-TSB02
CABT3-TSB04
CABT3-TSB06
CABT3-TSB08
CABT3-TSB12
CABT4-TIME_S
CABT4-LCST
CABT4-TSB20
CABT4-TSB30
CABT4-TSB50
CABSFZZ-TIME_S
CABSFZZ-LCST
CABSFZZ-SFZZ05_FL
CABSFZZ-SFZZ05_EF
CABSFZZ-SFZZ05_ES
CABSFZZ-SFZZ10_FL
CABSFZZ-SFZZ10_EF
CABSFZZ-SFZZ10_ES
CABSFNW-TIME_S
CABSFNW-LCST
CABSFNW-SFNW05_FL
CABSFNW-SFNW05_EF
CABSFNW-SFNW05_ES
CABSFNW-SFNW10_FL
CABSFNW-SFNW10_EF
CABSFNW-SFNW10_ES
CABSFNO-TIME_S
CABSFNO-LCST
CABSFNO-SFNO05_FL
CABSFNO-SFNO05_EF
CABSFNO-SFNO05_ES
CABSFNO-SFNO10_FL
CABSFNO-SFNO10_EF
CABSFNO-SFNO10_ES
TIRRSS-TIME_S
TIRRSS-LCST
TIRRSS-TIRDA
TIRRSS-THUDA
TIR200-TIME_S
TIR200-LCST
TIR200-TIRUH
TIR200-THUUH
TIREBF-TIME_S
TIREBF-LCST
TIREBF-TIREB
TIREBF-THUEB
CABBVL-TIME_S
CABBVL-D45_STAT
CABBVL-D45_SN
CABBVL-D45_MOIST
CABBVL-D45_TEMP
CABBVL-D45_EC
CABBVL-D190_STAT
CABBVL-D190_SN
CABBVL-D190_MOIST
CABBVL-D190_TEMP
CABBVL-D190_EC
CABBVL-D320_STAT
CABBVL-D320_SN
CABBVL-D320_MOIST
CABBVL-D320_TEMP
CABBVL-D320EC
CABBVL-D400_STAT
CABBVL-D400_SN
CABBVL-D400_MOIST
CABBVL-D400_TEMP
CABBVL-D400_EC
CABBVL-D560_STAT
CABBVL-D560_SN
CABBVL-D560_MOIST
CABBVL-D560_TEMP
CABBVL-D560_EC
CABBVR-TIME_S
CABBVR-D45_STAT
CABBVR-D45_SN
CABBVR-D45_MOIST
CABBVR-D45_TEMP
CABBVR-D45_EC
CABBVR-D200_STAT
CABBVR-D200_SN
CABBVR-D200_MOIST
CABBVR-D200_TEMP
CABBVR-D200_EC
CABBVR-D340_STAT
CABBVR-D340_SN
CABBVR-D340_MOIST
CABBVR-D340_TEMP
CABBVR-D340EC
CABBVR-D410_STAT
CABBVR-D410_SN
CABBVR-D410_MOIST
CABBVR-D410_TEMP
CABBVR-D410_EC
CABBVR-D570_STAT
CABBVR-D570_SN
CABBVR-D570_MOIST
CABBVR-D570_TEMP
CABBVR-D570_EC
CABGW0-TIME_S
CABGW0-W2_STAT
CABGW0-W2_TEMP
CABGW0-W2_P
CABGW0-W1_STAT
CABGW0-W1_TEMP
CABGW0-W1_P
CABGW0-SL_STAT
CABGW0-SL_TEMP
CABGW0-SL_P
CABGW0-WS_STAT
CABGW0-WS_TEMP
CABGW0-WS_P
   float CABGW0-WS_P(dim_0.0166667Hz=1440);
     :RDATE = 1528416000; // int
     :START = 1528416000; // int
     :STOP = 1528502400; // int
     :instrum = "0";
     :sn = "434047";
     :Unit = "Pa";
     :caldate = "";
     :CURPOS = 0; // int
     :A = 1.0f; // float
     :B = 0.0f; // float
     :C = 0.0f; // float
     :D = 0.0f; // float
     :k1 = 0.0f; // float
     :k2 = 0.0f; // float
     :dt = 59.999996f; // float
     :_FillValue = -99999.0f; // float
CABGW0-WN_STAT
CABGW0-WN_TEMP
CABGW0-WN_P
CABGW1-TIME_S
CABGW1-EB_STAT
CABGW1-EB_TEMP
CABGW1-EB_P
XLAS-TIME_S
XLAS-STATF
XLAS-GPSSECS
XLAS-DMODV
XLAS-SDMODV
XLAS-CN2
XLAS-CN2DEV
XLAS-CN2MAX
XLAS-CN2MIN
```
