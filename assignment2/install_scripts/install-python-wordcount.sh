#!/bin/bash
cd
sudo apt-get update -y
sudo apt-get install git -y
sudo apt-get install python-setuptools -y
sudo easy_install docopt
git clone https://github.com/vlandeiro/cs553-cloudcomputing-2014.git