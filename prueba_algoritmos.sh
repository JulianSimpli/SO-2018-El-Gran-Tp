#!/bin/bash

DIR=/home/utnso/tp-2018-2c-Nene-Malloc
DIR_CPU=$DIR/cpu/src/CPU.config
DIR_DIEGO=$DIR/dam/src/DAM.config
DIR_FM9=$DIR/fm9/src/FM9.config
DIR_MDJ=$DIR/mdj/src/MDJ.config
DIR_SAFA=$DIR/safa/src/SAFA.config

echo 'CPU.config'
sed -i "s/RETARDO=\([0-9\.]*\)/RETARDO=200/" $DIR_CPU

echo 'DIEGO.config'
sed -i "s/TRANSFER_SIZE=\([0-9\.]*\)/TRANSFER_SIZE=16/" $DIR_DIEGO

echo 'FM9.config'
sed -i "s/MODO=\([A-Z]*\)/MODO=SPA/" $DIR_FM9
sed -i "s/TAMANIO=\([0-9]*\)/TAMANIO=8192/" $DIR_FM9
sed -i "s/MAX_LINEA=\([0-9]*\)/MAX_LINEA=64/" $DIR_FM9
sed -i "s/TAM_PAGINA=\([0-9]*\)/TAM_PAGINA=128/" $DIR_FM9

echo 'MDJ.config'
sed -i "s/RETARDO=\([0-9\.]*\)/RETARDO=500/" $DIR_MDJ

echo 'SAFA.config'
sed -i "s/ALGORITMO=\([A-Z]*\)/ALGORITMO=VRR/" $DIR_SAFA
sed -i "s/QUANTUM=\([0-9]*\)/QUANTUM=2/" $DIR_SAFA
sed -i "s/MULTIPROGRAMACION=\([0-9]*\)/MULTIPROGRAMACION=2/" $DIR_SAFA
sed -i "s/RETARDO_PLANIF=\([0-9]*\)/RETARDO_PLANIF=500/" $DIR_SAFA
