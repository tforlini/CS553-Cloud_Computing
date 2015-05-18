#!/bin/bash
workers="1 2 4 8 16"
secs="1 2 4 8"
secs1=80
secs2=40
secs4=20
secs8=10
for w in $workers; do
    workdir=$w"work"
    mkdir $workdir
    cd $workdir
    for s in $secs; do
	val=$(eval echo \$secs$s)
	for i in $(seq 1 $w); do
	    for j in $(seq 1 $val); do
		echo "sleep $s" >> $s
	    done
	done
    done
    cd ..
done
	
    