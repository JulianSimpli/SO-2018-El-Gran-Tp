#!/bin/bash

USUARIO=utnso

IP_CPU=192.168.3.32
IP_DIEGO=192.168.3.32
IP_FM9=192.168.3.20
IP_MDJ=192.168.3.56
IP_SAFA=192.168.2.104

DIR=/home/utnso/tp-2018-2c-Nene-Malloc
DIR_CPU=$DIR/cpu/src/CPU.config
DIR_DIEGO=$DIR/dam/src/DAM.config
DIR_FM9=$DIR/fm9/src/FM9.config
DIR_MDJ=$DIR/mdj/src/MDJ.config
DIR_SAFA=$DIR/safa/src/SAFA.config

git clone https://github.com/sisoputnfrba/so-commons-library /home/utnso/so-commons-library
cd /home/utnso/so-commons-library
git pull
sudo make install
echo "Reviso que esten en la carpeta correspondiente"
ls /usr/include/commons

cd $DIR

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
: '
echo "A todos les copio las commons y los sh de pruebas"

echo "Copio SAFA"
scp pruebas_* $USUARIO@$IP_SAFA:/home/utnso/
scp -r ../so-commons-library $USUARIO@$IP_SAFA:/home/utnso/
scp -r safa $USUARIO@$IP_SAFA:/home/utnso/

ssh $USUARIO@$IP_SAFA
	cd /home/utnso/so-commons-library && 
	sudo make install &&
	mkdir $DIR &&
	move /home/utnso/safa $DIR

echo "Copio FM9"
scp pruebas_* $USUARIO@$IP_FM9:/home/utnso/
scp -r ../so-commons-library $USUARIO@$IP_FM9:/home/utnso/
scp -r fm9 $USUARIO@$IP_FM9:/home/utnso/

ssh $USUARIO@$IP_FM9
	cd /home/utnso/so-commons-library && 
	sudo make install &&
	mkdir $DIR &&
	move /home/utnso/fm9 $DIR

echo "Copio DAM"
scp pruebas_* $USUARIO@$IP_DIEGO:/home/utnso/
scp -r ../so-commons-library $USUARIO@$IP_DIEGO:/home/utnso/
scp -r dam $USUARIO@$IP_DIEGO:/home/utnso/

ssh $USUARIO@$IP_DIEGO
	cd /home/utnso/so-commons-library && 
	sudo make install &&
	mkdir $DIR &&
	move /home/utnso/dam $DIR

echo "Copio MDJ"
scp pruebas_* $USUARIO@$IP_MDJ:/home/utnso/
scp -r ../so-commons-library $USUARIO@$IP_MDJ:/home/utnso/
scp -r mdj $USUARIO@$IP_MDJ:/home/utnso/

ssh $USUARIO@$IP_MDJ
	cd /home/utnso/so-commons-library && 
	sudo make install &&
       	git clone https://github.com/sisoputnfrba/fifa-examples /home/utnso/fifa-examples &&
	mkdir $DIR &&
	move /home/utnso/mdj $DIR

echo "Copio CPUs"
scp -r cpu $USUARIO@$IP_SAFA:$DIR
scp -r cpu $USUARIO@$IP_DIEGO:$DIR
scp -r cpu $USUARIO@$IP_FM9:$DIR
	'
