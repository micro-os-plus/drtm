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

#ifndef DRTM_RUN_TIME_DATA_H_
#define DRTM_RUN_TIME_DATA_H_

#if defined(__cplusplus)

#include <drtm/types.h>

#include <memory>

namespace drtm
{

  template<typename B, typename M, typename T, typename A>
    class run_time_data
    {
    public:

      using backend_type = B;
      using metadata_type = M;
      using threads_type = T;
      using allocator_type = A;

      using thread_type = typename threads_type::thread_type;

      using thread_addr_t = typename thread_type::thread_addr_t;

      using addr_t = target_addr_t;

      // Address of a target list node.
      using list_node_addr_t = target_addr_t;

      // Target iterator, an object that includes a single target pointer.
      using iterator_t = target_addr_t;

      // Make a new allocator, for characters.
      using char_allocator_type =
      typename std::allocator_traits<allocator_type>::template rebind_alloc<char>;

    public:

      run_time_data (backend_type& backend, metadata_type& metadata,
                     threads_type& threads, allocator_type& allocator) :
          backend_
            { backend }, //
          metadata_
            { metadata }, //
          threads_
            { threads }, //
          allocator_
            { allocator }
      {
#if defined(DEBUG)
        printf ("%s(%p, %p, %p) @%p\n", __func__, &backend, &metadata,
                &allocator, this);
#endif /* defined(DEBUG) */
      }

      // The rule of five.
      run_time_data (const run_time_data&) = delete;
      run_time_data (run_time_data&&) = delete;
      run_time_data&
      operator= (const run_time_data&) = delete;
      run_time_data&
      operator= (run_time_data&&) = delete;

      ~run_time_data () = default;

    public:

      bool
      is_scheduler_started (void)
      {
#if defined(DEBUG)
        printf ("%s()\n", __func__);
#endif /* defined(DEBUG) */

        bool ret;

        int err;
        err = backend_.read_byte (metadata_.scheduler.is_started_addr,
                                  (uint8_t*) &ret);
        if (err < 0)
          {
            backend_.output_error ("Could not read 'is_started'.\n");
            return false;
          }

#if defined(DEBUG)
        printf ("%s() @0x%08X 0x%02X\n", __func__,
                metadata_.scheduler.is_started_addr, ret);
#endif /* defined(DEBUG) */

        return ret;
      }

      /**
       * @brief Update the local list of threads.
       */
      void
      update_threads (void)
      {
        iterate_threads (0, 0);

        update_current_thread ();
      }

      /**
       * @brief Iterate through the thread children, each with its children.
       */
      void
      iterate_threads (thread_addr_t ta, unsigned int depth)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X, %u)\n", __func__, ta, depth);
#endif /* defined(DEBUG) */

        iterator_t it = children_threads_iter_begin (ta);
        iterator_t end = children_threads_iter_end (ta);

        while (it != end)
          {
            // Get the pointer to the thread from the iterator.
            thread_addr_t thread_addr = children_threads_iter_get (it);

            thread_type* th = threads_.new_thread ();
            threads_.push_back (th);

            // Remember the thread address, it is used to determine the
            // current thread.
            // This will also set the ID.
            th->addr (thread_addr);

            // Copy the name, byte by byte. A large array copy is risky, since the
            // thread might be allocated right at the end of RAM, and
            // the copy might try to access past the limit.
            char tmp_name[thread_type::name_max_size_bytes];
            int ret;
            addr_t name_addr;
            ret = backend_.read_long (
                thread_addr + metadata_.thread.name_offset, &name_addr);
            if (ret < 0)
              {
                backend_.output_error ("Could not read 'thread.name*'.\n");
              }

            addr_t addr = name_addr;
            char* p = tmp_name;
            int count = 0;
            uint8_t b;
            while (true)
              {
                ret = backend_.read_byte (addr, &b);
                if (ret < 0)
                  {
                    backend_.output_error ("Could not read 'thread.name'.\n");
                  }
                *p = b;
                ++p;
                ++addr;
                ++count;
                if (b == '\0' || count >= sizeof(tmp_name))
                  {
                    break;
                  }
              }

            th->name = std::allocator_traits<char_allocator_type>::allocate (
                reinterpret_cast<char_allocator_type&> (allocator_), count);

            strcpy (th->name, tmp_name);

            addr = thread_addr + metadata_.thread.prio_assigned_offset;
            ret = backend_.read_byte (addr, &b);
            if (ret < 0)
              {
                backend_.output_error (
                    "Could not read 'thread.prio_assigned'.\n");
              }
            th->prio_assigned = b;

            addr = thread_addr + metadata_.thread.prio_inherited_offset;
            ret = backend_.read_byte (addr, &b);
            if (ret < 0)
              {
                backend_.output_error (
                    "Could not read 'thread.prio_inherited'.\n");
              }
            th->prio_inherited = b;

            addr = thread_addr + metadata_.thread.state_offset;
            ret = backend_.read_byte (addr, &b);
            if (ret < 0)
              {
                backend_.output_error ("Could not read 'thread.state'.\n");
              }
            th->state = b;

            addr = thread_addr + metadata_.thread.stack_offset;
            ret = backend_.read_byte_array (addr, &th->stack.sp_addr[0],
                                            thread_type::register_size_bytes);
            if (ret < 0)
              {
                backend_.output_error ("Could not read 'thread.stack_ptr'.\n");
              }

            th->stack.addr = backend_.load_long (&th->stack.sp_addr[0]);

            uint32_t exc_return;
            ret = backend_.read_long (
                th->stack.addr
                    + (metadata_.thread.stack_exc_offset_words
                        * thread_type::register_size_bytes),
                &exc_return);
            if (ret < 0)
              {
                backend_.output_error ("Could not read 'thread.stack_ptr'.\n");
              }

#if defined(DEBUG)
            printf ("thread EXC_RETURN 0x%08X\n", exc_return);
#endif /* defined(DEBUG) */

            if (((exc_return & 0xFFFFFFE3) == 0xFFFFFFE1)
                && ((exc_return & 0x10) == 0))
              {
                th->stack.info = rtos.stack_info_vfp;
                th->stack.is_floating_point = true;
              }
            else
              {
                th->stack.info = rtos.stack_info;
                th->stack.is_floating_point = false;
              }

#if defined(DEBUG)
            printf ("thread @0x%08X '%s' S:%u P:%u(%u) %s\n", thread_addr,
                    th->name, th->state, th->prio_inherited, th->prio_assigned,
                    (th->stack.is_floating_point ? "FP" : ""));
#endif /* defined(DEBUG) */

            // Go down one level.
            iterate_threads (thread_addr, depth + 1);

            // Increment the iterator to the next element in the list.
            it = children_threads_iter_next (it);
          }
      }

      /**
       * @brief Read the address of the current thread and cache
       * its details and ID.
       */
      void
      update_current_thread (void)
      {
        thread_addr_t current_thread_addr;
        int ret;
        ret = backend_.read_long (metadata_.scheduler.current_thread_addr,
                                  &current_thread_addr);
        if (ret < 0)
          {
            backend_.output_error (
                "Could not read 'scheduler.current_thread_addr'.\n");
            return;
          }

        thread_type* current_th = nullptr;

        for (auto* th : threads_)
          {
            if (th->addr () == current_thread_addr)
              {
                current_th = th;
                break;
              }
          }

#if defined(DEBUG)
        if (current_th != nullptr)
          {
            printf ("current thread @0x%08X '%s'\n", current_thread_addr,
                    current_th->name);
          }
        else
          {
            printf ("current thread @0x%08X\n", current_thread_addr);
          }
#endif /* defined(DEBUG) */

        threads_.current (current_th);
      }

    private:

      // ----------------------------------------------------------------------
      // The Children Threads methods.

      /**
       * @brief Get the list of children threads.
       *
       * @details
       * Get the address of the list node associated with a thread,
       * or the top list if the thread address is 0.
       *
       * @return The target address of the list node.
       */
      list_node_addr_t
      children_threads_get_list (thread_addr_t ta)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)\n", __func__, ta);
#endif /* defined(DEBUG) */

        list_node_addr_t list_node_addr;
        if (ta == 0)
          {
#if defined(DEBUG) && defined(DEBUG_LISTS)
            printf ("%s(0x%08X) top list\n", __func__, ta);
#endif /* defined(DEBUG) */
            list_node_addr = metadata_.scheduler.top_threads_list_addr;
          }
        else
          {
            list_node_addr = ta + metadata_.thread.children_node_offset;
          }

#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)=0x%08X\n", __func__, th, list_node_addr);
#endif /* defined(DEBUG) */

        return list_node_addr;
      }

      /**
       * @brief Iterator begin().
       *
       * @details
       * As per C++ convention, this is the address of the first node
       * element in the list.
       *
       * @return An iterator pointing to the beginning of the list.
       */
      iterator_t
      children_threads_iter_begin (thread_addr_t ta)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)\n", __func__, th);
#endif /* defined(DEBUG) */

        list_node_addr_t list_node_addr = children_threads_get_list (ta);
        iterator_t it;
        int ret;
        ret = backend_.read_long (
            list_node_addr + metadata_.list_links.next_offset, &it);
        if (ret < 0)
          {
            backend_.output_error (
                "Could not read 'list_links.next_offset'.\n");
          }

#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)=0x%08X\n", __func__, th, it);
#endif /* defined(DEBUG) */

        return it;
      }

      /**
       * @brief Iterator end().
       *
       * @details
       * As per C++ conventions, this is the address **after** the last
       * node element, that should never be reached.
       *
       * @return An iterator pointing to the end of the list.
       */
      iterator_t
      children_threads_iter_end (thread_addr_t ta)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)\n", __func__, ta);
#endif /* defined(DEBUG) */

        iterator_t it = children_threads_get_list (ta);

#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)=0x%08X\n", __func__, ta, it);
#endif /* defined(DEBUG) */

        return it;
      }

      /**
       * @brief Get the thread pointed by the iterator.
       *
       * @details
       * The iterator is actually a pointer to the list node.
       * The list is intrusive in the object; somewhere in the middle of it
       * there is a list node, pointing to other list nodes, not to the object.
       *
       * To get the thread address, subtract the list node offset.
       *
       * @return The thread target address.
       */
      thread_addr_t
      children_threads_iter_get (iterator_t it)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)\n", __func__, it);
#endif /* defined(DEBUG) */

        list_node_addr_t list_node_addr = it;

#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)=0x%08X\n", __func__, it,
            list_node_addr - metadata_.thread.list_node_offset);
#endif /* defined(DEBUG) */

        return list_node_addr - metadata_.thread.list_node_offset;
      }

      /**
       * @brief Advance the iterator to the next thread.
       *
       * @details
       * The iterator points to a list node, a structure with two pointers,
       * prev & next, so advancing means getting the next pointer.
       *
       * @return An iterator pointing to the next thread.
       */
      iterator_t
      children_threads_iter_next (iterator_t it)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)\n", __func__, it);
#endif /* defined(DEBUG) */

        iterator_t next = 0;
        int ret;
        ret = backend_.read_long (it + metadata_.list_links.next_offset, &next);
        if (ret < 0)
          {
            backend_.output_error (
                "Could not read 'list_links.next_offset'.\n");
          }

#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)=0x%08X\n", __func__, it, next);
#endif /* defined(DEBUG) */

        return next;
      }

    private:

      backend_type& backend_;
      metadata_type& metadata_;
      threads_type& threads_;

      allocator_type& allocator_;

    };
} /* namespace drtm */

#endif /* defined(__cplusplus) */

#endif /* DRTM_RUN_TIME_DATA_H_ */
