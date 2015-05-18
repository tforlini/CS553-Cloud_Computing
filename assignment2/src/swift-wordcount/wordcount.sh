#!/bin/bash
cat $1 | tr -d ",;.|'" | tr " \t\v\b" "\n" | tr -s "\n" | sort | uniq -c