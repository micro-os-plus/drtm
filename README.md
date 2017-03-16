## DRTM

An xPack with the Debug Run-Time Metadata library. 

This library provies support for parsing the Debug Run-Time Metadata, used by thread-aware GDB servers. The Debug Run-Time Metadata is stored in the application flash space; it includes addresses of various scheduler data and offsets inside TCBs (Thread Control Blocks).

The purpose is to improve portability of debugging tools, by removing hard-coded constants, like offsets into objects, that may vary between builds.

## Easy install

```
npm install @ilg/drtm
```

## License

The original content is released under the MIT License, with
all rights reserved to Liviu Ionescu.
