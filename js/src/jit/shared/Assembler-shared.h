





#ifndef jit_shared_Assembler_shared_h
#define jit_shared_Assembler_shared_h

#include "mozilla/PodOperations.h"

#include <limits.h>

#include "asmjs/AsmJSFrameIterator.h"
#include "jit/JitAllocPolicy.h"
#include "jit/Label.h"
#include "jit/Registers.h"
#include "jit/RegisterSets.h"
#include "vm/HelperThreads.h"

#if defined(JS_CODEGEN_ARM)
#define JS_USE_LINK_REGISTER
#endif

#if defined(JS_CODEGEN_X64) || defined(JS_CODEGEN_ARM)


#    define JS_SMALL_BRANCH
#endif
namespace js {
namespace jit {

namespace Disassembler {
class HeapAccess;
};

static const uint32_t Simd128DataSize = 4 * sizeof(int32_t);
static_assert(Simd128DataSize == 4 * sizeof(int32_t), "SIMD data should be able to contain int32x4");
static_assert(Simd128DataSize == 4 * sizeof(float), "SIMD data should be able to contain float32x4");
static_assert(Simd128DataSize == 2 * sizeof(double), "SIMD data should be able to contain float64x2");

enum Scale {
    TimesOne = 0,
    TimesTwo = 1,
    TimesFour = 2,
    TimesEight = 3
};

static_assert(sizeof(JS::Value) == 8,
              "required for TimesEight and 3 below to be correct");
static const Scale ValueScale = TimesEight;
static const size_t ValueShift = 3;

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
    MOZ_ASSERT(IsShiftInScaleRange(i));
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

    MOZ_CRASH("Invalid scale");
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
        MOZ_CRASH("Invalid scale");
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
    
    JitContext* jctx = MaybeGetJitContext();
    return jctx && jctx->compartment == nullptr;
}

static inline bool
CanUsePointerImmediates()
{
    if (!IsCompilingAsmJS())
        return true;

    
    
    JitContext* jctx = MaybeGetJitContext();
    if (jctx && jctx->runtime->profilingScripts())
        return true;

    return false;
}
#endif


struct ImmPtr
{
    void* value;

    explicit ImmPtr(const void* value) : value(const_cast<void*>(value))
    {
        
        
        MOZ_ASSERT(CanUsePointerImmediates());
    }

    template <class R>
    explicit ImmPtr(R (*pf)())
      : value(JS_FUNC_TO_DATA_PTR(void*, pf))
    {
        MOZ_ASSERT(CanUsePointerImmediates());
    }

    template <class R, class A1>
    explicit ImmPtr(R (*pf)(A1))
      : value(JS_FUNC_TO_DATA_PTR(void*, pf))
    {
        MOZ_ASSERT(CanUsePointerImmediates());
    }

    template <class R, class A1, class A2>
    explicit ImmPtr(R (*pf)(A1, A2))
      : value(JS_FUNC_TO_DATA_PTR(void*, pf))
    {
        MOZ_ASSERT(CanUsePointerImmediates());
    }

    template <class R, class A1, class A2, class A3>
    explicit ImmPtr(R (*pf)(A1, A2, A3))
      : value(JS_FUNC_TO_DATA_PTR(void*, pf))
    {
        MOZ_ASSERT(CanUsePointerImmediates());
    }

    template <class R, class A1, class A2, class A3, class A4>
    explicit ImmPtr(R (*pf)(A1, A2, A3, A4))
      : value(JS_FUNC_TO_DATA_PTR(void*, pf))
    {
        MOZ_ASSERT(CanUsePointerImmediates());
    }

};




struct PatchedImmPtr {
    void* value;

    explicit PatchedImmPtr()
      : value(nullptr)
    { }
    explicit PatchedImmPtr(const void* value)
      : value(const_cast<void*>(value))
    { }
};

class AssemblerShared;
class ImmGCPtr;


class ImmMaybeNurseryPtr
{
    friend class AssemblerShared;
    friend class ImmGCPtr;
    const gc::Cell* value;

    ImmMaybeNurseryPtr() : value(0) {}

  public:
    explicit ImmMaybeNurseryPtr(const gc::Cell* ptr) : value(ptr)
    {
        
        MOZ_ASSERT(!IsCompilingAsmJS());
    }
};



class IonNurseryPtr
{
    const gc::Cell* ptr;

  public:
    friend class ImmGCPtr;

    explicit IonNurseryPtr(const gc::Cell* ptr) : ptr(ptr)
    {
        MOZ_ASSERT(ptr);
        MOZ_ASSERT(uintptr_t(ptr) & 0x1);
    }
};


class ImmGCPtr
{
  public:
    const gc::Cell* value;

    explicit ImmGCPtr(const gc::Cell* ptr) : value(ptr)
    {
        MOZ_ASSERT_IF(ptr, ptr->isTenured());

        
        MOZ_ASSERT(!IsCompilingAsmJS());
    }

    explicit ImmGCPtr(IonNurseryPtr ptr) : value(ptr.ptr)
    {
        MOZ_ASSERT(value);

        
        MOZ_ASSERT(!IsCompilingAsmJS());
    }

  private:
    ImmGCPtr() : value(0) {}

    friend class AssemblerShared;
    explicit ImmGCPtr(ImmMaybeNurseryPtr ptr) : value(ptr.value)
    {
        
        MOZ_ASSERT(!IsCompilingAsmJS());
    }
};



struct AbsoluteAddress
{
    void* addr;

    explicit AbsoluteAddress(const void* addr)
      : addr(const_cast<void*>(addr))
    {
        MOZ_ASSERT(CanUsePointerImmediates());
    }

    AbsoluteAddress offset(ptrdiff_t delta) {
        return AbsoluteAddress(((uint8_t*) addr) + delta);
    }
};




struct PatchedAbsoluteAddress
{
    void* addr;

    explicit PatchedAbsoluteAddress()
      : addr(nullptr)
    { }
    explicit PatchedAbsoluteAddress(const void* addr)
      : addr(const_cast<void*>(addr))
    { }
    explicit PatchedAbsoluteAddress(uintptr_t addr)
      : addr(reinterpret_cast<void*>(addr))
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







struct BaseValueIndex : BaseIndex
{
    BaseValueIndex(Register base, Register index, int32_t offset = 0)
      : BaseIndex(base, index, ValueScale, offset)
    { }
};



struct BaseObjectElementIndex : BaseValueIndex
{
    BaseObjectElementIndex(Register base, Register index, int32_t offset = 0)
      : BaseValueIndex(base, index, offset)
    {
        NativeObject::elementsSizeMustNotOverflow();
    }
};


struct BaseObjectSlotIndex : BaseValueIndex
{
    BaseObjectSlotIndex(Register base, Register index)
      : BaseValueIndex(base, index)
    {
        NativeObject::slotsSizeMustNotOverflow();
    }
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
        MOZ_ASSERT(offset_ == INVALID_OFFSET);
        MOZ_ASSERT(newOffset != (uint32_t)INVALID_OFFSET);
        offset_ = newOffset;
    }
    bool bound() const {
        return bound_;
    }
    void bind(int32_t dest) {
        MOZ_ASSERT(!bound_);
        MOZ_ASSERT(dest != INVALID_OFFSET);
        offset_ = dest;
        bound_ = true;
    }
    int32_t target() {
        MOZ_ASSERT(bound());
        int32_t ret = offset_;
        offset_ = INVALID_OFFSET;
        return ret;
    }
    int32_t offset() {
        MOZ_ASSERT(!bound());
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
    AbsoluteLabel(const AbsoluteLabel& label) : LabelBase(label)
    { }
    int32_t prev() const {
        MOZ_ASSERT(!bound());
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
    explicit CodeLabel(const AbsoluteLabel& dest)
       : dest_(dest)
    { }
    AbsoluteLabel* dest() {
        return &dest_;
    }
    Label* src() {
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
    void fixup(MacroAssembler* masm);
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
    void fixup(MacroAssembler* masm);

};






class CodeLocationJump
{
    uint8_t* raw_;
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
    uint8_t* jumpTableEntry_;
#endif

  public:
    CodeLocationJump() {
        raw_ = nullptr;
        setUninitialized();
#ifdef JS_SMALL_BRANCH
        jumpTableEntry_ = (uint8_t*) 0xdeadab1e;
#endif
    }
    CodeLocationJump(JitCode* code, CodeOffsetJump base) {
        *this = base;
        repoint(code);
    }

    void operator = (CodeOffsetJump base) {
        raw_ = (uint8_t*) base.offset();
        setRelative();
#ifdef JS_SMALL_BRANCH
        jumpTableEntry_ = (uint8_t*) base.jumpTableIndex();
#endif
    }

    void repoint(JitCode* code, MacroAssembler* masm = nullptr);

    uint8_t* raw() const {
        MOZ_ASSERT(state_ == Absolute);
        return raw_;
    }
    uint8_t* offset() const {
        MOZ_ASSERT(state_ == Relative);
        return raw_;
    }

#ifdef JS_SMALL_BRANCH
    uint8_t* jumpTableEntry() const {
        MOZ_ASSERT(state_ == Absolute);
        return jumpTableEntry_;
    }
#endif
};

class CodeLocationLabel
{
    uint8_t* raw_;
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
    CodeLocationLabel(JitCode* code, CodeOffsetLabel base) {
        *this = base;
        repoint(code);
    }
    explicit CodeLocationLabel(JitCode* code) {
        raw_ = code->raw();
        setAbsolute();
    }
    explicit CodeLocationLabel(uint8_t* raw) {
        raw_ = raw;
        setAbsolute();
    }

    void operator = (CodeOffsetLabel base) {
        raw_ = (uint8_t*)base.offset();
        setRelative();
    }
    ptrdiff_t operator - (const CodeLocationLabel& other) {
        return raw_ - other.raw_;
    }

    void repoint(JitCode* code, MacroAssembler* masm = nullptr);

#ifdef DEBUG
    bool isSet() const {
        return state_ != Uninitialized;
    }
#endif

    uint8_t* raw() const {
        MOZ_ASSERT(state_ == Absolute);
        return raw_;
    }
    uint8_t* offset() const {
        MOZ_ASSERT(state_ == Relative);
        return raw_;
    }
};





class CallSiteDesc
{
    uint32_t line_;
    uint32_t column_ : 31;
    uint32_t kind_ : 1;
  public:
    enum Kind {
        Relative,  
        Register   
    };
    CallSiteDesc() {}
    explicit CallSiteDesc(Kind kind)
      : line_(0), column_(0), kind_(kind)
    {}
    CallSiteDesc(uint32_t line, uint32_t column, Kind kind)
      : line_(line), column_(column), kind_(kind)
    {
        MOZ_ASSERT(column <= INT32_MAX);
    }
    uint32_t line() const { return line_; }
    uint32_t column() const { return column_; }
    Kind kind() const { return Kind(kind_); }
};



class CallSite : public CallSiteDesc
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

    
    
    
    
    uint32_t stackDepth() const { return stackDepth_; }
};

typedef Vector<CallSite, 0, SystemAllocPolicy> CallSiteVector;






struct AsmJSFrame
{
    
    
    
    
    uint8_t* callerFP;

    
    
    void* returnAddress;
};
static_assert(sizeof(AsmJSFrame) == 2 * sizeof(void*), "?!");
static const uint32_t AsmJSFrameBytesAfterReturnAddress = sizeof(void*);



static const unsigned AsmJSActivationGlobalDataOffset = 0;
static const unsigned AsmJSHeapGlobalDataOffset = sizeof(void*);
static const unsigned AsmJSNaN64GlobalDataOffset = 2 * sizeof(void*);
static const unsigned AsmJSNaN32GlobalDataOffset = 2 * sizeof(void*) + sizeof(double);






class AsmJSHeapAccess
{
#if defined(JS_CODEGEN_X64)
  public:
    enum WhatToDoOnOOB {
        CarryOn, 
        Throw    
    };
#endif

  private:
    uint32_t insnOffset_;
#if defined(JS_CODEGEN_X86)
    uint8_t opLength_;  
#endif
#if defined(JS_CODEGEN_X64)
    uint8_t offsetWithinWholeSimdVector_; 
    bool throwOnOOB_;   
#endif
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
    uint8_t cmpDelta_;  
#endif

    JS_STATIC_ASSERT(AnyRegister::Total < UINT8_MAX);

  public:
    AsmJSHeapAccess() {}
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
    static const uint32_t NoLengthCheck = UINT32_MAX;
#endif

#if defined(JS_CODEGEN_X86)
    
    
    AsmJSHeapAccess(uint32_t insnOffset, uint32_t after, uint32_t cmp = NoLengthCheck)
    {
        mozilla::PodZero(this);  
        insnOffset_ = insnOffset;
        opLength_ = after - insnOffset;
        cmpDelta_ = cmp == NoLengthCheck ? 0 : insnOffset - cmp;
    }
#elif defined(JS_CODEGEN_X64)
    
    
    AsmJSHeapAccess(uint32_t insnOffset, WhatToDoOnOOB oob,
                    uint32_t cmp = NoLengthCheck,
                    uint32_t offsetWithinWholeSimdVector = 0)
    {
        mozilla::PodZero(this);  
        insnOffset_ = insnOffset;
        offsetWithinWholeSimdVector_ = offsetWithinWholeSimdVector;
        throwOnOOB_ = oob == Throw;
        cmpDelta_ = cmp == NoLengthCheck ? 0 : insnOffset - cmp;
        MOZ_ASSERT(offsetWithinWholeSimdVector_ == offsetWithinWholeSimdVector);
    }
#elif defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    explicit AsmJSHeapAccess(uint32_t insnOffset)
    {
        mozilla::PodZero(this);  
        insnOffset_ = insnOffset;
    }
#endif

    uint32_t insnOffset() const { return insnOffset_; }
    void setInsnOffset(uint32_t insnOffset) { insnOffset_ = insnOffset; }
#if defined(JS_CODEGEN_X86)
    void* patchHeapPtrImmAt(uint8_t* code) const { return code + (insnOffset_ + opLength_); }
#endif
#if defined(JS_CODEGEN_X64)
    bool throwOnOOB() const { return throwOnOOB_; }
    uint32_t offsetWithinWholeSimdVector() const { return offsetWithinWholeSimdVector_; }
#endif
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
    bool hasLengthCheck() const { return cmpDelta_ > 0; }
    void* patchLengthAt(uint8_t* code) const {
        MOZ_ASSERT(hasLengthCheck());
        return code + (insnOffset_ - cmpDelta_);
    }
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
    AsmJSImm_ToInt32         = AsmJSExit::Builtin_ToInt32,
#if defined(JS_CODEGEN_ARM)
    AsmJSImm_aeabi_idivmod   = AsmJSExit::Builtin_IDivMod,
    AsmJSImm_aeabi_uidivmod  = AsmJSExit::Builtin_UDivMod,
    AsmJSImm_AtomicCmpXchg   = AsmJSExit::Builtin_AtomicCmpXchg,
    AsmJSImm_AtomicFetchAdd  = AsmJSExit::Builtin_AtomicFetchAdd,
    AsmJSImm_AtomicFetchSub  = AsmJSExit::Builtin_AtomicFetchSub,
    AsmJSImm_AtomicFetchAnd  = AsmJSExit::Builtin_AtomicFetchAnd,
    AsmJSImm_AtomicFetchOr   = AsmJSExit::Builtin_AtomicFetchOr,
    AsmJSImm_AtomicFetchXor  = AsmJSExit::Builtin_AtomicFetchXor,
#endif
    AsmJSImm_ModD            = AsmJSExit::Builtin_ModD,
    AsmJSImm_SinD            = AsmJSExit::Builtin_SinD,
    AsmJSImm_CosD            = AsmJSExit::Builtin_CosD,
    AsmJSImm_TanD            = AsmJSExit::Builtin_TanD,
    AsmJSImm_ASinD           = AsmJSExit::Builtin_ASinD,
    AsmJSImm_ACosD           = AsmJSExit::Builtin_ACosD,
    AsmJSImm_ATanD           = AsmJSExit::Builtin_ATanD,
    AsmJSImm_CeilD           = AsmJSExit::Builtin_CeilD,
    AsmJSImm_CeilF           = AsmJSExit::Builtin_CeilF,
    AsmJSImm_FloorD          = AsmJSExit::Builtin_FloorD,
    AsmJSImm_FloorF          = AsmJSExit::Builtin_FloorF,
    AsmJSImm_ExpD            = AsmJSExit::Builtin_ExpD,
    AsmJSImm_LogD            = AsmJSExit::Builtin_LogD,
    AsmJSImm_PowD            = AsmJSExit::Builtin_PowD,
    AsmJSImm_ATan2D          = AsmJSExit::Builtin_ATan2D,
    AsmJSImm_Runtime,
    AsmJSImm_RuntimeInterruptUint32,
    AsmJSImm_StackLimit,
    AsmJSImm_ReportOverRecursed,
    AsmJSImm_OnDetached,
    AsmJSImm_OnOutOfBounds,
    AsmJSImm_OnImpreciseConversion,
    AsmJSImm_HandleExecutionInterrupt,
    AsmJSImm_InvokeFromAsmJS_Ignore,
    AsmJSImm_InvokeFromAsmJS_ToInt32,
    AsmJSImm_InvokeFromAsmJS_ToNumber,
    AsmJSImm_CoerceInPlace_ToInt32,
    AsmJSImm_CoerceInPlace_ToNumber,
    AsmJSImm_Limit
};

static inline AsmJSImmKind
BuiltinToImmKind(AsmJSExit::BuiltinKind builtin)
{
    return AsmJSImmKind(builtin);
}

static inline bool
ImmKindIsBuiltin(AsmJSImmKind imm, AsmJSExit::BuiltinKind* builtin)
{
    if (unsigned(imm) >= unsigned(AsmJSExit::Builtin_Limit))
        return false;
    *builtin = AsmJSExit::BuiltinKind(imm);
    return true;
}


class AsmJSImmPtr
{
    AsmJSImmKind kind_;
  public:
    AsmJSImmKind kind() const { return kind_; }
    
    MOZ_IMPLICIT AsmJSImmPtr(AsmJSImmKind kind) : kind_(kind) { MOZ_ASSERT(IsCompilingAsmJS()); }
    AsmJSImmPtr() {}
};



class AsmJSAbsoluteAddress
{
    AsmJSImmKind kind_;
  public:
    AsmJSImmKind kind() const { return kind_; }
    explicit AsmJSAbsoluteAddress(AsmJSImmKind kind) : kind_(kind) { MOZ_ASSERT(IsCompilingAsmJS()); }
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

  protected:
    Vector<CodeOffsetLabel, 0, SystemAllocPolicy> profilerCallSites_;
    bool enoughMemory_;
    bool embedsNurseryPointers_;

  public:
    AssemblerShared()
     : enoughMemory_(true),
       embedsNurseryPointers_(false)
    {}

    void propagateOOM(bool success) {
        enoughMemory_ &= success;
    }

    void setOOM() {
        enoughMemory_ = false;
    }

    bool oom() const {
        return !enoughMemory_;
    }

    void appendProfilerCallSite(CodeOffsetLabel label) {
        enoughMemory_ &= profilerCallSites_.append(label);
    }

    bool embedsNurseryPointers() const {
        return embedsNurseryPointers_;
    }

    ImmGCPtr noteMaybeNurseryPtr(ImmMaybeNurseryPtr ptr) {
        if (ptr.value && gc::IsInsideNursery(ptr.value)) {
            
            
            MOZ_ASSERT(GetJitContext()->runtime->onMainThread());
            
            MOZ_ASSERT(!GetJitContext()->runtime->mainThread()->ionCompiling);
            embedsNurseryPointers_ = true;
        }
        return ImmGCPtr(ptr);
    }

    void append(const CallSiteDesc& desc, size_t currentOffset, size_t framePushed) {
        
        
        CallSite callsite(desc, currentOffset, framePushed + sizeof(AsmJSFrame));
        enoughMemory_ &= callsites_.append(callsite);
    }
    CallSiteVector&& extractCallSites() { return Move(callsites_); }

    void append(AsmJSHeapAccess access) { enoughMemory_ &= asmJSHeapAccesses_.append(access); }
    AsmJSHeapAccessVector&& extractAsmJSHeapAccesses() { return Move(asmJSHeapAccesses_); }

    void append(AsmJSGlobalAccess access) { enoughMemory_ &= asmJSGlobalAccesses_.append(access); }
    size_t numAsmJSGlobalAccesses() const { return asmJSGlobalAccesses_.length(); }
    AsmJSGlobalAccess asmJSGlobalAccess(size_t i) const { return asmJSGlobalAccesses_[i]; }

    void append(AsmJSAbsoluteLink link) { enoughMemory_ &= asmJSAbsoluteLinks_.append(link); }
    size_t numAsmJSAbsoluteLinks() const { return asmJSAbsoluteLinks_.length(); }
    AsmJSAbsoluteLink asmJSAbsoluteLink(size_t i) const { return asmJSAbsoluteLinks_[i]; }

    static bool canUseInSingleByteInstruction(Register reg) { return true; }
};

} 
} 

#endif 
