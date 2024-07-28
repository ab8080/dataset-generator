#!/bin/bash

rm -r build
cmake -S. -Bbuild

cd build 

make

cd ..

pip install -r requirements.txt
#
#./build/imgen $generate_config $dest $noize_config 
