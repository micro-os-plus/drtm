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

/*
 * This sample file can be used to prepare a custom memory manager
 * for the DRTM library. It is mandatory when integrating the
 * DRTM library in multi-threading environments that redefine the
 * standard malloc()/free() with thread save version.
 *
 * If not, simply use the std::allocator() when instantiating the
 * DRTM class templates.
 *
 * If the backend memory management functions are global symbols,
 * it is perfectly possible to call them directly, without any
 * pointer forwarders.
 *
 * If the backend memory management is accessible via some
 * forwarding pointers, use the second version, where the
 * server_api_type must define two functions, with prototypes
 * similar to malloc()/free().
 */

#include <stdio.h>

#include <your-application.h>

#if defined(__cplusplus)

#include <drtm/memory.h>

namespace your_namespace
{
  namespace drtm
  {

    /**
     * @brief A standard allocator that allocates memory via
     * the backend memory management functions.
     *
     * @tparam T type of allocator values
     */
    template<typename T>
      class allocator
      {
      public:

        // Standard types.
        using value_type = T;

      public:

        /**
         * @brief Construct an allocator object instance.
         */
        allocator () noexcept
        {
#if defined(DEBUG)
          printf ("%s() @%p\n", __func__, this);
#endif /* defined(DEBUG) */
        }

        /**
         * @brief Copy construct an allocator object instance.
         */
        allocator (allocator const & a) = default;

        /**
         * @brief Construct a sibling allocator object instance,
         * for a different type.
         */
        template<typename U>
          allocator (allocator<U> const & other __attribute__((unused))) noexcept
          {
            ; // No members to copy.
          }

        /**
         * @brief Assign an allocator object instance.
         */
        allocator&
        operator= (allocator const & a) = default;

        /**
         * @brief Allocate a number of objects of the allocator type.
         */
        value_type*
        allocate (std::size_t objects)
        {
#if defined(DEBUG)
          printf ("%s(%zu) %p\n", __func__, objects, this);
#endif /* defined(DEBUG) */

          if (objects > max_size ())
            {
              throw std::system_error (
                  std::error_code (EINVAL, std::system_category ()));
            }

          value_type*p = static_cast<value_type*> (yapp_malloc (
              objects * sizeof(value_type)));

#if defined(DEBUG)
          printf ("%s(%zu)=%p %p\n", __func__, objects, p, this);
#endif /* defined(DEBUG) */

          return p;
        }

        /**
         * @brief Deallocate the number of objects.
         */
        void
        deallocate (value_type* p, std::size_t objects) noexcept
        {
#if defined(DEBUG)
          printf ("%s(%p,%zu) %p\n", __func__, p, objects, this);
#endif /* defined(DEBUG) */

          assert(objects <= max_size ());
          yapp_free (p);
        }

        /**
         * @brief Get the maximum number of objects that can be allocated.
         */
        std::size_t
        max_size (void) const noexcept
        {
          return std::numeric_limits<std::size_t>::max () / sizeof(value_type);
        }

        allocator
        select_on_container_copy_construction (void) const noexcept
        {
          return allocator ();
        }

      private:

        // None.
      };

    ;
  // Avoid formatter bug
  // ==========================================================================
  } /* namespace drtm */
} /* namespace your_namespace */

#endif /* defined(__cplusplus) */
