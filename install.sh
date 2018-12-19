#!/bin/bash

IP_CPU=192.168.1.148
IP_DIEGO=192.168.1.129
IP_FM9=192.168.1.129
IP_MDJ=192.168.1.148
IP_SAFA=192.168.1.148

DIR=/home/utnso/tp-2018-2c-Nene-Malloc
DIR_CPU=$DIR/cpu/src/CPU.config
DIR_DIEGO=$DIR/dam/src/DAM.config
DIR_FM9=$DIR/fm9/src/FM9.config
DIR_MDJ=$DIR/mdj/src/MDJ.config
DIR_SAFA=$DIR/safa/src/SAFA.config

cd ../
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
git pull
sudo make install
echo "Reviso que esten en la carpeta correspondiente"
ls /usr/include/commons
echo "Descargo el file system de ejemplo"
cd ../
git clone https://github.com/sisoputnfrba/fifa-examples

cd tp-2018-2c-Nene-Malloc

echo 'CPU.config'
sed -i "s/IP_SAFA=\([0-9\.]*\)/IP_SAFA=$IP_SAFA/" $DIR_CPU
sed -i "s/IP_FM9=\([0-9\.]*\)/IP_FM9=$IP_FM9/" $DIR_CPU
sed -i "s/IP_DIEGO=\([0-9\.]*\)/IP_DIEGO=$IP_DIEGO/" $DIR_CPU

echo 'SAFA.config'
sed -i "s/IP=\([0-9\.]*\)/IP=$IP_SAFA/" $DIR_SAFA

echo 'DAM.config'
sed -i "s/IP=\([0-9\.]*\)/IP=$IP_DIEGO/" $DIR_DIEGO
sed -i "s/IP_SAFA=\([0-9\.]*\)/IP_SAFA=$IP_SAFA/" $DIR_DIEGO
sed -i "s/IP_MDJ=\([0-9\.]*\)/IP_MDJ=$IP_MDJ/" $DIR_DIEGO
sed -i "s/IP_FM9=\([0-9\.]*\)/IP_FM9=$IP_FM9/" $DIR_DIEGO

echo 'MDJ.config'
sed -i "s/IP=\([0-9\.]*\)/IP=$IP_MDJ/" $DIR_MDJ

echo 'FM9.config'
sed -i "s/IP_FM9=\([0-9\.]*\)/IP_FM9=$IP_FM9/" $DIR_FM9

scp -R safa utnso@$IP_SAFA:/home/utnso/
scp -R dam utnso@$IP_DIEGO:/home/utnso/
scp -R cpu utnso@$IP_CPU:/home/utnso/
scp -R mdj utnso@$IP_MDJ:/home/utnso/
scp -R fm9 utnso@$IP_FM9:/home/utnso/
