# NPORTRAW
Multi threaded acquisition system
## xml configuration
* project parameters
* device parameters
```
<project>
<localpath>/home/moxa/data</localpath>
<pname>CABSURF</pname>
<shortname>cabsurf</shortname>
<longname>Cabauw surface waarnemingen</longname>
<measured_quantities>datetime,air-soil-temperature,groundwaterlevel,soilfluxes,scintillo</measured_quantities>
<version>V1.0</version>
<location>CABAUW - CESAR observatory, the Netherlands</location>
<affiliation>KNMI - Royal Netherlands Meteorological Institute</affiliation>
<longitude>4.92619</longitude>
<latitude>51.97027</latitude>
<altitude>1.8</altitude>
<PI>John Doe(doe@doe.do)</PI>
<url>http://projects.knmi.nl/cabauw/insitu/index2.htm</url>
<RunLength>86400</RunLength>
<upload_interval>600</upload_interval>
<format>cdf</format>
<style>archived</style>
<auto>1</auto>
<upload>0</upload>
<ftp>knmi-cbmb-w01p.knmi.nl</ftp>
<smtp>smtp.knmi.nl</smtp>
<recp>johndoe@doe.do</recp>
<devs>psychrometer, pyrometer, fluxmeter, temp, precipitation</devs>
<xml_status_file>/var/www/html/status.xml</xml_status_file>
<nodb_stat>1</nodb_stat>

<devices>

<SIAM>
<name>WEBSOCK</name>
<server>IP</server>
<ip>0.0.0.0</ip>
<port>667</port>
<tx>0</tx>
<TS>X</TS>
<LC>0x00</LC>
<Snumber>7</Snumber>
<type>WEBSOCK</type>
<cport>COM25</cport>
<baud>1200</baud>
<parity>Even</parity>
<dbits>7</dbits>
<sb>1</sb>
<freq>0.08333</freq>
<dt>12</dt>
<ADS_min>-32768</ADS_min>
<ADS_max>32767</ADS_max>
<Missing_value>-999</Missing_value>
<comment>enable websocket server capabillity</comment>
<altitude>1.8</altitude>
<no_write>1</no_write>
<signals>
<signal><p>0</p><name>TIME_S</name><ADHard>0</ADHard><AD_Mode>T</AD_Mode><units>s</units><AD_min>0</AD_min><AD_max>86400</AD_max><Range_min>0</Range_min><Range_max>86400</Range_max><Degree>1</Degree><Instrum>0</Instrum></signal>
</signals>
</SIAM>

<!--  RAIN SIAM -->
<SIAM>
<ip>0.0.0.0</ip>
<port>955</port>
<tx>0</tx>
<TS>X</TS>
<LC>0xCE</LC>
<name>RAINE</name>
<Snumber>12</Snumber>
<type>NX_SIAM</type>
<server>IP</server>
<cport>COM24</cport>
<baud>1200</baud>
<parity>Even</parity>
<dbits>7</dbits>
<sb>1</sb>
<freq>0.083333</freq>
<dt>12</dt>
<ADS_min>-32768</ADS_min>
<ADS_max>32767</ADS_max>
<Missing_value>-999</Missing_value>
<comment>engelse put</comment>
<Missing_value>-999999</Missing_value>
<altitude>0</altitude>
<no_write>0</no_write>
<webclient>0</webclient>
<!--
example
X44 NI16ND3FSH68SP91 NI 0 2 0000 0000 3900 0000 2240 3133 00  ND 0 2 0000 0000 //// //// 0288 //// 00  SH h 3 0000 0000 0000 0000 0000 0000 00  SP h 3 0094 0084 0144 0073 0093 0011 00
mark the 2 spaces between parms
-->
<signals>
<signal> <p>0</p> <name>TIME_S</name> <ADHard>0</ADHard> <AD_Mode>T</AD_Mode> <units>s</units> <AD_min>0</AD_min> <AD_max>86400</AD_max> <Range_min>0</Range_min> <Range_max>86400</Range_max> <Degree>1</Degree> <Instrum>0</Instrum></signal>
<signal> <p>1</p> <name>NI_LCST</name> <status>1</status> <mail>1</mail> <ADHard>3</ADHard> <AD_Mode>NI</AD_Mode> <units>no</units> <AD_min>0</AD_min> <AD_max>65535</AD_max> <Range_min>0</Range_min> <Range_max>65535</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>2</p> <name>NI</name> <ADHard>5</ADHard> <AD_Mode>NI</AD_Mode> <units>Enot/1000</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>3</p> <name>NI_60S</name> <ADHard>6</ADHard> <AD_Mode>NI</AD_Mode> <units>Enot/1000</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function>ex</Function> <funits></funits></signal>
<signal> <p>4</p> <name>NI_MAX</name> <ADHard>7</ADHard> <AD_Mode>NI</AD_Mode> <units>Enot/1000</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>5</p> <name>NI_MIN</name> <ADHard>8</ADHard> <AD_Mode>NI</AD_Mode> <units>Enot/1000</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>6</p> <name>NI_600S</name> <ADHard>9</ADHard> <AD_Mode>NI</AD_Mode> <units>Enot/1000</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>7</p> <name>NI_STD</name> <ADHard>10</ADHard> <AD_Mode>NI</AD_Mode> <units>Enot/1000</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>8</p> <name>ND_LCST</name> <status>1</status> <mail>1</mail> <ADHard>14</ADHard> <AD_Mode>ND</AD_Mode> <units>no</units> <AD_min>0</AD_min> <AD_max>65535</AD_max> <Range_min>0</Range_min> <Range_max>65535</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>9</p> <name>ND_12S</name> <ADHard>16</ADHard> <AD_Mode>ND</AD_Mode> <units>s</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>10</p> <name>ND_60S</name> <ADHard>17</ADHard> <AD_Mode>ND</AD_Mode> <units>s</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
<signal> <p>11</p> <name>ND_600S</name> <ADHard>20</ADHard> <AD_Mode>ND</AD_Mode> <units>s</units> <AD_min>0</AD_min> <AD_max>9999</AD_max> <Range_min>0</Range_min> <Range_max>9.999</Range_max> <Degree>1</Degree> <Instrum>0</Instrum> <Coef_3></Coef_3> <Coef_4></Coef_4> <Function></Function> <funits></funits></signal>
</signals>
</SIAM>

</devices>
</project>
```

