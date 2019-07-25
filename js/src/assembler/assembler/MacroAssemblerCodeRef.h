




























#ifndef MacroAssemblerCodeRef_h
#define MacroAssemblerCodeRef_h

#include "assembler/wtf/Platform.h"
#if !ENABLE_ASSEMBLER
# warning "assembler not enabled at point 1"
#endif
#include "assembler/jit/ExecutableAllocator.h"
#if !ENABLE_ASSEMBLER
# warning "assembler not enabled at point 2"
#endif

#if ENABLE_ASSEMBLER



#if WTF_CPU_ARM_THUMB2





#define ASSERT_VALID_CODE_POINTER(ptr) \
    ASSERT(reinterpret_cast<intptr_t>(ptr) & ~1); \
    ASSERT(reinterpret_cast<intptr_t>(ptr) & 1)
#define ASSERT_VALID_CODE_OFFSET(offset) \
    ASSERT(!(offset & 1)) // Must be multiple of 2.
#else
#define ASSERT_VALID_CODE_POINTER(ptr) \
    ASSERT(ptr)
#define ASSERT_VALID_CODE_OFFSET(offset)
#endif

namespace JSC {





class FunctionPtr {
public:
    FunctionPtr()
        : m_value(0)
    {
    }

    template<typename FunctionType>
    explicit FunctionPtr(FunctionType* value)
#if WTF_COMPILER_RVCT
     
     
        : m_value((void*)(value))
#else
        : m_value(reinterpret_cast<void*>(value))
#endif
    {
        ASSERT_VALID_CODE_POINTER(m_value);
    }

    void* value() const { return m_value; }
    void* executableAddress() const { return m_value; }


private:
    void* m_value;
};







class ReturnAddressPtr {
public:
    ReturnAddressPtr()
        : m_value(0)
    {
    }

    explicit ReturnAddressPtr(void* value)
        : m_value(value)
    {
        ASSERT_VALID_CODE_POINTER(m_value);
    }

    explicit ReturnAddressPtr(FunctionPtr function)
        : m_value(function.value())
    {
        ASSERT_VALID_CODE_POINTER(m_value);
    }

    void* value() const { return m_value; }

private:
    void* m_value;
};




class MacroAssemblerCodePtr {
public:
    MacroAssemblerCodePtr()
        : m_value(0)
    {
    }

    explicit MacroAssemblerCodePtr(void* value)
#if WTF_CPU_ARM_THUMB2
        
        : m_value(reinterpret_cast<char*>(value) + 1)
#else
        : m_value(value)
#endif
    {
        ASSERT_VALID_CODE_POINTER(m_value);
    }

    explicit MacroAssemblerCodePtr(ReturnAddressPtr ra)
        : m_value(ra.value())
    {
        ASSERT_VALID_CODE_POINTER(m_value);
    }

    void* executableAddress() const {
        return m_value;
    }
#if WTF_CPU_ARM_THUMB2
    
    void* dataLocation() const { ASSERT_VALID_CODE_POINTER(m_value); return reinterpret_cast<char*>(m_value) - 1; }
#else
    void* dataLocation() const { ASSERT_VALID_CODE_POINTER(m_value); return m_value; }
#endif

    bool operator!()
    {
        return !m_value;
    }

    ptrdiff_t operator -(const MacroAssemblerCodePtr &other) const
    {
        JS_ASSERT(m_value);
        return reinterpret_cast<uint8 *>(m_value) -
               reinterpret_cast<uint8 *>(other.m_value);
    }

private:
    void* m_value;
};






class MacroAssemblerCodeRef {
public:
    MacroAssemblerCodeRef()
        : m_executablePool(NULL),
          m_size(0)
    {
    }

    MacroAssemblerCodeRef(void* code, ExecutablePool* executablePool, size_t size)
        : m_code(code)
        , m_executablePool(executablePool)
        , m_size(size)
    {
    }

    
    void release()
    {
        if (!m_executablePool)
            return;

#if defined DEBUG && (defined WTF_CPU_X86 || defined WTF_CPU_X86_64) 
        void *addr = m_code.executableAddress();
        memset(addr, 0xcc, m_size);
#endif
        m_executablePool->release();
        m_executablePool = NULL;
    }

    MacroAssemblerCodePtr m_code;
    ExecutablePool* m_executablePool;
    size_t m_size;
};

} 

#endif 

#endif 
