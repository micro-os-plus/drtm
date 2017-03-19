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

For a proper display of the stack trace and navigation amongst stack frames, the `-mapcs-frame` option must be added to the GCC command line to generate a stack frame that is compliant with the ARM Procedure Call Standard for all functions, even if this is not strictly necessary for correct execution of the code.

## License

The original content is released under the MIT License, with
all rights reserved to Liviu Ionescu.
