#!/bin/bash
cd ../
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
sudo make install
echo "Reviso que esten en la carpeta correspondiente"
ls /usr/include/commons
echo "Descargo el file system de ejemplo"
cd ../
git clone https://github.com/sisoputnfrba/fifa-examples