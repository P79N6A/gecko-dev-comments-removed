




























#ifndef LinkBuffer_h
#define LinkBuffer_h

#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER

#include <assembler/MacroAssembler.h>

namespace JSC {















class LinkBuffer {
    typedef MacroAssemblerCodeRef CodeRef;
    typedef MacroAssembler::Label Label;
    typedef MacroAssembler::Jump Jump;
    typedef MacroAssembler::JumpList JumpList;
    typedef MacroAssembler::Call Call;
    typedef MacroAssembler::DataLabel32 DataLabel32;
    typedef MacroAssembler::DataLabelPtr DataLabelPtr;

public:
    
    LinkBuffer(MacroAssembler* masm, ExecutableAllocator* executableAllocator,
               ExecutablePool** poolp, bool* ok)
    {
        m_code = executableAllocAndCopy(*masm, executableAllocator, poolp);
        m_executablePool = *poolp;
        m_size = masm->m_assembler.size();  
#ifndef NDEBUG
        m_completed = false;
#endif
        *ok = !!m_code;
    }

    LinkBuffer()
        : m_executablePool(NULL)
        , m_code(NULL)
        , m_size(0)
#ifndef NDEBUG
        , m_completed(false)
#endif
    {
    }

    LinkBuffer(uint8* ncode, size_t size)
        : m_executablePool(NULL)
        , m_code(ncode)
        , m_size(size)
#ifndef NDEBUG
        , m_completed(false)
#endif
    {
    }

    ~LinkBuffer()
    {
        ASSERT(!m_executablePool || m_completed);
    }

    

    void link(Call call, FunctionPtr function)
    {
        ASSERT(call.isFlagSet(Call::Linkable));
        MacroAssembler::linkCall(code(), call, function);
    }
    
    void link(Jump jump, CodeLocationLabel label)
    {
        MacroAssembler::linkJump(code(), jump, label);
    }

    void link(JumpList list, CodeLocationLabel label)
    {
        for (unsigned i = 0; i < list.m_jumps.length(); ++i)
            MacroAssembler::linkJump(code(), list.m_jumps[i], label);
    }

    void patch(DataLabelPtr label, void* value)
    {
        MacroAssembler::linkPointer(code(), label.m_label, value);
    }

    void patch(DataLabelPtr label, CodeLocationLabel value)
    {
        MacroAssembler::linkPointer(code(), label.m_label, value.executableAddress());
    }

    

    CodeLocationCall locationOf(Call call)
    {
        ASSERT(call.isFlagSet(Call::Linkable));
        ASSERT(!call.isFlagSet(Call::Near));
        return CodeLocationCall(MacroAssembler::getLinkerAddress(code(), call.m_jmp));
    }

    CodeLocationJump locationOf(Jump j)
    {
        return CodeLocationJump(MacroAssembler::getLinkerAddress(code(), j.m_jmp));
    }

    CodeLocationNearCall locationOfNearCall(Call call)
    {
        ASSERT(call.isFlagSet(Call::Linkable));
        ASSERT(call.isFlagSet(Call::Near));
        return CodeLocationNearCall(MacroAssembler::getLinkerAddress(code(), call.m_jmp));
    }

    CodeLocationLabel locationOf(Label label)
    {
        return CodeLocationLabel(MacroAssembler::getLinkerAddress(code(), label.m_label));
    }

    CodeLocationDataLabelPtr locationOf(DataLabelPtr label)
    {
        return CodeLocationDataLabelPtr(MacroAssembler::getLinkerAddress(code(), label.m_label));
    }

    CodeLocationDataLabel32 locationOf(DataLabel32 label)
    {
        return CodeLocationDataLabel32(MacroAssembler::getLinkerAddress(code(), label.m_label));
    }

    
    
    unsigned returnAddressOffset(Call call)
    {
        return MacroAssembler::getLinkerCallReturnOffset(call);
    }

    
    
    
    
    CodeRef finalizeCode()
    {
        performFinalization();

        return CodeRef(m_code, m_executablePool, m_size);
    }
    CodeLocationLabel finalizeCodeAddendum()
    {
        performFinalization();

        return CodeLocationLabel(code());
    }

protected:
    
    
    void* code()
    {
        return m_code;
    }

    void *executableAllocAndCopy(MacroAssembler &masm, ExecutableAllocator *allocator,
                                 ExecutablePool **poolp)
    {
        return masm.m_assembler.executableAllocAndCopy(allocator, poolp);
    }

    void performFinalization()
    {
#ifndef NDEBUG
        ASSERT(!m_completed);
        m_completed = true;
#endif

        ExecutableAllocator::makeExecutable(code(), m_size);
        ExecutableAllocator::cacheFlush(code(), m_size);
    }

    ExecutablePool* m_executablePool;
    void* m_code;
    size_t m_size;
#ifndef NDEBUG
    bool m_completed;
#endif
};

} 

#endif 

#endif 
