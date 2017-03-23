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
 * This sample file can be used to prepare a custom backend
 * for the DRTM library.
 *
 * It should provide the DRTM library access to the underlying
 * GDB server.
 *
 * If the support functions are global symbols, they can be
 * called directly, without any pointer forwarders.
 */

#include <stdio.h>

#include <your-application.h>

#if defined(__cplusplus)

#include <cstring>
#include <cassert>
#include <cstdarg>

namespace your_namespace
{
  namespace drtm
  {

    /**
     * This template class provides an implementation for the DRTM backend
     * that forwards calls to the <your application> SDK C API.
     *
     * @tparam S type of the symbol structure, with name & address
     */
    template<typename S>
      class backend
      {
      public:

        constexpr static std::size_t tmp_buf_size_bytes = 256;
        using symbols_type = S;
        using target_addr_t = decltype(S::address);

      public:
        /**
         * @brief Construct a <your application> backend.
         *
         * @param symbols Pointer to the symbols table.
         */
        backend (const symbols_type* symbols) :
            symbols_ (symbols)
        {
#if defined(DEBUG)
          printf ("%s(%p) @%p\n", __func__, symbols, this);
#endif /* defined(DEBUG) */
        }

        // The rule of five.
        backend (const backend&) = delete;
        backend (backend&&) = delete;
        backend&
        operator= (const backend&) = delete;
        backend&
        operator= (backend&&) = delete;

        ~backend () = default;

      public:

        // TODO: adjust it ot match the members in your symbols table.
        target_addr_t
        get_symbol_address (const char* name)
        {
          assert(name != nullptr);

          const symbols_type* p = symbols_;
          for (; p->name; ++p)
            {
              if (std::strcmp (name, p->name) == 0)
                {
                  return p->address;
                }
            }

          return 0;
        }

        /**
         * @brief Output a formatted log message to the
         * <your application> server window.
         *
         * @details
         * If a log file is specified, the message will also be printed to
         * the log file.
         */
        int
        output (const char* fmt, ...)
        {
          std::va_list args;
          va_start(args, fmt);

          int ret = yapp_voutput (fmt, args);

          va_end(args);
          return ret;
        }

        inline int
        voutput (const char* fmt, va_list args)
        {
          return yapp_voutput (fmt, args);
        }

        /**
         * @brief Output a formatted warning message to the
         * <your application> server window.
         *
         * @details
         * The line starts with “WARNING: ”. If a log file is specified, the
         * message will also be printed to the log file.
         */
        int
        output_warning (const char* fmt, ...)
        {
          std::va_list args;
          va_start(args, fmt);

          int ret = voutput_warning (fmt, args);

          va_end(args);
          return ret;
        }

        int
        voutput_warning (const char* fmt, va_list args)
        {
          char buf[tmp_buf_size_bytes];

          int ret = vsnprintf (buf, sizeof(buf), fmt, args);
          yapp_output_warning (buf);

          return ret;
        }

        /**
         * @brief Output a formatted error message to the
         * <your application> server window.
         *
         * @details
         * The line starts with “ERROR: ”. If a log file is specified, the
         * message will also be printed to the log file.
         */
        int
        output_error (const char* fmt, ...)
        {
          std::va_list args;
          va_start(args, fmt);

          int ret = voutput_error (fmt, args);

          va_end(args);
          return ret;
        }

        int
        voutput_error (const char* fmt, va_list args)
        {
          char buf[tmp_buf_size_bytes];

          int ret = vsnprintf (buf, sizeof(buf), fmt, args);
          yapp_output_error (buf);

          return ret;
        }

        inline bool
        is_target_little_endian (void)
        {
          return yapp_is_target_little_endian ();
        }

        /**
         * @brief Read memory from the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to read from.
         * @param [out] out_array Pointer to buffer for target memory.
         * @param [in] nbytes Number of bytes to read.
         *
         * @retval 0 Reading memory OK.
         * @retval <0 Reading memory failed.
         */
        inline int
        read_byte_array (target_addr_t addr, uint8_t* out_array,
                         std::size_t bytes)
        {
          return yapp_read_byte_array (addr, out_array, bytes);
        }

        /**
         * @brief Read one byte from the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to read from.
         * @param [out] out_byte Pointer to byte.
         *
         * @retval 0 Reading memory OK.
         * @retval <0 Reading memory failed.
         */
        inline int
        read_byte (target_addr_t addr, uint8_t* out_value)
        {
          uint8_t buf[1];
          int ret = read_byte_array (addr, &buf[0], sizeof(buf));
          if (ret >= 0)
            {
              *out_value = buf[0];
            }

          return ret;
        }

        /**
         * @brief Read two bytes from the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to read from.
         * @param [out] out_value Pointer to two bytes.
         *
         * @retval 0 Reading memory OK.
         * @retval <0 Reading memory failed.
         */
        int
        read_short (target_addr_t addr, uint16_t* out_value)
        {
          uint8_t buf[2];
          int ret = read_byte_array (addr, &buf[0], sizeof(buf));
          if (ret >= 0)
            {
              *out_value = load_short (&buf[0]);
            }
          return ret;
        }

        /**
         * @brief Read four bytes from the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to read from.
         * @param [out] out_value Pointer to four bytes.
         *
         * @retval 0 Reading memory OK.
         * @retval <0 Reading memory failed.
         */
        int
        read_long (target_addr_t addr, uint32_t* out_value)
        {
          uint8_t buf[4];
          int ret = read_byte_array (addr, &buf[0], sizeof(buf));
          if (ret >= 0)
            {
              *out_value = load_long (&buf[0]);
            }
          return ret;
        }

        /**
         * @brief Read eight bytes from the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to read from.
         * @param [out] out_value Pointer to eight bytes.
         *
         * @retval 0 Reading memory OK.
         * @retval <0 Reading memory failed.
         */
        int
        read_long_long (target_addr_t addr, uint64_t* out_value)
        {
          uint8_t buf[8];
          int ret = read_byte_array (addr, &buf[0], sizeof(buf));
          if (ret >= 0)
            {
              *out_value = load_long_long (&buf[0]);
            }
          return ret;
        }

        /**
         * @brief Write memory to the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to write to.
         * @param [in] out_array Pointer to buffer for target memory.
         * @param [in] nbytes Number of bytes to write.
         *
         * @retval 0 Writing memory OK.
         * @retval <0 Writing memory failed.
         */
        inline int
        write_byte_array (target_addr_t addr, const uint8_t* out_array,
                          std::size_t bytes)
        {
          return yapp_write_byte_array (addr, out_array, bytes);
        }

        /**
         * @brief Write one byte to the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to write to.
         * @param [in] value Byte to write.
         *
         * @retval 0 Writing memory OK.
         * @retval <0 Writing memory failed.
         */
        void
        write_byte (target_addr_t addr, uint8_t value)
        {
          uint8_t array[1];
          array[0] = value;
          write_byte_array (addr, &array[0], 1);
        }

        /**
         * @brief Write two bytes to the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to write to.
         * @param [in] value Bytes to write.
         *
         * @retval 0 Writing memory OK.
         * @retval <0 Writing memory failed.
         */
        void
        write_short (target_addr_t addr, uint16_t value)
        {
          uint8_t array[2];
          if (is_target_little_endian ())
            {
              array[0] = value & 0xFF;
              value >>= 8;
              array[1] = value & 0xFF;
            }
          else
            {
              array[1] = value & 0xFF;
              value >>= 8;
              array[0] = value & 0xFF;
            }
          write_byte_array (addr, &array[0], 2);
        }

        /**
         * @brief Write four bytes to the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to write to.
         * @param [in] value Bytes to write.
         *
         * @retval 0 Writing memory OK.
         * @retval <0 Writing memory failed.
         */
        void
        write_long (target_addr_t addr, uint32_t value)
        {
          uint8_t array[4];
          if (is_target_little_endian ())
            {
              array[0] = value & 0xFF;
              value >>= 8;
              array[1] = value & 0xFF;
              value >>= 8;
              array[2] = value & 0xFF;
              value >>= 8;
              array[3] = value & 0xFF;
            }
          else
            {
              array[3] = value & 0xFF;
              value >>= 8;
              array[2] = value & 0xFF;
              value >>= 8;
              array[1] = value & 0xFF;
              value >>= 8;
              array[0] = value & 0xFF;
            }
          write_byte_array (addr, &array[0], 4);
        }

        /**
         * @brief Write eight bytes to the target system.
         *
         * @details
         * If necessary, the target CPU is halted in order to read memory.
         *
         * @param [in] addr Target address to write to.
         * @param [in] value Bytes to write.
         *
         * @retval 0 Writing memory OK.
         * @retval <0 Writing memory failed.
         */
        void
        write_long_long (target_addr_t addr, uint64_t value)
        {
          uint8_t array[8];
          if (is_target_little_endian ())
            {
              array[0] = value & 0xFF;
              value >>= 8;
              array[1] = value & 0xFF;
              value >>= 8;
              array[2] = value & 0xFF;
              value >>= 8;
              array[3] = value & 0xFF;
              value >>= 8;
              array[4] = value & 0xFF;
              value >>= 8;
              array[5] = value & 0xFF;
              value >>= 8;
              array[6] = value & 0xFF;
              value >>= 8;
              array[7] = value & 0xFF;
            }
          else
            {
              array[7] = value & 0xFF;
              value >>= 8;
              array[6] = value & 0xFF;
              value >>= 8;
              array[5] = value & 0xFF;
              value >>= 8;
              array[4] = value & 0xFF;
              value >>= 8;
              array[3] = value & 0xFF;
              value >>= 8;
              array[2] = value & 0xFF;
              value >>= 8;
              array[1] = value & 0xFF;
              value >>= 8;
              array[0] = value & 0xFF;
            }
          write_byte_array (addr, &array[0], 8);
        }

        /**
         * @brief Load two bytes from a memory buffer according to the
         * target endianness.
         *
         * @param [in] p Pointer to memory buffer.
         *
         * @return The converted value.
         */
        uint16_t
        load_short (const uint8_t* p)
        {
          uint16_t val;
          if (is_target_little_endian ())
            {
              val = p[1];
              val <<= 8;
              val |= p[0];
            }
          else
            {
              val = p[0];
              val <<= 8;
              val |= p[1];
            }
          return val;
        }

        /**
         * @brief Load four bytes from a memory buffer according to the
         * target endianness.
         *
         * @param [in] p Pointer to memory buffer.
         *
         * @return The converted value.
         */
        uint32_t
        load_long (const uint8_t* p)
        {
          uint32_t val;
          if (is_target_little_endian ())
            {
              val = p[3];
              val <<= 8;
              val = p[2];
              val <<= 8;
              val = p[1];
              val <<= 8;
              val |= p[0];
            }
          else
            {
              val = p[0];
              val <<= 8;
              val |= p[1];
              val <<= 8;
              val |= p[2];
              val <<= 8;
              val |= p[3];
            }
          return val;
        }

        /**
         * @brief Load eight bytes from a memory buffer according to the
         * target endianness.
         *
         * @param [in] p Pointer to memory buffer.
         *
         * @return The converted value.
         */
        inline uint64_t
        load_long_long (const uint8_t* p)
        {
          uint64_t val;
          if (is_target_little_endian ())
            {
              val = p[7];
              val <<= 8;
              val = p[6];
              val <<= 8;
              val = p[5];
              val <<= 8;
              val = p[4];
              val <<= 8;
              val = p[3];
              val <<= 8;
              val = p[2];
              val <<= 8;
              val = p[1];
              val <<= 8;
              val |= p[0];
            }
          else
            {
              val = p[0];
              val <<= 8;
              val |= p[1];
              val <<= 8;
              val |= p[2];
              val <<= 8;
              val |= p[3];
              val <<= 8;
              val |= p[4];
              val <<= 8;
              val |= p[5];
              val <<= 8;
              val |= p[6];
              val <<= 8;
              val |= p[7];
            }
          return val;
        }

        // ----------------------------------------------------------------------

      private:

        const symbols_type* symbols_;
      };

    ;
  // Avoid formatter bug
  // ==========================================================================
  } /* namespace drtm */
} /* namespace your_namespace */

#endif /* defined(__cplusplus) */
