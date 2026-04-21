#!/usr/bin/env bash
BUILD_DIR="$1"
BINARY_CREATOR="$2"
REPOGEN="$3"
WINDEPLOY="$4"
FROM_PROG=$5
TO_PROG=$6

mkdir -p install
echo "Gathering build files"
cd install
rm -rf  config packages
cp -ruf ../packages packages
cp -ruf ../config config
cp -uf  ../Installer.ico config/Installer.ico
cp -uf  ../Background.png config/Background.png

# Update versions
sed -e '/<!-- VERSION -->/r ../TSVersion' -e '/<!-- VERSION -->/d' config/config.xml > config/config.xml2
awk '/<Version>/{getline ver; getline; print "<Version>" ver "</Version>"; next} 1' config/config.xml2 > config/config.xml
rm config/config.xml2
sed -e '/<!-- VERSION -->/r ../TSVersion' -e '/<!-- VERSION -->/d' packages/com.vendor.product/meta/package.xml > packages/com.vendor.product/meta/package.xml2
awk '/<Version>/{getline ver; getline; print "<Version>" ver "</Version>"; next} 1' packages/com.vendor.product/meta/package.xml2 > packages/com.vendor.product/meta/package.xml
rm packages/com.vendor.product/meta/package.xml2

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
cp -ruf ../scripts packages/com.vendor.product/data/scripts   # fixed typo: ventor→vendor

cd packages/com.vendor.product/data

# windeployqt is Windows-only; Linux handles Qt libs via other means
if [[ -n "$EXE" && -n "$WINDEPLOY" ]]; then
    echo "Fetching libraries"
    cp ../../../../../vcpkg/packages/hunspell_x64-windows/bin/hunspell-1.7-0.dll .
    cp ../../../../../zlib-1.3.1/build/libzlib.dll .
    cp ../../../../../libzip/build/lib/libzip.dll .
    $WINDEPLOY --no-translations ${TO_PROG}${EXE}
else
    echo "Configuring linux post install"
    cp ../../../../TextSmithIcon.png .
    cp ../meta/installScripts.qs.linux ../meta/installScripts.qs
fi

cd ../../..
rm -rf repo
echo "Building update repository"
$REPOGEN -p packages repo
cp ../repo/Updates.xml repo/Updates.xml
sed -e '/<!-- VERSION -->/r ../TSVersion' -e '/<!-- VERSION -->/d' repo/Updates.xml > repo/Updates.xml2
sed -e '/<!-- VERSION -->/r ../TSVersion' -e '/<!-- VERSION -->/d' repo/Updates.xml2 > repo/Updates.xml3
awk '/<Version>/{getline ver; getline; print "<Version>" ver "</Version>"; next} 1' repo/Updates.xml3 > repo/Updates.xml
rm repo/Updates.xml[23]

echo "Building installer"
$BINARY_CREATOR -c config/config.xml -p packages --offline-only ${TO_PROG}Installer${INSTALLER_EXT}
