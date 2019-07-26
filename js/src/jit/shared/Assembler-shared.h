





#ifndef jit_shared_Assembler_shared_h
#define jit_shared_Assembler_shared_h

#include "mozilla/PodOperations.h"

#include <limits.h>

#include "jsworkers.h"

#include "jit/IonAllocPolicy.h"
#include "jit/Label.h"
#include "jit/Registers.h"
#include "jit/RegisterSets.h"

#if defined(JS_CODEGEN_X64) || defined(JS_CODEGEN_ARM)


#    define JS_SMALL_BRANCH
#endif
namespace js {
namespace jit {

enum Scale {
    TimesOne = 0,
    TimesTwo = 1,
    TimesFour = 2,
    TimesEight = 3
};

static inline unsigned
ScaleToShift(Scale scale)
{
    return unsigned(scale);
}

static inline bool
IsShiftInScaleRange(int i)
{
    return i >= TimesOne && i <= TimesEight;
}

static inline Scale
ShiftToScale(int i)
{
    JS_ASSERT(IsShiftInScaleRange(i));
    return Scale(i);
}

static inline Scale
ScaleFromElemWidth(int shift)
{
    switch (shift) {
      case 1:
        return TimesOne;
      case 2:
        return TimesTwo;
      case 4:
        return TimesFour;
      case 8:
        return TimesEight;
    }

    MOZ_ASSUME_UNREACHABLE("Invalid scale");
}


struct Imm32
{
    int32_t value;

    explicit Imm32(int32_t value) : value(value)
    { }

    static inline Imm32 ShiftOf(enum Scale s) {
        switch (s) {
          case TimesOne:
            return Imm32(0);
          case TimesTwo:
            return Imm32(1);
          case TimesFour:
            return Imm32(2);
          case TimesEight:
            return Imm32(3);
        };
        MOZ_ASSUME_UNREACHABLE("Invalid scale");
    }

    static inline Imm32 FactorOf(enum Scale s) {
        return Imm32(1 << ShiftOf(s).value);
    }
};


struct ImmWord
{
    uintptr_t value;

    explicit ImmWord(uintptr_t value) : value(value)
    { }
};

#ifdef DEBUG
static inline bool
IsCompilingAsmJS()
{
    
    IonContext *ictx = MaybeGetIonContext();
    return ictx && ictx->compartment == nullptr;
}
#endif


struct ImmPtr
{
    void *value;

    explicit ImmPtr(const void *value) : value(const_cast<void*>(value))
    {
        
        
        JS_ASSERT(!IsCompilingAsmJS());
    }

    template <class R>
    explicit ImmPtr(R (*pf)())
      : value(JS_FUNC_TO_DATA_PTR(void *, pf))
    {
        JS_ASSERT(!IsCompilingAsmJS());
    }

    template <class R, class A1>
    explicit ImmPtr(R (*pf)(A1))
      : value(JS_FUNC_TO_DATA_PTR(void *, pf))
    {
        JS_ASSERT(!IsCompilingAsmJS());
    }

    template <class R, class A1, class A2>
    explicit ImmPtr(R (*pf)(A1, A2))
      : value(JS_FUNC_TO_DATA_PTR(void *, pf))
    {
        JS_ASSERT(!IsCompilingAsmJS());
    }

    template <class R, class A1, class A2, class A3>
    explicit ImmPtr(R (*pf)(A1, A2, A3))
      : value(JS_FUNC_TO_DATA_PTR(void *, pf))
    {
        JS_ASSERT(!IsCompilingAsmJS());
    }

    template <class R, class A1, class A2, class A3, class A4>
    explicit ImmPtr(R (*pf)(A1, A2, A3, A4))
      : value(JS_FUNC_TO_DATA_PTR(void *, pf))
    {
        JS_ASSERT(!IsCompilingAsmJS());
    }

};




struct PatchedImmPtr {
    void *value;

    explicit PatchedImmPtr()
      : value(nullptr)
    { }
    explicit PatchedImmPtr(const void *value)
      : value(const_cast<void*>(value))
    { }
};


struct ImmGCPtr
{
    const gc::Cell *value;

    explicit ImmGCPtr(const gc::Cell *ptr) : value(ptr)
    {
        JS_ASSERT(!IsPoisonedPtr(ptr));
        JS_ASSERT_IF(ptr, ptr->isTenured());

        
        JS_ASSERT(!IsCompilingAsmJS());
    }

  protected:
    ImmGCPtr() : value(0) {}
};


struct ImmMaybeNurseryPtr : public ImmGCPtr
{
    explicit ImmMaybeNurseryPtr(gc::Cell *ptr)
    {
        this->value = ptr;
        JS_ASSERT(!IsPoisonedPtr(ptr));

        
        JS_ASSERT(!IsCompilingAsmJS());
    }
};



struct AbsoluteAddress
{
    void *addr;

    explicit AbsoluteAddress(const void *addr)
      : addr(const_cast<void*>(addr))
    {
        
        JS_ASSERT(!IsCompilingAsmJS());
    }

    AbsoluteAddress offset(ptrdiff_t delta) {
        return AbsoluteAddress(((uint8_t *) addr) + delta);
    }
};




struct PatchedAbsoluteAddress
{
    void *addr;

    explicit PatchedAbsoluteAddress()
      : addr(nullptr)
    { }
    explicit PatchedAbsoluteAddress(const void *addr)
      : addr(const_cast<void*>(addr))
    { }
};



struct Address
{
    Register base;
    int32_t offset;

    Address(Register base, int32_t offset) : base(base), offset(offset)
    { }

    Address() { mozilla::PodZero(this); }
};



struct BaseIndex
{
    Register base;
    Register index;
    Scale scale;
    int32_t offset;

    BaseIndex(Register base, Register index, Scale scale, int32_t offset = 0)
      : base(base), index(index), scale(scale), offset(offset)
    { }

    BaseIndex() { mozilla::PodZero(this); }
};

class Relocation {
  public:
    enum Kind {
        
        
        HARDCODED,

        
        
        JITCODE
    };
};

class RepatchLabel
{
    static const int32_t INVALID_OFFSET = 0xC0000000;
    int32_t offset_ : 31;
    uint32_t bound_ : 1;
  public:

    RepatchLabel() : offset_(INVALID_OFFSET), bound_(0) {}

    void use(uint32_t newOffset) {
        JS_ASSERT(offset_ == INVALID_OFFSET);
        JS_ASSERT(newOffset != (uint32_t)INVALID_OFFSET);
        offset_ = newOffset;
    }
    bool bound() const {
        return bound_;
    }
    void bind(int32_t dest) {
        JS_ASSERT(!bound_);
        JS_ASSERT(dest != INVALID_OFFSET);
        offset_ = dest;
        bound_ = true;
    }
    int32_t target() {
        JS_ASSERT(bound());
        int32_t ret = offset_;
        offset_ = INVALID_OFFSET;
        return ret;
    }
    int32_t offset() {
        JS_ASSERT(!bound());
        return offset_;
    }
    bool used() const {
        return !bound() && offset_ != (INVALID_OFFSET);
    }

};



struct AbsoluteLabel : public LabelBase
{
  public:
    AbsoluteLabel()
    { }
    AbsoluteLabel(const AbsoluteLabel &label) : LabelBase(label)
    { }
    int32_t prev() const {
        JS_ASSERT(!bound());
        if (!used())
            return INVALID_OFFSET;
        return offset();
    }
    void setPrev(int32_t offset) {
        use(offset);
    }
    void bind() {
        bound_ = true;

        
        offset_ = -1;
    }
};



class CodeLabel
{
    
    AbsoluteLabel dest_;

    
    
    Label src_;

  public:
    CodeLabel()
    { }
    explicit CodeLabel(const AbsoluteLabel &dest)
       : dest_(dest)
    { }
    AbsoluteLabel *dest() {
        return &dest_;
    }
    Label *src() {
        return &src_;
    }
};




class CodeOffsetJump
{
    size_t offset_;

#ifdef JS_SMALL_BRANCH
    size_t jumpTableIndex_;
#endif

  public:

#ifdef JS_SMALL_BRANCH
    CodeOffsetJump(size_t offset, size_t jumpTableIndex)
        : offset_(offset), jumpTableIndex_(jumpTableIndex)
    {}
    size_t jumpTableIndex() const {
        return jumpTableIndex_;
    }
#else
    CodeOffsetJump(size_t offset) : offset_(offset) {}
#endif

    CodeOffsetJump() {
        mozilla::PodZero(this);
    }

    size_t offset() const {
        return offset_;
    }
    void fixup(MacroAssembler *masm);
};

class CodeOffsetLabel
{
    size_t offset_;

  public:
    explicit CodeOffsetLabel(size_t offset) : offset_(offset) {}
    CodeOffsetLabel() : offset_(0) {}

    size_t offset() const {
        return offset_;
    }
    void fixup(MacroAssembler *masm);

};






class CodeLocationJump
{
    uint8_t *raw_;
#ifdef DEBUG
    enum State { Uninitialized, Absolute, Relative };
    State state_;
    void setUninitialized() {
        state_ = Uninitialized;
    }
    void setAbsolute() {
        state_ = Absolute;
    }
    void setRelative() {
        state_ = Relative;
    }
#else
    void setUninitialized() const {
    }
    void setAbsolute() const {
    }
    void setRelative() const {
    }
#endif

#ifdef JS_SMALL_BRANCH
    uint8_t *jumpTableEntry_;
#endif

  public:
    CodeLocationJump() {
        raw_ = nullptr;
        setUninitialized();
#ifdef JS_SMALL_BRANCH
        jumpTableEntry_ = (uint8_t *) 0xdeadab1e;
#endif
    }
    CodeLocationJump(JitCode *code, CodeOffsetJump base) {
        *this = base;
        repoint(code);
    }

    void operator = (CodeOffsetJump base) {
        raw_ = (uint8_t *) base.offset();
        setRelative();
#ifdef JS_SMALL_BRANCH
        jumpTableEntry_ = (uint8_t *) base.jumpTableIndex();
#endif
    }

    void repoint(JitCode *code, MacroAssembler* masm = nullptr);

    uint8_t *raw() const {
        JS_ASSERT(state_ == Absolute);
        return raw_;
    }
    uint8_t *offset() const {
        JS_ASSERT(state_ == Relative);
        return raw_;
    }

#ifdef JS_SMALL_BRANCH
    uint8_t *jumpTableEntry() const {
        JS_ASSERT(state_ == Absolute);
        return jumpTableEntry_;
    }
#endif
};

class CodeLocationLabel
{
    uint8_t *raw_;
#ifdef DEBUG
    enum State { Uninitialized, Absolute, Relative };
    State state_;
    void setUninitialized() {
        state_ = Uninitialized;
    }
    void setAbsolute() {
        state_ = Absolute;
    }
    void setRelative() {
        state_ = Relative;
    }
#else
    void setUninitialized() const {
    }
    void setAbsolute() const {
    }
    void setRelative() const {
    }
#endif

  public:
    CodeLocationLabel() {
        raw_ = nullptr;
        setUninitialized();
    }
    CodeLocationLabel(JitCode *code, CodeOffsetLabel base) {
        *this = base;
        repoint(code);
    }
    explicit CodeLocationLabel(JitCode *code) {
        raw_ = code->raw();
        setAbsolute();
    }
    explicit CodeLocationLabel(uint8_t *raw) {
        raw_ = raw;
        setAbsolute();
    }

    void operator = (CodeOffsetLabel base) {
        raw_ = (uint8_t *)base.offset();
        setRelative();
    }
    ptrdiff_t operator - (const CodeLocationLabel &other) {
        return raw_ - other.raw_;
    }

    void repoint(JitCode *code, MacroAssembler *masm = nullptr);

#ifdef DEBUG
    bool isSet() const {
        return state_ != Uninitialized;
    }
#endif

    uint8_t *raw() const {
        JS_ASSERT(state_ == Absolute);
        return raw_;
    }
    uint8_t *offset() const {
        JS_ASSERT(state_ == Relative);
        return raw_;
    }
};















class CallSiteDesc
{
    uint32_t line_;
    uint32_t column_;
    uint32_t functionNameIndex_;

    static const uint32_t sEntryTrampoline = UINT32_MAX;
    static const uint32_t sExit = UINT32_MAX - 1;

  public:
    static const uint32_t FUNCTION_NAME_INDEX_MAX = UINT32_MAX - 2;

    CallSiteDesc() {}

    CallSiteDesc(uint32_t line, uint32_t column, uint32_t functionNameIndex)
     : line_(line), column_(column), functionNameIndex_(functionNameIndex)
    {}

    static CallSiteDesc Entry() { return CallSiteDesc(0, 0, sEntryTrampoline); }
    static CallSiteDesc Exit() { return CallSiteDesc(0, 0, sExit); }

    bool isEntry() const { return functionNameIndex_ == sEntryTrampoline; }
    bool isExit() const { return functionNameIndex_ == sExit; }
    bool isNormal() const { return !(isEntry() || isExit()); }

    uint32_t line() const { JS_ASSERT(isNormal()); return line_; }
    uint32_t column() const { JS_ASSERT(isNormal()); return column_; }
    uint32_t functionNameIndex() const { JS_ASSERT(isNormal()); return functionNameIndex_; }
};



struct CallSite : public CallSiteDesc
{
    uint32_t returnAddressOffset_;
    uint32_t stackDepth_;

  public:
    CallSite() {}

    CallSite(CallSiteDesc desc, uint32_t returnAddressOffset, uint32_t stackDepth)
      : CallSiteDesc(desc),
        returnAddressOffset_(returnAddressOffset),
        stackDepth_(stackDepth)
    { }

    void setReturnAddressOffset(uint32_t r) { returnAddressOffset_ = r; }
    uint32_t returnAddressOffset() const { return returnAddressOffset_; }

    
    
    
    uint32_t stackDepth() const { JS_ASSERT(!isEntry()); return stackDepth_; }
};

typedef Vector<CallSite, 0, SystemAllocPolicy> CallSiteVector;






class AsmJSHeapAccess
{
    uint32_t offset_;
#if defined(JS_CODEGEN_X86)
    uint8_t cmpDelta_;  
#endif
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
    uint8_t opLength_;  
    uint8_t isFloat32Load_;
    AnyRegister::Code loadedReg_ : 8;
#endif

    JS_STATIC_ASSERT(AnyRegister::Total < UINT8_MAX);

  public:
    AsmJSHeapAccess() {}
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
    
    
    AsmJSHeapAccess(uint32_t offset, uint32_t after, ArrayBufferView::ViewType vt,
                    AnyRegister loadedReg, uint32_t cmp = UINT32_MAX)
      : offset_(offset),
# if defined(JS_CODEGEN_X86)
        cmpDelta_(cmp == UINT32_MAX ? 0 : offset - cmp),
# endif
        opLength_(after - offset),
        isFloat32Load_(vt == ArrayBufferView::TYPE_FLOAT32),
        loadedReg_(loadedReg.code())
    {}
    AsmJSHeapAccess(uint32_t offset, uint8_t after, uint32_t cmp = UINT32_MAX)
      : offset_(offset),
# if defined(JS_CODEGEN_X86)
        cmpDelta_(cmp == UINT32_MAX ? 0 : offset - cmp),
# endif
        opLength_(after - offset),
        isFloat32Load_(false),
        loadedReg_(UINT8_MAX)
    {}
#elif defined(JS_CODEGEN_ARM)
    explicit AsmJSHeapAccess(uint32_t offset)
      : offset_(offset)
    {}
#endif

    uint32_t offset() const { return offset_; }
    void setOffset(uint32_t offset) { offset_ = offset; }
#if defined(JS_CODEGEN_X86)
    bool hasLengthCheck() const { return cmpDelta_ > 0; }
    void *patchLengthAt(uint8_t *code) const { return code + (offset_ - cmpDelta_); }
    void *patchOffsetAt(uint8_t *code) const { return code + (offset_ + opLength_); }
#endif
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
    unsigned opLength() const { return opLength_; }
    bool isLoad() const { return loadedReg_ != UINT8_MAX; }
    bool isFloat32Load() const { return isFloat32Load_; }
    AnyRegister loadedReg() const { return AnyRegister::FromCode(loadedReg_); }
#endif
};

typedef Vector<AsmJSHeapAccess, 0, SystemAllocPolicy> AsmJSHeapAccessVector;

struct AsmJSGlobalAccess
{
    CodeOffsetLabel patchAt;
    unsigned globalDataOffset;

    AsmJSGlobalAccess(CodeOffsetLabel patchAt, unsigned globalDataOffset)
      : patchAt(patchAt), globalDataOffset(globalDataOffset)
    {}
};




enum AsmJSImmKind
{
    AsmJSImm_Runtime,
    AsmJSImm_StackLimit,
    AsmJSImm_ReportOverRecursed,
    AsmJSImm_HandleExecutionInterrupt,
    AsmJSImm_InvokeFromAsmJS_Ignore,
    AsmJSImm_InvokeFromAsmJS_ToInt32,
    AsmJSImm_InvokeFromAsmJS_ToNumber,
    AsmJSImm_CoerceInPlace_ToInt32,
    AsmJSImm_CoerceInPlace_ToNumber,
    AsmJSImm_ToInt32,
#if defined(JS_CODEGEN_ARM)
    AsmJSImm_aeabi_idivmod,
    AsmJSImm_aeabi_uidivmod,
#endif
    AsmJSImm_ModD,
    AsmJSImm_SinD,
    AsmJSImm_CosD,
    AsmJSImm_TanD,
    AsmJSImm_ASinD,
    AsmJSImm_ACosD,
    AsmJSImm_ATanD,
    AsmJSImm_CeilD,
    AsmJSImm_CeilF,
    AsmJSImm_FloorD,
    AsmJSImm_FloorF,
    AsmJSImm_ExpD,
    AsmJSImm_LogD,
    AsmJSImm_PowD,
    AsmJSImm_ATan2D,
#ifdef DEBUG
    AsmJSImm_AssumeUnreachable,
#endif
    AsmJSImm_Invalid
};


class AsmJSImmPtr
{
    AsmJSImmKind kind_;
  public:
    AsmJSImmKind kind() const { return kind_; }
    
    MOZ_IMPLICIT AsmJSImmPtr(AsmJSImmKind kind) : kind_(kind) { JS_ASSERT(IsCompilingAsmJS()); }
    AsmJSImmPtr() {}
};



class AsmJSAbsoluteAddress
{
    AsmJSImmKind kind_;
  public:
    AsmJSImmKind kind() const { return kind_; }
    explicit AsmJSAbsoluteAddress(AsmJSImmKind kind) : kind_(kind) { JS_ASSERT(IsCompilingAsmJS()); }
    AsmJSAbsoluteAddress() {}
};




struct AsmJSAbsoluteLink
{
    AsmJSAbsoluteLink(CodeOffsetLabel patchAt, AsmJSImmKind target)
      : patchAt(patchAt), target(target) {}
    CodeOffsetLabel patchAt;
    AsmJSImmKind target;
};


class AssemblerShared
{
    Vector<CallSite, 0, SystemAllocPolicy> callsites_;
    Vector<AsmJSHeapAccess, 0, SystemAllocPolicy> asmJSHeapAccesses_;
    Vector<AsmJSGlobalAccess, 0, SystemAllocPolicy> asmJSGlobalAccesses_;
    Vector<AsmJSAbsoluteLink, 0, SystemAllocPolicy> asmJSAbsoluteLinks_;

  public:
    bool append(CallSite callsite) { return callsites_.append(callsite); }
    CallSiteVector &&extractCallSites() { return Move(callsites_); }

    bool append(AsmJSHeapAccess access) { return asmJSHeapAccesses_.append(access); }
    AsmJSHeapAccessVector &&extractAsmJSHeapAccesses() { return Move(asmJSHeapAccesses_); }

    bool append(AsmJSGlobalAccess access) { return asmJSGlobalAccesses_.append(access); }
    size_t numAsmJSGlobalAccesses() const { return asmJSGlobalAccesses_.length(); }
    AsmJSGlobalAccess asmJSGlobalAccess(size_t i) const { return asmJSGlobalAccesses_[i]; }

    bool append(AsmJSAbsoluteLink link) { return asmJSAbsoluteLinks_.append(link); }
    size_t numAsmJSAbsoluteLinks() const { return asmJSAbsoluteLinks_.length(); }
    AsmJSAbsoluteLink asmJSAbsoluteLink(size_t i) const { return asmJSAbsoluteLinks_[i]; }
};

} 
} 

#endif 
