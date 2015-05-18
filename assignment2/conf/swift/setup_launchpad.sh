#!/bin/bash
# Install python and libcloud library
sudo apt-get update -y -q
sudo apt-get install -y -q python python-pip git
sudo pip install apache-libcloud
if [ ! -d "cloud-tutorials" ]; then
        # Clone the Swift/EC2 tutorial
        git clone https://github.com/yadudoc/cloud-tutorials.git
fi
if [ ! -d "cs553-cloudcomputing-2014" ]; then
        git clone https://github.com/vlandeiro/cs553-cloudcomputing-2014.git
        chmod 640 $HOME/cs553-cloudcomputing-2014/assignment2/conf/swift/cs553_swift_node.pem
fi
cp $HOME/cs553-cloudcomputing-2014/assignment2/conf/swift/configs $HOME/cloud-tutorials/ec2/
