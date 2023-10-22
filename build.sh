#!/usr/bin/env bash

NAME=JV8
V8_SRC_ROOT_DEFAULT=./support/v8
NUM_CPUS=1
PLATFORM_VERSION=android-8
ANT_TARGET=debug
ARCHITECTURES="arm,x86"
INSTALL=false
DEBUG_RELEASE=true

usage()
{
cat <<EOF
Usage: $0 options...

This script builds v8 against the Android NDK and a sample project skeleton that uses it.
Options:
  -h                  Show this help message and exit
  -s <v8_src>         The path to v8's project sourcetree's root. (default $V8_SRC_ROOT_DEFAULT)
  -a <archs>          Comma-separated build architectures.
                        Available architectures: arm, x86. (default $ARCHITECTURES)
  -b                  Create a build release (symbolicated, with GDB)
  -n <ndk_dir>        The path to the Android NDK. (default \$ANDROID_NDK_ROOT)
  -t <toolchain_dir>  The path to the Android's toolchain binaries. (default \$ANDROID_TOOLCHAIN)
  -p <platform>       The Android SDK version to support (default $PLATFORM_VERSION)
  -j <num-cpus>       The number of processors to use in building (default $NUM_CPUS)
  -i                  Install resulting example APK onto default device.
EOF
}

red='\e[31;1m'
green='\e[32;3m'

function msg() {
    echo -n -e "$green"
    echo -n \-\>\  
    tput sgr0
    echo $@ >&1
}

function error() {
    echo -n -e "$red"
    echo -n \-\> ERROR:\  
    tput sgr0
    echo $@ >&2
}

function checkForErrors() {
    ret=$?
    if [ $ret -ne 0 ]
    then
        if [ -n "$1" ]
        then
            error "$@"
        else
            error "Unexpected error. Aborting."
        fi
        exit $ret
    fi
}

while getopts "hs:ba:n:t:p:j:id" OPTION; do
  case $OPTION in
    h)
      usage
      exit
      ;;
    b)
      DEBUG_RELEASE=false
      ;;
    s)
      V8_SRC_ROOT=$OPTARG
      ;;
    a)
      ARCHITECTURES=$OPTARG
      ;;
    n)
      ANDROID_NDK_ROOT=$OPTARG
      ;;
    t)
      ANDROID_TOOLCHAIN=$OPTARG
      ;;
    j)
      NUM_CPUS=$OPTARG
      ;;
    p)
      PLATFORM_VERSION=$OPTARG
      ;;
    i)
      INSTALL=true
      ;;
    ?)
      usage
      exit
      ;;
  esac
done

if [[ -z "$V8_SRC_ROOT" ]]
then
  V8_SRC_ROOT=$V8_SRC_ROOT_DEFAULT
fi

if [[ -z "$ANDROID_NDK_ROOT" ]]
then
  msg Please set \$ANDROID_NDK_ROOT or use the -n option.
  usage
  exit
fi

if [[ -z "$ANDROID_TOOLCHAIN" ]]
then
  msg Please set \$ANDROID_TOOLCHAIN or use the -t option.
  usage
  exit
fi

msg "Building v8..."

if [ ! -d "./build/gyp" ]
then
  msg "Building GYP first"
  make dependencies -j$NUM_CPUS
fi

echo "Patching v8..."
patch -N $V8_SRC_ROOT/Makefile.android ./v8_makefile.patch

# Builds libv8_*.arm/ia32.a
# 
# $1 = <ARCH>
# $2 = <MODE>
# $3 = <FLAGS>
# $4 = <FOLDER_TO_CHECK> (will not compile if )
function build {
  
  if [ -f "support/android/libs/$2/$4/libv8_base.a" ]; then
    echo "Lib in $4 already exists. Skipping build."
    return;
  fi

  TARGET="android_$1.$2"

  pushd $V8_SRC_ROOT
  make "android_$1.clean"
  GYP_DEFINES="$3" make $TARGET -j$NUM_CPUS debuggersupport=on $3
  checkForErrors "Error building v8 (arch=$1, mode=$2, flags=$2)"
  popd
}

# Moves generated v8 libs from
# $V8_SRC_ROOT/out/$ARCH/obj.target/tools/gyp/
# to support/android/libs/
# in the appropriate subfolder
# 
# $1 = <ARCH_TYPE>
# $2 = <MODE>
# $3 = <DEST_FOLDER>
function move_v8_libs {
  # ex: support/android/libs/release/armeabi
  DEST_DIR="support/android/libs/$2/$3/"

  if [ -f "$DEST_DIR/libv8_base.a" ]; then
    echo "Lib in $3 already exists. Skipping copy."
    return
  fi

  TARGET="android_$1.$2"
  mkdir -p $DEST_DIR
  ls -l $V8_SRC_ROOT/out/$TARGET/obj.target/tools/gyp/libv8_base.$1.a
  cp -f $V8_SRC_ROOT/out/$TARGET/obj.target/tools/gyp/libv8_base.$1.a $DEST_DIR/libv8_base.a
  cp -f $V8_SRC_ROOT/out/$TARGET/obj.target/tools/gyp/libv8_nosnapshot.$1.a $DEST_DIR/libv8_nosnapshot.a
  checkForErrors "Error copying v8 static library (arch=$1, dest=$DEST_DIR)"
}

if $DEBUG_RELEASE; then
  V8_BUILD_SUFFIX="debug"
else
  V8_BUILD_SUFFIX="release"
fi

# Iterate over archs and build v8
for i in $(echo $ARCHITECTURES | tr "," "\n")
do
  if [ "$i" == "arm" ]; then

    # ARM v6
    build "arm" $V8_BUILD_SUFFIX "armv7=false" "armeabi"
    move_v8_libs "arm" $V8_BUILD_SUFFIX "armeabi"

    # ARM v7
    build "arm" $V8_BUILD_SUFFIX "armv7=true" "armeabi-v7a"
    move_v8_libs "arm" $V8_BUILD_SUFFIX "armeabi-v7a"

  elif [ "$i" == "x86" ]; then
    echo ""
    # x86
    build "ia32" $V8_BUILD_SUFFIX "" "x86"
    move_v8_libs "ia32" $V8_BUILD_SUFFIX "x86"

  fi
done

msg "Copying v8 header files..."
mkdir -p support/include
rsync -tr $V8_SRC_ROOT/include/* support/include/.
checkForErrors "Error copying v8 header files"


msg "Building NDK libraries..."
ABI=""
if $DEBUG_RELEASE; then
  NDK_DEBUG_VALUE=1
else
  NDK_DEBUG_VALUE=0
fi
for i in $(echo $ARCHITECTURES | tr "," "\n")
do
  if [ "$i" == "arm" ]; then
    ABI="$ABI armeabi armeabi-v7a"
  elif [ "$i" == "x86" ]; then
    ABI="$ABI x86"
  fi
  NDK_DEBUG=$NDK_DEBUG_VALUE $ANDROID_NDK_ROOT/ndk-build -j$NUM_CPUS V=1 "APP_ABI=$ABI"
  checkForErrors "Error building NDK libraries (ABI=$ABI)"
done

msg "Creating jv8-native.jar..."
rm -fr lib
mv libs lib
mkdir -p dist/android/libs
rm dist/android/libs/jv8-native.jar
zip -r dist/android/libs/jv8-native.jar lib
checkForErrors "Error creating jv8-native.jar"

msg "Creating jv8.jar"
rm -fr lib
ant dist
checkForErrors "Error creating jv8.jar"

msg "Libs are in dist/android/libs"
