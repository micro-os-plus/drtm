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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"

// ---------------------------------------------------------------------------
// Templates.

// Template explicit instantiation.
template class your_namespace::drtm::allocator<void*>;
// Define a type alias.
using backend_allocator_type = class your_namespace::drtm::allocator<void*>;

// Template explicit instantiation.
template class your_namespace::drtm::backend<yapp_symbols_t>;
// Define a type alias.
using backend_type = class your_namespace::drtm::backend<yapp_symbols_t>;

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

#pragma GCC diagnostic pop

// ---------------------------------------------------------------------------
// The C API.

static struct
{
  backend_allocator_type* allocator;
  backend_type* backend;

  frontend_type* frontend;
} drtm_;

int
drtm_init (void)
{
  // Allocate space for the DRTM allocator object instance.
  drtm_.allocator = reinterpret_cast<backend_allocator_type*> (yapp_malloc (
      sizeof(backend_allocator_type)));

  // Construct the already allocated DRTM allocator object instance.
  new (drtm_.allocator) backend_allocator_type
    { };

  // Allocate space for the DRTM backend object instance.
  drtm_.backend = reinterpret_cast<backend_type*> (yapp_malloc (
      sizeof(backend_type)));

  // Construct the already allocated DRTM backend object instance.
  new (drtm_.backend) backend_type
    { yapp_symbols };

  // Allocate space for the DRTM frontend object instance.
  drtm_.frontend = reinterpret_cast<frontend_type*> (yapp_malloc (
      sizeof(frontend_type)));

  // Construct the already allocated DRTM frontend object instance.
  new (drtm_.frontend) frontend_type
    { *drtm_.backend, *drtm_.allocator };

  return 0;
}

void
drtm_shutdown (void)
{
  drtm_.frontend->~frontend_type ();
  yapp_free (drtm_.frontend);

  drtm_.backend->~backend_type ();
  yapp_free (drtm_.backend);

  drtm_.allocator->~backend_allocator_type ();
  yapp_free (drtm_.allocator);
}

int
drtm_update_thread_list (void)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->update_thread_list ();
}

size_t
drtm_get_threads_count (void)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->get_threads_count ();
}

drtm_thread_id_t
drtm_get_thread_id (size_t index)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->get_thread_id (index);
}

drtm_thread_id_t
drtm_get_current_thread_id (void)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->get_current_thread_id ();
}

size_t
drtm_get_thread_description (drtm_thread_id_t tid, char* out_description,
                             size_t out_size_bytes)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->get_thread_description (tid, out_description,
                                                 out_size_bytes);
}

int
drtm_get_thread_register (drtm_thread_id_t tid, size_t reg_index,
                          char* out_hex_value, size_t out_size_bytes)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->get_thread_register (tid, reg_index, out_hex_value,
                                              out_size_bytes);
}

int
drtm_get_thread_registers (drtm_thread_id_t tid, char* out_hex_values,
                           size_t out_size_bytes)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->get_thread_registers (tid, out_hex_values,
                                               out_size_bytes);
}

int
drtm_set_thread_register (drtm_thread_id_t tid, size_t reg_index,
                          const char* hex_value)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->set_thread_register (tid, reg_index, hex_value);
}

int
drtm_set_thread_registers (drtm_thread_id_t tid, const char* hex_values)
{
  assert(drtm_.frontend != nullptr);
  return drtm_.frontend->set_thread_registers (tid, hex_values);
}

// ---------------------------------------------------------------------------
