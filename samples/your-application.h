/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2017 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef YOUR_APPLICATION_H_
#define YOUR_APPLICATION_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C"
{
#endif /* defined(__cplusplus) */

  // Addresses in the target memory space.
  typedef uint32_t yapp_target_addr_t;

  // This should be the structure used to query the GDB for symbol values.
  // If your structure has different member names, adjust the backend
  // template.
  typedef struct
  {
    char* name;
    yapp_target_addr_t address;
  } yapp_symbols_t;

  extern yapp_symbols_t yapp_symbols[];

  // Functions to output/log different types of messages.
  // If you have available functions that take va_list args, they
  // are preferred, otherwise a local buffer will be used and the
  // string version of the function is also acceptable.
  int
  yapp_voutput (const char* fmt, va_list args);
  void
  yapp_output (const char* msg);

  int
  yapp_voutput_warning (const char* fmt, va_list args);
  void
  yapp_output_warning (const char* msg);

  int
  yapp_voutput_error (const char* fmt, va_list args);
  void
  yapp_output_error (const char* msg);

  // If not available, the backend should assume a default,
  // Cortex-M devices generally cannot change endianness at run-time.
  bool
  yapp_is_target_little_endian (void);

  // These functions read/write data from the target address space.
  // They transfer the bytes as originally
  // stored by the target, without any reordering.
  int
  yapp_read_byte_array (yapp_target_addr_t addr, uint8_t* in_array,
                        size_t bytes);

  int
  yapp_write_byte_array (yapp_target_addr_t addr, const uint8_t* out_array,
                         size_t bytes);

  // Functions to handle memory allocation.
  void*
  yapp_malloc (size_t bytes);

  void
  yapp_free (void* p);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* YOUR_APPLICATION_H_ */
