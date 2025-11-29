#!/user/bin/bash
mkdir -p install
cp -ruf ../../packages install/packages
cp -ruf ../../config install/config
cp -uf release/TextSmith2.exe install/packages/com.vender.product/data/TextSmith.exe
# copy documentation into data dir as well
cd install/packages/com.vender.product/data
/D/ProgramFiles/Qt/6.10.2/mingw_64/bin/windeployqt --no-translations TextSmith.exe
cd ../../..
cp -uf ../../Installer.ico config/Installer.ico
cp -uf ../../Background.png config/Background.png
/D/ProgramFiles/Qt/bin/QtInstallerFramework/4.19/bin/binarycreator -c config/config.xml -p packages --offline-only TextSmithInstaller.exe
