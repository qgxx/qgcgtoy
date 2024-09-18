mkdir build
pushd build
rm -rf *
cmake -G "Visual Studio 17 2022" -Thost=x64 ..
cmake --build . --config release
popd
echo "Completed build project."
mkdir build\Release\test
copy build\rendering\global_illumination\SSAO\Release build\Release
copy build\rendering\global_illumination\SSR\Release build\Release
copy build\rendering\global_illumination\RSM\Release build\Release
copy build\rendering\physically_based_rendering\Cook-Torrance_BRDF\Release build\Release
copy build\rendering\physically_based_rendering\LTC\Release build\Release
copy build\rendering\environment_mapping\IBL\Release build\Release
copy build\rendering\non-photorealistic-rendering\Release build\Release
copy build\rendering\ray_tracing\path_tracing\Release build\Release
copy build\rendering\misc\forward_plus\Release build\Release
copy build\test\Release build\Release\test