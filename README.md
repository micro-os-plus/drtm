[![npm (scoped)](https://img.shields.io/npm/v/@ilg/drtm.svg)](https://www.npmjs.com/package/@ilg/drtm) 
[![license](https://img.shields.io/github/license/micro-os-plus/drtm.svg)](https://github.com/micro-os-plus/drtm/blob/xpack/LICENSE) 
[![Travis](https://img.shields.io/travis/micro-os-plus/drtm.svg?label=linux)](https://travis-ci.org/micro-os-plus/drtm)

## DRTM

An xPack with the **D**ebug **R**un-**T**ime **M**etadata library. 

This library provies support for parsing the RTOS specific Debug Run-Time Metadata, used by thread-aware GDB servers. The Debug Run-Time Metadata is stored in the RTOS application flash space; it includes addresses of various scheduler data and offsets inside TCBs (Thread Control Blocks).

The purpose of DRTM is to improve portability of debugging tools, by removing the need to use hard-coded constants, like offsets into objects, that may vary between RTOS builds.

## Thread-aware debugging overview

By default, modern cross-toolchain GDBs like `arm-none-eaby-gdb` have all the required support to display and navigate the stack trace for the current thread, including reading/writing the processor registers, even if the device specific definitions come from a custom implementation of the GDB server, like SEGGER J-Link GDBServer, or OpenOCD.

Given the available resources in today microcontrollers, most applications can now be  multi-threaded, and there are many RTOSes that implement multi-threading, like FreeRTOS or [µOS++ IIIe](http://micro-os-plus.github.io).

However, generic GDB servers cannot have the required knowledge of RTOS internals, and are not able to recognise when multiple threads are used.

To acomplish this, the GDB servers need to be extended, either by use of plug-ins, like the J-Link GDB Server, or by adding code to the monolithic distribution, like OpenOCD.

Basically, a thread-aware server should provide:

- if the scheduler was started, the number of threads
- a thread ID for each thread
- a description string for each thread
- the ID of the current thread
- for all threads different than the current thread, the server should be able to read (and optionally write) all thread registers from the place where the thread context was saved.

## User info

Not related to a specific thread aware implementation or GDB server, in all cases it is recommended to have the debugged application built with the proper stack frames.

### Compiler option `-mapcs-frame`

When using `arm-none-eabi-gdb`, for a proper display of the stack trace and navigation amongst stack frames, the `-mapcs-frame` option must be added to the compiler command line used to build the embedded application. The purpose is to always generate a stack frame that is compliant with the ARM Procedure Call Standard for all functions, even if this is not strictly necessary for correct execution of the code.

Without this option, GDB will randomly, but quite often, display unrealistic stack traces, possibly with illegal addresses, totally useless in a debug session.

## Developer info

This section is intended to developers that plan to include DRTM in their own GDB servers.

### Easy install

The source files are available from [GitHub](https://github.com/micro-os-plus/drtm):

```
$ git clone https://github.com/micro-os-plus/drtm.git drtm.git
```

The library is also available from the npm registry:

```
$ npm install @ilg/drtm
```

### Prerequisites

The source code require a modern C++ compiler, preferably GCC 5 or higher, but was also compiled with GCC 4.8. Be sure `-std=c++1y` or higher is used when compiling source files that include these templates.

### No warnings

Special care was taken to avoid compiler warnings. For this the tests were compiled with 

- clang `-Weverything -Wno-format-pedantic -Wno-c++98-compat -Wno-c99-extensions -Wno-c++98-compat-pedantic -Wno-documentation-unknown-command` and 
- GCC 6 `-Wall -Wextra -Wunused -Wuninitialized -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Wfloat-equal`

### Testing

As for any xPack, the standard way to run the project tests is via `npm`:

```
$ cd drtm.git
$ npm test
```

The tests can also be exectuted directly, without `npm`:

```
$ cd drtm.git
$ bash scripts/xmake.sh tests 
```

To clean the previous test builds, use:

```
$ cd drtm.git
$ bash scripts/xmake.sh tests -- clean
```

### Templates only, no source files

The DRTM library is distributed as a set of C++ templates, that must be instantiated with classes that define the backend and memory allocator specific for the application.

For an example how to use it, please take a look at the `test/sample/main.c` file and the files in the `samples` folder.

### Integration with C projects

Although written in C++, this library can be easily integrated into a C project, using a C wrapper.

### Integration details

The DRTM C++ implementation uses two templates, one for the backend (mandatory) and one for a custom allocator (optional).

Both these templates call existing functions from the application, directly or via a forwarder.

Samples of these templates, that can be used as a starting point, can be found in the `samples` folder.

#### The backend template

This template adapts the DRTM to the actual application, for things like reading/writing the target memory and sending messages to the output terminal or logger.

The usual implementation is a stateless class, but DRTM takes the safer path and instantiates this class, in case you need to keep state, like pointers to objects, forwarders, etc.

* Variable argument functions

The `v*` versions of the output functions implement the variable argument specialisation. If similar `var_args` variants of the output/logging system functions are available, forward the calls to them. If not, use a temporary buffer, perform the formatting with `vsnprintf()` and send the final string to the output/logger (please check the sample implementation of `output_warning()` for a functional version).

* Type specialisations for read/write

The minimum requirement is to have a pair a functions that read/write a byte array. If specialised functions are already available, forward the calls to them, otherwise implement the endianness conversions in the backend template, as shown in the sample implementation.

* The symbols table

Regardless of the actual implementation, the only way GDB can construct the list of threads is by reading specific locations in the target memory. The addresses of these locations are generally provided by the linker, as the values of some public/global symbols, so the GDB server needs a method to get the values of certain symbols from the debugged ELD.

Depending on the actual implementaion of the GDB server, this might be very simple or somehow tricky.

The idea is that the GDB server, as the name implies, is... a server, i.e. it responds to requests from the GDB client, so it cannot directly call the GDB client (which has the symbols loaded from the ELF file) and ask for the value of a given symbol, and a more elaborate protocol is required.

The details of this protocol, or the details of the implementation, are not relevant for DRTM. DRTM requires the value of a single symbol (`os_rtos_drtm`) and for this the backend template must include a function (`get_symbol_address()`) that must call any support available in the application to get the value of this symbol.


#### The memory allocator template

The need to use a custom allocator allocator instead of the standard C++ allocator is related to memory allocator consistency in multi-threaded environments. If the multi-threaded scheduler is preemptive, and the threads do need to allocate memory at any time, a synchronisation mechanism (like a mutex) should be used when accessing the memory allocator. If the allocator in the system library is not thread safe (most moderm POSIX implementations are), but is implementd at application level, than this application specific allocator must be passed to the DRTM library.

As a general recommandation, if the application uses a custom memory manager, pass it to the DRTM library as a custom allocator. If not, do not define a custom allocator but use the `std::allocator`.

#### The aplication specific header

In the sample implementation, all definitions relating to the applications are grouped in the `your-application.h` file, which is included in the templates. In a real life case, either directly include all required application headers in the templates, or group these headers in a file, and include only this file in the templates.

#### The template instantiations

Being a header only library, DRTM itself does not have any source files (well, except the `version.c` file, which is not actually used).

According to current C++ specifications, templates are automatically instantiated by the compiler, when needed. To make things more clear, explicit instantiation is used in the `drtm.cpp` file, which implements the C wrapper.

## Future developments

The first version (0.x) of the metadata will be a simple structure with various members, so the library will still need to use hard-coded offsets in this structure to access the data.
 
For version 1.x it is planed to improve the structure of the metadata, by using `id:value` pairs, in a sort of **compile-time binary JSON**, so the fixed point will be a list of numeric IDs, not offsets in a structure.

The first versions will focus on describing the metadata specific to µOS++ IIIe, but given that JSONs are generic enough to describe any data structures, it shouldn't be that difficult to describe most of the data types and memory structures in use by popular RTOSes. 

The assumption is that the number of such different data structures used to manage the list of threads is relatively low and manageable, so it should be easier to add a new definition to an existing framework that is already fully functional, than to redo an implementation completely from scratch. And once the DRTM data is in, the result should be directly available to all servers that use the DRTM library (OpenOCD, J-Link, QEMU being on my TODO list).

## Forum

The DRTM proposal was posted on the [project forum](https://www.element14.com/community/thread/59299/l/drtm-debug-run-time-metadata-proposal).

## License

The original content is released under the MIT License, with
all rights reserved to Liviu Ionescu.
