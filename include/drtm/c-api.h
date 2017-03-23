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

#ifndef DRTM_C_API_H_
#define DRTM_C_API_H_

#include <stdint.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C"
{
#endif /* defined(__cplusplus) */

  // Generally these definitions must be kept in sync
  // with the ones in the application.
  // C++ definitions also use them.
  typedef uint32_t drtm_thread_id_t;
  typedef uint32_t drtm_target_addr_t;

  int
  drtm_init (void);

  void
  drtm_shutdown (void);

  int
  drtm_update_thread_list (void);

  size_t
  drtm_get_threads_count (void);

  drtm_thread_id_t
  drtm_get_thread_id (size_t index);

  drtm_thread_id_t
  drtm_get_current_thread_id (void);

  int
  drtm_get_thread_description (drtm_thread_id_t tid, char* out_description,
                               size_t out_size_bytes);

  int
  drtm_get_thread_register (drtm_thread_id_t tid, size_t reg_index,
                            char* out_hex_value, size_t out_size_bytes);

  int
  drtm_get_thread_registers (drtm_thread_id_t tid, char* out_hex_values,
                             size_t out_size_bytes);

  int
  drtm_set_thread_register (drtm_thread_id_t tid, size_t reg_index,
                            const char* hex_value);

  int
  drtm_set_thread_registers (drtm_thread_id_t tid, const char* hex_values);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* DRTM_C_API_H_ */
