@echo off
git submodule update --init external\src\glfw
rm -rf external\build\glfw
mkdir external\build\glfw
pushd external\build\glfw
cmake -DCMAKE_INSTALL_PREFIX=..\..\ -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RELEASE -Thost=x64 ..\..\src\glfw
cmake --build . --config release --target install
popd
echo "Completed build glfw."