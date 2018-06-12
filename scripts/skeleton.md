When NPORTRAW is running it will create the next netcdf file, so it is available when the current run has ended. 
```
#!/bin/bash
source /etc/profile
aprc=$(pidof nportraw)
if [ "$aprc" == "" ]
then
   sleep 1
   # when always running needed uncomment next line
   #/home/moxa/bin/nportraw /etc/projectname.xml -r
else
   /home/moxa/bin/nportraw /etc/projectname.xml -n
fi

```
