# ctpktool

A tool for exporting/importing ctpk file.

## History

- v1.1.0 @ 2018.01.04 - A new beginning
- v1.1.1 @ 2018.07.27 - Update cmake
- v1.1.2 @ 2019.07.09 - Update dep

### v1.0

- v1.0.0 @ 2015.03.11 - First release
- v1.0.1 @ 2017.04.04 - Change encoding and support ctpk icon
- v1.0.2 @ 2017.04.06 - Avoid ctpk contain backslash

## Platforms

- Windows
- Linux
- macOS

## Building

### Dependencies

- cmake
- zlib
- libpng

### Compiling

- make 64-bit version
~~~
mkdir build
cd build
cmake -DUSE_DEP=OFF ..
make
~~~

- make 32-bit version
~~~
mkdir build
cd build
cmake -DBUILD64=OFF -DUSE_DEP=OFF ..
make
~~~

### Installing

~~~
make install
~~~

## Usage

### Windows

~~~
ctpktool [option...] [option]...
~~~

### Other

~~~
ctpktool.sh [option...] [option]...
~~~

## Options

See `ctpktool --help` messages.
