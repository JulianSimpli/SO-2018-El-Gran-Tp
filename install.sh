#!/bin/bash

IP_CPU=127.0.0.1
IP_DIEGO=127.0.0.1
IP_FM9=127.0.0.1
IP_MDJ=127.0.0.1
IP_SAFA=127.0.0.1

DIR=/home/utnso/tp-2018-2c-Nene-Malloc
DIR_CPU=$DIR/cpu/src/CPU.config
DIR_DIEGO=$DIR/dam/src/DAM.config
DIR_FM9=$DIR/fm9/src/FM9.config
DIR_MDJ=$DIR/mdj/src/MDJ.config
DIR_SAFA=$DIR/safa/src/SAFA.config

cd ../
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
sudo make install
echo "Reviso que esten en la carpeta correspondiente"
ls /usr/include/commons
echo "Descargo el file system de ejemplo"
cd ../
git clone https://github.com/sisoputnfrba/fifa-examples

cd tp-2018-2c-Nene-Malloc

echo 'CPU.config'
sed "s/IP_SAFA=\([0-9\.]*\)/IP_SAFA=$IP_SAFA/" $DIR_CPU
sed "s/IP_FM9=\([0-9\.]*\)/IP_FM9=$IP_FM9/" $DIR_CPU
sed "s/IP_DIEGO=\([0-9\.]*\)/IP_DIEGO=$IP_DIEGO/" $DIR_CPU

echo 'SAFA.config'
sed "s/IP=\([0-9\.]*\)/IP=$IP_SAFA/" $DIR_SAFA

echo 'DAM.config'
sed "s/IP=\([0-9\.]*\)/IP=$IP_DIEGO/" $DIR_DIEGO
sed "s/IP_SAFA=\([0-9\.]*\)/IP_SAFA=$IP_SAFA/" $DIR_DIEGO
sed "s/IP_MDJ=\([0-9\.]*\)/IP_MDJ=$IP_MDJ/" $DIR_DIEGO
sed "s/IP_FM9=\([0-9\.]*\)/IP_FM9=$IP_FM9/" $DIR_DIEGO

echo 'MDJ.config'
sed "s/IP=\([0-9\.]*\)/IP=$IP_MDJ/" $DIR_MDJ

echo 'FM9.config'
sed "s/IP_FM9=\([0-9\.]*\)/IP_FM9=$IP_FM9/" $DIR_FM9

./copiar_archivos.sh
