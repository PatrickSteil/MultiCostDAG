#!/bin/bash

rm -rf build
rm -rf build-debug

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j

cd ..

mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
