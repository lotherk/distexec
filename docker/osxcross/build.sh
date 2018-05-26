#!/bin/bash

cd /opt/distexec
rm -rf build
git pull
mkdir build; cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/OSX.toolchain.txt ..
make
make package
mv *zip /releases/

