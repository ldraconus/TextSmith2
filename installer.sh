#!/user/bin/bash
BUILD_DIR="$1"
echo "Build dir: $BUILD_DIR"
BINARY_CREATOR="$2"
echo "Binaray creator: $BINARY_CREATOR"
WINDEPLOY="$3"
echo "Win deploy: $WINDEPLOY"
mkdir -p install
echo "Gathering build files"
cd install
rm -rf config packages
cp -ruf ../packages packages
cp -ruf ../config config
cp -uf ../Installer.ico config/Installer.ico
cp -uf ../Background.png config/Background.png
cp -uf $BUILD_DIR/TextSmith2.exe packages/com.vendor.product/data/TextSmith.exe
# copy documentation into data dir as well
# copy scripts into scripts sub directory of data dir
cd packages/com.vendor.product/data
echo "Fetching libraries"
$WINDEPLOY --no-translations TextSmith.exe
echo "Building installer"
cd ../../..
$BINARY_CREATOR -c config/config.xml -p packages --offline-only TextSmithInstaller.exe
