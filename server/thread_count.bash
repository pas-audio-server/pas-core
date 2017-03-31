#!/bin/bash 
COUNTER=0
while [  $COUNTER -lt 100 ]; do
	ps -eo nlwp | tail -n +2 | awk '{ num_threads += $1 } END { print num_threads }'
	sleep 0.25
    let COUNTER=COUNTER+1 
done
