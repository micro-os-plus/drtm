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

#ifndef DRTM_TYPES_H_
#define DRTM_TYPES_H_

#if defined(__cplusplus)

#include <cstdint>

namespace drtm
{

  // Generic target address.
  using target_addr_t = uint32_t;
  using target_offset_t = uint16_t;

  // ---

  // RTOS symbols.
  typedef struct symbols_s
  {
    const char *name = nullptr;
    int optional = 0;
    target_addr_t address = 0;
  } symbols_t;

  typedef int8_t register_offset_t;
  typedef struct stack_info_s
  {
    uint32_t in_registers;
    uint32_t out_registers;
    const register_offset_t* offsets;
    uint32_t offsets_size;
  } stack_info_t;

  typedef struct rtos_s
  {
    const stack_info_t* stack_info;
    const stack_info_t* stack_info_vfp;
  } rtos_t;

  extern rtos_t rtos;

  extern const char* thread_states[6];

// ----------------------------------------------------------------------------
} /* namespace drtm */

#endif /* defined(__cplusplus) */

#endif /* DRTM_TYPES_H_ */
