#!/bin/bash
cat $*| sed 's/^[[:blank:]]*//;s/[[:blank:]]*$//' | awk -F " " '{arr[$2] += $1} END {for (i in arr) { print arr[i], i}}' | sort -nr | less