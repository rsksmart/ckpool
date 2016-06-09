#!/bin/bash 

echo "Time,%CPU,Threads,%Mem,RSS,VSZ,Sockets"

while true; do

	process_number=$(pgrep ckpool | head -1)

	date=$(date +%Y-%m-%d:%H:%M:%S)
	stats=$(ps -p $process_number -o %cpu,nlwp,%mem,rssize,vsize --no-headers)

	sockets_in_use=$(ls -l /proc/$process_number/fd | wc -l)

	stats+=' '
	stats+=$sockets_in_use

	date+=$stats

	# replace spaces for commas and print result
	echo $date | sed 's/ /,/g'

	sleep 5s
done
