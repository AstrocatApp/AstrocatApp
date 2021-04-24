# AstrocatApp
Image Organization for Astrophotography


## Building Sources

* The source code is activley being developed on MacOS. The build has not been verified yet on Windows or Linux.
* Feel free to contribute to make it build on Windows and Linux.


## First build the third party libraries
On Windows, instead of make, you can use `cmake.exe --build . --config Release`

This should put all third party binaries in the `external/build/libs` folder

### On MacOS
First build cfitsio
```
cd external/cfitsio
./configure --enable-reentrant --disable-curl
make
cp libcfitsio.a ../build/libs
```

Next build the other libs
```
cd external/build
cmake ..
cmake --build . --parallel --config Release
cd ../..
```

### On Windows
We need to build pthreads for Windows before we can build cfitsio
```
cd external/pthreads
nmake clean VC
copy pthreadVC2.dll ..\build\libs\Release\
copy pthreadVC2.lib ..\build\libs\Release\
```
To run the app from explorer, copy pthreadVC2.dll to the app binary release folder.

Now build cfitsio and the other libs
```
cd external/build
cmake ..
cmake --build . --parallel --config Release
```

### Build the App
Then either build AstrocatApp.pro from Qt Creator, or from command line:
```
mkdir build
cd build
qmake ../src/AstrocatApp.pro -spec macx-clang CONFIG+=x86_64 CONFIG+=qtquickcompiler
make
```
