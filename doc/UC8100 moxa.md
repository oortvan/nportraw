# INSTALLATIE UC8100LX DEBIAN8 jessie

## Wat algemene opmerkingen
```
1.	Om te onderzoeken of een package bestaat, of wat de naam precies is
	gebruik dan dit commando:
	apt-cache search --names-only '....'
```
## MOXA DIRECTORIES AANMAKEN
```
1.	in /home/moxa/ 
	source - bevat de software source code, 
	bin - bevat scripts en applicaties, 
	data - is nfs koppeling met knmi-cbsql-w01p/KNMI_ACQ, 
	doc - bevat documentatie
```	
## HOSTNAME
```
1.	file /etc/hostname aanpassen
	Moxa
```
## NETWERK INSTELLINGEN
```
1.	file /etc/network/interfaces aanpassen hier een voorbeeld, 
	voor CABAUW 145.23.cabsubnet.[...] gebruiken in eth1
"
	# interfaces(5) file used by ifup(8) and ifdown(8)
	# Include files from /etc/network/interfaces.d:
	source-directory /etc/network/interfaces.d
	auto eth0 eth1 lo
	iface lo inet loopback
	#iface eth0 inet dhcp
	iface eth0 inet static
			address 192.168.3.127
			network 192.168.3.0
			netmask 255.255.255.0
			broadcast 192.168.3.255

	iface eth1 inet static
		address 145.23.abc.???
		network 145.23.abc.0
		netmask 255.255.255.0
		gateway 145.23.abc.1
		broadcast 145.23.abc.255
"
```
## DNS INSTELLINGEN
```
1.	file /etc/hosts aanpassen
"
	127.0.0.1	localhost Moxa
	dns1	nameserver
	dns2	nameserver
"
2.	file /etc/resolv.conf aanpassen
"
	#nameserver 8.8.4.4
	#nameserver 8.8.8.8
	#nameserver 10.128.8.5
	nameserver dns1
	nameserver dns2
"
3.	file nsswitch.conf moet OK zijn
"
	passwd:         compat
	group:          compat
	shadow:         compat
	gshadow:        files
	hosts:          files dns
	networks:       files
	protocols:      db files
	services:       db files
	ethers:         db files
	rpc:            db files
	netgroup:       nis
"
```
## NETCDF LIBRARIES INSTALLEREN met apt-get
```
1.	apt-get install
	libnetcdf-dev  1:4.1.3-7.2  armhf        Development kit for NetCDF
	libnetcdfc++4  1:4.1.3-7.2  armhf        Interface for scientific data acc
	libnetcdfc7    1:4.1.3-7.2  armhf        Interface for scientific data acc
	libnetcdff5    1:4.1.3-7.2  armhf        Interface for scientific data acc
```
## PUGIXML LIBRARIES INSTALLEREN met apt-get
```
1.	apt-get install
	libpugixml-dev 1.4-1        armhf        Light-weight C++ XML processing l
	libpugixml1:ar 1.4-1        armhf        Light-weight C++ XML processing l
```
## INSTALLATIE LIBRARIE VOOR MSSQL met apt-get
```
1.	apt-get install
	freetds-common 0.91-6       all          configuration files for FreeTDS S
	freetds-dev    0.91-6+b1    armhf        MS SQL and Sybase client library
```
## TIJD ZAKEN
```
1.	command instellen tijdzone naar UTC
	timedatectl set-timezone Europe/London
2.	lijst met mogelijk in te stellen tijdzone's via command weergeven
	timedatectl list-timezone
3.	tijd synchronisatie mbv ntp server, via volgende commands in een script
	ntpdate dns1\dns2
	hwclock -w
4.	In crontab iedere 10 min */10 * * *  het script van 3 starten
```	
## HTTP SERVER APACHE2
```
1.	php5 installeren
	apt-get install php5-cli php5-fpm php5-mysql libapache2-mod-php5
	of misschien
	apt-get install php5 libapache2-mod-php5
2.	handmatig starten en stoppen van de APACHE2 service
	/etc/init.d/apache2 start|stop|restart
3.	php testen met /var/www/html/info.php
	<?php phpinfo(); ?>
4.	Apache deamon automatisch starten on boot.
	insserv -r apache2 // enables service
	insserv -d apache2 // disables service
5.	Path to default website
	/var/www/html/
6.	php5 deamon starten, niet helemaal duidelijk of dit nodig is
	/etc/init.d/php5-fpm start
```
[Zie ook](https://www.howtoforge.com/installing-apache2-with-php5-and-mysql-support-on-debian-wheezy) - howtoforge
## SYSTEM LOGGING MET RSYSLOG
```
1.	enabling
	sudo systemctl enable rsyslog
2.	start en stoppen
	/etc/init.d/rsyslog start|stoppen
NB.	
Nog onderzoeken hoe rsyslog precies werkt
```
## NFS FILE SHARE GEBRUIKEN
```
1.	NFS package install
	apt-get -y install nfs-common
2.	hoe de mount doen
	if [ ! -e "localpath" ] # project folder present
	then
		mkdir localpath
		mount -t nfs servername:/sharename localpath
	fi
3.	KNMI_ACQ share vanuit de servername voor de UC8100 vrijgeven
	Via RDP dit uitvoeren, zie folder properties nfs sharing 
```
## GCC en G++ COMPILERS INSTALLEREN
```
1.	install the gcc compiler
	apt-get -y install gcc
	apt-get -y install gcc-4.9
2.	install the g++ compiler
	apt-get -y install g++
	apt-get -y install g++-4.9
3.	install the make utillity
	apt-get -y install make
```

	
	
