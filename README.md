# csi-avro-utils
=========

A C++ version of the late apache avrogencpp that adds some improvements
 - embeddes normalized schema in generated classes

Platforms: Windows / Linux / Mac

## Ubuntu 16 x64:

Install build tools
```
sudo apt-get install -y automake autogen shtool libtool git wget cmake unzip build-essential libboost-all-dev g++ python-dev autotools-dev libicu-dev zlib1g-dev openssl libssl-dev libbz2-dev libsnappy-dev

```
Build
```
wget --no-check-certificate http://apache.mirrors.spacedump.net/avro/avro-1.8.1/cpp/avro-cpp-1.8.1.tar.gz
tar xfv avro-cpp-1.8.1.tar.gz
cd avro-cpp-1.8.1
./build.sh test
sudo ./build.sh install

git clone https://github.com/bitbouncer/csi-avro-utils.git
cd csi-avro-utils
bash rebuild_linux.sh
cd ..
```

## MacOS X

Install build tools (using Homebrew)
```
# Install Xcode
xcode-select --install
brew install cmake
brew install snappy
brew install boost
```

Check out source code
```
git clone https://github.com/bitbouncer/csi-avro-utils.git
```

Run the build
```
./rebuild_macos.sh
```

## Windows x64:

Install build tools
```
- CMake (https://cmake.org/)
- Visual Studio 14 (https://www.visualstudio.com/downloads/)
- nasm (https://sourceforge.net/projects/nasm/)
- perl (http://www.activestate.com/activeperl)
```
Build
```
wget --no-check-certificate http://downloads.sourceforge.net/project/boost/boost/1.62.0/boost_1_62_0.zip
unzip boost_1_62_0.zip
rename boost_1_62_0 boost

git clone https://github.com/madler/zlib.git
git clone https://github.com/openssl/openssl.git
git clone https://github.com/bitbouncer/avro.git
git clone https://github.com/bitbouncer/csi-avro-utils.git

set VISUALSTUDIO_VERSION_MAJOR=14
call "C:\Program Files (x86)\Microsoft Visual Studio %VISUALSTUDIO_VERSION_MAJOR%.0\VC\vcvarsall.bat" amd64

cd zlib
mkdir build & cd build
cmake -G "Visual Studio 14 Win64" ..
msbuild zlib.sln
msbuild zlib.sln /p:Configuration=Release
cd ../..

cd openssl
#git checkout OpenSSL_1_1_0c
perl Configure VC-WIN64A
nmake 
#you need to be Administrator for the next step)
nmake install 
cd ..

cd boost
call bootstrap.bat
.\b2.exe -toolset=msvc-%VisualStudioVersion% variant=release,debug link=static address-model=64 architecture=x86 --stagedir=stage\lib\x64 stage -s ZLIB_SOURCE=%CD%\..\zlib headers log_setup log date_time timer thread system program_options filesystem regex chrono
cd ..

cd csi-avro-utils
call rebuild_windows_vs14.bat
cd ..

```

