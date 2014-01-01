#!/bin/sh

CURDIR=`pwd`
APPDIR=`dirname $0`
cd ${APPDIR}

case "$1" in 
  debug|Debug|DEBUG)
  BUILD_TYPE="-DCMAKE_BUILD_TYPE=Debug"
  shift
  ;;
  release|Release|RELEASE)
  BUILD_TYPE="-DCMAKE_BUILD_TYPE=Release"
  shift
  ;;
  *)
  BUILD_TYPE="-DCMAKE_BUILD_TYPE=Debug"
  shift
  ;;
esac

which cmake
if [ $? -eq 0 ];then
  echo "use cmake build all project"
  rm -rf cmake_build
  mkdir -p cmake_build
  cd cmake_build
  cmake ../. ${BUILD_TYPE} $1 $2 $3 $4 $5
  make 
else 
  cd build/gnu
  make clean
  if [ "X${BUILD_TYPE}" = "X-DCMAKE_BUILD_TYPE=Debug" ] ; then
      make debug
  else 
      make 
  fi
fi

cd ${CURDIR}