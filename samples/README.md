# Integration 

These files can be used as a starting point for a real implementation.

## `your-application.h`

This is an example of a header file with the application definitions used in the C++ templates.

Either directly include all required application headers in the templates, or group these headers in a file, and include only this file in the templates.

## `drtm-backend.h`

This class template provides a complete definition in C++ of the application resources required by the C++ DRTM library.

Update the template to match the environment where the DRTM library is included.

## `drtm-memory.h`

If, for any reasons, the application uses a custom memory manager, pass it to the DRTM library as a custom allocator. If not, do not define a custom allocator but use the `std::allocator`.

## `drtm.cpp`

This is the only source code file neeeded to include the DRTM library in an application. If needed, it provides a C API for C applications. it also provides an explicit instantioation for all DRTM templates.

## Functional sample

A full sample that compiles as a test is available in the `tests/sample` folder.


