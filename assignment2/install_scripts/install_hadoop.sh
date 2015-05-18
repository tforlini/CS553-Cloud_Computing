#!/bin/bash
MIRROR=http://mirror.tcpdiag.net/apache/hadoop/common/
VERSION=hadoop-2.4.0

#IP_WORKER_1=$1
#IP_WORKER_2=$2
#IP_WORKER_3=$3
#IP_WORKER_4=$4
#KEY=$2

sudo apt-get update
sudo apt-get -y install openjdk-7-jdk

#download hadoop, untar, put in /usr/local
cd /home/ubuntu
wget "$MIRROR/$VERSION/$VERSION".tar.gz
tar -xzf "$VERSION".tar.gz
sudo rm "$VERSION".tar.gz

export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64/jre
export HADOOP_PREFIX=/home/ubuntu/hadoop-2.4.0

#modify hadoop-env
cd /home/ubuntu
echo "export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64/jre" >> .bashrc
echo "export HADOOP_PREFIX=/home/ubuntu/hadoop-2.4.0" >> .bashrc

sudo chmod a+x ~/.bashrc
PS1='$ '
source ~/.bashrc

echo "export HADOOP_HOME=$HADOOP_PREFIX" >> .bashrc
echo "export HADOOP_COMMON_HOME=$HADOOP_PREFIX" >> .bashrc
echo "export HADOOP_CONF_DIR=$HADOOP_PREFIX/etc/hadoop" >> .bashrc
echo "export HADOOP_HDFS_HOME=$HADOOP_PREFIX" >> .bashrc
echo "export HADOOP_MAPRED_HOME=$HADOOP_PREFIX" >> .bashrc
echo "export HADOOP_YARN_HOME=$HADOOP_PREFIX" >> .bashrc
echo "export HADOOP_OPTS=-Djava.net.preferIPv4Stack=true" >> .bashrc

sudo chmod a+x ~/.bashrc
PS1='$ '
source ~/.bashrc

cd /home/ubuntu/hadoop-2.4.0/etc/hadoop
echo "export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64/jre" >> hadoop-env.sh
echo "export HADOOP_PREFIX=/home/ubuntu/hadoop-2.4.0" >> hadoop-env.sh
echo "export HADOOP_HOME=$HADOOP_PREFIX" >> hadoop-env.sh
echo "export HADOOP_COMMON_HOME=$HADOOP_PREFIX" >> hadoop-env.sh
echo "export HADOOP_CONF_DIR=$HADOOP_PREFIX/etc/hadoop" >> hadoop-env.sh
echo "export HADOOP_HDFS_HOME=$HADOOP_PREFIX" >> hadoop-env.sh
echo "export HADOOP_MAPRED_HOME=$HADOOP_PREFIX" >> hadoop-env.sh
echo "export HADOOP_YARN_HOME=$HADOOP_PREFIX" >> hadoop-env.sh
echo "export HADOOP_OPTS=-Djava.net.preferIPv4Stack=true" >> hadoop-env.sh

sed -i 's/\(ssh\).*/\ ssh -i \/home\/ubuntu\/key_cluster.pem \$HADOOP\_SSH\_OPTS \$slave \$\"\$\{\@\/\/ \/\\\ \}\" \\/' /home/ubuntu/hadoop-2.4.0/sbin/slaves.sh

