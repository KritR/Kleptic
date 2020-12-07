#!/bin/bash

BUILD_TYPE="Debug"

if [[ $1 == "rel" ]]; then
  echo "Building Release. ./build to build debug."
  BUILD_TYPE="Release"
else
  echo "Building Debug. ./build rel to build release."
fi

git checkout master
git add .
git commit -a -m "Commit"
git push origin master

PROJECT_DIR="$(dirname "$(readlink -f "$0")")"



mkdir -p bld/$BUILD_TYPE
cd bld/$BUILD_TYPE

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../..
cmake --build .

# mv bin/httpd ../../
