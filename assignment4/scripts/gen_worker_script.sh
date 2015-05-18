#!/bin/bash
usage="Usage: $0 <killtime>"
if [ "$#" -ne 1 ]; then
    echo "[ERROR] - Illegal number of arguments."
    echo $usage
    exit 1
fi
credentials=$(cat ~/.aws/credentials)
config=$(cat ~/.aws/config)
killtime=$1
echo "#!/bin/bash"
echo "mkdir /root/.aws"
echo "echo \"$credentials\" > /root/.aws/credentials"
echo "echo \"$config\" > /root/.aws/config"
echo "echo Worker script is launching... > /var/log/sqs_worker.log"
echo "ant worker -f /home/ec2-user/cs553-cloudcomputing-2014/assignment4/build.xml -Darg0=-i -Darg1=$killtime &>> /var/log/sqs_worker.log &"