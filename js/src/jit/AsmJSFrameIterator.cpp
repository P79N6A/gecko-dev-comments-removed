





#include "jit/AsmJSFrameIterator.h"

#include "jit/AsmJS.h"
#include "jit/AsmJSModule.h"
#include "jit/IonMacroAssembler.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;




static void *
ReturnAddressFromFP(void *fp)
{
    return reinterpret_cast<AsmJSFrame*>(fp)->returnAddress;
}

static uint8_t *
CallerFPFromFP(void *fp)
{
    return reinterpret_cast<AsmJSFrame*>(fp)->callerFP;
}

AsmJSFrameIterator::AsmJSFrameIterator(const AsmJSActivation &activation)
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
    JS_ASSERT(!done());
    DebugOnly<uint8_t*> oldfp = fp_;
    fp_ += callsite_->stackDepth();
    JS_ASSERT_IF(module_->profilingEnabled(), fp_ == CallerFPFromFP(oldfp));
    settle();
}

void
AsmJSFrameIterator::settle()
{
    void *returnAddress = ReturnAddressFromFP(fp_);

    const AsmJSModule::CodeRange *codeRange = module_->lookupCodeRange(returnAddress);
    JS_ASSERT(codeRange);
    codeRange_ = codeRange;

    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Function:
        callsite_ = module_->lookupCallSite(returnAddress);
        JS_ASSERT(callsite_);
        break;
      case AsmJSModule::CodeRange::Entry:
        fp_ = nullptr;
        JS_ASSERT(done());
        break;
      case AsmJSModule::CodeRange::FFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Inline:
      case AsmJSModule::CodeRange::Thunk:
        MOZ_ASSUME_UNREACHABLE("Should not encounter an exit during iteration");
    }
}

JSAtom *
AsmJSFrameIterator::functionDisplayAtom() const
{
    JS_ASSERT(!done());
    return reinterpret_cast<const AsmJSModule::CodeRange*>(codeRange_)->functionName(*module_);
}

unsigned
AsmJSFrameIterator::computeLine(uint32_t *column) const
{
    JS_ASSERT(!done());
    if (column)
        *column = callsite_->column();
    return callsite_->line();
}







#if defined(JS_CODEGEN_X64)
static const unsigned PushedRetAddr = 0;
static const unsigned PushedFP = 11;
static const unsigned StoredFP = 15;
#elif defined(JS_CODEGEN_X86)
static const unsigned PushedRetAddr = 0;
static const unsigned PushedFP = 9;
static const unsigned StoredFP = 12;
#elif defined(JS_CODEGEN_ARM)
static const unsigned PushedRetAddr = 4;
static const unsigned PushedFP = 16;
static const unsigned StoredFP = 20;
#elif defined(JS_CODEGEN_NONE)
static const unsigned PushedRetAddr = 0;
static const unsigned PushedFP = 1;
static const unsigned StoredFP = 1;
#else
# error "Unknown architecture!"
#endif

static void
PushRetAddr(MacroAssembler &masm)
{
#if defined(JS_CODEGEN_ARM)
    masm.push(lr);
#elif defined(JS_CODEGEN_MIPS)
    masm.push(ra);
#else
    
#endif
}




static void
GenerateProfilingPrologue(MacroAssembler &masm, unsigned framePushed, AsmJSExit::Reason reason,
                          Label *begin)
{
    Register act = ABIArgGenerator::NonArgReturnVolatileReg0;

    
    
    
    
    
    {
#if defined(JS_CODEGEN_ARM)
        AutoForbidPools afp(&masm,  5);
#endif
        DebugOnly<uint32_t> offsetAtBegin = masm.currentOffset();
        masm.bind(begin);

        PushRetAddr(masm);
        JS_ASSERT(PushedRetAddr == masm.currentOffset() - offsetAtBegin);

        masm.loadAsmJSActivation(act);
        masm.push(Address(act, AsmJSActivation::offsetOfFP()));
        JS_ASSERT(PushedFP == masm.currentOffset() - offsetAtBegin);

        masm.storePtr(StackPointer, Address(act, AsmJSActivation::offsetOfFP()));
        JS_ASSERT(StoredFP == masm.currentOffset() - offsetAtBegin);
    }

    if (reason != AsmJSExit::None)
        masm.store32(Imm32(reason), Address(act, AsmJSActivation::offsetOfExitReason()));

    if (framePushed)
        masm.subPtr(Imm32(framePushed), StackPointer);
}


static void
GenerateProfilingEpilogue(MacroAssembler &masm, unsigned framePushed, AsmJSExit::Reason reason,
                          Label *profilingReturn)
{
    Register act = ABIArgGenerator::NonArgReturnVolatileReg0;

    if (framePushed)
        masm.addPtr(Imm32(framePushed), StackPointer);

    masm.loadAsmJSActivation(act);

    if (reason != AsmJSExit::None)
        masm.store32(Imm32(AsmJSExit::None), Address(act, AsmJSActivation::offsetOfExitReason()));

    
    
    
    
    
    {
#if defined(JS_CODEGEN_ARM)
        AutoForbidPools afp(&masm,  3);
#endif
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
        masm.pop(Operand(act, AsmJSActivation::offsetOfFP()));
#else
        Register fp = ABIArgGenerator::NonArgReturnVolatileReg1;
        masm.pop(fp);
        masm.storePtr(fp, Address(act, AsmJSActivation::offsetOfFP()));
#endif
        masm.bind(profilingReturn);
        masm.ret();
    }
}








void
js::GenerateAsmJSFunctionPrologue(MacroAssembler &masm, unsigned framePushed,
                                  AsmJSFunctionLabels *labels)
{
#if defined(JS_CODEGEN_ARM)
    
    
    masm.flushBuffer();
#endif

    masm.align(CodeAlignment);

    GenerateProfilingPrologue(masm, framePushed, AsmJSExit::None, &labels->begin);
    Label body;
    masm.jump(&body);

    
    masm.align(CodeAlignment);
    masm.bind(&labels->entry);
    PushRetAddr(masm);
    masm.subPtr(Imm32(framePushed + AsmJSFrameBytesAfterReturnAddress), StackPointer);

    
    masm.bind(&body);
    masm.setFramePushed(framePushed);

    
    
    
    if (!labels->overflowThunk.empty()) {
        
        Label *target = framePushed ? labels->overflowThunk.addr() : &labels->overflowExit;
        masm.branchPtr(Assembler::AboveOrEqual,
                       AsmJSAbsoluteAddress(AsmJSImm_StackLimit),
                       StackPointer,
                       target);
    }
}






void
js::GenerateAsmJSFunctionEpilogue(MacroAssembler &masm, unsigned framePushed,
                                  AsmJSFunctionLabels *labels)
{
    JS_ASSERT(masm.framePushed() == framePushed);

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
#elif defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        masm.nop();
#endif
    }

    
    masm.addPtr(Imm32(framePushed + AsmJSFrameBytesAfterReturnAddress), StackPointer);
    masm.ret();
    masm.setFramePushed(0);

    
    masm.bind(&labels->profilingEpilogue);
    GenerateProfilingEpilogue(masm, framePushed, AsmJSExit::None, &labels->profilingReturn);

    if (!labels->overflowThunk.empty() && labels->overflowThunk.ref().used()) {
        
        
        
        masm.bind(labels->overflowThunk.addr());
        masm.addPtr(Imm32(framePushed), StackPointer);
        masm.jump(&labels->overflowExit);
    }
}

void
js::GenerateAsmJSStackOverflowExit(MacroAssembler &masm, Label *overflowExit, Label *throwLabel)
{
    masm.bind(overflowExit);

    
    
    
    
    
    
    Register activation = ABIArgGenerator::NonArgReturnVolatileReg0;
    masm.loadAsmJSActivation(activation);
    masm.storePtr(StackPointer, Address(activation, AsmJSActivation::offsetOfFP()));

    
    if (unsigned stackDec = StackDecrementForCall(sizeof(AsmJSFrame), ShadowStackSpace))
        masm.subPtr(Imm32(stackDec), StackPointer);

    
    masm.assertStackAlignment();
    masm.call(AsmJSImmPtr(AsmJSImm_ReportOverRecursed));
    masm.jump(throwLabel);
}

void
js::GenerateAsmJSEntryPrologue(MacroAssembler &masm, Label *begin)
{
    
    
    
    masm.align(CodeAlignment);
    masm.bind(begin);
    PushRetAddr(masm);
    masm.subPtr(Imm32(AsmJSFrameBytesAfterReturnAddress), StackPointer);
    masm.setFramePushed(0);
}

void
js::GenerateAsmJSEntryEpilogue(MacroAssembler &masm)
{
    
    JS_ASSERT(masm.framePushed() == 0);
    masm.addPtr(Imm32(AsmJSFrameBytesAfterReturnAddress), StackPointer);
    masm.ret();
    masm.setFramePushed(0);
}

void
js::GenerateAsmJSExitPrologue(MacroAssembler &masm, unsigned framePushed, AsmJSExit::Reason reason,
                              Label *begin)
{
    masm.align(CodeAlignment);
    GenerateProfilingPrologue(masm, framePushed, reason, begin);
    masm.setFramePushed(framePushed);
}

void
js::GenerateAsmJSExitEpilogue(MacroAssembler &masm, unsigned framePushed, AsmJSExit::Reason reason,
                              Label *profilingReturn)
{
    
    JS_ASSERT(masm.framePushed() == framePushed);
    GenerateProfilingEpilogue(masm, framePushed, reason, profilingReturn);
    masm.setFramePushed(0);
}




AsmJSProfilingFrameIterator::AsmJSProfilingFrameIterator(const AsmJSActivation &activation)
  : module_(&activation.module()),
    callerFP_(nullptr),
    callerPC_(nullptr),
    exitReason_(AsmJSExit::None),
    codeRange_(nullptr)
{
    initFromFP(activation);
}

static inline void
AssertMatchesCallSite(const AsmJSModule &module, const AsmJSModule::CodeRange *calleeCodeRange,
                      void *callerPC, void *callerFP, void *fp)
{
#ifdef DEBUG
    const AsmJSModule::CodeRange *callerCodeRange = module.lookupCodeRange(callerPC);
    JS_ASSERT(callerCodeRange);
    if (callerCodeRange->isEntry()) {
        JS_ASSERT(callerFP == nullptr);
        return;
    }

    const CallSite *callsite = module.lookupCallSite(callerPC);
    if (calleeCodeRange->isThunk()) {
        JS_ASSERT(!callsite);
        JS_ASSERT(callerCodeRange->isFunction());
    } else {
        JS_ASSERT(callsite);
        JS_ASSERT(callerFP == (uint8_t*)fp + callsite->stackDepth());
    }
#endif
}

void
AsmJSProfilingFrameIterator::initFromFP(const AsmJSActivation &activation)
{
    uint8_t *fp = activation.fp();

    
    
    if (!fp) {
        JS_ASSERT(done());
        return;
    }

    
    
    
    
    
    
    
    
    
    

    exitReason_ = activation.exitReason();

    void *pc = ReturnAddressFromFP(fp);
    const AsmJSModule::CodeRange *codeRange = module_->lookupCodeRange(pc);
    JS_ASSERT(codeRange);
    codeRange_ = codeRange;

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
      case AsmJSModule::CodeRange::FFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Inline:
      case AsmJSModule::CodeRange::Thunk:
        MOZ_CRASH("Unexpected CodeRange kind");
    }

    JS_ASSERT(!done());
}

typedef JS::ProfilingFrameIterator::RegisterState RegisterState;

AsmJSProfilingFrameIterator::AsmJSProfilingFrameIterator(const AsmJSActivation &activation,
                                                         const RegisterState &state)
  : module_(&activation.module()),
    callerFP_(nullptr),
    callerPC_(nullptr),
    exitReason_(AsmJSExit::None),
    codeRange_(nullptr)
{
    
    
    
    
    
    if (!module_->profilingEnabled()) {
        JS_ASSERT(done());
        return;
    }

    
    
    if (!module_->containsCodePC(state.pc)) {
        initFromFP(activation);
        return;
    }

    
    uint8_t *fp = activation.fp();

    const AsmJSModule::CodeRange *codeRange = module_->lookupCodeRange(state.pc);
    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Function:
      case AsmJSModule::CodeRange::FFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Thunk: {
        
        
        
        
        
        
        
        
        
        uint32_t offsetInModule = ((uint8_t*)state.pc) - module_->codeBase();
        JS_ASSERT(offsetInModule < module_->codeBytes());
        JS_ASSERT(offsetInModule >= codeRange->begin());
        JS_ASSERT(offsetInModule < codeRange->end());
        uint32_t offsetInCodeRange = offsetInModule - codeRange->begin();
        void **sp = (void**)state.sp;
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        if (offsetInCodeRange < PushedRetAddr) {
            callerPC_ = state.lr;
            callerFP_ = fp;
            AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, sp - 2);
        } else
#endif
        if (offsetInCodeRange < PushedFP || offsetInModule == codeRange->profilingReturn()) {
            callerPC_ = *sp;
            callerFP_ = fp;
            AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, sp - 1);
        } else if (offsetInCodeRange < StoredFP) {
            JS_ASSERT(fp == CallerFPFromFP(sp));
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
        
        
        
        JS_ASSERT(!fp);
        callerPC_ = nullptr;
        callerFP_ = nullptr;
        break;
      }
      case AsmJSModule::CodeRange::Inline: {
        
        if (!fp) {
            JS_ASSERT(done());
            return;
        }

        
        
        
        callerPC_ = ReturnAddressFromFP(fp);
        callerFP_ = CallerFPFromFP(fp);
        AssertMatchesCallSite(*module_, codeRange, callerPC_, callerFP_, fp);
        break;
      }
    }

    codeRange_ = codeRange;
    JS_ASSERT(!done());
}

void
AsmJSProfilingFrameIterator::operator++()
{
    if (exitReason_ != AsmJSExit::None) {
        JS_ASSERT(codeRange_);
        exitReason_ = AsmJSExit::None;
        JS_ASSERT(!done());
        return;
    }

    if (!callerPC_) {
        JS_ASSERT(!callerFP_);
        codeRange_ = nullptr;
        JS_ASSERT(done());
        return;
    }

    JS_ASSERT(callerPC_);
    const AsmJSModule::CodeRange *codeRange = module_->lookupCodeRange(callerPC_);
    JS_ASSERT(codeRange);
    codeRange_ = codeRange;

    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Entry:
        callerPC_ = nullptr;
        callerFP_ = nullptr;
        break;
      case AsmJSModule::CodeRange::Function:
      case AsmJSModule::CodeRange::FFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Inline:
      case AsmJSModule::CodeRange::Thunk:
        callerPC_ = ReturnAddressFromFP(callerFP_);
        AssertMatchesCallSite(*module_, codeRange, callerPC_, CallerFPFromFP(callerFP_), callerFP_);
        callerFP_ = CallerFPFromFP(callerFP_);
        break;
    }

    JS_ASSERT(!done());
}

AsmJSProfilingFrameIterator::Kind
AsmJSProfilingFrameIterator::kind() const
{
    JS_ASSERT(!done());

    switch (AsmJSExit::ExtractReasonKind(exitReason_)) {
      case AsmJSExit::Reason_None:
        break;
      case AsmJSExit::Reason_Interrupt:
      case AsmJSExit::Reason_FFI:
        return JS::ProfilingFrameIterator::AsmJSTrampoline;
      case AsmJSExit::Reason_Builtin:
        return JS::ProfilingFrameIterator::CppFunction;
    }

    auto codeRange = reinterpret_cast<const AsmJSModule::CodeRange*>(codeRange_);
    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Function:
        return JS::ProfilingFrameIterator::Function;
      case AsmJSModule::CodeRange::Entry:
      case AsmJSModule::CodeRange::FFI:
      case AsmJSModule::CodeRange::Interrupt:
      case AsmJSModule::CodeRange::Inline:
        return JS::ProfilingFrameIterator::AsmJSTrampoline;
      case AsmJSModule::CodeRange::Thunk:
        return JS::ProfilingFrameIterator::CppFunction;
    }

    MOZ_ASSUME_UNREACHABLE("Bad kind");
}

JSAtom *
AsmJSProfilingFrameIterator::functionDisplayAtom() const
{
    JS_ASSERT(kind() == JS::ProfilingFrameIterator::Function);
    return reinterpret_cast<const AsmJSModule::CodeRange*>(codeRange_)->functionName(*module_);
}

const char *
AsmJSProfilingFrameIterator::functionFilename() const
{
    JS_ASSERT(kind() == JS::ProfilingFrameIterator::Function);
    return module_->scriptSource()->filename();
}

static const char *
BuiltinToName(AsmJSExit::BuiltinKind builtin)
{
    switch (builtin) {
      case AsmJSExit::Builtin_ToInt32:   return "ToInt32";
#if defined(JS_CODEGEN_ARM)
      case AsmJSExit::Builtin_IDivMod:   return "software idivmod";
      case AsmJSExit::Builtin_UDivMod:   return "software uidivmod";
#endif
      case AsmJSExit::Builtin_ModD:      return "fmod";
      case AsmJSExit::Builtin_SinD:      return "Math.sin";
      case AsmJSExit::Builtin_CosD:      return "Math.cos";
      case AsmJSExit::Builtin_TanD:      return "Math.tan";
      case AsmJSExit::Builtin_ASinD:     return "Math.asin";
      case AsmJSExit::Builtin_ACosD:     return "Math.acos";
      case AsmJSExit::Builtin_ATanD:     return "Math.atan";
      case AsmJSExit::Builtin_CeilD:
      case AsmJSExit::Builtin_CeilF:     return "Math.ceil";
      case AsmJSExit::Builtin_FloorD:
      case AsmJSExit::Builtin_FloorF:    return "Math.floor";
      case AsmJSExit::Builtin_ExpD:      return "Math.exp";
      case AsmJSExit::Builtin_LogD:      return "Math.log";
      case AsmJSExit::Builtin_PowD:      return "Math.pow";
      case AsmJSExit::Builtin_ATan2D:    return "Math.atan2";
      case AsmJSExit::Builtin_Limit:     break;
    }
    MOZ_ASSUME_UNREACHABLE("Bad builtin kind");
}

const char *
AsmJSProfilingFrameIterator::nonFunctionDescription() const
{
    JS_ASSERT(!done());
    JS_ASSERT(kind() != JS::ProfilingFrameIterator::Function);

    
    
    const char *ffiDescription = "asm.js FFI trampoline";
    const char *interruptDescription = "asm.js slow script interrupt";

    switch (AsmJSExit::ExtractReasonKind(exitReason_)) {
      case AsmJSExit::Reason_None:
        break;
      case AsmJSExit::Reason_FFI:
        return ffiDescription;
      case AsmJSExit::Reason_Interrupt:
        return interruptDescription;
      case AsmJSExit::Reason_Builtin:
        return BuiltinToName(AsmJSExit::ExtractBuiltinKind(exitReason_));
    }

    auto codeRange = reinterpret_cast<const AsmJSModule::CodeRange*>(codeRange_);
    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Function:  MOZ_ASSUME_UNREACHABLE("non-functions only");
      case AsmJSModule::CodeRange::Entry:     return "asm.js entry trampoline";
      case AsmJSModule::CodeRange::FFI:       return ffiDescription;
      case AsmJSModule::CodeRange::Interrupt: return interruptDescription;
      case AsmJSModule::CodeRange::Inline:    return "asm.js inline stub";
      case AsmJSModule::CodeRange::Thunk:     return BuiltinToName(codeRange->thunkTarget());
    }

    MOZ_ASSUME_UNREACHABLE("Bad exit kind");
}

