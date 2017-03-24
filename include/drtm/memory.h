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

#ifndef DRTM_MEMORY_H_
#define DRTM_MEMORY_H_

#if defined(__cplusplus)

#include <cstdint>
#include <cstddef>
#include <cerrno>
#include <cassert>
#include <limits>
#include <memory>
#include <system_error>

namespace drtm
{
  // --------------------------------------------------------------------------

  /**
   * @brief Memory resource manager (abstract class).
   *
   * @details
   * This class is based on the standard C++17 memory manager, with
   * several extensions, to control the throw behaviour and to
   * add statistics.
   */
  class memory_resource
  {
    static const std::size_t max_align_ = alignof(max_align_t);

  public:

    virtual
    ~memory_resource () = default;

    inline
    __attribute__ ((__always_inline__))
    void*
    allocate (std::size_t bytes, std::size_t align = max_align_)
    {
#if defined(DEBUG_)
      printf ("%s(%zu,%zu) @%p\n", __PRETTY_FUNCTION__, bytes, align, this);
#endif
      return do_allocate (bytes, align);
    }

    inline
    __attribute__ ((__always_inline__))
    void
    deallocate (void* p, std::size_t bytes, std::size_t align = max_align_)
    {
      do_deallocate (p, bytes, align);
    }

    inline
    __attribute__ ((__always_inline__))
    bool
    is_equal (memory_resource const & other) const noexcept
    {
      return do_is_equal (other);
    }

  protected:

    virtual void*
    do_allocate (std::size_t nbytes, std::size_t align) = 0;

    virtual void
    do_deallocate (void* p, std::size_t bytes, std::size_t align) = 0;

    virtual bool
    do_is_equal (memory_resource const & other) const noexcept = 0;
  };

  inline __attribute__ ((__always_inline__))
  bool
  operator== (memory_resource const & lhs, memory_resource const & rhs) noexcept
  {
    return &lhs == &rhs || lhs.is_equal (rhs);
  }

  inline __attribute__ ((__always_inline__))
  bool
  operator!= (memory_resource const & lhs, memory_resource const & rhs) noexcept
  {
    return !(lhs == rhs);
  }

  // --------------------------------------------------------------------------

  /**
   * @brief Polymorphic allocator.
   *
   * This class template is an Allocator whose
   * allocation behavior depends on the memory resource it is
   * constructed with. Thus, different instances of polymorphic_allocator
   * can exhibit entirely different allocation behavior. This runtime
   * polymorphism allows objects using polymorphic_allocator to behave
   * as if they used different allocator types at run time despite the
   * identical static allocator type.
   */
  template<typename T>
    class polymorphic_allocator
    {
    public:

      // Standard types.
      using value_type = T;

    public:

      polymorphic_allocator () noexcept = default;

      polymorphic_allocator (memory_resource* mr) noexcept
      {
#if defined(DEBUG)
        printf ("%s(%p) @%p\n", __func__, mr, this);
#endif /* defined(DEBUG) */

        mr_ = mr;
      }

      polymorphic_allocator (polymorphic_allocator const & a) = default;

      template<typename U>
        polymorphic_allocator (polymorphic_allocator<U> const & other) noexcept
        {
          mr_ = other.mr_;
        }

      polymorphic_allocator&
      operator= (polymorphic_allocator const & a) = default;

      value_type*
      allocate (std::size_t objects)
      {
#if defined(DEBUG_)
        printf ("%s(%zu) %p\n", __func__, objects, this);
#endif /* defined(DEBUG) */

        if (objects > max_size ())
          {
            throw std::system_error (
                std::error_code (EINVAL, std::system_category ()));
          }

        value_type*p = static_cast<value_type*> (mr_->allocate (
            objects * sizeof(value_type), alignof(value_type)));

#if defined(DEBUG_)
        printf ("%s(%zu)=%p %p\n", __func__, objects, p, this);
#endif /* defined(DEBUG) */

        return p;
      }

      void
      deallocate (value_type* p, std::size_t objects) noexcept
      {
#if defined(DEBUG)
        printf ("%s(%p,%zu) %p\n", __func__, p, objects, this);
#endif /* defined(DEBUG) */

        assert(objects <= max_size ());
        mr_->deallocate (p, objects * sizeof(value_type), alignof(value_type));
      }

      std::size_t
      max_size (void) const noexcept
      {
        return std::numeric_limits<std::size_t>::max () / sizeof(value_type);
      }

      polymorphic_allocator
      select_on_container_copy_construction (void) const noexcept
      {
        return polymorphic_allocator ();
      }

      memory_resource*
      resource (void) const noexcept
      {
        return mr_;
      }

    private:

      memory_resource* mr_ = nullptr;
    };

// ----------------------------------------------------------------------------
}
#endif /* #if defined(__cplusplus) */

#endif /* DRTM_MEMORY_H_ */
