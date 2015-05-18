#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "Usage: ./launch_wordcount.sh <nshards> <dataset>"
    exit 1
fi
mkdir -p /data/inputs
mkdir -p /data/outputs
export NSHARDS=$1
export DATASET=$2
split -n l/$NSHARDS -d $DATASET /data/inputs/d_ --additional-suffix=".shard"
swift wordcount.swift -n="$NSHARDS"
