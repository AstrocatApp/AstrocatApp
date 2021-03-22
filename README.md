# AstrocatApp
Image Organization for Astrophotography


## Building Sources

* The source code is activley being developed on MacOS. The build has not been verified yet on Windows or Linux.
* Feel free to contribute to make it build on Windows and Linux.


First build the third party libraries
```
cd external/build
cmake ..
make
cd ../..
```

This should put all third party binaries in the `external/build/libs` folder

Then either build AstrocatApp.pro from Qt Creator, or from command line:
```
mkdir build
cd build
qmake ../src/AstrocatApp.pro -spec macx-clang CONFIG+=x86_64 CONFIG+=qtquickcompiler
make
```

