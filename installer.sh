#!/user/bin/bash
BUILD_DIR="$1"
BINARY_CREATOR="$2"
WINDEPLOY="$3"
FROM_RPGRAM=$4
TO_PROGRAM=$5
mkdir -p install
echo "Gathering build files"
cd install
rm -rf config packages
cp -ruf ../packages packages
cp -ruf ../config config
cp -uf ../Installer.ico config/Installer.ico
cp -uf ../Background.png config/Background.png
cp -uf $BUILD_DIR/${FROM_PROG}.exe packages/com.vendor.product/data/${TO_PROG}.exe
cp -uf $BUILD_DIR/Documentation.html packages/com.vendor.product/data
cp -ruf $BUILD_DIR/Documentation packages/com.vendor.product/data/Documentation
# copy scripts into scripts sub directory of data dir
cd packages/com.vendor.product/data
echo "Fetching libraries"
$WINDEPLOY --no-translations $TO_PROG.exe
echo "Building installer"
cd ../../..
$BINARY_CREATOR -c config/config.xml -p packages --offline-only ${TO_PROG}Installer.exe
