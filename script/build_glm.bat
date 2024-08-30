@echo off
git submodule update --init external\src\glm
rm -rf external\build\glm
mkdir external\build\glm
pushd external\build\glm
cmake -DCMAKE_INSTALL_PREFIX=..\..\ -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RELEASE -DGLM_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF ..\..\src\glm
cmake --build . --config release --target install
popd
echo "Completed build glm."