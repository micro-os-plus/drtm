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

#ifndef DRTM_METADATA_H_
#define DRTM_METADATA_H_

#if defined(__cplusplus)

#include <drtm/types.h>

#include <cstring>

#define DRTM_SYMBOL_NAME    "os_rtos_drtm"

// Debug Run Time Information v0.1.x offsets.

#define OS_RTOS_DRTM_OFFSETOF_MAGIC 0x00
#define OS_RTOS_DRTM_OFFSETOF_VERSION 0x04
#define OS_RTOS_DRTM_OFFSETOF_SCHEDULER_IS_STARTED_ADDR 0x08
#define OS_RTOS_DRTM_OFFSETOF_SCHEDULER_TOP_THREADS_LIST_ADDR 0x0C
#define OS_RTOS_DRTM_OFFSETOF_SCHEDULER_CURRENT_THREAD_ADDR 0x10
#define OS_RTOS_DRTM_OFFSETOF_THREAD_NAME_OFFSET 0x14
#define OS_RTOS_DRTM_OFFSETOF_THREAD_PARENT_OFFSET 0x16
#define OS_RTOS_DRTM_OFFSETOF_THREAD_LIST_NODE_OFFSET 0x18
#define OS_RTOS_DRTM_OFFSETOF_THREAD_CHILDREN_NODE_OFFSET 0x1A
#define OS_RTOS_DRTM_OFFSETOF_THREAD_STATE_OFFSET 0x1C
#define OS_RTOS_DRTM_OFFSETOF_THREAD_STACK_OFFSET 0x1E
#define OS_RTOS_DRTM_OFFSETOF_THREAD_PRIO_ASSIGNED 0x20
#define OS_RTOS_DRTM_OFFSETOF_THREAD_PRIO_INHERITED 0x22

// Tells how far up is the EXC word.
// TODO: make this configurable
#define STACK_EXC_OFFSET_WORDS   8

namespace drtm
{
  template<typename B>
    class metadata
    {
    public:

      using backend_type = B;

      // These come from types.h
      using addr_t = target_addr_t;
      using offset_t = target_offset_t;

    public:

      /**
       * @brief Construct the metadata collection object instance.
       */
      metadata (backend_type& backend) :
          backend_ (backend) // Parenthesis used to compile with 4.8
      {
#if defined(DEBUG)
        printf ("%s(%p) @%p\n", __func__, &backend, this);
#endif /* defined(DEBUG) */
      }

      // The rule of five.
      metadata (const metadata&) = delete;
      metadata (metadata&&) = delete;
      metadata&
      operator= (const metadata&) = delete;
      metadata&
      operator= (metadata&&) = delete;

      /**
       * @brief Destruct the metadata collection object instance.
       */
      ~metadata () = default;

    public:

      bool
      parse (void)
      {
#if defined(DEBUG)
        printf ("%s()\n", __func__);
#endif /* defined(DEBUG) */

        if (was_parsed)
          {
#if defined(DEBUG)
            printf ("%s()=%s\n", __func__, is_available ? "true" : "false");
#endif /* defined(DEBUG) */
            return is_available;
          }

        addr_t drtm_addr = backend_.get_symbol_address (DRTM_SYMBOL_NAME);

        if (drtm_addr == 0x0)
          {
            backend_.output_error ("The '%s' symbol was not resolved.\n",
            DRTM_SYMBOL_NAME);
            return false;
          }

        // Set this early, to prevent useless checks if parse fails.
        was_parsed = true;

        backend_.read_byte_array (drtm_addr + OS_RTOS_DRTM_OFFSETOF_MAGIC,
                                  &magic[0], sizeof(magic));
        // backend_->output ("%c%c%c%c\n", drtm_.magic[0], drtm_.magic[1], drtm_.magic[2], drtm_.magic[3]);
        if (std::strncmp ((char*) &magic[0], "DRTM", 4) != 0)
          {
            backend_.output_error ("DRTM magic not found, abort.\n",
            DRTM_SYMBOL_NAME);
            return false;
          }

        backend_.read_byte_array (drtm_addr + OS_RTOS_DRTM_OFFSETOF_VERSION,
                                  &version.v, sizeof(version));
        if (version.v != 'v')
          {
            backend_.output_error ("DRTM version field not found, abort.\n",
            DRTM_SYMBOL_NAME);
            return false;
          }
        backend_.output ("DRTM v%u.%u.%u header @0x%08X\n", version.major,
                         version.minor, version.patch, drtm_addr);

        if (version.major == 0)
          {
            int ret;
            ret = backend_.read_long (
                drtm_addr + OS_RTOS_DRTM_OFFSETOF_SCHEDULER_IS_STARTED_ADDR,
                &scheduler.is_started_addr);
            if (ret >= 0)
              {
                ret = backend_.read_long (
                    drtm_addr
                        + OS_RTOS_DRTM_OFFSETOF_SCHEDULER_TOP_THREADS_LIST_ADDR,
                    &scheduler.top_threads_list_addr);
              }
            if (ret >= 0)
              {
                ret = backend_.read_long (
                    drtm_addr
                        + OS_RTOS_DRTM_OFFSETOF_SCHEDULER_CURRENT_THREAD_ADDR,
                    &scheduler.current_thread_addr);
              }

            if (ret >= 0)
              {
                ret = backend_.read_short (
                    drtm_addr + OS_RTOS_DRTM_OFFSETOF_THREAD_NAME_OFFSET,
                    &thread.name_offset);
              }
            if (ret >= 0)
              {
                ret = backend_.read_short (
                    drtm_addr + OS_RTOS_DRTM_OFFSETOF_THREAD_PARENT_OFFSET,
                    &thread.parent_offset);
              }
            if (ret >= 0)
              {
                ret = backend_.read_short (
                    drtm_addr + OS_RTOS_DRTM_OFFSETOF_THREAD_LIST_NODE_OFFSET,
                    &thread.list_node_offset);
              }
            if (ret >= 0)
              {
                ret = backend_.read_short (
                    drtm_addr
                        + OS_RTOS_DRTM_OFFSETOF_THREAD_CHILDREN_NODE_OFFSET,
                    &thread.children_node_offset);
              }
            if (ret >= 0)
              {
                ret = backend_.read_short (
                    drtm_addr + OS_RTOS_DRTM_OFFSETOF_THREAD_STATE_OFFSET,
                    &thread.state_offset);
              }
            if (ret >= 0)
              {
                ret = backend_.read_short (
                    drtm_addr + OS_RTOS_DRTM_OFFSETOF_THREAD_STACK_OFFSET,
                    &thread.stack_offset);
              }
            if (ret >= 0)
              {
                ret = backend_.read_short (
                    drtm_addr + OS_RTOS_DRTM_OFFSETOF_THREAD_PRIO_ASSIGNED,
                    &thread.prio_assigned_offset);
              }
            if (ret >= 0)
              {
                ret = backend_.read_short (
                    drtm_addr + OS_RTOS_DRTM_OFFSETOF_THREAD_PRIO_INHERITED,
                    &thread.prio_inherited_offset);
              }
            if (ret < 0)
              {
                backend_.output_error ("Could not read DRTM.\n");
              }

#if defined(DEBUG)
            printf ("%08X scheduler.is_started_addr\n",
                    scheduler.is_started_addr);
            printf ("%08X scheduler.top_threads_list_addr\n",
                    scheduler.top_threads_list_addr);
            printf ("%08X scheduler.current_thread_addr\n",
                    scheduler.current_thread_addr);
            printf ("%04X thread.name_offset\n", thread.name_offset);
            printf ("%04X thread.parent_offset\n", thread.parent_offset);
            printf ("%04X thread.list_node_offset\n", thread.list_node_offset);
            printf ("%04X thread.children_node_offset\n",
                    thread.children_node_offset);
            printf ("%04X thread.state_offset\n", thread.state_offset);
            printf ("%04X thread.stack_offset\n", thread.stack_offset);
            printf ("%04X thread.prio_assigned_offset\n",
                    thread.prio_assigned_offset);
            printf ("%04X thread.prio_inherited_offset\n",
                    thread.prio_inherited_offset);
#endif /* defined(DEBUG) */

            // TODO: get from DRTM
            list_links.prev_offset = 0;
            list_links.next_offset = 4;

            thread.stack_exc_offset_words = STACK_EXC_OFFSET_WORDS;

          }
        else
          {
            backend_.output_error ("Version not supported.\n");
            return false;
          }

        is_available = true;
        return true;
      }

    private:

      backend_type& backend_;

      // Used to prevent checking the presence of the DRTM multiple times.
      bool was_parsed = false;

      // Once checked, tell if the structure was properly parsed.
      bool is_available = false;

      // 0x00, 4 bytes
      uint8_t magic[4];

      // 0x04, 4 bytes
      struct
      {
        uint8_t v;
        uint8_t major;
        uint8_t minor;
        uint8_t patch;
      } version;

    public:

      struct scheduler_s
      {
        // 0x08, 32-bits pointer
        addr_t is_started_addr;

        // 0x0C, 32-bits pointer
        addr_t top_threads_list_addr;

        // 0x10, 32-bits pointer
        addr_t current_thread_addr;
      } scheduler;

      struct thread_s
      {
        // 0x14, 16-bits unsigned int
        offset_t name_offset;

        // 0x16, 16-bits unsigned int
        offset_t parent_offset;

        // 0x18, 16-bits unsigned int
        offset_t list_node_offset;

        // 0x1C, 16-bits unsigned int
        offset_t children_node_offset;

        // 0x20, 16-bits unsigned int
        offset_t state_offset;

        // 0x22, 16-bits unsigned int
        offset_t stack_offset;

        // 0x24, 16-bits unsigned int
        offset_t prio_assigned_offset;

        // 0x26, 16-bits unsigned int
        offset_t prio_inherited_offset;

        offset_t stack_exc_offset_words;
      } thread;

      struct list_links_s
      {
        // TODO
        offset_t prev_offset;

        // TODO
        offset_t next_offset;
      } list_links;

    };

// ----------------------------------------------------------------------------
} /* namespace drtm */

#endif /* defined(__cplusplus) */

#endif /* DRTM_METADATA_H_ */
