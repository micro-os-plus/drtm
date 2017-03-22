[![npm (scoped)](https://img.shields.io/npm/v/@ilg/drtm.svg)](https://www.npmjs.com/package/@ilg/drtm) [![license](https://img.shields.io/github/license/micro-os-plus/drtm.svg)](https://github.com/micro-os-plus/drtm/blob/xpack/LICENSE)

## DRTM

An xPack with the Debug Run-Time Metadata library. 

This library provies support for parsing the Debug Run-Time Metadata, used by thread-aware GDB servers. The Debug Run-Time Metadata is stored in the application flash space; it includes addresses of various scheduler data and offsets inside TCBs (Thread Control Blocks).

The purpose is to improve portability of debugging tools, by removing hard-coded constants, like offsets into objects, that may vary between builds.

## Easy install

```
npm install @ilg/drtm
```

## `-mapcs-frame`

For a proper display of the stack trace and navigation amongst stack frames, the `-mapcs-frame` option must be added to the GCC command line used to build the embedded application. The purpose is to always generate a stack frame that is compliant with the ARM Procedure Call Standard for all functions, even if this is not strictly necessary for correct execution of the code.

Without this option, GDB will randomly, but quite often, display unrealistic stack traces, possibly with illegal addresses, totally useless in a debug session.

## Prerequisites

The source code require a modern C++ compiler, preferably GCC 5 or higher, but was also compiled with GCC 4.8. Be sure `-std=c++1y` or higher is used when compiling source files that include these templates.

## Templates only, no source files

The DRTM library is distributed as a set of C++ templates, that must be instantiated with classes that define the backend and memory allocator specific for the application.

For an example how to use it, please take a look at the `test/sample/main.cpp` file.

## Integration with C projects

Although written in C++, this library can be easily be integrated into a C project, using a C wrapper.

## License

The original content is released under the MIT License, with
all rights reserved to Liviu Ionescu.
