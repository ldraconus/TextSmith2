#!/user/bin/bash
mkdir -p install
cd install
rm -rf config packages
cd ..
cp -ruf ../../packages install/packages
cp -ruf ../../config install/config
cp -uf release/TextSmith2.exe install/packages/com.vendor.product/data/TextSmith.exe
# copy documentation into data dir as well
# copy scripts into scripts directory
cd install/packages/com.vendor.product/data
/D/ProgramFiles/Qt/6.10.1/mingw_64/bin/windeployqt --no-translations TextSmith.exe
cd ../../..
cp -uf ../../../Installer.ico config/Installer.ico
cp -uf ../../../Background.png config/Background.png
/D/ProgramFiles/Qt/Tools/QtInstallerFramework/4.10/bin/binarycreator -c config/config.xml -p packages --offline-only TextSmithInstaller.exe
