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

#include <drtm/drtm.h>
#include <drtm-backend.h>
#include <drtm-memory.h>
#include <drtm/version.h>

using drtm_target_addr_t = uint32_t;

typedef struct symbol_s
{
  using element_type=drtm_target_addr_t;

  const char* name;
  element_type address;
} symbol_type;

typedef struct server_api_s
{
  void
  (*output) (const char* fmt, ...);

  void
  (*output_debug) (const char* fmt, ...);

  void
  (*output_warning) (const char* fmt, ...);

  void
  (*output_error) (const char* fmt, ...);

  int
  (*read_byte_array) (drtm_target_addr_t addr, uint8_t* in_array,
                      size_t nbytes);

  int
  (*read_byte) (drtm_target_addr_t addr, uint8_t* in_byte);

  int
  (*read_short) (drtm_target_addr_t addr, uint16_t* in_short);

  int
  (*read_long) (drtm_target_addr_t addr, uint32_t* in_long);

  int
  (*write_byte_array) (drtm_target_addr_t addr, const uint8_t* out_array,
                       size_t nbytes);

  void
  (*write_byte) (drtm_target_addr_t addr, uint8_t out_byte);

  void
  (*write_short) (drtm_target_addr_t addr, uint16_t out_short);

  void
  (*write_long) (drtm_target_addr_t addr, uint32_t out_long);

  uint32_t
  (*load_short) (const uint8_t* p);

  uint32_t
  (*load_3bytes) (const uint8_t* p);

  uint32_t
  (*load_long) (const uint8_t* p);

} server_api_type;

namespace your_namespace
{
  namespace drtm
  {

    void*
    your_application_malloc (std::size_t bytes)
    {
      return malloc (bytes);
    }

    void
    your_application_free (void* p)
    {
      free (p);
    }
    ;
  // Avoid formatter bug
  // ==========================================================================
  } /* namespace drtm */
} /* namespace your_namespace */

// ---------------------------------------------------------------------------
// Templates.

// Template explicit instantiation.
template class your_namespace::drtm::backend<server_api_type, symbol_type>;
// Define a type alias.
using backend_type = class your_namespace::drtm::backend<server_api_type,
symbol_type>;

// Template explicit instantiation.
template class your_namespace::drtm::allocator<void*>;
// Define a type alias.
using backend_allocator_type = class your_namespace::drtm::allocator<void*>;

// ---------------------------------------------------------------------------

// Template explicit instantiation.
template class drtm::metadata<backend_type>;
// Define a type alias.
using metadata_type = class drtm::metadata<backend_type>;

// Template explicit instantiation.
template class drtm::thread<backend_type, backend_allocator_type>;
// Define a type alias.
using thread_type = class drtm::thread<backend_type, backend_allocator_type>;

// Template explicit instantiation.
template class drtm::threads<backend_type, backend_allocator_type>;
// Define a type alias.
using threads_type = class drtm::threads<backend_type, backend_allocator_type>;

// Template explicit instantiation.
template class drtm::run_time_data<backend_type, backend_allocator_type>;
// Define a type alias.
using rtd_type = class drtm::run_time_data<backend_type, backend_allocator_type>;

// Template explicit instantiation.
template class drtm::frontend<backend_type, backend_allocator_type>;
// Define a type alias.
using frontend_type = class drtm::frontend<backend_type, backend_allocator_type>;

int
main (int argc, char* argv[])
{
  printf ("DRTM library, v%d.%d.%d build test\n",
  XPACK_ILG_DRTM_VERSION_MAJOR,
          XPACK_ILG_DRTM_VERSION_MINOR,
          XPACK_ILG_DRTM_VERSION_PATCH);

  printf ("Built with " __VERSION__ "\n");

  printf ("Done.\n");
  return 0;
}
