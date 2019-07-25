
























#ifndef RepatchBuffer_h
#define RepatchBuffer_h

#include <wtf/Platform.h>

#if ENABLE_ASSEMBLER

#include <assembler/MacroAssembler.h>
#include <moco/MocoStubs.h> 

namespace JSC {







class RepatchBuffer {
    typedef MacroAssemblerCodePtr CodePtr;

public:
    RepatchBuffer(void *start, size_t size)
    : m_start(start), m_size(size)
    {
        ExecutableAllocator::makeWritable(m_start, m_size);
    }

    RepatchBuffer(CodeBlock* codeBlock)
    {
        JITCode& code = codeBlock->getJITCode();
        m_start = code.start();
        m_size = code.size();

        ExecutableAllocator::makeWritable(m_start, m_size);
    }

    ~RepatchBuffer()
    {
        ExecutableAllocator::makeExecutable(m_start, m_size);
    }

    void relink(CodeLocationJump jump, CodeLocationLabel destination)
    {
        MacroAssembler::repatchJump(jump, destination);
    }

    void relink(CodeLocationCall call, CodeLocationLabel destination)
    {
        MacroAssembler::repatchCall(call, destination);
    }

    void relink(CodeLocationCall call, FunctionPtr destination)
    {
        MacroAssembler::repatchCall(call, destination);
    }

    void relink(CodeLocationNearCall nearCall, CodePtr destination)
    {
        MacroAssembler::repatchNearCall(nearCall, CodeLocationLabel(destination));
    }

    void relink(CodeLocationNearCall nearCall, CodeLocationLabel destination)
    {
        MacroAssembler::repatchNearCall(nearCall, destination);
    }

    void repatch(CodeLocationDataLabel32 dataLabel32, int32_t value)
    {
        MacroAssembler::repatchInt32(dataLabel32, value);
    }

    void repatch(CodeLocationDataLabelPtr dataLabelPtr, void* value)
    {
        MacroAssembler::repatchPointer(dataLabelPtr, value);
    }

    void repatchLoadPtrToLEA(CodeLocationInstruction instruction)
    {
        MacroAssembler::repatchLoadPtrToLEA(instruction);
    }

    void repatchLEAToLoadPtr(CodeLocationInstruction instruction)
    {
        MacroAssembler::repatchLEAToLoadPtr(instruction);
    }

    void relinkCallerToTrampoline(ReturnAddressPtr returnAddress, CodeLocationLabel label)
    {
        relink(CodeLocationCall(CodePtr(returnAddress)), label);
    }
    
    void relinkCallerToTrampoline(ReturnAddressPtr returnAddress, CodePtr newCalleeFunction)
    {
        relinkCallerToTrampoline(returnAddress, CodeLocationLabel(newCalleeFunction));
    }

    void relinkCallerToFunction(ReturnAddressPtr returnAddress, FunctionPtr function)
    {
        relink(CodeLocationCall(CodePtr(returnAddress)), function);
    }
    
    void relinkNearCallerToTrampoline(ReturnAddressPtr returnAddress, CodeLocationLabel label)
    {
        relink(CodeLocationNearCall(CodePtr(returnAddress)), label);
    }
    
    void relinkNearCallerToTrampoline(ReturnAddressPtr returnAddress, CodePtr newCalleeFunction)
    {
        relinkNearCallerToTrampoline(returnAddress, CodeLocationLabel(newCalleeFunction));
    }

protected:
    void* m_start;
    size_t m_size;
};

} 

#endif 

#endif 
