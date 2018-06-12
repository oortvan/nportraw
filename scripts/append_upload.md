NPORTRAW writes upload info to a file, this file is red by another script that will use this info to upload the run data to a backup system.
```
#!/bin/bash

# inputs
# $1 the line that must be appended to the todo file
# $2 the path/name of the todo file
# echo "/mnt/cf/KTS100/2018/01/04/KTS100_20180104_0720.cdf/V1.0/raw/600" >> /tmp/eas_todo
# this script must prevent that double upload lines are written to the upload command file

source /etc/profile
hasline=$(fgrep "${1}" ${2})

if [ "$hasline" == "" ]
then
	echo "${1}" >> ${2}
fi
```
