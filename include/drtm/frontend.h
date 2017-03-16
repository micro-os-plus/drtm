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

#ifndef DRTM_FRONTEND_H_
#define DRTM_FRONTEND_H_

#if defined(__cplusplus)

#include <drtm/types.h>
#include <stdio.h>

#include <cassert>
#include <cstring>

namespace drtm
{

  template<typename B, typename M, typename T, typename R, typename A>
    class frontend
    {
    public:

      using backend_type = B;
      using metadata_type = M;
      using threads_type = T;
      using rtd_type = R;
      using allocator_type = A;

      using thread_type = typename threads_type::thread_type;
      using thread_id_t = typename thread_type::thread_id_t;

    public:

      frontend (backend_type& backend, allocator_type& allocator) :
          backend_
            { backend }, //
          allocator_
            { allocator }
      {
#if defined(DEBUG)
        printf ("%s(%p, %p) @%p\n", __func__, &backend, &allocator, this);
#endif /* defined(DEBUG) */
      }

      // The rule of five.
      frontend (const frontend&) = delete;
      frontend (frontend&&) = delete;
      frontend&
      operator= (const frontend&) = delete;
      frontend&
      operator= (frontend&&) = delete;

      ~frontend () = default;

    public:

      /**
       * @brief Update the thread information from the target.
       *
       * @details
       * For efficiency purposes, the plug-in should read all required
       * information within this function at once, so later requests can
       * be served without further communication to the target.
       *
       * @par Parameters
       *  None.
       *
       * @retval 0 Updating threads OK.
       * @retval <0 Updating threads failed.
       */
      int
      update_thread_list (void)
      {
#if defined(DEBUG)
        printf ("%s()\n", __func__);
#endif /* defined(DEBUG) */

        if (!metadata_.parse ())
          {
#if defined(DEBUG)
            printf ("%s()=-1 no drtm\n", __func__);
#endif /* defined(DEBUG) */
            return -1;
          }

        is_scheduler_started_ = rt_.is_scheduler_started ();

        if (!is_scheduler_started_)
          {
            threads_.clear ();

#if defined(DEBUG)
            printf ("%s()=0 no scheduler\n", __func__);
#endif /* defined(DEBUG) */
            return 0;
          }

        rt_.update_threads ();

        return 0;
      }

      /**
       * @brief Get the number of threads.
       *
       * @details
       * After calling this function, the GDB server will request the
       * thread ID by calling RTOS_GetThreadId() for every thread.
       *
       * @par Parameters
       *  None.
       *
       * @return The number of threads.
       */
      uint32_t
      get_threads_count (void)
      {
#if defined(DEBUG)
        printf ("%s()\n", __func__);
#endif /* defined(DEBUG) */

        if (!is_scheduler_started_)
          {
#if defined(DEBUG)
            printf ("%s()=0 no scheduler\n", __func__);
#endif /* defined(DEBUG) */
            return 0;
          }

#if defined(DEBUG)
        printf ("%s()=%zu\n", __func__, threads_.size ());
#endif /* defined(DEBUG) */

        return threads_.size ();
      }

      /**
       * @brief Get the ID of the thread by index.
       *
       * @details
       * Index numbers for threads run from 0..[m-1], where m is the
       * number of threads returned by RTOS_GetNumThreads().
       *
       * @param [in] index Index of the thread; expected to be valid.
       *
       * @return The ID of the thread.
       */
      thread_id_t
      get_thread_id (uint32_t index)
      {
#if defined(DEBUG)
        printf ("%s(%u)\n", __func__, index);
#endif /* defined(DEBUG) */

        assert(index < threads_.size ());
#if defined(DEBUG)
        printf ("%s(%u)=%u\n", __func__, index, threads_[index]->id ());
#endif /* defined(DEBUG) */

        return threads_[index]->id ();
      }

      /**
       * @brief Get the ID of the currently running thread.
       *
       * @details
       * For single core devices, there is only one thread running
       * and the scheduler obviously knows it.
       *
       * @par Parameters
       *  None.
       *
       * @return The ID of the currently running thread.
       */
      thread_id_t
      get_current_thread_id (void)
      {
#if defined(DEBUG)
        printf ("%s()\n", __func__);
#endif /* defined(DEBUG) */

        if (!is_scheduler_started_)
          {
#if defined(DEBUG)
            printf ("%s()=%d no scheduler\n", __func__, thread_type::id_none);
#endif /* defined(DEBUG) */

            return thread_type::id_none;
          }

        if (threads_.current () == nullptr)
          {
#if defined(DEBUG)
            printf ("%s()=%d null\n", __func__, thread_type::id_none);
#endif /* defined(DEBUG) */

            return thread_type::id_none;
          }

#if defined(DEBUG)
        printf ("%s()=%u\n", __func__, threads_.current ()->id ());
#endif /* defined(DEBUG) */

        return threads_.current ()->id ();
      }

      /**
       * @brief Get the printable thread name.
       *
       * @details
       * The name may contain extra information about the tread’s
       * status (running/suspended, priority, etc.).
       *
       * @param [in] tid ID of the thread.
       * @param [out] out_description Pointer to the string where the name
       *  has to be copied to; the space reserved for the name is 256 bytes
       *  (including terminating zero).
       *
       * @return The length of the name string.
       */
      int
      get_thread_description (thread_id_t tid, char* out_description)
      {
#if defined(DEBUG)
        printf ("%s(*, %u)\n", __func__, tid);
#endif /* defined(DEBUG) */

        assert(out_description != NULL);

        thread_type* td = threads_.thread (tid);
        int count = 0;
        if (is_scheduler_started_ && td != nullptr)
          {
            count = td->prepare_description (out_description);
          }
        else
          {
            assert(std::strlen (thread_type::default_description) < 256);
            std::strcpy (out_description, thread_type::default_description);
            count = std::strlen (thread_type::default_description);
          }

#if defined(DEBUG)
        printf ("%s(*, %u)='%s'\n", __func__, tid, out_description);
#endif /* defined(DEBUG) */

        return count;
      }

      /**
       * @brief Get the thread’s register value as HEX string.
       *
       * @details
       * If the register value has to be read directly from the CPU,
       * the function must return a value <0. The register value is then
       * read from the CPU by the GDB server itself.
       *
       * @param [in] tid ID of the thread.
       * @param [in] reg_index Index of the register.
       * @param [out] out_hex_value Pointer to the string, the value has
       *  to be copied to.
       *
       * @retval 0 Reading register OK.
       * @retval <0 Reading register failed.
       */
      int
      get_thread_register (thread_id_t tid, uint32_t reg_index,
                           char* out_hex_value)
      {
#if defined(DEBUG)
        printf ("%s(*, %u, %u)\n", __func__, reg_index, tid);
#endif /* defined(DEBUG) */

        if (!is_scheduler_started_)
          {
            // No scheduler, GDB should use current registers.
#if defined(DEBUG)
            printf ("%s(*, %u, %u)=-1 no scheduler\n", __func__, reg_index,
                    tid);
#endif /* defined(DEBUG) */
            return -1;
          }

        if (tid == thread_type::id_none || threads_.is_current (tid))
          {
            // Current thread, GDB should use current CPU registers.
#if defined(DEBUG)
            printf ("%s(*, %u, %u)=-1 current thread\n", __func__, reg_index,
                    tid);
#endif /* defined(DEBUG) */
            return -1;
          }

        thread_type* td = threads_.thread (tid);
        assert(td != NULL);

        td->read_stack ();

        // Note: The FP registers are not returned, only the main registers.

        const stack_info_t* si = td->stack.info;
        assert(si != NULL);

        char* out = out_hex_value;
        if (reg_index < si->out_registers)
          {
#if defined(DEBUG)
            printf ("out ");
#endif /* defined(DEBUG) */

            out = td->output_register (reg_index, out);

#if defined(DEBUG)
            printf ("%s \n", out - 8);
#endif /* defined(DEBUG) */

            return 0;
          }

        int ret = -1;
        printf ("%s(*, %u, %u)=%d outside range\n", __func__, reg_index, tid,
                ret);

        return ret;
      }

      /**
       * @brief Get the thread's general registers as HEX string.
       *
       * @details
       * If the register values have to be read directly from the CPU,
       * the function must return a value <0. The register values are then
       * read from the CPU by the GDB server itself.
       *
       * @param [in] thread_id ID of the thread.
       * @param [out] out_hex_values Pointer to the string, the values
       *  have to be copied to.
       *
       * @retval 0 Reading registers OK.
       * @retval <0 Reading register failed.
       */
      int
      get_thread_registers (thread_id_t tid, char* out_hex_values)
      {
#if defined(DEBUG)
        printf ("%s(*, %u)\n", __func__, tid);
#endif /* defined(DEBUG) */

        if (!is_scheduler_started_)
          {
            // No scheduler, GDB should use current registers.
#if defined(DEBUG)
            printf ("%s(*, %u)=-1 no scheduler\n", __func__, tid);
#endif /* defined(DEBUG) */
            return -1;
          }

        if (tid == thread_type::id_none || threads_.is_current (tid))
          {
            // Current thread, GDB should use current CPU registers.
#if defined(DEBUG)
            printf ("%s(*, %u)=-1 current thread\n", __func__, tid);
#endif /* defined(DEBUG) */

            return -1;
          }

        thread_type* th = threads_.thread (tid);
        assert(th != NULL);

        th->read_stack ();

        // Note: The FP registers are not returned, only the main registers.

        assert(rtos.stack_info != NULL);

#if defined(DEBUG)
        printf ("out ");
#endif /* defined(DEBUG) */
        char* out = out_hex_values;
        for (int i = 0; i < rtos.stack_info->out_registers; ++i)
          {
            out = th->output_register (i, out);

#if defined(DEBUG)
            printf ("%s ", out - 8);
#endif /* defined(DEBUG) */
          }
#if defined(DEBUG)
        printf ("\n");
#endif /* defined(DEBUG) */

        return 0;
      }

      /**
       * @brief Set a thread register to the value of a HEX string.
       *
       * @details
       * If the register value has to be written directly to the CPU,
       * the function must return a value <0. The register value is then
       * written to the CPU by the GDB server itself.
       *
       * @param [in] thread_id ID of the thread.
       * @param [in] reg_index Index of the register.
       * @param [in] hex_value Pointer to the string, containing
       *  the value to write.
       *
       * @retval 0 Writing register OK.
       * @retval <0 Writing register failed.
       */
      int
      set_thread_register (thread_id_t tid, uint32_t reg_index,
                           const char* hex_value)
      {
#if defined(DEBUG)
        printf ("%s(\"%s\", %u, %u)\n", __func__, hex_value, reg_index, tid);
#endif /* defined(DEBUG) */

        if (!is_scheduler_started_)
          {
            // No scheduler, GDB should set current registers.
#if defined(DEBUG)
            printf ("%s(\"%s\", %u, %u)=-1 no scheduler\n", __func__, hex_value,
                    reg_index, tid);
#endif /* defined(DEBUG) */
            return -1;
          }

        if (tid == thread_type::id_none || threads_.is_current (tid))
          {
            // Current thread, GDB should set current CPU registers.
#if defined(DEBUG)
            printf ("%s(\"%s\", %u, %u)=-1 current thread\n", __func__,
                    hex_value, reg_index, tid);
#endif /* defined(DEBUG) */
            return -1;
          }

        int ret = 0;
        backend_.output_warning ("%s() not yet implemented (%d)\n", __func__,
                                 ret);

        return ret;
      }

      /**
       * @brief Set the thread registers to the given HEX string.
       *
       * @details
       * If the register values have to be written directly to the CPU,
       * the function must return a value <0. The register values are
       * then written to the CPU by the GDB server itself.
       *
       * @param [in] thread_id ID of the thread.
       * @param [in] hex_values Pointer to the string, containing the
       *  values to write.
       *
       * @retval 0 Writing registers OK.
       * @retval <0 Writing registers failed.
       */
      int
      set_thread_registers (thread_id_t tid, const char* hex_values)
      {
#if defined(DEBUG)
        printf ("%s(\"%s\", %d)\n", __func__, hex_values, tid);
#endif /* defined(DEBUG) */

        if (!is_scheduler_started_)
          {
            // No scheduler, GDB should set current registers.
#if defined(DEBUG)
            printf ("%s(\"%s\", %d)=-1 no scheduler\n", __func__, hex_values,
                    tid);
#endif /* defined(DEBUG) */
            return -1;
          }

        if (tid == thread_type::id_none || threads_.is_current (tid))
          {
            // Current thread, GDB should set current CPU registers.
#if defined(DEBUG)
            printf ("%s(\"%s\", %d)=-1 current thread\n", __func__, hex_values,
                    tid);
#endif /* defined(DEBUG) */
            return -1;
          }

        int ret = 0;
        backend_.output_warning ("%s() not yet implemented (%d)\n", __func__,
                                 ret);
        return ret;
      }

    private:

      // ----------------------------------------------------------------------
      // Support functions.

      // ----------------------------------------------------------------------

    private:

      backend_type& backend_;
      allocator_type& allocator_;

      metadata_type metadata_
        { backend_ };
      threads_type threads_
        { backend_, allocator_ };
      rtd_type rt_
        { backend_, metadata_, threads_, allocator_ };

      bool is_scheduler_started_ = false;

    };

// ----------------------------------------------------------------------------
} /* namespace drtm */

#endif /* defined(__cplusplus) */

#endif /* DRTM_FRONTEND_H_ */
