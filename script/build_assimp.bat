@echo off
git submodule update --init external\src\assimp
rm -rf external\build\assimp
mkdir external\build\assimp
pushd external\build\assimp
rm -rf *
cmake -DCMAKE_INSTALL_PREFIX=..\..\ -DCMAKE_INSTALL_RPATH=..\..\ -DCMAKE_BUILD_TYPE=RELEASE ..\..\src\assimp
cmake --build . --config release --target install
popd
echo "Completed build assimp."