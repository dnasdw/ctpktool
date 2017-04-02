# ctpktool

A tool for exporting/importing ctpk file.

## History

- v1.0.0 @ 2015.03.11 - First release

## Platforms

- Windows
- Linux
- macOS

## Building

### Dependencies

- cmake
- libpng

### Compiling

- make 64-bit version
~~~
mkdir project
cd project
cmake -DUSE_DEP=OFF ..
make
~~~

- make 32-bit version
~~~
mkdir project
cd project
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
