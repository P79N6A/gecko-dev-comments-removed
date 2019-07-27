




























#ifndef jit_shared_AssemblerBuffer_x86_shared_h
#define jit_shared_AssemblerBuffer_x86_shared_h

#include <limits.h>
#include <stdarg.h>
#include <string.h>

#include "jsfriendapi.h"
#include "jsopcode.h"
#include "jsutil.h"

#include "jit/ExecutableAllocator.h"
#include "jit/JitSpewer.h"
#include "js/RootingAPI.h"


#define PRETTYHEX(x)                       (((x)<0)?"-":""),(((x)<0)?-(x):(x))

#define MEM_o     "%s0x%x"
#define MEM_os    MEM_o   "(,%s,%d)"
#define MEM_ob    MEM_o   "(%s)"
#define MEM_obs   MEM_o   "(%s,%s,%d)"

#define MEM_o32   "%s0x%04x"
#define MEM_o32s  MEM_o32 "(,%s,%d)"
#define MEM_o32b  MEM_o32 "(%s)"
#define MEM_o32bs MEM_o32 "(%s,%s,%d)"

#define ADDR_o(offset)                       PRETTYHEX(offset)
#define ADDR_os(offset, index, scale)        ADDR_o(offset), nameIReg((index)), (1<<(scale))
#define ADDR_ob(offset, base)                ADDR_o(offset), nameIReg((base))
#define ADDR_obs(offset, base, index, scale) ADDR_ob(offset, base), nameIReg((index)), (1<<(scale))

#define ADDR_o32(offset)                       ADDR_o(offset)
#define ADDR_o32s(offset, index, scale)        ADDR_os(offset, index, scale)
#define ADDR_o32b(offset, base)                ADDR_ob(offset, base)
#define ADDR_o32bs(offset, base, index, scale) ADDR_obs(offset, base, index, scale)

#define FIXME_INSN_PRINTING                                 \
    do {                                                    \
        spew("FIXME insn printing %s:%d",                   \
             __FILE__, __LINE__);                           \
    } while (0)

namespace js {
namespace jit {

    class AssemblerBuffer {
        static const size_t inlineCapacity = 256;
    public:
        AssemblerBuffer()
            : m_buffer(m_inlineBuffer)
            , m_capacity(inlineCapacity)
            , m_size(0)
            , m_allocSize(0)
            , m_oom(false)
        {
        }

        ~AssemblerBuffer()
        {
            if (m_buffer != m_inlineBuffer)
                js_free(m_buffer);
        }

        void ensureSpace(size_t space)
        {
            if (m_size > m_capacity - space)
                grow();
        }

        bool isAligned(size_t alignment) const
        {
            return !(m_size & (alignment - 1));
        }

        void putByteUnchecked(int value)
        {
            MOZ_ASSERT(!(m_size > m_capacity - 4));
            m_buffer[m_size] = char(value);
            m_size++;
        }

        void putByte(int value)
        {
            if (m_size > m_capacity - 4)
                grow();
            putByteUnchecked(value);
        }

        void putShortUnchecked(int value)
        {
            MOZ_ASSERT(!(m_size > m_capacity - 4));
            *reinterpret_cast<short*>(&m_buffer[m_size]) = short(value);
            m_size += 2;
        }

        void putShort(int value)
        {
            if (m_size > m_capacity - 4)
                grow();
            putShortUnchecked(value);
        }

        void putIntUnchecked(int value)
        {
            MOZ_ASSERT(!(m_size > m_capacity - 4));
            *reinterpret_cast<int*>(&m_buffer[m_size]) = value;
            m_size += 4;
        }

        void putInt64Unchecked(int64_t value)
        {
            MOZ_ASSERT(!(m_size > m_capacity - 8));
            *reinterpret_cast<int64_t*>(&m_buffer[m_size]) = value;
            m_size += 8;
        }

        void putInt(int value)
        {
            if (m_size > m_capacity - 4)
                grow();
            putIntUnchecked(value);
        }

        void* data() const
        {
            return m_buffer;
        }

        size_t size() const
        {
            return m_size;
        }

        size_t allocSize() const
        {
            return m_allocSize;
        }

        bool oom() const
        {
            return m_oom;
        }

        unsigned char *buffer() const {
            MOZ_ASSERT(!m_oom);
            return reinterpret_cast<unsigned char *>(m_buffer);
        }

    protected:
        void append(const char* data, size_t size)
        {
            if (m_size > m_capacity - size)
                grow(size);

            
            if (m_oom)
                return;
            memcpy(m_buffer + m_size, data, size);
            m_size += size;
        }

        














        void grow(size_t extraCapacity = 0)
        {
            char* newBuffer;

            



            size_t doubleCapacity = m_capacity + m_capacity;

            
            if (doubleCapacity < m_capacity) {
                m_size = 0;
                m_oom = true;
                return;
            }

            size_t newCapacity = doubleCapacity + extraCapacity;

            
            if (newCapacity < doubleCapacity) {
                m_size = 0;
                m_oom = true;
                return;
            }

            if (m_buffer == m_inlineBuffer) {
                newBuffer = static_cast<char*>(js_malloc(newCapacity));
                if (!newBuffer) {
                    m_size = 0;
                    m_oom = true;
                    return;
                }
                memcpy(newBuffer, m_buffer, m_size);
            } else {
                newBuffer = static_cast<char*>(js_realloc(m_buffer, newCapacity));
                if (!newBuffer) {
                    m_size = 0;
                    m_oom = true;
                    return;
                }
            }

            m_buffer = newBuffer;
            m_capacity = newCapacity;
        }

        char m_inlineBuffer[inlineCapacity];
        char* m_buffer;
        size_t m_capacity;
        size_t m_size;
        size_t m_allocSize;
        bool m_oom;
    };

    class GenericAssembler
    {
        js::Sprinter *printer;

      public:

        bool isOOLPath;

        GenericAssembler()
          : printer(NULL)
          , isOOLPath(false)
        {}

        void setPrinter(js::Sprinter *sp) {
            printer = sp;
        }

        void spew(const char *fmt, ...)
#ifdef __GNUC__
            __attribute__ ((format (printf, 2, 3)))
#endif
        {
            if (printer || js::jit::JitSpewEnabled(js::jit::JitSpew_Codegen)) {
                
                
                char buf[200];

                va_list va;
                va_start(va, fmt);
                int i = vsnprintf(buf, sizeof(buf), fmt, va);
                va_end(va);

                if (i > -1) {
                    if (printer)
                        printer->printf("%s\n", buf);
                    js::jit::JitSpew(js::jit::JitSpew_Codegen, "%s", buf);
                }
            }
        }

        static void staticSpew(const char *fmt, ...)
#ifdef __GNUC__
            __attribute__ ((format (printf, 1, 2)))
#endif
        {
            if (js::jit::JitSpewEnabled(js::jit::JitSpew_Codegen)) {
                char buf[200];

                va_list va;
                va_start(va, fmt);
                int i = vsnprintf(buf, sizeof(buf), fmt, va);
                va_end(va);

                if (i > -1)
                    js::jit::JitSpew(js::jit::JitSpew_Codegen, "%s", buf);
            }
        }
    };

} 
} 

#endif 
