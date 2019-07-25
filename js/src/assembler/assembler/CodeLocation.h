




























#ifndef CodeLocation_h
#define CodeLocation_h

#include "assembler/wtf/Platform.h"
#include "assembler/assembler/MacroAssemblerCodeRef.h"

#if ENABLE_ASSEMBLER

namespace JSC {

class CodeLocationInstruction;
class CodeLocationLabel;
class CodeLocationJump;
class CodeLocationCall;
class CodeLocationNearCall;
class CodeLocationDataLabel32;
class CodeLocationDataLabelPtr;












class CodeLocationCommon : public MacroAssemblerCodePtr {
public:
    CodeLocationInstruction instructionAtOffset(int offset);
    CodeLocationLabel labelAtOffset(int offset);
    CodeLocationJump jumpAtOffset(int offset);
    CodeLocationCall callAtOffset(int offset);
    CodeLocationNearCall nearCallAtOffset(int offset);
    CodeLocationDataLabelPtr dataLabelPtrAtOffset(int offset);
    CodeLocationDataLabel32 dataLabel32AtOffset(int offset);

protected:
    CodeLocationCommon()
    {
    }

    CodeLocationCommon(MacroAssemblerCodePtr location)
        : MacroAssemblerCodePtr(location)
    {
    }
};

class CodeLocationInstruction : public CodeLocationCommon {
public:
    CodeLocationInstruction() {}
    explicit CodeLocationInstruction(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationInstruction(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationLabel : public CodeLocationCommon {
public:
    CodeLocationLabel() {}
    explicit CodeLocationLabel(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationLabel(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationJump : public CodeLocationCommon {
public:
    CodeLocationJump() {}
    explicit CodeLocationJump(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationJump(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationCall : public CodeLocationCommon {
public:
    CodeLocationCall() {}
    explicit CodeLocationCall(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationCall(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationNearCall : public CodeLocationCommon {
public:
    CodeLocationNearCall() {}
    explicit CodeLocationNearCall(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationNearCall(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationDataLabel32 : public CodeLocationCommon {
public:
    CodeLocationDataLabel32() {}
    explicit CodeLocationDataLabel32(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationDataLabel32(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationDataLabelPtr : public CodeLocationCommon {
public:
    CodeLocationDataLabelPtr() {}
    explicit CodeLocationDataLabelPtr(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationDataLabelPtr(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

inline CodeLocationInstruction CodeLocationCommon::instructionAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationInstruction(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationLabel CodeLocationCommon::labelAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationLabel(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationJump CodeLocationCommon::jumpAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationJump(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationCall CodeLocationCommon::callAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationCall(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationNearCall CodeLocationCommon::nearCallAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationNearCall(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationDataLabelPtr CodeLocationCommon::dataLabelPtrAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationDataLabelPtr(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationDataLabel32 CodeLocationCommon::dataLabel32AtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationDataLabel32(reinterpret_cast<char*>(dataLocation()) + offset);
}

} 

#endif 

#endif 
