




























#ifndef jit_x86_shared_AssemblerBuffer_x86_shared_h
#define jit_x86_shared_AssemblerBuffer_x86_shared_h

#include <stdarg.h>
#include <string.h>

#include "jit/ExecutableAllocator.h"
#include "jit/JitSpewer.h"


#define PRETTYHEX(x)                       (((x)<0)?"-":""),(((x)<0)?-(x):(x))

#define MEM_o     "%s0x%x"
#define MEM_os    MEM_o   "(,%s,%d)"
#define MEM_ob    MEM_o   "(%s)"
#define MEM_obs   MEM_o   "(%s,%s,%d)"

#define MEM_o32   "%s0x%04x"
#define MEM_o32s  MEM_o32 "(,%s,%d)"
#define MEM_o32b  MEM_o32 "(%s)"
#define MEM_o32bs MEM_o32 "(%s,%s,%d)"
#define MEM_o32r  ".Lfrom%d(%%rip)"

#define ADDR_o(offset)                       PRETTYHEX(offset)
#define ADDR_os(offset, index, scale)        ADDR_o(offset), GPRegName((index)), (1<<(scale))
#define ADDR_ob(offset, base)                ADDR_o(offset), GPRegName((base))
#define ADDR_obs(offset, base, index, scale) ADDR_ob(offset, base), GPRegName((index)), (1<<(scale))

#define ADDR_o32(offset)                       ADDR_o(offset)
#define ADDR_o32s(offset, index, scale)        ADDR_os(offset, index, scale)
#define ADDR_o32b(offset, base)                ADDR_ob(offset, base)
#define ADDR_o32bs(offset, base, index, scale) ADDR_obs(offset, base, index, scale)
#define ADDR_o32r(offset)                      (offset)

namespace js {

    class Sprinter;

namespace jit {

    class AssemblerBuffer {
    public:
        AssemblerBuffer()
            : m_oom(false)
        {
        }

        void ensureSpace(size_t space)
        {
            if (MOZ_UNLIKELY(!m_buffer.reserve(m_buffer.length() + space)))
                oomDetected();
        }

        bool isAligned(size_t alignment) const
        {
            return !(m_buffer.length() & (alignment - 1));
        }

        void putByteUnchecked(int value)
        {
            m_buffer.infallibleAppend(char(value));
        }

        void putByte(int value)
        {
            if (MOZ_UNLIKELY(!m_buffer.append(char(value))))
                oomDetected();
        }

        void putShortUnchecked(int value)
        {
            m_buffer.infallibleGrowByUninitialized(2);
            memcpy(m_buffer.end() - 2, &value, 2);
        }

        void putShort(int value)
        {
            if (MOZ_UNLIKELY(!m_buffer.growByUninitialized(2))) {
                oomDetected();
                return;
            }
            memcpy(m_buffer.end() - 2, &value, 2);
        }

        void putIntUnchecked(int value)
        {
            m_buffer.infallibleGrowByUninitialized(4);
            memcpy(m_buffer.end() - 4, &value, 4);
        }

        void putInt64Unchecked(int64_t value)
        {
            m_buffer.infallibleGrowByUninitialized(8);
            memcpy(m_buffer.end() - 8, &value, 8);
        }

        void putInt(int value)
        {
            if (MOZ_UNLIKELY(!m_buffer.growByUninitialized(4))) {
                oomDetected();
                return;
            }
            memcpy(m_buffer.end() - 4, &value, 4);
        }

        unsigned char* data()
        {
            return m_buffer.begin();
        }

        size_t size() const
        {
            return m_buffer.length();
        }

        bool oom() const
        {
            return m_oom;
        }

        const unsigned char* buffer() const {
            MOZ_ASSERT(!m_oom);
            return m_buffer.begin();
        }

    protected:
        













        void oomDetected() {
            m_oom = true;
            m_buffer.clear();
        }

        mozilla::Vector<unsigned char, 256, SystemAllocPolicy> m_buffer;
        bool m_oom;
    };

    class GenericAssembler
    {
        Sprinter* printer;

      public:

        GenericAssembler()
          : printer(NULL)
        {}

        void setPrinter(Sprinter* sp) {
            printer = sp;
        }

        void spew(const char* fmt, ...)
#ifdef __GNUC__
            __attribute__ ((format (printf, 2, 3)))
#endif
        {
            if (MOZ_UNLIKELY(printer || JitSpewEnabled(JitSpew_Codegen))) {
                va_list va;
                va_start(va, fmt);
                spew(fmt, va);
                va_end(va);
            }
        }

        MOZ_COLD void spew(const char* fmt, va_list va);
    };

} 
} 

#endif 
