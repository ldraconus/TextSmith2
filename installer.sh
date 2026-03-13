#!/usr/bin/env bash
BUILD_DIR="$1"
BINARY_CREATOR="$2"
WINDEPLOY="$3"
FROM_PROG=$4
TO_PROG=$5

mkdir -p install
echo "Gathering build files"
cd install
rm -rf  config packages
cp -ruf ../packages packages
cp -ruf ../config config
cp -uf  ../Installer.ico config/Installer.ico
cp -uf  ../Background.png config/Background.png

# Handle platform differences
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    EXE=".exe"
    INSTALLER_EXT=".exe"
else
    EXE=""
    INSTALLER_EXT=".run"
fi

mkdir -p packages/com.vendor.product/data
cp -uf  $BUILD_DIR/${FROM_PROG}${EXE} packages/com.vendor.product/data/${TO_PROG}${EXE}
cp -uf  ../Documentation.html packages/com.vendor.product/data
cp -ruf ../Documentation packages/com.vendor.product/data/Documentation
cp -ruf ../scripts packages/com.vendor.product/scripts   # fixed typo: ventor→vendor

cd packages/com.vendor.product/data

# windeployqt is Windows-only; Linux handles Qt libs via other means
if [[ -n "$EXE" && -n "$WINDEPLOY" ]]; then
    echo "Fetching libraries"
    $WINDEPLOY --no-translations ${TO_PROG}${EXE}
fi

echo "Building installer"
cd ../../..
$BINARY_CREATOR -c config/config.xml -p packages --offline-only ${TO_PROG}Installer${INSTALLER_EXT}
