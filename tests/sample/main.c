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

#include <stdio.h>
#include <stdlib.h>

#include <your-application.h>
#include <drtm/drtm.h>

int
main (int argc, char* argv[])
{
  printf ("DRTM library, v%d.%d.%d build test\n",
  XPACK_ILG_DRTM_VERSION_MAJOR,
          XPACK_ILG_DRTM_VERSION_MINOR,
          XPACK_ILG_DRTM_VERSION_PATCH);

  printf ("Built with " __VERSION__ "\n");

#if 0

  // Usage sample:

  drtm_init();

  drtm_update_thread_list();

  size_t nt = drtm_get_threads_count();

  char* buf[200];

  for (size_t i = 0; i < nt; ++i)
    {
      drtm_thread_id_t tid = drtm_get_thread_id(i);
      drtm_get_thread_description(tid, buf, sizeof(buf));

      printf("%s\n", buf);
    }

#endif

  printf ("Done.\n");
  return 0;
}

// ----------------------------------------------------------------------------
// (yapp stands for your-application; update it accordingly)

// A structure similar to this should be used to query the GDB server for
// the address of a symbol.

yapp_symbols_t yapp_symbols[] =
  {
    { .name = "DRTM_SYMBOL_NAME" },
    { }
  /**/
  };

// ----------------------------------------------------------------------------

// Functions with similar prototypes should be already available in
// your application, use them directly and adjust the backend template.

int
yapp_voutput (const char* fmt, va_list args)
{
  return vprintf (fmt, args);
}

void
yapp_output (const char* msg)
{
  printf ("%s", msg);
}

void
yapp_output_warning (const char* msg)
{
  printf ("WARNING: ");
  printf ("%s", msg);
}

void
yapp_output_error (const char* msg)
{
  printf ("ERROR: ");
  printf ("%s", msg);
}

// If not available, the backend should assume a default,
// Cortex-M devices generally cannot change endianness at run-time.
bool
yapp_is_target_little_endian (void)
{
  // TODO: return true if little endian, false otherwise.
  return true;
}

// These functions read/write data from the target address space.
// They transfer the bytes as originally
// stored by the target, without any reordering.
int
yapp_read_byte_array (yapp_target_addr_t addr, uint8_t* in_array, size_t bytes)
{
  // TODO: read bytes.
  // Return 0 if ok, <0 if error.
  return 0;
}

int
yapp_write_byte_array (yapp_target_addr_t addr, const uint8_t* out_array,
                       size_t bytes)
{
  // TODO write bytes.
  // Return 0 if ok, <0 if error.
  return 0;
}

void*
yapp_malloc (size_t bytes)
{
  return malloc (bytes);
}

void
yapp_free (void* p)
{
  free (p);
}

// ----------------------------------------------------------------------------
