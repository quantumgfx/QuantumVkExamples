@echo off

echo Generating Examples

cd ..

cmake -B "build" -G "Visual Studio 16 2019"

cmake --build "build" --config "Debug"
cmake --install "build" --prefix "bin" --config "Debug"

pause