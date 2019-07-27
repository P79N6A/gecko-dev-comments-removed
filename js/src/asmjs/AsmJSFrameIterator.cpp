

















#include "asmjs/AsmJSFrameIterator.h"

#include "asmjs/AsmJSModule.h"
#include "asmjs/AsmJSValidate.h"
#include "jit/MacroAssembler.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;




static void*
ReturnAddressFromFP(void* fp)
{
    return reinterpret_cast<AsmJSFrame*>(fp)->returnAddress;
}

static uint8_t*
CallerFPFromFP(void* fp)
{
    return reinterpret_cast<AsmJSFrame*>(fp)->callerFP;
}

AsmJSFrameIterator::AsmJSFrameIterator(const AsmJSActivation& activation)
  : module_(&activation.module()),
    fp_(activation.fp())
{
    if (!fp_)
        return;
    settle();
}

void
AsmJSFrameIterator::operator++()
{
    MOZ_ASSERT(!done());
    DebugOnly<uint8_t*> oldfp = fp_;
    fp_ += callsite_->stackDepth();
    MOZ_ASSERT_IF(module_->profilingEnabled(), fp_ == CallerFPFromFP(oldfp));
    settle();
}

void
AsmJSFrameIterator::settle()
{
    void* returnAddress = ReturnAddressFromFP(fp_);

    const AsmJSModule::CodeRange* codeRange = module_->lookupCodeRange(returnAddress);
    MOZ_ASSERT(codeRange);
    codeRange_ = codeRange;

    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Function:
        callsite_ = module_->lookupCallSite(returnAddress);
        MOZ_ASSERT(callsite_);
        break;
      case AsmJSModule::CodeRange::Entry:
        fp_ = nullptr;
        MOZ_ASSERT(done());
        break;
      case AsmJSModule::CodeRange::JitFFI:
      case AsmJSModule::CodeRange::SlowFFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Inline:
      case AsmJSModule::CodeRange::Thunk:
        MOZ_CRASH("Should not encounter an exit during iteration");
    }
}

JSAtom*
AsmJSFrameIterator::functionDisplayAtom() const
{
    MOZ_ASSERT(!done());
    return reinterpret_cast<const AsmJSModule::CodeRange*>(codeRange_)->functionName(*module_);
}

unsigned
AsmJSFrameIterator::computeLine(uint32_t* column) const
{
    MOZ_ASSERT(!done());
    if (column)
        *column = callsite_->column();
    return callsite_->line();
}







#if defined(JS_CODEGEN_X64)
# if defined(DEBUG)
static const unsigned PushedRetAddr = 0;
static const unsigned PostStorePrePopFP = 0;
# endif
static const unsigned PushedFP = 13;
static const unsigned StoredFP = 20;
#elif defined(JS_CODEGEN_X86)
# if defined(DEBUG)
static const unsigned PushedRetAddr = 0;
static const unsigned PostStorePrePopFP = 0;
# endif
static const unsigned PushedFP = 8;
static const unsigned StoredFP = 11;
#elif defined(JS_CODEGEN_ARM)
static const unsigned PushedRetAddr = 4;
static const unsigned PushedFP = 16;
static const unsigned StoredFP = 20;
static const unsigned PostStorePrePopFP = 4;
#elif defined(JS_CODEGEN_MIPS)
static const unsigned PushedRetAddr = 8;
static const unsigned PushedFP = 24;
static const unsigned StoredFP = 28;
static const unsigned PostStorePrePopFP = 4;
#elif defined(JS_CODEGEN_NONE)
# if defined(DEBUG)
static const unsigned PushedRetAddr = 0;
static const unsigned PostStorePrePopFP = 0;
# endif
static const unsigned PushedFP = 1;
static const unsigned StoredFP = 1;
#else
# error "Unknown architecture!"
#endif

static void
PushRetAddr(MacroAssembler& masm)
{
#if defined(JS_CODEGEN_ARM)
    masm.push(lr);
#elif defined(JS_CODEGEN_MIPS)
    masm.push(ra);
#else
    
#endif
}




static void
GenerateProfilingPrologue(MacroAssembler& masm, unsigned framePushed, AsmJSExit::Reason reason,
                          Label* begin)
{
#if !defined (JS_CODEGEN_ARM)
    Register scratch = ABIArgGenerator::NonArg_VolatileReg;
#else
    
    
    
    Register scratch = lr;
    masm.setSecondScratchReg(InvalidReg);
#endif

    
    
    
    
    
    {
#if defined(JS_CODEGEN_ARM)
        AutoForbidPools afp(&masm,  5);
#endif
        DebugOnly<uint32_t> offsetAtBegin = masm.currentOffset();
        masm.bind(begin);

        PushRetAddr(masm);
        MOZ_ASSERT(PushedRetAddr == masm.currentOffset() - offsetAtBegin);

        masm.loadAsmJSActivation(scratch);
        masm.push(Address(scratch, AsmJSActivation::offsetOfFP()));
        MOZ_ASSERT(PushedFP == masm.currentOffset() - offsetAtBegin);

        masm.storePtr(masm.getStackPointer(), Address(scratch, AsmJSActivation::offsetOfFP()));
        MOZ_ASSERT(StoredFP == masm.currentOffset() - offsetAtBegin);
    }

    if (reason != AsmJSExit::None)
        masm.store32_NoSecondScratch(Imm32(reason), Address(scratch, AsmJSActivation::offsetOfExitReason()));

#if defined(JS_CODEGEN_ARM)
    masm.setSecondScratchReg(lr);
#endif

    if (framePushed)
        masm.subFromStackPtr(Imm32(framePushed));
}


static void
GenerateProfilingEpilogue(MacroAssembler& masm, unsigned framePushed, AsmJSExit::Reason reason,
                          Label* profilingReturn)
{
    Register scratch = ABIArgGenerator::NonReturn_VolatileReg0;
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
    Register scratch2 = ABIArgGenerator::NonReturn_VolatileReg1;
#endif

    if (framePushed)
        masm.addToStackPtr(Imm32(framePushed));

    masm.loadAsmJSActivation(scratch);

    if (reason != AsmJSExit::None)
        masm.store32(Imm32(AsmJSExit::None), Address(scratch, AsmJSActivation::offsetOfExitReason()));

    
    
    
    {
#if defined(JS_CODEGEN_ARM)
        AutoForbidPools afp(&masm,  4);
#endif

        
        
        
        
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        masm.loadPtr(Address(masm.getStackPointer(), 0), scratch2);
        masm.storePtr(scratch2, Address(scratch, AsmJSActivation::offsetOfFP()));
        DebugOnly<uint32_t> prePop = masm.currentOffset();
        masm.add32(Imm32(4), masm.getStackPointer());
        MOZ_ASSERT(PostStorePrePopFP == masm.currentOffset() - prePop);
#else
        masm.pop(Address(scratch, AsmJSActivation::offsetOfFP()));
        MOZ_ASSERT(PostStorePrePopFP == 0);
#endif

        masm.bind(profilingReturn);
        masm.ret();
    }
}








void
js::GenerateAsmJSFunctionPrologue(MacroAssembler& masm, unsigned framePushed,
                                  AsmJSFunctionLabels* labels)
{
#if defined(JS_CODEGEN_ARM)
    
    
    masm.flushBuffer();
#endif

    masm.haltingAlign(CodeAlignment);

    GenerateProfilingPrologue(masm, framePushed, AsmJSExit::None, &labels->begin);
    Label body;
    masm.jump(&body);

    
    masm.haltingAlign(CodeAlignment);
    masm.bind(&labels->entry);
    PushRetAddr(masm);
    masm.subFromStackPtr(Imm32(framePushed + AsmJSFrameBytesAfterReturnAddress));

    
    masm.bind(&body);
    masm.setFramePushed(framePushed);

    
    
    
    if (labels->overflowThunk) {
        
        Label* target = framePushed ? labels->overflowThunk.ptr() : &labels->overflowExit;
        masm.branchPtr(Assembler::AboveOrEqual,
                       AsmJSAbsoluteAddress(AsmJSImm_StackLimit),
                       masm.getStackPointer(),
                       target);
    }
}






void
js::GenerateAsmJSFunctionEpilogue(MacroAssembler& masm, unsigned framePushed,
                                  AsmJSFunctionLabels* labels)
{
    MOZ_ASSERT(masm.framePushed() == framePushed);

#if defined(JS_CODEGEN_ARM)
    
    
    
    masm.flushBuffer();
#endif

    {
#if defined(JS_CODEGEN_ARM)
        
        
        AutoForbidPools afp(&masm, 1);
#endif

        
        
        masm.bind(&labels->profilingJump);
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
        masm.twoByteNop();
#elif defined(JS_CODEGEN_ARM)
        masm.nop();
#elif defined(JS_CODEGEN_MIPS)
        masm.nop();
        masm.nop();
        masm.nop();
        masm.nop();
#endif
    }

    
    masm.addToStackPtr(Imm32(framePushed + AsmJSFrameBytesAfterReturnAddress));
    masm.ret();
    masm.setFramePushed(0);

    
    masm.bind(&labels->profilingEpilogue);
    GenerateProfilingEpilogue(masm, framePushed, AsmJSExit::None, &labels->profilingReturn);

    if (labels->overflowThunk && labels->overflowThunk->used()) {
        
        
        
        masm.bind(labels->overflowThunk.ptr());
        masm.addToStackPtr(Imm32(framePushed));
        masm.jump(&labels->overflowExit);
    }
}

void
js::GenerateAsmJSStackOverflowExit(MacroAssembler& masm, Label* overflowExit, Label* throwLabel)
{
    masm.bind(overflowExit);

    
    
    
    
    
    
    Register activation = ABIArgGenerator::NonArgReturnReg0;
    masm.loadAsmJSActivation(activation);
    masm.storePtr(masm.getStackPointer(), Address(activation, AsmJSActivation::offsetOfFP()));

    
    if (uint32_t d = StackDecrementForCall(ABIStackAlignment, sizeof(AsmJSFrame), ShadowStackSpace))
        masm.subFromStackPtr(Imm32(d));

    
    masm.assertStackAlignment(ABIStackAlignment);
    masm.call(AsmJSImmPtr(AsmJSImm_ReportOverRecursed));
    masm.jump(throwLabel);
}

void
js::GenerateAsmJSExitPrologue(MacroAssembler& masm, unsigned framePushed, AsmJSExit::Reason reason,
                              Label* begin)
{
    masm.haltingAlign(CodeAlignment);
    GenerateProfilingPrologue(masm, framePushed, reason, begin);
    masm.setFramePushed(framePushed);
}

void
js::GenerateAsmJSExitEpilogue(MacroAssembler& masm, unsigned framePushed, AsmJSExit::Reason reason,
                              Label* profilingReturn)
{
    
    MOZ_ASSERT(masm.framePushed() == framePushed);
    GenerateProfilingEpilogue(masm, framePushed, reason, profilingReturn);
    masm.setFramePushed(0);
}




AsmJSProfilingFrameIterator::AsmJSProfilingFrameIterator(const AsmJSActivation& activation)
  : module_(&activation.module()),
    callerFP_(nullptr),
    callerPC_(nullptr),
    stackAddress_(nullptr),
    exitReason_(AsmJSExit::None),
    codeRange_(nullptr)
{
    
    
    
    
    
    if (!module_->profilingEnabled()) {
        MOZ_ASSERT(done());
        return;
    }

    initFromFP(activation);
}

static inline void
AssertMatchesCallSite(const AsmJSModule& module, const AsmJSModule::CodeRange* calleeCodeRange,
                      void* callerPC, void* callerFP, void* fp)
{
#ifdef DEBUG
    const AsmJSModule::CodeRange* callerCodeRange = module.lookupCodeRange(callerPC);
    MOZ_ASSERT(callerCodeRange);
    if (callerCodeRange->isEntry()) {
        MOZ_ASSERT(callerFP == nullptr);
        return;
    }

    const CallSite* callsite = module.lookupCallSite(callerPC);
    if (calleeCodeRange->isThunk()) {
        MOZ_ASSERT(!callsite);
        MOZ_ASSERT(callerCodeRange->isFunction());
    } else {
        MOZ_ASSERT(callsite);
        MOZ_ASSERT(callerFP == (uint8_t*)fp + callsite->stackDepth());
    }
#endif
}

void
AsmJSProfilingFrameIterator::initFromFP(const AsmJSActivation& activation)
{
    uint8_t* fp = activation.fp();

    
    
    if (!fp) {
        MOZ_ASSERT(done());
        return;
    }

    
    
    
    
    
    
    
    
    void* pc = ReturnAddressFromFP(fp);
    const AsmJSModule::CodeRange* codeRange = module_->lookupCodeRange(pc);
    MOZ_ASSERT(codeRange);
    codeRange_ = codeRange;
    stackAddress_ = fp;

    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Entry:
        callerPC_ = nullptr;
        callerFP_ = nullptr;
        break;
      case AsmJSModule::CodeRange::Function:
        fp = CallerFPFromFP(fp);
        callerPC_ = ReturnAddressFromFP(fp);
        callerFP_ = CallerFPFromFP(fp);
        AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, fp);
        break;
      case AsmJSModule::CodeRange::JitFFI:
      case AsmJSModule::CodeRange::SlowFFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Inline:
      case AsmJSModule::CodeRange::Thunk:
        MOZ_CRASH("Unexpected CodeRange kind");
    }

    
    
    
    
    
    
    
    exitReason_ = activation.exitReason();
    if (exitReason_ == AsmJSExit::None)
        exitReason_ = AsmJSExit::Interrupt;

    MOZ_ASSERT(!done());
}

typedef JS::ProfilingFrameIterator::RegisterState RegisterState;

AsmJSProfilingFrameIterator::AsmJSProfilingFrameIterator(const AsmJSActivation& activation,
                                                         const RegisterState& state)
  : module_(&activation.module()),
    callerFP_(nullptr),
    callerPC_(nullptr),
    exitReason_(AsmJSExit::None),
    codeRange_(nullptr)
{
    
    
    
    
    
    if (!module_->profilingEnabled()) {
        MOZ_ASSERT(done());
        return;
    }

    
    
    if (!module_->containsCodePC(state.pc)) {
        initFromFP(activation);
        return;
    }

    
    uint8_t* fp = activation.fp();

    const AsmJSModule::CodeRange* codeRange = module_->lookupCodeRange(state.pc);
    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Function:
      case AsmJSModule::CodeRange::JitFFI:
      case AsmJSModule::CodeRange::SlowFFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Thunk: {
        
        
        
        
        
        
        
        uint32_t offsetInModule = (uint8_t*)state.pc - module_->codeBase();
        MOZ_ASSERT(offsetInModule < module_->codeBytes());
        MOZ_ASSERT(offsetInModule >= codeRange->begin());
        MOZ_ASSERT(offsetInModule < codeRange->end());
        uint32_t offsetInCodeRange = offsetInModule - codeRange->begin();
        void** sp = (void**)state.sp;
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        if (offsetInCodeRange < PushedRetAddr) {
            
            
            callerPC_ = state.lr;
            callerFP_ = fp;
            AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, sp - 2);
        } else if (offsetInModule == codeRange->profilingReturn() - PostStorePrePopFP) {
            
            
            callerPC_ = ReturnAddressFromFP(sp);
            callerFP_ = CallerFPFromFP(sp);
            AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, sp);
        } else
#endif
        if (offsetInCodeRange < PushedFP || offsetInModule == codeRange->profilingReturn()) {
            
            
            callerPC_ = *sp;
            callerFP_ = fp;
            AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, sp - 1);
        } else if (offsetInCodeRange < StoredFP) {
            
            
            MOZ_ASSERT(fp == CallerFPFromFP(sp));
            callerPC_ = ReturnAddressFromFP(sp);
            callerFP_ = CallerFPFromFP(sp);
            AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, sp);
        } else {
            
            callerPC_ = ReturnAddressFromFP(fp);
            callerFP_ = CallerFPFromFP(fp);
            AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, fp);
        }
        break;
      }
      case AsmJSModule::CodeRange::Entry: {
        
        
        
        MOZ_ASSERT(!fp);
        callerPC_ = nullptr;
        callerFP_ = nullptr;
        break;
      }
      case AsmJSModule::CodeRange::Inline: {
        
        if (!fp) {
            MOZ_ASSERT(done());
            return;
        }

        
        
        
        
        
        callerPC_ = ReturnAddressFromFP(fp);
        callerFP_ = CallerFPFromFP(fp);
        AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, fp);
        break;
      }
    }

    codeRange_ = codeRange;
    stackAddress_ = state.sp;
    MOZ_ASSERT(!done());
}

void
AsmJSProfilingFrameIterator::operator++()
{
    if (exitReason_ != AsmJSExit::None) {
        MOZ_ASSERT(codeRange_);
        exitReason_ = AsmJSExit::None;
        MOZ_ASSERT(!done());
        return;
    }

    if (!callerPC_) {
        MOZ_ASSERT(!callerFP_);
        codeRange_ = nullptr;
        MOZ_ASSERT(done());
        return;
    }

    MOZ_ASSERT(callerPC_);
    const AsmJSModule::CodeRange* codeRange = module_->lookupCodeRange(callerPC_);
    MOZ_ASSERT(codeRange);
    codeRange_ = codeRange;

    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Entry:
        MOZ_ASSERT(callerFP_ == nullptr);
        callerPC_ = nullptr;
        break;
      case AsmJSModule::CodeRange::Function:
      case AsmJSModule::CodeRange::JitFFI:
      case AsmJSModule::CodeRange::SlowFFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Inline:
      case AsmJSModule::CodeRange::Thunk:
        stackAddress_ = callerFP_;
        callerPC_ = ReturnAddressFromFP(callerFP_);
        AssertMatchesCallSite(*module_, codeRange, callerPC_, CallerFPFromFP(callerFP_), callerFP_);
        callerFP_ = CallerFPFromFP(callerFP_);
        break;
    }

    MOZ_ASSERT(!done());
}

static const char*
BuiltinToName(AsmJSExit::BuiltinKind builtin)
{
    
    

    switch (builtin) {
      case AsmJSExit::Builtin_ToInt32:   return "ToInt32 (in asm.js)";
#if defined(JS_CODEGEN_ARM)
      case AsmJSExit::Builtin_IDivMod:   return "software idivmod (in asm.js)";
      case AsmJSExit::Builtin_UDivMod:   return "software uidivmod (in asm.js)";
      case AsmJSExit::Builtin_AtomicCmpXchg:  return "Atomics.compareExchange (in asm.js)";
      case AsmJSExit::Builtin_AtomicFetchAdd: return "Atomics.add (in asm.js)";
      case AsmJSExit::Builtin_AtomicFetchSub: return "Atomics.sub (in asm.js)";
      case AsmJSExit::Builtin_AtomicFetchAnd: return "Atomics.and (in asm.js)";
      case AsmJSExit::Builtin_AtomicFetchOr:  return "Atomics.or (in asm.js)";
      case AsmJSExit::Builtin_AtomicFetchXor: return "Atomics.xor (in asm.js)";
#endif
      case AsmJSExit::Builtin_ModD:      return "fmod (in asm.js)";
      case AsmJSExit::Builtin_SinD:      return "Math.sin (in asm.js)";
      case AsmJSExit::Builtin_CosD:      return "Math.cos (in asm.js)";
      case AsmJSExit::Builtin_TanD:      return "Math.tan (in asm.js)";
      case AsmJSExit::Builtin_ASinD:     return "Math.asin (in asm.js)";
      case AsmJSExit::Builtin_ACosD:     return "Math.acos (in asm.js)";
      case AsmJSExit::Builtin_ATanD:     return "Math.atan (in asm.js)";
      case AsmJSExit::Builtin_CeilD:
      case AsmJSExit::Builtin_CeilF:     return "Math.ceil (in asm.js)";
      case AsmJSExit::Builtin_FloorD:
      case AsmJSExit::Builtin_FloorF:    return "Math.floor (in asm.js)";
      case AsmJSExit::Builtin_ExpD:      return "Math.exp (in asm.js)";
      case AsmJSExit::Builtin_LogD:      return "Math.log (in asm.js)";
      case AsmJSExit::Builtin_PowD:      return "Math.pow (in asm.js)";
      case AsmJSExit::Builtin_ATan2D:    return "Math.atan2 (in asm.js)";
      case AsmJSExit::Builtin_Limit:     break;
    }
    MOZ_CRASH("Bad builtin kind");
}

const char*
AsmJSProfilingFrameIterator::label() const
{
    MOZ_ASSERT(!done());

    
    
    
    
    
    const char* jitFFIDescription = "fast FFI trampoline (in asm.js)";
    const char* slowFFIDescription = "slow FFI trampoline (in asm.js)";
    const char* interruptDescription = "interrupt due to out-of-bounds or long execution (in asm.js)";

    switch (AsmJSExit::ExtractReasonKind(exitReason_)) {
      case AsmJSExit::Reason_None:
        break;
      case AsmJSExit::Reason_JitFFI:
        return jitFFIDescription;
      case AsmJSExit::Reason_SlowFFI:
        return slowFFIDescription;
      case AsmJSExit::Reason_Interrupt:
        return interruptDescription;
      case AsmJSExit::Reason_Builtin:
        return BuiltinToName(AsmJSExit::ExtractBuiltinKind(exitReason_));
    }

    auto codeRange = reinterpret_cast<const AsmJSModule::CodeRange*>(codeRange_);
    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Function:  return codeRange->functionProfilingLabel(*module_);
      case AsmJSModule::CodeRange::Entry:     return "entry trampoline (in asm.js)";
      case AsmJSModule::CodeRange::JitFFI:    return jitFFIDescription;
      case AsmJSModule::CodeRange::SlowFFI:   return slowFFIDescription;
      case AsmJSModule::CodeRange::Interrupt: return interruptDescription;
      case AsmJSModule::CodeRange::Inline:    return "inline stub (in asm.js)";
      case AsmJSModule::CodeRange::Thunk:     return BuiltinToName(codeRange->thunkTarget());
    }

    MOZ_CRASH("Bad exit kind");
}
