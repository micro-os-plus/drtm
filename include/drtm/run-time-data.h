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
#include <drtm/metadata.h>
#include <drtm/threads.h>

#include <memory>

namespace drtm
{

  /**
   * A class template to manage the run-time data, like iterate through
   * the thread lists, tell if scheduler started, etc.
   */
  template<typename B, typename A>
    class run_time_data
    {
    public:

      using backend_type = B;
      using allocator_type = A;

      using metadata_type = class metadata<B>;
      using threads_type = class threads<B, A>;

      using thread_type = typename threads_type::thread_type;

      using thread_addr_t = typename thread_type::thread_addr_t;

      using addr_t = typename backend_type::target_addr_t;

      // Address of a target list node.
      using list_node_addr_t = addr_t;

      // Target iterator, an object that includes a single target pointer.
      using iterator = addr_t;

      // Make a new allocator, for characters.
      using char_allocator_type =
      typename std::allocator_traits<allocator_type>::template rebind_alloc<char>;

    public:

      run_time_data (backend_type& backend, metadata_type& metadata,
                     threads_type& threads, allocator_type& allocator) :
          backend_ (backend), // Parenthesis used to compile with 4.8
          metadata_ (metadata), //
          threads_ (threads), //
          allocator_ (allocator)
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
                                  reinterpret_cast<uint8_t*> (&ret));
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
        threads_.clear ();
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

        iterator it = children_threads_iter_begin (ta);
        iterator end = children_threads_iter_end (ta);

        while (it != end)
          {
            // Get the pointer to the thread from the iterator.
            thread_addr_t thread_addr = children_threads_iter_get (it);

            thread_type* th = threads_.new_thread ();

            // Remember the thread address, it is used to determine the
            // current thread.
            // This will also set the ID.
            th->addr (thread_addr);

            // Get the address of the name string.
            int ret;
            addr_t name_addr = 0;
            ret = backend_.read_long (
                thread_addr + metadata_.thread.name_offset, &name_addr);
            if (ret < 0)
              {
                backend_.output_error ("Could not read 'thread.name*'.\n");
              }

            addr_t addr;
            uint8_t b = 0;

            // Copy the name, byte by byte. A large array copy is risky, since the
            // thread might be allocated right at the end of RAM, and
            // the copy might try to access past the limit.
            if (name_addr != 0)
              {
                addr = name_addr;
                char* p = &th->name[0];
                std::size_t count = 0;
                while (count < thread_type::name_max_size_bytes - 1)
                  {
                    ret = backend_.read_byte (addr, &b);
                    if (ret < 0)
                      {
                        backend_.output_error (
                            "Could not read 'thread.name'.\n");
                        break;
                      }
                    *p = static_cast<char> (b);
                    ++p;
                    ++addr;
                    ++count;
                    if (b == '\0')
                      {
                        break;
                      }
                  }
                *p = '\0'; // Be sure the string is terminated.
              }

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

            uint32_t exc_return = 0;
            ret = backend_.read_long (
                static_cast<addr_t> (th->stack.addr
                    + (metadata_.thread.stack_exc_offset_words
                        * thread_type::register_size_bytes)),
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
                th->stack.info = &cortex_m4_vfp_stack_info;
                th->stack.is_floating_point = true;
              }
            else
              {
                th->stack.info = &cortex_m4_stack_info;
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
      iterator
      children_threads_iter_begin (thread_addr_t ta)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)\n", __func__, th);
#endif /* defined(DEBUG) */

        list_node_addr_t list_node_addr = children_threads_get_list (ta);
        iterator it = 0;
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
      iterator
      children_threads_iter_end (thread_addr_t ta)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)\n", __func__, ta);
#endif /* defined(DEBUG) */

        iterator it = children_threads_get_list (ta);

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
      children_threads_iter_get (iterator it)
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
      iterator
      children_threads_iter_next (iterator it)
      {
#if defined(DEBUG) && defined(DEBUG_LISTS)
        printf ("%s(0x%08X)\n", __func__, it);
#endif /* defined(DEBUG) */

        iterator next = 0;
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

    public:

      static const register_offset_t cortex_m4_stack_offsets[];
      static const stack_info_t cortex_m4_stack_info;
      static const register_offset_t cortex_m4_vfp_stack_offsets[];
      static const stack_info_t cortex_m4_vfp_stack_info;
    };

  // ---------------------------------------------------------------------------
  // Local data structures.

  // Non FP stack context, 17 words.
  // Offsets in words, from SP up.

  // Saved always by ARM.
  // (17 optional padding/aligner)
  // 16 xPSR (xPSR bit 9 = 1 if padded)
  // 15 return address (PC, R15)
  // 14 LR (R14)
  // 13 R12
  // 12 R3
  // 11 R2
  // 10 R1
  //  9 R0

  // Saved always by context switch handler.
  // "stmdb %[r]!, {r4-r9,sl,fp,lr}"
  //  8 EXC_RETURN (R14)
  //  7 FP (R11)
  //  6 SL (R10)
  //  5 R9
  //  4 R8
  //  3 R7
  //  2 R6
  //  1 R5
  //  0 R4 <-- new SP value

  /*
   <!DOCTYPE feature SYSTEM "gdb-target.dtd">
   <target version="1.0">
   <architecture>arm</architecture>
   <feature name="org.gnu.gdb.arm.m-profile">
   <reg name="r0" bitsize="32" regnum="0" type="uint32" group="general"/>
   <reg name="r1" bitsize="32" regnum="1" type="uint32" group="general"/>
   <reg name="r2" bitsize="32" regnum="2" type="uint32" group="general"/>
   <reg name="r3" bitsize="32" regnum="3" type="uint32" group="general"/>
   <reg name="r4" bitsize="32" regnum="4" type="uint32" group="general"/>
   <reg name="r5" bitsize="32" regnum="5" type="uint32" group="general"/>
   <reg name="r6" bitsize="32" regnum="6" type="uint32" group="general"/>
   <reg name="r7" bitsize="32" regnum="7" type="uint32" group="general"/>
   <reg name="r8" bitsize="32" regnum="8" type="uint32" group="general"/>
   <reg name="r9" bitsize="32" regnum="9" type="uint32" group="general"/>
   <reg name="r10" bitsize="32" regnum="10" type="uint32" group="general"/>
   <reg name="r11" bitsize="32" regnum="11" type="uint32" group="general"/>
   <reg name="r12" bitsize="32" regnum="12" type="uint32" group="general"/>
   <reg name="sp" bitsize="32" regnum="13" type="data_ptr" group="general"/>
   <reg name="lr" bitsize="32" regnum="14" type="uint32" group="general"/>
   <reg name="pc" bitsize="32" regnum="15" type="code_ptr" group="general"/>
   <reg name="xpsr" bitsize="32" regnum="25" type="uint32" group="general"/>
   </feature>
   <feature name="org.gnu.gdb.arm.m-system">
   <reg name="msp" bitsize="32" regnum="26" type="uint32" group="general"/>
   <reg name="psp" bitsize="32" regnum="27" type="uint32" group="general"/>
   <reg name="primask" bitsize="32" regnum="28" type="uint32" group="general"/>
   <reg name="basepri" bitsize="32" regnum="29" type="uint32" group="general"/>
   <reg name="faultmask" bitsize="32" regnum="30" type="uint32" group="general"/>
   <reg name="control" bitsize="32" regnum="31" type="uint32" group="general"/>
   </feature>
   </target>
   */

  // This table encodes the offsets in the µOS++ non-VFP stack frame for the
  // registers defined in the previous XML.
  // Offsets are from the saved SP to each register, in 32-bits words.
  //
  // Special cases:
  // -1 return as 0x00000000
  // -2 SP should be taken from TCB, not from the context.
  template<typename B, typename A>
    const register_offset_t run_time_data<B, A>::cortex_m4_stack_offsets[] =
      {
      //
          9,// R0
          10, // R1
          11, // R2
          12, // R3
          0, // R4
          1, // R5
          2, // R6
          3, // R7
          4, // R8
          5, // R9
          6, // R10
          7, // R11
          13, // R12
          -2, // SP
          14, // LR
          15, // PC
          16, // XPSR

          -1, // MSP
          -1, // PSP
          -1, // PRIMASK
          -1, // BASEPRI
          -1, // FAULTMASK
          -1, // CONTROL
        };

  template<typename B, typename A>
    const stack_info_t run_time_data<B, A>::cortex_m4_stack_info =
      {
      //
          .in_registers = 16 + 1 + 1, // R0-R15 + EXC_RETURN, +XPSR
          .out_registers = 16 + 1, // R0-R15 + xPSR
          .offsets = cortex_m4_stack_offsets, //
          .offsets_size = sizeof(cortex_m4_stack_offsets)
              / sizeof(cortex_m4_stack_offsets[0]),
      /**/
      };

  // FP stack context, 50 words.
  // Offsets in words, from SP up.

  // Saved always by ARM.
  // (50 optional padding/aligner)
  // 49 FPSCR
  // 48 S15
  // ...
  // 34 S1
  // 33 S0
  // 32 xPSR (xPSR bit 9 = 1 if padded)
  // 31 return address (PC, R15)
  // 30 LR (R14)
  // 29 R12
  // 28 R3
  // 27 R2
  // 26 R1
  // 25 R0

  // Saved conditionally if EXC_RETURN, bit 4 is 0 (zero).
  // "vldmiaeq %[r]!, {s16-s31}"
  // 24 S31
  // 23 S30
  // ...
  // 10 S17
  //  9 S16

  // Saved always by context switch handler.
  // "stmdb %[r]!, {r4-r9,sl,fp,lr}"
  //  8 EXC_RETURN (R14)
  //  7 FP (R11)
  //  6 SL (R10)
  //  5 R9
  //  4 R8
  //  3 R7
  //  2 R6
  //  1 R5
  //  0 R4 <-- new SP value

  /*
   <!DOCTYPE feature SYSTEM "gdb-target.dtd">
   <target version="1.0">
   <architecture>arm</architecture>
   <feature name="org.gnu.gdb.arm.m-profile">
   <reg name="r0" bitsize="32" regnum="0" type="uint32" group="general"/>
   <reg name="r1" bitsize="32" regnum="1" type="uint32" group="general"/>
   <reg name="r2" bitsize="32" regnum="2" type="uint32" group="general"/>
   <reg name="r3" bitsize="32" regnum="3" type="uint32" group="general"/>
   <reg name="r4" bitsize="32" regnum="4" type="uint32" group="general"/>
   <reg name="r5" bitsize="32" regnum="5" type="uint32" group="general"/>
   <reg name="r6" bitsize="32" regnum="6" type="uint32" group="general"/>
   <reg name="r7" bitsize="32" regnum="7" type="uint32" group="general"/>
   <reg name="r8" bitsize="32" regnum="8" type="uint32" group="general"/>
   <reg name="r9" bitsize="32" regnum="9" type="uint32" group="general"/>
   <reg name="r10" bitsize="32" regnum="10" type="uint32" group="general"/>
   <reg name="r11" bitsize="32" regnum="11" type="uint32" group="general"/>
   <reg name="r12" bitsize="32" regnum="12" type="uint32" group="general"/>
   <reg name="sp" bitsize="32" regnum="13" type="data_ptr" group="general"/>
   <reg name="lr" bitsize="32" regnum="14" type="uint32" group="general"/>
   <reg name="pc" bitsize="32" regnum="15" type="code_ptr" group="general"/>
   <reg name="xpsr" bitsize="32" regnum="25" type="uint32" group="general"/>
   </feature>
   <feature name="org.gnu.gdb.arm.m-system">
   <reg name="msp" bitsize="32" regnum="26" type="uint32" group="general"/>
   <reg name="psp" bitsize="32" regnum="27" type="uint32" group="general"/>
   <reg name="primask" bitsize="32" regnum="28" type="uint32" group="general"/>
   <reg name="basepri" bitsize="32" regnum="29" type="uint32" group="general"/>
   <reg name="faultmask" bitsize="32" regnum="30" type="uint32" group="general"/>
   <reg name="control" bitsize="32" regnum="31" type="uint32" group="general"/>
   </feature>
   <feature name="org.gnu.gdb.arm.m-float">
   <reg name="fpscr" bitsize="32" regnum="32" type="uint32" group="float"/>
   <reg name="s0" bitsize="32" regnum="33" type="float" group="float"/>
   <reg name="s1" bitsize="32" regnum="34" type="float" group="float"/>
   <reg name="s2" bitsize="32" regnum="35" type="float" group="float"/>
   <reg name="s3" bitsize="32" regnum="36" type="float" group="float"/>
   <reg name="s4" bitsize="32" regnum="37" type="float" group="float"/>
   <reg name="s5" bitsize="32" regnum="38" type="float" group="float"/>
   <reg name="s6" bitsize="32" regnum="39" type="float" group="float"/>
   <reg name="s7" bitsize="32" regnum="40" type="float" group="float"/>
   <reg name="s8" bitsize="32" regnum="41" type="float" group="float"/>
   <reg name="s9" bitsize="32" regnum="42" type="float" group="float"/>
   <reg name="s10" bitsize="32" regnum="43" type="float" group="float"/>
   <reg name="s11" bitsize="32" regnum="44" type="float" group="float"/>
   <reg name="s12" bitsize="32" regnum="45" type="float" group="float"/>
   <reg name="s13" bitsize="32" regnum="46" type="float" group="float"/>
   <reg name="s14" bitsize="32" regnum="47" type="float" group="float"/>
   <reg name="s15" bitsize="32" regnum="48" type="float" group="float"/>
   <reg name="s16" bitsize="32" regnum="49" type="float" group="float"/>
   <reg name="s17" bitsize="32" regnum="50" type="float" group="float"/>
   <reg name="s18" bitsize="32" regnum="51" type="float" group="float"/>
   <reg name="s19" bitsize="32" regnum="52" type="float" group="float"/>
   <reg name="s20" bitsize="32" regnum="53" type="float" group="float"/>
   <reg name="s21" bitsize="32" regnum="54" type="float" group="float"/>
   <reg name="s22" bitsize="32" regnum="55" type="float" group="float"/>
   <reg name="s23" bitsize="32" regnum="56" type="float" group="float"/>
   <reg name="s24" bitsize="32" regnum="57" type="float" group="float"/>
   <reg name="s25" bitsize="32" regnum="58" type="float" group="float"/>
   <reg name="s26" bitsize="32" regnum="59" type="float" group="float"/>
   <reg name="s27" bitsize="32" regnum="60" type="float" group="float"/>
   <reg name="s28" bitsize="32" regnum="61" type="float" group="float"/>
   <reg name="s29" bitsize="32" regnum="62" type="float" group="float"/>
   <reg name="s30" bitsize="32" regnum="63" type="float" group="float"/>
   <reg name="s31" bitsize="32" regnum="64" type="float" group="float"/>
   </feature>
   </target>
   */

  // This table encodes the offsets in the µOS++ VFP stack frame for the
  // registers defined in the previous XML.
  // Used conditionally if EXC_RETURN, bit 4 is 0 (zero).
  template<typename B, typename A>
    const register_offset_t run_time_data<B, A>::cortex_m4_vfp_stack_offsets[] =
      {
      //
          25,//R0
          26, // R1
          27, // R2
          28, // R3
          0, // R4
          1, // R5
          2, // R6
          3, // R7
          4, // R8
          5, // R9
          6, // R10
          7, // R11
          29, // R12
          -2, // SP
          30, // LR
          31, // PC
          32, // XPSR

          -1, // MSP
          -1, // PSP
          -1, // PRIMASK
          -1, // BASEPRI
          -1, // FAULTMASK
          -1, // CONTROL
          49, // FPSCR
          33, // S0
          34, // S1
          35, // S2
          36, // S3
          37, // S4
          38, // S5
          39, // S6
          40, // S7
          41, // S8
          42, // S9
          43, // S10
          44, // S11
          45, // S12
          46, // S13
          47, // S14
          48, // S15
          9, // S16
          10, // S17
          11, // S18
          12, // S19
          13, // S20
          14, // S21
          15, // S22
          16, // S23
          17, // S24
          18, // S25
          19, // S26
          20, // S27
          21, // S28
          22, // S29
          23, // S30
          24, // S31
        };

  template<typename B, typename A>
    const stack_info_t run_time_data<B, A>::cortex_m4_vfp_stack_info =
      {
      //
          .in_registers = 16 + 1 + 1 + 32 + 1, // R0-R15 + EXC_RETURN +XPSR + S0-S31 + FPSCR
          .out_registers = 16 + 1, // R0-R15 + xPSR
          .offsets = cortex_m4_vfp_stack_offsets, //
          .offsets_size = sizeof(cortex_m4_vfp_stack_offsets)
              / sizeof(cortex_m4_vfp_stack_offsets[0]),
      /**/
      };

// ---------------------------------------------------------------------------
} /* namespace drtm */

#endif /* defined(__cplusplus) */

#endif /* DRTM_RUN_TIME_DATA_H_ */
