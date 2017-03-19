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

#ifndef DRTM_THREADS_H_
#define DRTM_THREADS_H_

#if defined(__cplusplus)

#include <drtm/types.h>

#include <vector>
#include <memory>
#include <cassert>
#include <cstring>

// Initial reservation for the threads collection.
#define THREADS_ALLOCATED_SIZE_POINTERS   20

// TODO: make this configurable
#define STACK_CONTEXT_REGISTERS_SIZE_WORDS    50

namespace drtm
{
  /**
   * @brief A class template to store the information related to a thread.
   *
   * @details
   * The first purpose is to be able to compose a meaningful description,
   * using the thread name, status, priorities, etc.
   *
   * The second purpose is to store a copy of the registers, retrieved
   * from the thread context.
   */
  template<typename B, typename A>
    class thread
    {
    public:

      using backend_type = B;
      using allocator_type = A;

      // This comes from types.h
      using addr_t = target_addr_t;

      // Address of a target thread.
      using thread_addr_t = addr_t;

      using thread_id_t = uint32_t;

    public:

      // Thread ID when the scheduler is not started.
      static constexpr thread_id_t id_none = 0;

      static constexpr std::size_t name_max_size_bytes = 256;

      static constexpr const char* default_description = "none";

      static constexpr std::size_t register_size_bytes = 4;

    public:

      /**
       * @brief Construct a thread object instance.
       */
      thread (backend_type& backend, allocator_type& allocator) :
          backend_
            { backend }, //
          allocator_
            { allocator }
      {
#if defined(DEBUG)
        printf ("%s(%p, %p) @%p\n", __func__, &backend, &allocator, this);
#endif /* defined(DEBUG) */

        clear ();
      }

      // The rule of five.
      thread (const thread&) = delete;
      thread (thread&&) = delete;
      thread&
      operator= (const thread&) = delete;
      thread&
      operator= (thread&&) = delete;

      /**
       * @brief Destruct the thread object instance.
       */
      ~thread ()
      {
#if defined(DEBUG)
        printf ("%s() @%p\n", __func__, this);
#endif /* defined(DEBUG) */
      }

    public:

      // Accessors and mutators.

      inline addr_t
      addr ()
      {
        return addr_;
      }

      void
      addr (addr_t a)
      {
        addr_ = a;

        // Since currently threads do not have an explicit ID (to be fixed),
        // synthesize the ID from the address (which is known to be unique).
        if ((addr_ & 0x3) != 0)
          {
            backend_.output_warning ("Thread address 0x%08X not aligned.\n",
                                     addr_);
          }
        id_ = (addr_ >> 2);
      }

      inline thread_id_t
      id ()
      {
        return id_;
      }

      inline void
      id (thread_id_t tid)
      {
        id_ = tid;
      }

      void
      clear (void)
      {
#if defined(DEBUG)
        printf ("%s() @%p\n", __func__, this);
#endif /* defined(DEBUG) */

        // DO NOT clear the entire object, this will destroy references;
        // instead, clear individual members.
        addr_ = 0;
        id_ = 0;

        name[0] = '\0';
        prio_assigned = 0;
        prio_inherited = 0;
        state = 0;

        // Clearing the entire stack is ok, no references inside.
        std::memset (&stack, 0, sizeof(stack));
      }

      /**
       * @brief Compose the thread description, from name, state and priority.
       *
       * @details
       * If inherited priority is different from assigned priority,
       * the later is shown in parenthesis.
       *
       * @return Number of characters in description, excluding '\0'.
       */
      int
      prepare_description (char* out_description)
      {
        const char* st;
        if (state < sizeof(thread_states) / sizeof(thread_states[0]))
          {
            st = thread_states[state];
          }
        else
          {
            st = "?";
          }

        int count = 0;
        char* out = out_description;

        count += snprintf (out + count, 256 - count, "%s [S:%s, P:", name, st);

        if (prio_inherited > prio_assigned)
          {
            count += snprintf (out + count, 256 - count, "%u(%u)",
                               prio_inherited, prio_assigned);
          }
        else
          {
            count += snprintf (out + count, 256 - count, "%u", prio_assigned);
          }

        if (stack.is_floating_point)
          {
            count += snprintf (out + count, 256 - count, ", FP");
          }

        count += snprintf (out + count, 256 - count, "]");

        return count;
      }

      /**
       * Serialize a register, one byte at a time.
       * Endianness is ensured, registers were read-in also one byte at a time
       * and so the order is not changed.
       *
       * @return The next free position in the output string.
       */
      char*
      output_register (uint32_t reg_index, char* out)
      {
        assert(stack.info != nullptr);
        for (int j = 0; j < 4; ++j)
          {
            register_offset_t offset = stack.info->offsets[reg_index];
            if (offset == -1)
              {
                out += snprintf (out, 3, "%02X", 0);
              }
            else if (offset == -2)
              {
                // SP is displayed separately, not from the thread context,
                // but from the TCB, it is fetched for each thread.
                out += snprintf (out, 3, "%02X", stack.sp_addr[j]);
              }
            else
              {
                out += snprintf (
                    out, 3, "%02X",
                    stack.context[offset * register_size_bytes + j]);
              }
          }

        return out;
      }

      // ----------------------------------------------------------------------

      /**
       * @brief Read registers from the stack context to a byte array.
       */
      void
      read_stack (void)
      {
        if (stack.has_registers)
          {
            return;
          }

#if defined(DEBUG)
        printf ("%s() @%p\n", __func__, this);
#endif /* defined(DEBUG) */

        const stack_info_t* si = stack.info;

        assert(sizeof(stack.context) >= si->in_registers * register_size_bytes);

        // Registers are read one byte at a time, in ascending memory order.
        backend_.read_byte_array (stack.addr, &stack.context[0],
                                  si->in_registers * register_size_bytes);

#if defined(DEBUG)
        printf ("in ");
        for (int i = 0; i < si->in_registers * register_size_bytes; i++)
          {
            if (i % 4 == 0)
              {
                printf (" ");
              }
            printf ("%02X", stack.context[i]);
          }
        printf ("\n");
#endif /* defined(DEBUG) */

        stack.has_registers = true;
      }

      // ----------------------------------------------------------------------

    private:

      backend_type& backend_;
      allocator_type& allocator_;

      addr_t addr_ = 0;
      thread_id_t id_ = 0;

    public:

      char name[name_max_size_bytes] = "";
      uint8_t prio_assigned = 0;
      uint8_t prio_inherited = 0;
      uint8_t state = 0;

      struct stack_s
      {
        addr_t addr;
        bool has_registers;
        bool is_floating_point;
        const stack_info_t* info;
        uint8_t context[STACK_CONTEXT_REGISTERS_SIZE_WORDS * register_size_bytes];
        uint8_t sp_addr[register_size_bytes];
      } stack;

    public:

      static const char* thread_states[];

    };

  // --------------------------------------------------------------------------

  /**
   * @brief A class template to manage a collection (an array)
   * of pointers o threads.
   */
  template<typename B, typename A>
    class threads
    {

    public:

      using backend_type = B;
      using allocator_type = A;

      using thread_type = class thread<B, A>;

      // Make a new allocator, for threads.
      using thread_allocator_type =
      typename std::allocator_traits<allocator_type>::template rebind_alloc<thread_type>;

      // Make a new allocator, for pointers to threads.
      using vector_allocator_type =
      typename std::allocator_traits<allocator_type>::template rebind_alloc<thread_type*>;

      using collection_type = class std::vector<thread_type*, vector_allocator_type>;

      using thread_id_t = typename thread_type::thread_id_t;
      using thread_addr_t = typename thread_type::thread_addr_t;

      // Standard types, inherited from container (vector).

      using value_type = typename collection_type::value_type;
      // using allocator_type = typename collection_type::allocator_type;
      using size_type = typename collection_type::size_type;
      using difference_type = typename collection_type::difference_type;
      using reference = typename collection_type::reference;
      using const_reference = typename collection_type::const_reference;
      using pointer = typename collection_type::pointer;
      using const_pointer = typename collection_type::const_pointer;
      using iterator = typename collection_type::iterator;
      using const_iterator = typename collection_type::const_iterator;
      using reverse_iterator = typename collection_type::reverse_iterator;
      using const_reverse_iterator = typename collection_type::const_reverse_iterator;

    public:

      /**
       * @brief Construct the threads collection object instance.
       */
      threads (backend_type& backend, allocator_type& allocator) :
          backend_
            { backend }, //
          allocator_
            { allocator }
      {
#if defined(DEBUG)
        printf ("%s(%p, %p) @%p\n", __func__, &backend, &allocator, this);
#endif /* defined(DEBUG) */

        threads_.reserve (THREADS_ALLOCATED_SIZE_POINTERS);

        clear ();
      }

      // The rule of five.
      threads (const threads&) = delete;
      threads (threads&&) = delete;
      threads&
      operator= (const threads&) = delete;
      threads&
      operator= (threads&&) = delete;

      /**
       * @brief Destruct the threads collection object instance.
       */
      ~threads ()
      {
#if defined(DEBUG)
        printf ("%s() @%p\n", __func__, this);
#endif /* defined(DEBUG) */

        for (auto* th : threads_)
          {
            delete_thread (th);
          }

        threads_.clear ();
      }

    public:

      // ----------------------------------------------------------------------
      // Vector methods.

      /**
       * @brief Clear the collection of threads.
       *
       * @details
       * Threads are not deallocated, but reused.
       */
      void
      clear (void)
      {
        count_ = 0;
        current_ = nullptr;
      }

      /**
       * @brief Get the number of threads in the collection.
       */
      inline std::size_t
      size (void)
      {
        return count_;
      }

      /**
       * @brief Return a reference to the element at specified position.
       */
      inline reference
      operator[] (size_type pos)
      {
        return threads_[pos];
      }

      inline iterator
      begin ()
      {
        return threads_.begin ();
      }

      inline iterator
      end ()
      {
        return threads_.end ();
      }

      // ----------------------------------------------------------------------
      // New & Delete.

      /**
       * @brief Allocate and construct a new thread object instance.
       */
      thread_type*
      new_thread (void)
      {
        if (count_ < threads_.size ())
          {
            auto* th = threads_[count_];
            th->clear ();

            ++count_;

            return th;
          }

        auto* th = std::allocator_traits<thread_allocator_type>::allocate (
            reinterpret_cast<thread_allocator_type&> (allocator_), 1);

        // Call the constructor.
        new (th) thread_type (backend_, allocator_);
        threads_.push_back (th);

        ++count_;

        return th;
      }

      /**
       * @brief Destruct and deallocate the thread object instance.
       */
      void
      delete_thread (thread_type* th)
      {
        // Call the destructor.
        th->~thread_type ();

        std::allocator_traits<thread_allocator_type>::deallocate (
            reinterpret_cast<thread_allocator_type&> (allocator_), th, 1);
      }

      /**
       * @brief Get the current thread.
       */
      inline thread_type*
      current (void)
      {
        return current_;
      }

      inline void
      current (thread_type* th)
      {
        current_ = th;
      }

      /**
       * @brief Check if a given thread is the current thread.
       */
      bool
      is_current (thread_id_t tid)
      {
        if (current_ == nullptr)
          {
            return false;
          }

        return (current_->id () == tid);
      }

      /**
       * @brief Get a thread by its ID.
       */
      thread_type*
      thread (thread_id_t tid)
      {
        for (auto* th : threads_)
          {
            if (th->id () == tid)
              {
                return th;
              }
          }

        return nullptr;
      }

    private:

      backend_type& backend_;
      allocator_type& allocator_;

      thread_type* current_ = nullptr;

      std::size_t count_ = 0;

      // A collection (vector) of pointers to
      // separately allocated thread objects.
      collection_type threads_
        { (vector_allocator_type&) allocator_ };

    };

  // --------------------------------------------------------------------------

  template<typename B, typename A>
    const char* thread<B, A>::thread_states[6] =
      {
      //
          "Undefined",//
          "Ready", //
          "Running", //
          "Suspended", //
          "Terminated", //
          "Destroyed"
      /**/
      };

// ----------------------------------------------------------------------------
} /* namespace drtm */

#endif /* defined(__cplusplus) */

#endif /* DRTM_THREADS_H_ */
