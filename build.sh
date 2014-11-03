#!/bin/sh

ScriptDir=`echo $(cd "$(dirname "$0")"; pwd)`
cd ${ScriptDir}

if [ $# -eq 0 ]; then
    BUILD_TYPE="-DCMAKE_BUILD_TYPE=Release"
else 
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
fi

which cmake > /dev/null 2>&1
if [ $? -eq 0 ];then
    echo "use cmake build all project"
    test -d cmake_build && rm -rf cmake_build
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

cd ${ScriptDir}
