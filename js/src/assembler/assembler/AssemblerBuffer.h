




























#ifndef assembler_assembler_AssemblerBuffer_h
#define assembler_assembler_AssemblerBuffer_h

#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER

#include <string.h>
#include <limits.h>
#include "assembler/jit/ExecutableAllocator.h"
#include "assembler/wtf/Assertions.h"

#include <stdarg.h>
#include "jsfriendapi.h"
#include "jsopcode.h"

#include "jit/IonSpewer.h"
#include "js/RootingAPI.h"

#define PRETTY_PRINT_OFFSET(os) (((os)<0)?"-":""), (((os)<0)?-(os):(os))

#define FIXME_INSN_PRINTING                                 \
    do {                                                    \
        spew("FIXME insn printing %s:%d",                   \
             __FILE__, __LINE__);                           \
    } while (0)

namespace JSC {

    class AssemblerBuffer {
        static const int inlineCapacity = 256;
    public:
        AssemblerBuffer()
            : m_buffer(m_inlineBuffer)
            , m_capacity(inlineCapacity)
            , m_size(0)
            , m_oom(false)
        {
        }

        ~AssemblerBuffer()
        {
            if (m_buffer != m_inlineBuffer)
                js_free(m_buffer);
        }

        void ensureSpace(int space)
        {
            if (m_size > m_capacity - space)
                grow();
        }

        bool isAligned(int alignment) const
        {
            return !(m_size & (alignment - 1));
        }

        void putByteUnchecked(int value)
        {
            ASSERT(!(m_size > m_capacity - 4));
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
            ASSERT(!(m_size > m_capacity - 4));
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
            ASSERT(!(m_size > m_capacity - 4));
            *reinterpret_cast<int*>(&m_buffer[m_size]) = value;
            m_size += 4;
        }

        void putInt64Unchecked(int64_t value)
        {
            ASSERT(!(m_size > m_capacity - 8));
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

        int size() const
        {
            return m_size;
        }

        bool oom() const
        {
            return m_oom;
        }

        



        void* executableAllocAndCopy(ExecutableAllocator* allocator, ExecutablePool** poolp, CodeKind kind)
        {
            if (m_oom || m_size == 0) {
                *poolp = NULL;
                return 0;
            }

            void* result = allocator->alloc(m_size, poolp, kind);
            if (!result) {
                *poolp = NULL;
                return 0;
            }
            JS_ASSERT(*poolp);

            ExecutableAllocator::makeWritable(result, m_size);

            return memcpy(result, m_buffer, m_size);
        }

        unsigned char *buffer() const {
            ASSERT(!m_oom);
            return reinterpret_cast<unsigned char *>(m_buffer);
        }

    protected:
        void append(const char* data, int size)
        {
            if (m_size > m_capacity - size)
                grow(size);

            
            if (m_oom)
                return;
            memcpy(m_buffer + m_size, data, size);
            m_size += size;
        }

        














        void grow(int extraCapacity = 0)
        {
            



            int newCapacity = m_capacity + m_capacity + extraCapacity;
            char* newBuffer;

            
            
            if (newCapacity >= INT_MAX / 2) {
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
        int m_capacity;
        int m_size;
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
            if (printer
#ifdef JS_ION
                || js::jit::IonSpewEnabled(js::jit::IonSpew_Codegen)
#endif
                )
            {
                
                
                char buf[200];

                va_list va;
                va_start(va, fmt);
                int i = vsnprintf(buf, sizeof(buf), fmt, va);
                va_end(va);

                if (i > -1) {
                    if (printer)
                        printer->printf("%s\n", buf);

#ifdef JS_ION
                    js::jit::IonSpew(js::jit::IonSpew_Codegen, "%s", buf);
#endif
                }
            }
        }

        static void staticSpew(const char *fmt, ...)
#ifdef __GNUC__
            __attribute__ ((format (printf, 1, 2)))
#endif
        {
#ifdef JS_ION
            if (js::jit::IonSpewEnabled(js::jit::IonSpew_Codegen)) {
                char buf[200];

                va_list va;
                va_start(va, fmt);
                int i = vsnprintf(buf, sizeof(buf), fmt, va);
                va_end(va);

                if (i > -1)
                    js::jit::IonSpew(js::jit::IonSpew_Codegen, "%s", buf);
            }
#endif
        }
    };

} 

#endif 

#endif 
