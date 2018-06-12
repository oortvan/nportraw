Scans the upload todo file for new actions. Tries to execute the action, if succesfull the actie info in the todo file will be deleted. Else the action info is kept.
```
#!/bin/bash
# $1 

function go_upload {
	FTPLOG=/tmp/easlog

	# next step is to ftp the compressed file to the remote backup server
	# the command line is extracted to the command and command variables
	# items[0] is the command, items[1] is the file with a full path from root
	items="$1"
	folders=(${items//\// })
	# presumed data path and indexes of folders array
	# /home/moxa/data/CABSURF/2018/05/22/cabsurf_20180522_1140.cdf/V1.0/raw/600
	# /home/moxa/data/[project]/[yyyy]/[MM]/[dd]/[project_yyyyMMdd_hhmm.cdf]/[version]/raw
 	# 0    1   2    3         4      5     6   7                            8        9
	file_items=(${folders[7]//_/ })
	# [project_yyyyMMdd_hhmm.cdf]
	#  0       1        2
	filename="${file_items[0]}_${file_items[1]}_*.cdf"

	project="$(echo ${folders[3]} | awk '{print toupper($0)}')"
	loc_dir="/${folders[0]}/${folders[1]}/${folders[2]}/${folders[3]}/${folders[4]}/${folders[5]}/${folders[6]}"
	nfs_path="/home/data/${project}/${folders[4]}/${folders[5]}/${folders[6]}"

	if [ ! -e "/home/data" ] # project folder present
	then
		mkdir /home/data
		mount -t nfs servername:/knmi_acq /home/data
	fi
	# nfs share $nfs_path folder checking
	if [ ! -e "/home/data/${project}" ] # project folder present
	then
		mkdir "/home/data/${project}"
	fi
	if [ ! -e "/home/data/${project}/${folders[4]}" ] # year folder present
	then
		mkdir "/home/data/${project}/${folders[4]}"
	fi
	if [ ! -e "/home/data/${project}/${folders[4]}/${folders[5]}" ] # month folder present
	then
		mkdir "/home/data/${project}/${folders[4]}/${folders[5]}"
	fi
	if [ ! -e "/home/data/${project}/${folders[4]}/${folders[5]}/${folders[6]}" ] # day folder present
	then
		mkdir "/home/data/${project}/${folders[4]}/${folders[5]}/${folders[6]}"
	fi

	echo "uploading ${folders[7]}" >> $FTPLOG
	echo "from local path ${loc_dir}" >> $FTPLOG
	echo "to remote path ${nfs_path}" >> $FTPLOG

	cd $loc_dir

	if [ -e "${nfs_path}" ] 
	then
		# -k keep original file, -1 fastest compression, -f force overwrite existing .gz
		gzip -k -1 -f "${folders[7]}"
		if [ -f "${folders[7]}.gz" ]
		then
			cp "${folders[7]}.gz" "${nfs_path}/${folders[7]}.gz.bu"
			echo `date` "copy/move to share:" >> $FTPLOG
			echo "${loc_dir}/${folders[7]}" >> $FTPLOG
			mv "${nfs_path}/${folders[7]}.gz.bu" "${nfs_path}/${folders[7]}.gz"
			echo `date` "copy done" >> $FTPLOG
		fi
	fi

	# check if the ftp upload was a success
	# by looking for the file in the nfs share path
	# make it visible in the FTPLOG file
	if [ -f "${nfs_path}/${folders[7]}.gz" ]
	then
		echo "backup OK" >> $FTPLOG
		return 1
	else
		echo "backup error" >> $FTPLOG
		return 0
	fi

}

# give nportraw tme to startup next file
sleep 10
# get all the enviroment needed by sourcing
source /etc/profile
FTP_SUCCESS_MSG="226 Transfer complete."
FTPLOG=/tmp/easlog
todo=/tmp/eas_todo
remtodo=/tmp/rem_todo # remembers unsuccesfull uploads
turb_upl=/tmp/tur_upl
rem=0

# leave this shell when busy
if [ -f $turb_upl ];then
	exit 1 
fi	

touch $remtodo # start with empty remtodo file
touch $turb_upl

exec 4< $todo
while read LINE <&4; do
	echo $(/bin/date +%m-%d-%Y" "%T)" - todo scan" > $FTPLOG
	echo $LINE >> $FTPLOG
	go_upload ${LINE}
	if [ $? == 0 ];then  # upload unsuccesfull
		echo $LINE >> $remtodo
		rem=1
	fi
	# remove the first line (active command) from gz_todo
	sed -i 1d $todo
	echo "UPLOAD READY - $LINE"
done

if [ $rem -eq 1 ];then  # reinstall the unsuccesfull uploads in todo
	echo "reinstalled unsuccesfull upload items"
	touch $todo
	mv -f $remtodo $todo
	touch $remtodo
fi	

rm $turb_upl

#exit

# delete files older than 98 days, $turbpath and $del_age found in /etc/profile
# be aware, always check that cd command was succesfull
echo $(/bin/date +%m-%d-%Y" "%T)" - Purge files older than "$del_age" days" >> $FTPLOG  
cd /home/moxa/data/${folders[3]}
if [ "$?" = "0" ] && [[ $del_age -gt 99 ]]; then
	echo "Changed directory succesfull" >> $FTPLOG
# first remove files older than $del_age days
	find . -type f -mtime $del_age | xargs rm
# second remove folders that are certainly empty: $del_age+31 days
	/usr/local/bin/getempty | xargs rm -rf
exit 0	
else
	echo "Cannot change directory, or $del_afe not found!" >> $FTPLOG
	echo $(/bin/date +%m-%d-%Y" "%T)" - READY" >> $FTPLOG  
	exit 1
fi
```
