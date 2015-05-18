#!/bin/bash
usage="Usage: $0 <count> <user_data_script>"
if [ "$#" -ne 2 ]; then
    echo "[ERROR] - Illegal number of arguments."
    echo $usage
    exit 1
fi
count=$1
worker_script=$2
ami_id=$(curl -s http://169.254.169.254/latest/meta-data/ami-id)
security_groups=$(curl -s http://169.254.169.254/latest/meta-data/security-groups)
tmp=$(curl -s http://169.254.169.254/latest/meta-data/public-keys/)
public_key=$(echo $tmp | cut -f 2 -d "=")
aws ec2 run-instances --image-id $ami_id --count $count --instance-type t2.micro --key-name pa4sqs --security-groups $security_groups --user-data file://$worker_script