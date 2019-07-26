





#include "jit/CodeGenerator.h"

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "jslibmath.h"
#include "jsmath.h"
#include "jsnum.h"

#include "builtin/Eval.h"
#include "builtin/TypedObject.h"
#ifdef JSGC_GENERATIONAL
# include "gc/Nursery.h"
#endif
#include "jit/ExecutionModeInlines.h"
#include "jit/IonCaches.h"
#include "jit/IonLinker.h"
#include "jit/IonSpewer.h"
#include "jit/MIRGenerator.h"
#include "jit/MoveEmitter.h"
#include "jit/ParallelFunctions.h"
#include "jit/ParallelSafetyAnalysis.h"
#include "jit/RangeAnalysis.h"
#include "vm/ForkJoin.h"

#include "jsboolinlines.h"

#include "jit/shared/CodeGenerator-shared-inl.h"
#include "vm/Interpreter-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;
using mozilla::DoubleExponentBias;
using mozilla::Maybe;
using mozilla::NegativeInfinity;
using mozilla::PositiveInfinity;
using JS::GenericNaN;

namespace js {
namespace jit {



class OutOfLineUpdateCache :
  public OutOfLineCodeBase<CodeGenerator>,
  public IonCacheVisitor
{
  private:
    LInstruction *lir_;
    size_t cacheIndex_;
    AddCacheState state_;

  public:
    OutOfLineUpdateCache(LInstruction *lir, size_t cacheIndex)
      : lir_(lir),
        cacheIndex_(cacheIndex)
    { }

    void bind(MacroAssembler *masm) {
        
        
    }

    size_t getCacheIndex() const {
        return cacheIndex_;
    }
    LInstruction *lir() const {
        return lir_;
    }
    AddCacheState &state() {
        return state_;
    }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineCache(this);
    }

    
#define VISIT_CACHE_FUNCTION(op)                                        \
    bool visit##op##IC(CodeGenerator *codegen) {                        \
        CodeGenerator::DataPtr<op##IC> ic(codegen, getCacheIndex());    \
        return codegen->visit##op##IC(this, ic);                        \
    }

    IONCACHE_KIND_LIST(VISIT_CACHE_FUNCTION)
#undef VISIT_CACHE_FUNCTION
};





bool
CodeGeneratorShared::addCache(LInstruction *lir, size_t cacheIndex)
{
    DataPtr<IonCache> cache(this, cacheIndex);
    MInstruction *mir = lir->mirRaw()->toInstruction();
    if (mir->resumePoint())
        cache->setScriptedLocation(mir->block()->info().script(),
                                   mir->resumePoint()->pc());
    else
        cache->setIdempotent();

    OutOfLineUpdateCache *ool = new OutOfLineUpdateCache(lir, cacheIndex);
    if (!addOutOfLineCode(ool))
        return false;

    
    cache->initializeAddCacheState(lir, &ool->state());

    cache->emitInitialJump(masm, ool->state());
    masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitOutOfLineCache(OutOfLineUpdateCache *ool)
{
    DataPtr<IonCache> cache(this, ool->getCacheIndex());

    
    cache->setFallbackLabel(masm.labelForPatch());
    cache->bindInitialJump(masm, ool->state());

    
    return cache->accept(this, ool);
}

StringObject *
MNewStringObject::templateObj() const {
    return &templateObj_->as<StringObject>();
}

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm)
  : CodeGeneratorSpecific(gen, graph, masm),
    unassociatedScriptCounts_(nullptr)
{
}

CodeGenerator::~CodeGenerator()
{
    JS_ASSERT_IF(!gen->compilingAsmJS(), masm.numAsmJSAbsoluteLinks() == 0);
    js_delete(unassociatedScriptCounts_);
}

typedef bool (*StringToNumberFn)(ThreadSafeContext *, JSString *, double *);
typedef ParallelResult (*StringToNumberParFn)(ForkJoinSlice *, JSString *, double *);
static const VMFunctionsModal StringToNumberInfo = VMFunctionsModal(
    FunctionInfo<StringToNumberFn>(StringToNumber),
    FunctionInfo<StringToNumberParFn>(StringToNumberPar));

bool
CodeGenerator::visitValueToInt32(LValueToInt32 *lir)
{
    ValueOperand operand = ToValue(lir, LValueToInt32::Input);
    Register output = ToRegister(lir->output());
    FloatRegister temp = ToFloatRegister(lir->tempFloat());

    MDefinition *input;
    if (lir->mode() == LValueToInt32::NORMAL)
        input = lir->mirNormal()->input();
    else
        input = lir->mirTruncate()->input();

    Label fails;
    if (lir->mode() == LValueToInt32::TRUNCATE) {
        OutOfLineCode *oolDouble = oolTruncateDouble(temp, output);
        if (!oolDouble)
            return false;

        
        
        Label *stringEntry, *stringRejoin;
        Register stringReg;
        if (input->mightBeType(MIRType_String)) {
            stringReg = ToRegister(lir->temp());
            OutOfLineCode *oolString = oolCallVM(StringToNumberInfo, lir, (ArgList(), stringReg),
                                                 StoreFloatRegisterTo(temp));
            if (!oolString)
                return false;
            stringEntry = oolString->entry();
            stringRejoin = oolString->rejoin();
        } else {
            stringReg = InvalidReg;
            stringEntry = nullptr;
            stringRejoin = nullptr;
        }

        masm.truncateValueToInt32(operand, input, stringEntry, stringRejoin, oolDouble->entry(),
                                  stringReg, temp, output, &fails);
        masm.bind(oolDouble->rejoin());
    } else {
        masm.convertValueToInt32(operand, input, temp, output, &fails,
                                 lir->mirNormal()->canBeNegativeZero());
    }

    return bailoutFrom(&fails, lir->snapshot());
}

bool
CodeGenerator::visitValueToDouble(LValueToDouble *lir)
{
    MToDouble *mir = lir->mir();
    ValueOperand operand = ToValue(lir, LValueToDouble::Input);
    FloatRegister output = ToFloatRegister(lir->output());

    Register tag = masm.splitTagForTest(operand);

    Label isDouble, isInt32, isBool, isNull, isUndefined, done;
    bool hasBoolean = false, hasNull = false, hasUndefined = false;

    masm.branchTestDouble(Assembler::Equal, tag, &isDouble);
    masm.branchTestInt32(Assembler::Equal, tag, &isInt32);

    if (mir->conversion() != MToDouble::NumbersOnly) {
        masm.branchTestBoolean(Assembler::Equal, tag, &isBool);
        masm.branchTestUndefined(Assembler::Equal, tag, &isUndefined);
        hasBoolean = true;
        hasUndefined = true;
        if (mir->conversion() != MToDouble::NonNullNonStringPrimitives) {
            masm.branchTestNull(Assembler::Equal, tag, &isNull);
            hasNull = true;
        }
    }

    if (!bailout(lir->snapshot()))
        return false;

    if (hasNull) {
        masm.bind(&isNull);
        masm.loadConstantDouble(0.0, output);
        masm.jump(&done);
    }

    if (hasUndefined) {
        masm.bind(&isUndefined);
        masm.loadConstantDouble(GenericNaN(), output);
        masm.jump(&done);
    }

    if (hasBoolean) {
        masm.bind(&isBool);
        masm.boolValueToDouble(operand, output);
        masm.jump(&done);
    }

    masm.bind(&isInt32);
    masm.int32ValueToDouble(operand, output);
    masm.jump(&done);

    masm.bind(&isDouble);
    masm.unboxDouble(operand, output);
    masm.bind(&done);

    return true;
}

bool
CodeGenerator::visitValueToFloat32(LValueToFloat32 *lir)
{
    MToFloat32 *mir = lir->mir();
    ValueOperand operand = ToValue(lir, LValueToFloat32::Input);
    FloatRegister output = ToFloatRegister(lir->output());

    Register tag = masm.splitTagForTest(operand);

    Label isDouble, isInt32, isBool, isNull, isUndefined, done;
    bool hasBoolean = false, hasNull = false, hasUndefined = false;

    masm.branchTestDouble(Assembler::Equal, tag, &isDouble);
    masm.branchTestInt32(Assembler::Equal, tag, &isInt32);

    if (mir->conversion() != MToFloat32::NumbersOnly) {
        masm.branchTestBoolean(Assembler::Equal, tag, &isBool);
        masm.branchTestUndefined(Assembler::Equal, tag, &isUndefined);
        hasBoolean = true;
        hasUndefined = true;
        if (mir->conversion() != MToFloat32::NonNullNonStringPrimitives) {
            masm.branchTestNull(Assembler::Equal, tag, &isNull);
            hasNull = true;
        }
    }

    if (!bailout(lir->snapshot()))
        return false;

    if (hasNull) {
        masm.bind(&isNull);
        masm.loadConstantFloat32(0.0f, output);
        masm.jump(&done);
    }

    if (hasUndefined) {
        masm.bind(&isUndefined);
        masm.loadConstantFloat32(float(GenericNaN()), output);
        masm.jump(&done);
    }

    if (hasBoolean) {
        masm.bind(&isBool);
        masm.boolValueToFloat32(operand, output);
        masm.jump(&done);
    }

    masm.bind(&isInt32);
    masm.int32ValueToFloat32(operand, output);
    masm.jump(&done);

    masm.bind(&isDouble);
    masm.unboxDouble(operand, output);
    masm.convertDoubleToFloat(output, output);
    masm.bind(&done);

    return true;
}

bool
CodeGenerator::visitInt32ToDouble(LInt32ToDouble *lir)
{
    masm.convertInt32ToDouble(ToRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitFloat32ToDouble(LFloat32ToDouble *lir)
{
    masm.convertFloatToDouble(ToFloatRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitDoubleToFloat32(LDoubleToFloat32 *lir)
{
    masm.convertDoubleToFloat(ToFloatRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitInt32ToFloat32(LInt32ToFloat32 *lir)
{
    masm.convertInt32ToFloat32(ToRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitDoubleToInt32(LDoubleToInt32 *lir)
{
    Label fail;
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    masm.convertDoubleToInt32(input, output, &fail, lir->mir()->canBeNegativeZero());
    if (!bailoutFrom(&fail, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitFloat32ToInt32(LFloat32ToInt32 *lir)
{
    Label fail;
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    masm.convertFloat32ToInt32(input, output, &fail, lir->mir()->canBeNegativeZero());
    if (!bailoutFrom(&fail, lir->snapshot()))
        return false;
    return true;
}

void
CodeGenerator::emitOOLTestObject(Register objreg, Label *ifTruthy, Label *ifFalsy, Register scratch)
{
    saveVolatile(scratch);
    masm.setupUnalignedABICall(1, scratch);
    masm.passABIArg(objreg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::EmulatesUndefined));
    masm.storeCallResult(scratch);
    restoreVolatile(scratch);

    masm.branchIfTrueBool(scratch, ifFalsy);
    masm.jump(ifTruthy);
}









class OutOfLineTestObject : public OutOfLineCodeBase<CodeGenerator>
{
    Register objreg_;
    Register scratch_;

    Label *ifTruthy_;
    Label *ifFalsy_;

#ifdef DEBUG
    bool initialized() { return ifTruthy_ != nullptr; }
#endif

  public:
    OutOfLineTestObject()
#ifdef DEBUG
      : ifTruthy_(nullptr), ifFalsy_(nullptr)
#endif
    { }

    bool accept(CodeGenerator *codegen) MOZ_FINAL MOZ_OVERRIDE {
        MOZ_ASSERT(initialized());
        codegen->emitOOLTestObject(objreg_, ifTruthy_, ifFalsy_, scratch_);
        return true;
    }

    
    
    
    void setInputAndTargets(Register objreg, Label *ifTruthy, Label *ifFalsy, Register scratch) {
        MOZ_ASSERT(!initialized());
        MOZ_ASSERT(ifTruthy);
        objreg_ = objreg;
        scratch_ = scratch;
        ifTruthy_ = ifTruthy;
        ifFalsy_ = ifFalsy;
    }
};





class OutOfLineTestObjectWithLabels : public OutOfLineTestObject
{
    Label label1_;
    Label label2_;

  public:
    OutOfLineTestObjectWithLabels() { }

    Label *label1() { return &label1_; }
    Label *label2() { return &label2_; }
};

void
CodeGenerator::testObjectTruthy(Register objreg, Label *ifTruthy, Label *ifFalsy, Register scratch,
                                OutOfLineTestObject *ool)
{
    ool->setInputAndTargets(objreg, ifTruthy, ifFalsy, scratch);

    
    
    
    Assembler::Condition cond = masm.branchTestObjectTruthy(true, objreg, scratch, ool->entry());
    masm.j(cond, ifTruthy);
    masm.jump(ifFalsy);
}

void
CodeGenerator::testValueTruthy(const ValueOperand &value,
                               const LDefinition *scratch1, const LDefinition *scratch2,
                               FloatRegister fr,
                               Label *ifTruthy, Label *ifFalsy,
                               OutOfLineTestObject *ool)
{
    Register tag = masm.splitTagForTest(value);
    Assembler::Condition cond;

    
    
    
    
    masm.branchTestUndefined(Assembler::Equal, tag, ifFalsy);
    masm.branchTestNull(Assembler::Equal, tag, ifFalsy);

    Label notBoolean;
    masm.branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
    masm.branchTestBooleanTruthy(false, value, ifFalsy);
    masm.jump(ifTruthy);
    masm.bind(&notBoolean);

    Label notInt32;
    masm.branchTestInt32(Assembler::NotEqual, tag, &notInt32);
    cond = masm.testInt32Truthy(false, value);
    masm.j(cond, ifFalsy);
    masm.jump(ifTruthy);
    masm.bind(&notInt32);

    if (ool) {
        Label notObject;

        masm.branchTestObject(Assembler::NotEqual, tag, &notObject);

        Register objreg = masm.extractObject(value, ToRegister(scratch1));
        testObjectTruthy(objreg, ifTruthy, ifFalsy, ToRegister(scratch2), ool);

        masm.bind(&notObject);
    } else {
        masm.branchTestObject(Assembler::Equal, tag, ifTruthy);
    }

    
    Label notString;
    masm.branchTestString(Assembler::NotEqual, tag, &notString);
    cond = masm.testStringTruthy(false, value);
    masm.j(cond, ifFalsy);
    masm.jump(ifTruthy);
    masm.bind(&notString);

    
    masm.unboxDouble(value, fr);
    cond = masm.testDoubleTruthy(false, fr);
    masm.j(cond, ifFalsy);
    masm.jump(ifTruthy);
}

Label *
CodeGenerator::getJumpLabelForBranch(MBasicBlock *block)
{
    if (!labelForBackedgeWithImplicitCheck(block))
        return block->lir()->label();

    
    
    
    
    
    Label *res = GetIonContext()->temp->lifoAlloc()->new_<Label>();
    Label after;
    masm.jump(&after);
    masm.bind(res);
    jumpToBlock(block);
    masm.bind(&after);
    return res;
}

bool
CodeGenerator::visitTestOAndBranch(LTestOAndBranch *lir)
{
    MOZ_ASSERT(lir->mir()->operandMightEmulateUndefined(),
               "Objects which can't emulate undefined should have been constant-folded");

    OutOfLineTestObject *ool = new OutOfLineTestObject();
    if (!addOutOfLineCode(ool))
        return false;

    Label *truthy = getJumpLabelForBranch(lir->ifTruthy());
    Label *falsy = getJumpLabelForBranch(lir->ifFalsy());

    testObjectTruthy(ToRegister(lir->input()), truthy, falsy,
                     ToRegister(lir->temp()), ool);
    return true;

}

bool
CodeGenerator::visitTestVAndBranch(LTestVAndBranch *lir)
{
    OutOfLineTestObject *ool = nullptr;
    if (lir->mir()->operandMightEmulateUndefined()) {
        ool = new OutOfLineTestObject();
        if (!addOutOfLineCode(ool))
            return false;
    }

    Label *truthy = getJumpLabelForBranch(lir->ifTruthy());
    Label *falsy = getJumpLabelForBranch(lir->ifFalsy());

    testValueTruthy(ToValue(lir, LTestVAndBranch::Input),
                    lir->temp1(), lir->temp2(),
                    ToFloatRegister(lir->tempFloat()),
                    truthy, falsy, ool);
    return true;
}

bool
CodeGenerator::visitFunctionDispatch(LFunctionDispatch *lir)
{
    MFunctionDispatch *mir = lir->mir();
    Register input = ToRegister(lir->input());
    Label *lastLabel;
    size_t casesWithFallback;

    
    if (!mir->hasFallback()) {
        JS_ASSERT(mir->numCases() > 0);
        casesWithFallback = mir->numCases();
        lastLabel = mir->getCaseBlock(mir->numCases() - 1)->lir()->label();
    } else {
        casesWithFallback = mir->numCases() + 1;
        lastLabel = mir->getFallback()->lir()->label();
    }

    
    for (size_t i = 0; i < casesWithFallback - 1; i++) {
        JS_ASSERT(i < mir->numCases());
        JSFunction *func = mir->getCase(i);
        LBlock *target = mir->getCaseBlock(i)->lir();
        masm.branchPtr(Assembler::Equal, input, ImmGCPtr(func), target->label());
    }

    
    masm.jump(lastLabel);

    return true;
}

bool
CodeGenerator::visitTypeObjectDispatch(LTypeObjectDispatch *lir)
{
    MTypeObjectDispatch *mir = lir->mir();
    Register input = ToRegister(lir->input());
    Register temp = ToRegister(lir->temp());

    
    masm.loadPtr(Address(input, JSObject::offsetOfType()), temp);

    
    InlinePropertyTable *propTable = mir->propTable();
    for (size_t i = 0; i < mir->numCases(); i++) {
        JSFunction *func = mir->getCase(i);
        LBlock *target = mir->getCaseBlock(i)->lir();

        DebugOnly<bool> found = false;
        for (size_t j = 0; j < propTable->numEntries(); j++) {
            if (propTable->getFunction(j) != func)
                continue;
            types::TypeObject *typeObj = propTable->getTypeObject(j);
            masm.branchPtr(Assembler::Equal, temp, ImmGCPtr(typeObj), target->label());
            found = true;
        }
        JS_ASSERT(found);
    }

    
    LBlock *fallback = mir->getFallback()->lir();
    masm.jump(fallback->label());
    return true;
}

typedef JSFlatString *(*IntToStringFn)(ThreadSafeContext *, int);
typedef ParallelResult (*IntToStringParFn)(ForkJoinSlice *, int, MutableHandleString);
static const VMFunctionsModal IntToStringInfo = VMFunctionsModal(
    FunctionInfo<IntToStringFn>(Int32ToString<CanGC>),
    FunctionInfo<IntToStringParFn>(IntToStringPar));

bool
CodeGenerator::visitIntToString(LIntToString *lir)
{
    Register input = ToRegister(lir->input());
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(IntToStringInfo, lir, (ArgList(), input),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    masm.branch32(Assembler::AboveOrEqual, input, Imm32(StaticStrings::INT_STATIC_LIMIT),
                  ool->entry());

    masm.movePtr(ImmPtr(&GetIonContext()->runtime->staticStrings.intStaticTable), output);
    masm.loadPtr(BaseIndex(output, input, ScalePointer), output);

    masm.bind(ool->rejoin());
    return true;
}

typedef JSString *(*DoubleToStringFn)(ThreadSafeContext *, double);
typedef ParallelResult (*DoubleToStringParFn)(ForkJoinSlice *, double, MutableHandleString);
static const VMFunctionsModal DoubleToStringInfo = VMFunctionsModal(
    FunctionInfo<DoubleToStringFn>(NumberToString<CanGC>),
    FunctionInfo<DoubleToStringParFn>(DoubleToStringPar));

bool
CodeGenerator::visitDoubleToString(LDoubleToString *lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register temp = ToRegister(lir->tempInt());
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(DoubleToStringInfo, lir, (ArgList(), input),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    masm.convertDoubleToInt32(input, temp, ool->entry(), true);
    masm.branch32(Assembler::AboveOrEqual, temp, Imm32(StaticStrings::INT_STATIC_LIMIT),
                  ool->entry());

    masm.movePtr(ImmPtr(&GetIonContext()->runtime->staticStrings.intStaticTable), output);
    masm.loadPtr(BaseIndex(output, temp, ScalePointer), output);

    masm.bind(ool->rejoin());
    return true;
}

typedef JSObject *(*CloneRegExpObjectFn)(JSContext *, JSObject *, JSObject *);
static const VMFunction CloneRegExpObjectInfo =
    FunctionInfo<CloneRegExpObjectFn>(CloneRegExpObject);

bool
CodeGenerator::visitRegExp(LRegExp *lir)
{
    JSObject *proto = lir->mir()->getRegExpPrototype();

    pushArg(ImmGCPtr(proto));
    pushArg(ImmGCPtr(lir->mir()->source()));
    return callVM(CloneRegExpObjectInfo, lir);
}

typedef bool (*RegExpTestRawFn)(JSContext *cx, HandleObject regexp,
                                HandleString input, bool *result);
static const VMFunction RegExpTestRawInfo = FunctionInfo<RegExpTestRawFn>(regexp_test_raw);

bool
CodeGenerator::visitRegExpTest(LRegExpTest *lir)
{
    pushArg(ToRegister(lir->string()));
    pushArg(ToRegister(lir->regexp()));
    return callVM(RegExpTestRawInfo, lir);
}

typedef JSObject *(*LambdaFn)(JSContext *, HandleFunction, HandleObject);
static const VMFunction LambdaInfo =
    FunctionInfo<LambdaFn>(js::Lambda);

bool
CodeGenerator::visitLambdaForSingleton(LLambdaForSingleton *lir)
{
    pushArg(ToRegister(lir->scopeChain()));
    pushArg(ImmGCPtr(lir->mir()->info().fun));
    return callVM(LambdaInfo, lir);
}

bool
CodeGenerator::visitLambda(LLambda *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());
    Register output = ToRegister(lir->output());
    const LambdaFunctionInfo &info = lir->mir()->info();

    OutOfLineCode *ool = oolCallVM(LambdaInfo, lir, (ArgList(), ImmGCPtr(info.fun), scopeChain),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    JS_ASSERT(gen->compartment == info.fun->compartment());
    JS_ASSERT(!info.singletonType);

    masm.newGCThing(output, info.fun, ool->entry());
    masm.initGCThing(output, info.fun);

    emitLambdaInit(output, scopeChain, info);

    masm.bind(ool->rejoin());
    return true;
}

void
CodeGenerator::emitLambdaInit(const Register &output, const Register &scopeChain,
                              const LambdaFunctionInfo &info)
{
    
    
    union {
        struct S {
            uint16_t nargs;
            uint16_t flags;
        } s;
        uint32_t word;
    } u;
    u.s.nargs = info.fun->nargs;
    u.s.flags = info.flags & ~JSFunction::EXTENDED;

    JS_STATIC_ASSERT(offsetof(JSFunction, flags) == offsetof(JSFunction, nargs) + 2);
    masm.store32(Imm32(u.word), Address(output, offsetof(JSFunction, nargs)));
    masm.storePtr(ImmGCPtr(info.scriptOrLazyScript),
                  Address(output, JSFunction::offsetOfNativeOrScript()));
    masm.storePtr(scopeChain, Address(output, JSFunction::offsetOfEnvironment()));
    masm.storePtr(ImmGCPtr(info.fun->displayAtom()), Address(output, JSFunction::offsetOfAtom()));
}

bool
CodeGenerator::visitLambdaPar(LLambdaPar *lir)
{
    Register resultReg = ToRegister(lir->output());
    Register sliceReg = ToRegister(lir->forkJoinSlice());
    Register scopeChainReg = ToRegister(lir->scopeChain());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    const LambdaFunctionInfo &info = lir->mir()->info();

    JS_ASSERT(scopeChainReg != resultReg);

    emitAllocateGCThingPar(lir, resultReg, sliceReg, tempReg1, tempReg2, info.fun);
    emitLambdaInit(resultReg, scopeChainReg, info);
    return true;
}

bool
CodeGenerator::visitLabel(LLabel *lir)
{
    masm.bind(lir->label());
    return true;
}

bool
CodeGenerator::visitNop(LNop *lir)
{
    return true;
}

bool
CodeGenerator::visitOsiPoint(LOsiPoint *lir)
{
    
    

    JS_ASSERT(masm.framePushed() == frameSize());

    uint32_t osiCallPointOffset;
    if (!markOsiPoint(lir, &osiCallPointOffset))
        return false;

    LSafepoint *safepoint = lir->associatedSafepoint();
    JS_ASSERT(!safepoint->osiCallPointOffset());
    safepoint->setOsiCallPointOffset(osiCallPointOffset);

#ifdef DEBUG
    
    
    
    for (LInstructionReverseIterator iter(current->rbegin(lir)); iter != current->rend(); iter++) {
        if (*iter == lir || iter->isNop())
            continue;
        JS_ASSERT(!iter->isMoveGroup());
        JS_ASSERT(iter->safepoint() == safepoint);
        break;
    }
#endif

#ifdef CHECK_OSIPOINT_REGISTERS
    if (shouldVerifyOsiPointRegs(safepoint))
        verifyOsiPointRegs(safepoint);
#endif

    return true;
}

bool
CodeGenerator::visitGoto(LGoto *lir)
{
    jumpToBlock(lir->target());
    return true;
}



class OutOfLineInterruptCheckImplicit : public OutOfLineCodeBase<CodeGenerator>
{
  public:
    LBlock *block;
    LInterruptCheckImplicit *lir;

    OutOfLineInterruptCheckImplicit(LBlock *block, LInterruptCheckImplicit *lir)
      : block(block), lir(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineInterruptCheckImplicit(this);
    }
};

bool
CodeGenerator::visitOutOfLineInterruptCheckImplicit(OutOfLineInterruptCheckImplicit *ool)
{
#ifdef CHECK_OSIPOINT_REGISTERS
    
    
    
    
    resetOsiPointRegs(ool->lir->safepoint());
#endif

    LInstructionIterator iter = ool->block->begin();
    for (; iter != ool->block->end(); iter++) {
        if (iter->isLabel()) {
            
        } else if (iter->isMoveGroup()) {
            
            
            
            visitMoveGroup(iter->toMoveGroup());
        } else {
            break;
        }
    }
    JS_ASSERT(*iter == ool->lir);

    saveLive(ool->lir);
    if (!callVM(InterruptCheckInfo, ool->lir))
        return false;
    restoreLive(ool->lir);
    masm.jump(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitInterruptCheckImplicit(LInterruptCheckImplicit *lir)
{
    OutOfLineInterruptCheckImplicit *ool = new OutOfLineInterruptCheckImplicit(current, lir);
    if (!addOutOfLineCode(ool))
        return false;

    lir->setOolEntry(ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitTableSwitch(LTableSwitch *ins)
{
    MTableSwitch *mir = ins->mir();
    Label *defaultcase = mir->getDefault()->lir()->label();
    const LAllocation *temp;

    if (ins->index()->isDouble()) {
        temp = ins->tempInt();

        
        
        masm.convertDoubleToInt32(ToFloatRegister(ins->index()), ToRegister(temp), defaultcase, false);
    } else {
        temp = ins->index();
    }

    return emitTableSwitchDispatch(mir, ToRegister(temp), ToRegisterOrInvalid(ins->tempPointer()));
}

bool
CodeGenerator::visitTableSwitchV(LTableSwitchV *ins)
{
    MTableSwitch *mir = ins->mir();
    Label *defaultcase = mir->getDefault()->lir()->label();

    Register index = ToRegister(ins->tempInt());
    ValueOperand value = ToValue(ins, LTableSwitchV::InputValue);
    Register tag = masm.extractTag(value, index);
    masm.branchTestNumber(Assembler::NotEqual, tag, defaultcase);

    Label unboxInt, isInt;
    masm.branchTestInt32(Assembler::Equal, tag, &unboxInt);
    {
        FloatRegister floatIndex = ToFloatRegister(ins->tempFloat());
        masm.unboxDouble(value, floatIndex);
        masm.convertDoubleToInt32(floatIndex, index, defaultcase, false);
        masm.jump(&isInt);
    }

    masm.bind(&unboxInt);
    masm.unboxInt32(value, index);

    masm.bind(&isInt);

    return emitTableSwitchDispatch(mir, index, ToRegisterOrInvalid(ins->tempPointer()));
}

bool
CodeGenerator::visitParameter(LParameter *lir)
{
    return true;
}

bool
CodeGenerator::visitCallee(LCallee *lir)
{
    
    Register callee = ToRegister(lir->output());
    Address ptr(StackPointer, frameSize() + IonJSFrameLayout::offsetOfCalleeToken());

    masm.loadPtr(ptr, callee);
    masm.clearCalleeTag(callee, gen->info().executionMode());
    return true;
}

bool
CodeGenerator::visitStart(LStart *lir)
{
    return true;
}

bool
CodeGenerator::visitReturn(LReturn *lir)
{
#if defined(JS_NUNBOX32)
    DebugOnly<LAllocation *> type    = lir->getOperand(TYPE_INDEX);
    DebugOnly<LAllocation *> payload = lir->getOperand(PAYLOAD_INDEX);
    JS_ASSERT(ToRegister(type)    == JSReturnReg_Type);
    JS_ASSERT(ToRegister(payload) == JSReturnReg_Data);
#elif defined(JS_PUNBOX64)
    DebugOnly<LAllocation *> result = lir->getOperand(0);
    JS_ASSERT(ToRegister(result) == JSReturnReg);
#endif
    
    if (current->mir() != *gen->graph().poBegin())
        masm.jump(&returnLabel_);
    return true;
}

bool
CodeGenerator::visitOsrEntry(LOsrEntry *lir)
{
    
    masm.flushBuffer();
    setOsrEntryOffset(masm.size());

#if JS_TRACE_LOGGING
    masm.tracelogLog(TraceLogging::INFO_ENGINE_IONMONKEY);
#endif

    
    uint32_t size = frameSize();
    if (size != 0)
        masm.subPtr(Imm32(size), StackPointer);
    return true;
}

bool
CodeGenerator::visitOsrScopeChain(LOsrScopeChain *lir)
{
    const LAllocation *frame   = lir->getOperand(0);
    const LDefinition *object  = lir->getDef(0);

    const ptrdiff_t frameOffset = StackFrame::offsetOfScopeChain();

    masm.loadPtr(Address(ToRegister(frame), frameOffset), ToRegister(object));
    return true;
}

bool
CodeGenerator::visitOsrArgumentsObject(LOsrArgumentsObject *lir)
{
    const LAllocation *frame   = lir->getOperand(0);
    const LDefinition *object  = lir->getDef(0);

    const ptrdiff_t frameOffset = StackFrame::offsetOfArgumentsObject();

    masm.loadPtr(Address(ToRegister(frame), frameOffset), ToRegister(object));
    return true;
}

bool
CodeGenerator::visitStackArgT(LStackArgT *lir)
{
    const LAllocation *arg = lir->getArgument();
    MIRType argType = lir->mir()->getArgument()->type();
    uint32_t argslot = lir->argslot();

    int32_t stack_offset = StackOffsetOfPassedArg(argslot);
    Address dest(StackPointer, stack_offset);

    if (arg->isFloatReg())
        masm.storeDouble(ToFloatRegister(arg), dest);
    else if (arg->isRegister())
        masm.storeValue(ValueTypeFromMIRType(argType), ToRegister(arg), dest);
    else
        masm.storeValue(*(arg->toConstant()), dest);

    return pushedArgumentSlots_.append(StackOffsetToSlot(stack_offset));
}

bool
CodeGenerator::visitStackArgV(LStackArgV *lir)
{
    ValueOperand val = ToValue(lir, 0);
    uint32_t argslot = lir->argslot();
    int32_t stack_offset = StackOffsetOfPassedArg(argslot);

    masm.storeValue(val, Address(StackPointer, stack_offset));
    return pushedArgumentSlots_.append(StackOffsetToSlot(stack_offset));
}

bool
CodeGenerator::visitMoveGroup(LMoveGroup *group)
{
    if (!group->numMoves())
        return true;

    MoveResolver &resolver = masm.moveResolver();

    for (size_t i = 0; i < group->numMoves(); i++) {
        const LMove &move = group->getMove(i);

        const LAllocation *from = move.from();
        const LAllocation *to = move.to();

        
        JS_ASSERT(*from != *to);
        JS_ASSERT(!from->isConstant());
        JS_ASSERT(from->isDouble() == to->isDouble());

        MoveResolver::Move::Kind kind = from->isDouble()
                                        ? MoveResolver::Move::DOUBLE
                                        : MoveResolver::Move::GENERAL;

        if (!resolver.addMove(toMoveOperand(from), toMoveOperand(to), kind))
            return false;
    }

    if (!resolver.resolve())
        return false;

    MoveEmitter emitter(masm);
    emitter.emit(resolver);
    emitter.finish();

    return true;
}

bool
CodeGenerator::visitInteger(LInteger *lir)
{
    masm.move32(Imm32(lir->getValue()), ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitPointer(LPointer *lir)
{
    if (lir->kind() == LPointer::GC_THING)
        masm.movePtr(ImmGCPtr(lir->gcptr()), ToRegister(lir->output()));
    else
        masm.movePtr(ImmPtr(lir->ptr()), ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitSlots(LSlots *lir)
{
    Address slots(ToRegister(lir->object()), JSObject::offsetOfSlots());
    masm.loadPtr(slots, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitStoreSlotV(LStoreSlotV *store)
{
    Register base = ToRegister(store->slots());
    int32_t offset = store->mir()->slot() * sizeof(Value);

    const ValueOperand value = ToValue(store, LStoreSlotV::Value);

    if (store->mir()->needsBarrier())
       emitPreBarrier(Address(base, offset), MIRType_Value);

    masm.storeValue(value, Address(base, offset));
    return true;
}

bool
CodeGenerator::emitGetPropertyPolymorphic(LInstruction *ins, Register obj, Register scratch,
                                          const TypedOrValueRegister &output)
{
    MGetPropertyPolymorphic *mir = ins->mirRaw()->toGetPropertyPolymorphic();
    JS_ASSERT(mir->numShapes() > 1);

    masm.loadObjShape(obj, scratch);

    Label done;
    for (size_t i = 0; i < mir->numShapes(); i++) {
        Label next;
        masm.branchPtr(Assembler::NotEqual, scratch, ImmGCPtr(mir->objShape(i)), &next);

        Shape *shape = mir->shape(i);
        if (shape->slot() < shape->numFixedSlots()) {
            
            masm.loadTypedOrValue(Address(obj, JSObject::getFixedSlotOffset(shape->slot())),
                                  output);
        } else {
            
            uint32_t offset = (shape->slot() - shape->numFixedSlots()) * sizeof(js::Value);
            masm.loadPtr(Address(obj, JSObject::offsetOfSlots()), scratch);
            masm.loadTypedOrValue(Address(scratch, offset), output);
        }

        masm.jump(&done);
        masm.bind(&next);
    }

    
    if (!bailout(ins->snapshot()))
        return false;

    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitGetPropertyPolymorphicV(LGetPropertyPolymorphicV *ins)
{
    Register obj = ToRegister(ins->obj());
    ValueOperand output = GetValueOutput(ins);
    return emitGetPropertyPolymorphic(ins, obj, output.scratchReg(), output);
}

bool
CodeGenerator::visitGetPropertyPolymorphicT(LGetPropertyPolymorphicT *ins)
{
    Register obj = ToRegister(ins->obj());
    TypedOrValueRegister output(ins->mir()->type(), ToAnyRegister(ins->output()));
    Register temp = (output.type() == MIRType_Double)
                    ? ToRegister(ins->temp())
                    : output.typedReg().gpr();
    return emitGetPropertyPolymorphic(ins, obj, temp, output);
}

bool
CodeGenerator::emitSetPropertyPolymorphic(LInstruction *ins, Register obj, Register scratch,
                                          const ConstantOrRegister &value)
{
    MSetPropertyPolymorphic *mir = ins->mirRaw()->toSetPropertyPolymorphic();
    JS_ASSERT(mir->numShapes() > 1);

    masm.loadObjShape(obj, scratch);

    Label done;
    for (size_t i = 0; i < mir->numShapes(); i++) {
        Label next;
        masm.branchPtr(Assembler::NotEqual, scratch, ImmGCPtr(mir->objShape(i)), &next);

        Shape *shape = mir->shape(i);
        if (shape->slot() < shape->numFixedSlots()) {
            
            Address addr(obj, JSObject::getFixedSlotOffset(shape->slot()));
            if (mir->needsBarrier())
                emitPreBarrier(addr, MIRType_Value);
            masm.storeConstantOrRegister(value, addr);
        } else {
            
            masm.loadPtr(Address(obj, JSObject::offsetOfSlots()), scratch);
            Address addr(scratch, (shape->slot() - shape->numFixedSlots()) * sizeof(js::Value));
            if (mir->needsBarrier())
                emitPreBarrier(addr, MIRType_Value);
            masm.storeConstantOrRegister(value, addr);
        }

        masm.jump(&done);
        masm.bind(&next);
    }

    
    if (!bailout(ins->snapshot()))
        return false;

    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitSetPropertyPolymorphicV(LSetPropertyPolymorphicV *ins)
{
    Register obj = ToRegister(ins->obj());
    Register temp = ToRegister(ins->temp());
    ValueOperand value = ToValue(ins, LSetPropertyPolymorphicV::Value);
    return emitSetPropertyPolymorphic(ins, obj, temp, TypedOrValueRegister(value));
}

bool
CodeGenerator::visitSetPropertyPolymorphicT(LSetPropertyPolymorphicT *ins)
{
    Register obj = ToRegister(ins->obj());
    Register temp = ToRegister(ins->temp());

    ConstantOrRegister value;
    if (ins->mir()->value()->isConstant())
        value = ConstantOrRegister(ins->mir()->value()->toConstant()->value());
    else
        value = TypedOrValueRegister(ins->mir()->value()->type(), ToAnyRegister(ins->value()));

    return emitSetPropertyPolymorphic(ins, obj, temp, value);
}

bool
CodeGenerator::visitElements(LElements *lir)
{
    Address elements(ToRegister(lir->object()), JSObject::offsetOfElements());
    masm.loadPtr(elements, ToRegister(lir->output()));
    return true;
}

typedef bool (*ConvertElementsToDoublesFn)(JSContext *, uintptr_t);
static const VMFunction ConvertElementsToDoublesInfo =
    FunctionInfo<ConvertElementsToDoublesFn>(ObjectElements::ConvertElementsToDoubles);

bool
CodeGenerator::visitConvertElementsToDoubles(LConvertElementsToDoubles *lir)
{
    Register elements = ToRegister(lir->elements());

    OutOfLineCode *ool = oolCallVM(ConvertElementsToDoublesInfo, lir,
                                   (ArgList(), elements), StoreNothing());
    if (!ool)
        return false;

    Address convertedAddress(elements, ObjectElements::offsetOfFlags());
    Imm32 bit(ObjectElements::CONVERT_DOUBLE_ELEMENTS);
    masm.branchTest32(Assembler::Zero, convertedAddress, bit, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitMaybeToDoubleElement(LMaybeToDoubleElement *lir)
{
    Register elements = ToRegister(lir->elements());
    Register value = ToRegister(lir->value());
    ValueOperand out = ToOutValue(lir);

    FloatRegister temp = ToFloatRegister(lir->tempFloat());
    Label convert, done;

    
    
    masm.branchTest32(Assembler::NonZero,
                      Address(elements, ObjectElements::offsetOfFlags()),
                      Imm32(ObjectElements::CONVERT_DOUBLE_ELEMENTS),
                      &convert);

    masm.tagValue(JSVAL_TYPE_INT32, value, out);
    masm.jump(&done);

    masm.bind(&convert);
    masm.convertInt32ToDouble(value, temp);
    masm.boxDouble(temp, out);

    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitFunctionEnvironment(LFunctionEnvironment *lir)
{
    Address environment(ToRegister(lir->function()), JSFunction::offsetOfEnvironment());
    masm.loadPtr(environment, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitForkJoinSlice(LForkJoinSlice *lir)
{
    const Register tempReg = ToRegister(lir->getTempReg());

    masm.setupUnalignedABICall(0, tempReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ForkJoinSlicePar));
    JS_ASSERT(ToRegister(lir->output()) == ReturnReg);
    return true;
}

bool
CodeGenerator::visitGuardThreadLocalObject(LGuardThreadLocalObject *lir)
{
    JS_ASSERT(gen->info().executionMode() == ParallelExecution);

    const Register tempReg = ToRegister(lir->getTempReg());
    masm.setupUnalignedABICall(2, tempReg);
    masm.passABIArg(ToRegister(lir->forkJoinSlice()));
    masm.passABIArg(ToRegister(lir->object()));
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, IsThreadLocalObject));

    OutOfLineAbortPar *bail = oolAbortPar(ParallelBailoutIllegalWrite, lir);
    if (!bail)
        return false;

    
    masm.branchIfFalseBool(ReturnReg, bail->entry());
    return true;
}

bool
CodeGenerator::visitTypeBarrierV(LTypeBarrierV *lir)
{
    ValueOperand operand = ToValue(lir, LTypeBarrierV::Input);
    Register scratch = ToTempRegisterOrInvalid(lir->temp());

    Label miss;
    masm.guardTypeSet(operand, lir->mir()->resultTypeSet(), scratch, &miss);
    if (!bailoutFrom(&miss, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitTypeBarrierO(LTypeBarrierO *lir)
{
    Register obj = ToRegister(lir->object());
    Register scratch = ToTempRegisterOrInvalid(lir->temp());

    Label miss;
    masm.guardObjectType(obj, lir->mir()->resultTypeSet(), scratch, &miss);
    if (!bailoutFrom(&miss, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitMonitorTypes(LMonitorTypes *lir)
{
    ValueOperand operand = ToValue(lir, LMonitorTypes::Input);
    Register scratch = ToTempUnboxRegister(lir->temp());

    Label matched, miss;
    masm.guardTypeSet(operand, lir->mir()->typeSet(), scratch, &miss);
    if (!bailoutFrom(&miss, lir->snapshot()))
        return false;
    return true;
}

#ifdef JSGC_GENERATIONAL

class OutOfLineCallPostWriteBarrier : public OutOfLineCodeBase<CodeGenerator>
{
    LInstruction *lir_;
    const LAllocation *object_;

  public:
    OutOfLineCallPostWriteBarrier(LInstruction *lir, const LAllocation *object)
      : lir_(lir), object_(object)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineCallPostWriteBarrier(this);
    }

    LInstruction *lir() const {
        return lir_;
    }
    const LAllocation *object() const {
        return object_;
    }
};

bool
CodeGenerator::visitOutOfLineCallPostWriteBarrier(OutOfLineCallPostWriteBarrier *ool)
{
    saveLive(ool->lir());

    const LAllocation *obj = ool->object();

    GeneralRegisterSet regs;
    regs.add(CallTempReg0);
    regs.add(CallTempReg1);
    regs.add(CallTempReg2);

    Register objreg;
    bool isGlobal = false;
    if (obj->isConstant()) {
        JSObject *object = &obj->toConstant()->toObject();
        isGlobal = object->is<GlobalObject>();
        objreg = regs.takeAny();
        masm.movePtr(ImmGCPtr(object), objreg);
    } else {
        objreg = ToRegister(obj);
        regs.takeUnchecked(objreg);
    }

    Register runtimereg = regs.takeAny();
    masm.mov(ImmPtr(GetIonContext()->runtime), runtimereg);

    void (*fun)(JSRuntime*, JSObject*) = isGlobal ? PostGlobalWriteBarrier : PostWriteBarrier;
    masm.setupUnalignedABICall(2, regs.takeAny());
    masm.passABIArg(runtimereg);
    masm.passABIArg(objreg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, fun));

    restoreLive(ool->lir());

    masm.jump(ool->rejoin());
    return true;
}
#endif

bool
CodeGenerator::visitPostWriteBarrierO(LPostWriteBarrierO *lir)
{
#ifdef JSGC_GENERATIONAL
    OutOfLineCallPostWriteBarrier *ool = new OutOfLineCallPostWriteBarrier(lir, lir->object());
    if (!addOutOfLineCode(ool))
        return false;

    Nursery &nursery = GetIonContext()->runtime->gcNursery;

    if (lir->object()->isConstant()) {
        JS_ASSERT(!nursery.isInside(&lir->object()->toConstant()->toObject()));
    } else {
        Label tenured;
        Register objreg = ToRegister(lir->object());
        masm.branchPtr(Assembler::Below, objreg, ImmWord(nursery.start()), &tenured);
        masm.branchPtr(Assembler::Below, objreg, ImmWord(nursery.heapEnd()), ool->rejoin());
        masm.bind(&tenured);
    }

    Register valuereg = ToRegister(lir->value());
    masm.branchPtr(Assembler::Below, valuereg, ImmWord(nursery.start()), ool->rejoin());
    masm.branchPtr(Assembler::Below, valuereg, ImmWord(nursery.heapEnd()), ool->entry());

    masm.bind(ool->rejoin());
#endif
    return true;
}

bool
CodeGenerator::visitPostWriteBarrierV(LPostWriteBarrierV *lir)
{
#ifdef JSGC_GENERATIONAL
    OutOfLineCallPostWriteBarrier *ool = new OutOfLineCallPostWriteBarrier(lir, lir->object());
    if (!addOutOfLineCode(ool))
        return false;

    ValueOperand value = ToValue(lir, LPostWriteBarrierV::Input);
    masm.branchTestObject(Assembler::NotEqual, value, ool->rejoin());

    Nursery &nursery = GetIonContext()->runtime->gcNursery;

    if (lir->object()->isConstant()) {
        JS_ASSERT(!nursery.isInside(&lir->object()->toConstant()->toObject()));
    } else {
        Label tenured;
        Register objreg = ToRegister(lir->object());
        masm.branchPtr(Assembler::Below, objreg, ImmWord(nursery.start()), &tenured);
        masm.branchPtr(Assembler::Below, objreg, ImmWord(nursery.heapEnd()), ool->rejoin());
        masm.bind(&tenured);
    }

    Register valuereg = masm.extractObject(value, ToTempUnboxRegister(lir->temp()));
    masm.branchPtr(Assembler::Below, valuereg, ImmWord(nursery.start()), ool->rejoin());
    masm.branchPtr(Assembler::Below, valuereg, ImmWord(nursery.heapEnd()), ool->entry());

    masm.bind(ool->rejoin());
#endif
    return true;
}

bool
CodeGenerator::visitPostWriteBarrierAllSlots(LPostWriteBarrierAllSlots *lir)
{
#ifdef JSGC_GENERATIONAL
    OutOfLineCallPostWriteBarrier *ool = new OutOfLineCallPostWriteBarrier(lir, lir->object());
    if (!addOutOfLineCode(ool))
        return false;

    Nursery &nursery = GetIonContext()->runtime->gcNursery;

    if (lir->object()->isConstant()) {
        JS_ASSERT(!nursery.isInside(&lir->object()->toConstant()->toObject()));
        return true;
    }

    Register objreg = ToRegister(lir->object());
    masm.branchPtr(Assembler::Below, objreg, ImmWord(nursery.start()), ool->entry());
    masm.branchPtr(Assembler::Below, objreg, ImmWord(nursery.heapEnd()), ool->rejoin());
    masm.bind(ool->rejoin());
#endif
    return true;
}

bool
CodeGenerator::visitCallNative(LCallNative *call)
{
    JSFunction *target = call->getSingleTarget();
    JS_ASSERT(target);
    JS_ASSERT(target->isNative());

    int callargslot = call->argslot();
    int unusedStack = StackOffsetOfPassedArg(callargslot);

    
    const Register argContextReg   = ToRegister(call->getArgContextReg());
    const Register argUintNReg     = ToRegister(call->getArgUintNReg());
    const Register argVpReg        = ToRegister(call->getArgVpReg());

    
    const Register tempReg = ToRegister(call->getTempReg());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    
    
    
    

    
    masm.adjustStack(unusedStack);

    
    
    masm.Push(ObjectValue(*target));

    
    
    
    
    
    ExecutionMode executionMode = gen->info().executionMode();
    masm.loadContext(argContextReg, tempReg, executionMode);
    masm.move32(Imm32(call->numStackArgs()), argUintNReg);
    masm.movePtr(StackPointer, argVpReg);

    masm.Push(argUintNReg);

    
    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(tempReg, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(argContextReg, tempReg, executionMode);

    if (!markSafepointAt(safepointOffset, call))
        return false;

    
    masm.setupUnalignedABICall(3, tempReg);
    masm.passABIArg(argContextReg);
    masm.passABIArg(argUintNReg);
    masm.passABIArg(argVpReg);

    switch (executionMode) {
      case SequentialExecution:
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->native()));

        
        masm.branchIfFalseBool(ReturnReg, masm.failureLabel(executionMode));
        break;

      case ParallelExecution:
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->parallelNative()));

        
        
        masm.branch32(Assembler::NotEqual, ReturnReg, Imm32(TP_SUCCESS),
                      masm.failureLabel(executionMode));
        break;

      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }

    
    masm.loadValue(Address(StackPointer, IonNativeExitFrameLayout::offsetOfResult()), JSReturnOperand);

    
    

    
    masm.adjustStack(IonNativeExitFrameLayout::Size() - unusedStack);
    JS_ASSERT(masm.framePushed() == initialStack);

    dropArguments(call->numStackArgs() + 1);
    return true;
}

bool
CodeGenerator::visitCallDOMNative(LCallDOMNative *call)
{
    JSFunction *target = call->getSingleTarget();
    JS_ASSERT(target);
    JS_ASSERT(target->isNative());
    JS_ASSERT(target->jitInfo());
    JS_ASSERT(call->mir()->isDOMFunction());

    int callargslot = call->argslot();
    int unusedStack = StackOffsetOfPassedArg(callargslot);

    
    const Register argJSContext = ToRegister(call->getArgJSContext());
    const Register argObj       = ToRegister(call->getArgObj());
    const Register argPrivate   = ToRegister(call->getArgPrivate());
    const Register argArgs      = ToRegister(call->getArgArgs());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    
    
    
    

    
    
    masm.adjustStack(unusedStack);
    
    Register obj = masm.extractObject(Address(StackPointer, 0), argObj);
    JS_ASSERT(obj == argObj);

    
    
    masm.Push(ObjectValue(*target));

    
    
    
    JS_STATIC_ASSERT(JSJitMethodCallArgsTraits::offsetOfArgv == 0);
    JS_STATIC_ASSERT(JSJitMethodCallArgsTraits::offsetOfArgc ==
                     IonDOMMethodExitFrameLayoutTraits::offsetOfArgcFromArgv);
    masm.computeEffectiveAddress(Address(StackPointer, 2 * sizeof(Value)), argArgs);

    
    masm.loadPrivate(Address(obj, JSObject::getFixedSlotOffset(0)), argPrivate);

    
    masm.Push(Imm32(call->numStackArgs()));

    
    masm.Push(argArgs);
    
    masm.movePtr(StackPointer, argArgs);

    
    
    
    masm.Push(argObj);
    masm.movePtr(StackPointer, argObj);

    
    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(argJSContext, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(ION_FRAME_DOMMETHOD);

    if (!markSafepointAt(safepointOffset, call))
        return false;

    
    masm.setupUnalignedABICall(4, argJSContext);

    masm.loadJSContext(argJSContext);

    masm.passABIArg(argJSContext);
    masm.passABIArg(argObj);
    masm.passABIArg(argPrivate);
    masm.passABIArg(argArgs);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->jitInfo()->method));

    if (target->jitInfo()->isInfallible) {
        masm.loadValue(Address(StackPointer, IonDOMMethodExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    } else {
        
        masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

        
        masm.loadValue(Address(StackPointer, IonDOMMethodExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    }

    
    

    
    masm.adjustStack(IonDOMMethodExitFrameLayout::Size() - unusedStack);
    JS_ASSERT(masm.framePushed() == initialStack);

    dropArguments(call->numStackArgs() + 1);
    return true;
}

typedef bool (*GetIntrinsicValueFn)(JSContext *cx, HandlePropertyName, MutableHandleValue);
static const VMFunction GetIntrinsicValueInfo =
    FunctionInfo<GetIntrinsicValueFn>(GetIntrinsicValue);

bool
CodeGenerator::visitCallGetIntrinsicValue(LCallGetIntrinsicValue *lir)
{
    pushArg(ImmGCPtr(lir->mir()->name()));
    return callVM(GetIntrinsicValueInfo, lir);
}

typedef bool (*InvokeFunctionFn)(JSContext *, HandleObject, uint32_t, Value *, Value *);
static const VMFunction InvokeFunctionInfo = FunctionInfo<InvokeFunctionFn>(InvokeFunction);

bool
CodeGenerator::emitCallInvokeFunction(LInstruction *call, Register calleereg,
                                      uint32_t argc, uint32_t unusedStack)
{
    
    
    masm.freeStack(unusedStack);

    pushArg(StackPointer); 
    pushArg(Imm32(argc));  
    pushArg(calleereg);    

    if (!callVM(InvokeFunctionInfo, call))
        return false;

    
    masm.reserveStack(unusedStack);
    return true;
}

bool
CodeGenerator::visitCallGeneric(LCallGeneric *call)
{
    Register calleereg = ToRegister(call->getFunction());
    Register objreg    = ToRegister(call->getTempObject());
    Register nargsreg  = ToRegister(call->getNargsReg());
    uint32_t unusedStack = StackOffsetOfPassedArg(call->argslot());
    ExecutionMode executionMode = gen->info().executionMode();
    Label invoke, thunk, makeCall, end;

    
    JS_ASSERT(!call->hasSingleTarget());

    
    IonCode *argumentsRectifier = gen->ionRuntime()->getArgumentsRectifier(executionMode);

    masm.checkStackAlignment();

    
    masm.loadObjClass(calleereg, nargsreg);
    masm.branchPtr(Assembler::NotEqual, nargsreg, ImmPtr(&JSFunction::class_), &invoke);

    
    
    if (call->mir()->isConstructing())
        masm.branchIfNotInterpretedConstructor(calleereg, nargsreg, &invoke);
    else
        masm.branchIfFunctionHasNoScript(calleereg, &invoke);

    
    masm.loadPtr(Address(calleereg, JSFunction::offsetOfNativeOrScript()), objreg);

    
    masm.loadBaselineOrIonRaw(objreg, objreg, executionMode, &invoke);

    
    masm.freeStack(unusedStack);

    
    uint32_t descriptor = MakeFrameDescriptor(masm.framePushed(), IonFrame_OptimizedJS);
    masm.Push(Imm32(call->numActualArgs()));
    masm.PushCalleeToken(calleereg, executionMode);
    masm.Push(Imm32(descriptor));

    
    masm.load16ZeroExtend(Address(calleereg, offsetof(JSFunction, nargs)), nargsreg);
    masm.cmp32(nargsreg, Imm32(call->numStackArgs()));
    masm.j(Assembler::Above, &thunk);

    masm.jump(&makeCall);

    
    masm.bind(&thunk);
    {
        JS_ASSERT(ArgumentsRectifierReg != objreg);
        masm.movePtr(ImmGCPtr(argumentsRectifier), objreg); 
        masm.loadPtr(Address(objreg, IonCode::offsetOfCode()), objreg);
        masm.move32(Imm32(call->numStackArgs()), ArgumentsRectifierReg);
    }

    
    masm.bind(&makeCall);
    uint32_t callOffset = masm.callIon(objreg);
    if (!markSafepointAt(callOffset, call))
        return false;

    
    
    int prefixGarbage = sizeof(IonJSFrameLayout) - sizeof(void *);
    masm.adjustStack(prefixGarbage - unusedStack);
    masm.jump(&end);

    
    masm.bind(&invoke);
    switch (executionMode) {
      case SequentialExecution:
        if (!emitCallInvokeFunction(call, calleereg, call->numActualArgs(), unusedStack))
            return false;
        break;

      case ParallelExecution:
        if (!emitCallToUncompiledScriptPar(call, calleereg))
            return false;
        break;

      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }

    masm.bind(&end);

    
    
    if (call->mir()->isConstructing()) {
        Label notPrimitive;
        masm.branchTestPrimitive(Assembler::NotEqual, JSReturnOperand, &notPrimitive);
        masm.loadValue(Address(StackPointer, unusedStack), JSReturnOperand);
        masm.bind(&notPrimitive);
    }

    if (!checkForAbortPar(call))
        return false;

    dropArguments(call->numStackArgs() + 1);
    return true;
}



bool
CodeGenerator::emitCallToUncompiledScriptPar(LInstruction *lir, Register calleeReg)
{
    OutOfLineCode *bail = oolAbortPar(ParallelBailoutCalledToUncompiledScript, lir);
    if (!bail)
        return false;

    masm.movePtr(calleeReg, CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, CallToUncompiledScriptPar));
    masm.jump(bail->entry());
    return true;
}

bool
CodeGenerator::visitCallKnown(LCallKnown *call)
{
    Register calleereg = ToRegister(call->getFunction());
    Register objreg    = ToRegister(call->getTempObject());
    uint32_t unusedStack = StackOffsetOfPassedArg(call->argslot());
    DebugOnly<JSFunction *> target = call->getSingleTarget();
    ExecutionMode executionMode = gen->info().executionMode();
    Label end, uncompiled;

    
    JS_ASSERT(!target->isNative());
    
    JS_ASSERT(target->nargs <= call->numStackArgs());

    JS_ASSERT_IF(call->mir()->isConstructing(), target->isInterpretedConstructor());

    masm.checkStackAlignment();

    
    
    masm.branchIfFunctionHasNoScript(calleereg, &uncompiled);

    
    masm.loadPtr(Address(calleereg, JSFunction::offsetOfNativeOrScript()), objreg);

    
    if (call->mir()->needsArgCheck())
        masm.loadBaselineOrIonRaw(objreg, objreg, executionMode, &uncompiled);
    else
        masm.loadBaselineOrIonNoArgCheck(objreg, objreg, executionMode, &uncompiled);

    
    masm.freeStack(unusedStack);

    
    uint32_t descriptor = MakeFrameDescriptor(masm.framePushed(), IonFrame_OptimizedJS);
    masm.Push(Imm32(call->numActualArgs()));
    masm.PushCalleeToken(calleereg, executionMode);
    masm.Push(Imm32(descriptor));

    
    uint32_t callOffset = masm.callIon(objreg);
    if (!markSafepointAt(callOffset, call))
        return false;

    
    
    int prefixGarbage = sizeof(IonJSFrameLayout) - sizeof(void *);
    masm.adjustStack(prefixGarbage - unusedStack);
    masm.jump(&end);

    
    masm.bind(&uncompiled);
    switch (executionMode) {
      case SequentialExecution:
        if (!emitCallInvokeFunction(call, calleereg, call->numActualArgs(), unusedStack))
            return false;
        break;

      case ParallelExecution:
        if (!emitCallToUncompiledScriptPar(call, calleereg))
            return false;
        break;

      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }

    masm.bind(&end);

    if (!checkForAbortPar(call))
        return false;

    
    
    if (call->mir()->isConstructing()) {
        Label notPrimitive;
        masm.branchTestPrimitive(Assembler::NotEqual, JSReturnOperand, &notPrimitive);
        masm.loadValue(Address(StackPointer, unusedStack), JSReturnOperand);
        masm.bind(&notPrimitive);
    }

    dropArguments(call->numStackArgs() + 1);
    return true;
}

bool
CodeGenerator::checkForAbortPar(LInstruction *lir)
{
    
    
    
    ExecutionMode executionMode = gen->info().executionMode();
    if (executionMode == ParallelExecution) {
        OutOfLinePropagateAbortPar *bail = oolPropagateAbortPar(lir);
        if (!bail)
            return false;
        masm.branchTestMagic(Assembler::Equal, JSReturnOperand, bail->entry());
    }
    return true;
}

bool
CodeGenerator::emitCallInvokeFunction(LApplyArgsGeneric *apply, Register extraStackSize)
{
    Register objreg = ToRegister(apply->getTempObject());
    JS_ASSERT(objreg != extraStackSize);

    
    masm.movePtr(StackPointer, objreg);
    masm.Push(extraStackSize);

    pushArg(objreg);                           
    pushArg(ToRegister(apply->getArgc()));     
    pushArg(ToRegister(apply->getFunction())); 

    
    if (!callVM(InvokeFunctionInfo, apply, &extraStackSize))
        return false;

    masm.Pop(extraStackSize);
    return true;
}



void
CodeGenerator::emitPushArguments(LApplyArgsGeneric *apply, Register extraStackSpace)
{
    
    Register argcreg = ToRegister(apply->getArgc());

    Register copyreg = ToRegister(apply->getTempObject());
    size_t argvOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs();
    Label end;

    
    masm.movePtr(argcreg, extraStackSpace);
    masm.branchTestPtr(Assembler::Zero, argcreg, argcreg, &end);

    
    {
        Register count = extraStackSpace; 
        Label loop;
        masm.bind(&loop);

        
        
        BaseIndex disp(StackPointer, argcreg, ScaleFromElemWidth(sizeof(Value)), argvOffset - sizeof(void*));

        
        
        masm.loadPtr(disp, copyreg);
        masm.push(copyreg);

        
        if (sizeof(Value) == 2 * sizeof(void*)) {
            masm.loadPtr(disp, copyreg);
            masm.push(copyreg);
        }

        masm.decBranchPtr(Assembler::NonZero, count, Imm32(1), &loop);
    }

    
    masm.movePtr(argcreg, extraStackSpace);
    masm.lshiftPtr(Imm32::ShiftOf(ScaleFromElemWidth(sizeof(Value))), extraStackSpace);

    
    masm.bind(&end);

    
    masm.addPtr(Imm32(sizeof(Value)), extraStackSpace);
    masm.pushValue(ToValue(apply, LApplyArgsGeneric::ThisIndex));
}

void
CodeGenerator::emitPopArguments(LApplyArgsGeneric *apply, Register extraStackSpace)
{
    
    masm.freeStack(extraStackSpace);
}

bool
CodeGenerator::visitApplyArgsGeneric(LApplyArgsGeneric *apply)
{
    
    Register calleereg = ToRegister(apply->getFunction());

    
    Register objreg = ToRegister(apply->getTempObject());
    Register copyreg = ToRegister(apply->getTempCopy());

    
    Register argcreg = ToRegister(apply->getArgc());

    
    if (!apply->hasSingleTarget()) {
        masm.loadObjClass(calleereg, objreg);
        masm.cmpPtr(objreg, ImmPtr(&JSFunction::class_));
        if (!bailoutIf(Assembler::NotEqual, apply->snapshot()))
            return false;
    }

    
    emitPushArguments(apply, copyreg);

    masm.checkStackAlignment();

    
    ExecutionMode executionMode = gen->info().executionMode();
    if (apply->hasSingleTarget()) {
        JSFunction *target = apply->getSingleTarget();
        if (!CanIonCompile(target, executionMode)) {
            if (!emitCallInvokeFunction(apply, copyreg))
                return false;
            emitPopArguments(apply, copyreg);
            return true;
        }
    }

    Label end, invoke;

    
    if (!apply->hasSingleTarget()) {
        masm.branchIfFunctionHasNoScript(calleereg, &invoke);
    } else {
        
        JS_ASSERT(!apply->getSingleTarget()->isNative());
    }

    
    masm.loadPtr(Address(calleereg, JSFunction::offsetOfNativeOrScript()), objreg);

    
    masm.loadBaselineOrIonRaw(objreg, objreg, executionMode, &invoke);

    
    {
        
        unsigned pushed = masm.framePushed();
        masm.addPtr(Imm32(pushed), copyreg);
        masm.makeFrameDescriptor(copyreg, IonFrame_OptimizedJS);

        masm.Push(argcreg);
        masm.Push(calleereg);
        masm.Push(copyreg); 

        Label underflow, rejoin;

        
        if (!apply->hasSingleTarget()) {
            masm.load16ZeroExtend(Address(calleereg, offsetof(JSFunction, nargs)), copyreg);
            masm.cmp32(argcreg, copyreg);
            masm.j(Assembler::Below, &underflow);
        } else {
            masm.cmp32(argcreg, Imm32(apply->getSingleTarget()->nargs));
            masm.j(Assembler::Below, &underflow);
        }

        
        
        masm.jump(&rejoin);

        
        {
            masm.bind(&underflow);

            
            IonCode *argumentsRectifier = gen->ionRuntime()->getArgumentsRectifier(executionMode);

            JS_ASSERT(ArgumentsRectifierReg != objreg);
            masm.movePtr(ImmGCPtr(argumentsRectifier), objreg); 
            masm.loadPtr(Address(objreg, IonCode::offsetOfCode()), objreg);
            masm.movePtr(argcreg, ArgumentsRectifierReg);
        }

        masm.bind(&rejoin);

        
        uint32_t callOffset = masm.callIon(objreg);
        if (!markSafepointAt(callOffset, apply))
            return false;

        
        masm.loadPtr(Address(StackPointer, 0), copyreg);
        masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), copyreg);
        masm.subPtr(Imm32(pushed), copyreg);

        
        
        int prefixGarbage = sizeof(IonJSFrameLayout) - sizeof(void *);
        masm.adjustStack(prefixGarbage);
        masm.jump(&end);
    }

    
    {
        masm.bind(&invoke);
        if (!emitCallInvokeFunction(apply, copyreg))
            return false;
    }

    
    masm.bind(&end);
    emitPopArguments(apply, copyreg);

    return true;
}

bool
CodeGenerator::visitBail(LBail *lir)
{
    return bailout(lir->snapshot());
}

bool
CodeGenerator::visitGetDynamicName(LGetDynamicName *lir)
{
    Register scopeChain = ToRegister(lir->getScopeChain());
    Register name = ToRegister(lir->getName());
    Register temp1 = ToRegister(lir->temp1());
    Register temp2 = ToRegister(lir->temp2());
    Register temp3 = ToRegister(lir->temp3());

    masm.loadJSContext(temp3);

    
    masm.adjustStack(-int32_t(sizeof(Value)));
    masm.movePtr(StackPointer, temp2);

    masm.setupUnalignedABICall(4, temp1);
    masm.passABIArg(temp3);
    masm.passABIArg(scopeChain);
    masm.passABIArg(name);
    masm.passABIArg(temp2);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, GetDynamicName));

    const ValueOperand out = ToOutValue(lir);

    masm.loadValue(Address(StackPointer, 0), out);
    masm.adjustStack(sizeof(Value));

    Assembler::Condition cond = masm.testUndefined(Assembler::Equal, out);
    return bailoutIf(cond, lir->snapshot());
}

bool
CodeGenerator::visitFilterArguments(LFilterArguments *lir)
{
    Register string = ToRegister(lir->getString());
    Register temp1 = ToRegister(lir->temp1());
    Register temp2 = ToRegister(lir->temp2());

    masm.loadJSContext(temp2);

    masm.setupUnalignedABICall(2, temp1);
    masm.passABIArg(temp2);
    masm.passABIArg(string);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, FilterArguments));

    Label bail;
    masm.branchIfFalseBool(ReturnReg, &bail);
    return bailoutFrom(&bail, lir->snapshot());
}

typedef bool (*DirectEvalFn)(JSContext *, HandleObject, HandleScript, HandleValue, HandleString,
                             jsbytecode *, MutableHandleValue);
static const VMFunction DirectEvalInfo = FunctionInfo<DirectEvalFn>(DirectEvalFromIon);

bool
CodeGenerator::visitCallDirectEval(LCallDirectEval *lir)
{
    Register scopeChain = ToRegister(lir->getScopeChain());
    Register string = ToRegister(lir->getString());

    pushArg(ImmPtr(lir->mir()->pc()));
    pushArg(string);
    pushArg(ToValue(lir, LCallDirectEval::ThisValueInput));
    pushArg(ImmGCPtr(gen->info().script()));
    pushArg(scopeChain);

    return callVM(DirectEvalInfo, lir);
}


static const uint32_t EntryTempMask = Registers::TempMask & ~(1 << OsrFrameReg.code());

bool
CodeGenerator::generateArgumentsChecks()
{
    MIRGraph &mir = gen->graph();
    MResumePoint *rp = mir.entryResumePoint();

    
    
    
    masm.reserveStack(frameSize());

    
    Register temp = GeneralRegisterSet(EntryTempMask).getAny();

    CompileInfo &info = gen->info();

    Label miss;
    for (uint32_t i = info.startArgSlot(); i < info.endArgSlot(); i++) {
        
        MParameter *param = rp->getOperand(i)->toParameter();
        const types::TypeSet *types = param->resultTypeSet();
        if (!types || types->unknown())
            continue;

        
        
        
        
        int32_t offset = ArgToStackOffset((i - info.startArgSlot()) * sizeof(Value));
        masm.guardTypeSet(Address(StackPointer, offset), types, temp, &miss);
    }

    if (miss.used() && !bailoutFrom(&miss, graph.entrySnapshot()))
        return false;

    masm.freeStack(frameSize());

    return true;
}


class CheckOverRecursedFailure : public OutOfLineCodeBase<CodeGenerator>
{
    LCheckOverRecursed *lir_;

  public:
    CheckOverRecursedFailure(LCheckOverRecursed *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitCheckOverRecursedFailure(this);
    }

    LCheckOverRecursed *lir() const {
        return lir_;
    }
};

bool
CodeGenerator::visitCheckOverRecursed(LCheckOverRecursed *lir)
{
    
    
    
    
    
    
    
    

    JSRuntime *rt = GetIonContext()->runtime;

    
    
    uintptr_t *limitAddr = &rt->mainThread.ionStackLimit;

    CheckOverRecursedFailure *ool = new CheckOverRecursedFailure(lir);
    if (!addOutOfLineCode(ool))
        return false;

    
    masm.branchPtr(Assembler::AboveOrEqual, AbsoluteAddress(limitAddr), StackPointer, ool->entry());
    masm.bind(ool->rejoin());

    return true;
}

typedef bool (*DefVarOrConstFn)(JSContext *, HandlePropertyName, unsigned, HandleObject);
static const VMFunction DefVarOrConstInfo =
    FunctionInfo<DefVarOrConstFn>(DefVarOrConst);

bool
CodeGenerator::visitDefVar(LDefVar *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());

    pushArg(scopeChain); 
    pushArg(Imm32(lir->mir()->attrs())); 
    pushArg(ImmGCPtr(lir->mir()->name())); 

    if (!callVM(DefVarOrConstInfo, lir))
        return false;

    return true;
}

typedef bool (*DefFunOperationFn)(JSContext *, HandleScript, HandleObject, HandleFunction);
static const VMFunction DefFunOperationInfo = FunctionInfo<DefFunOperationFn>(DefFunOperation);

bool
CodeGenerator::visitDefFun(LDefFun *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());

    pushArg(ImmGCPtr(lir->mir()->fun()));
    pushArg(scopeChain);
    pushArg(ImmGCPtr(current->mir()->info().script()));

    return callVM(DefFunOperationInfo, lir);
}

typedef bool (*ReportOverRecursedFn)(JSContext *);
static const VMFunction CheckOverRecursedInfo =
    FunctionInfo<ReportOverRecursedFn>(CheckOverRecursed);

bool
CodeGenerator::visitCheckOverRecursedFailure(CheckOverRecursedFailure *ool)
{
    
    

    
    
    
    saveLive(ool->lir());

    if (!callVM(CheckOverRecursedInfo, ool->lir()))
        return false;

    restoreLive(ool->lir());
    masm.jump(ool->rejoin());
    return true;
}


class CheckOverRecursedFailurePar : public OutOfLineCodeBase<CodeGenerator>
{
    LCheckOverRecursedPar *lir_;

  public:
    CheckOverRecursedFailurePar(LCheckOverRecursedPar *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitCheckOverRecursedFailurePar(this);
    }

    LCheckOverRecursedPar *lir() const {
        return lir_;
    }
};

bool
CodeGenerator::visitCheckOverRecursedPar(LCheckOverRecursedPar *lir)
{
    
    
    
    
    
    

    Register sliceReg = ToRegister(lir->forkJoinSlice());
    Register tempReg = ToRegister(lir->getTempReg());

    masm.loadPtr(Address(sliceReg, offsetof(ForkJoinSlice, perThreadData)), tempReg);
    masm.loadPtr(Address(tempReg, offsetof(PerThreadData, ionStackLimit)), tempReg);

    
    CheckOverRecursedFailurePar *ool = new CheckOverRecursedFailurePar(lir);
    if (!addOutOfLineCode(ool))
        return false;
    masm.branchPtr(Assembler::BelowOrEqual, StackPointer, tempReg, ool->entry());
    masm.checkInterruptFlagsPar(tempReg, ool->entry());
    masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitCheckOverRecursedFailurePar(CheckOverRecursedFailurePar *ool)
{
    OutOfLinePropagateAbortPar *bail = oolPropagateAbortPar(ool->lir());
    if (!bail)
        return false;

    
    
    
    LCheckOverRecursedPar *lir = ool->lir();
    Register tempReg = ToRegister(lir->getTempReg());
    RegisterSet saveSet(lir->safepoint()->liveRegs());
    saveSet.takeUnchecked(tempReg);

    masm.PushRegsInMask(saveSet);
    masm.movePtr(ToRegister(lir->forkJoinSlice()), CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, CheckOverRecursedPar));
    masm.movePtr(ReturnReg, tempReg);
    masm.PopRegsInMask(saveSet);
    masm.branchIfFalseBool(tempReg, bail->entry());
    masm.jump(ool->rejoin());

    return true;
}


class OutOfLineCheckInterruptPar : public OutOfLineCodeBase<CodeGenerator>
{
  public:
    LCheckInterruptPar *const lir;

    OutOfLineCheckInterruptPar(LCheckInterruptPar *lir)
      : lir(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineCheckInterruptPar(this);
    }
};

bool
CodeGenerator::visitCheckInterruptPar(LCheckInterruptPar *lir)
{
    
    OutOfLineCheckInterruptPar *ool = new OutOfLineCheckInterruptPar(lir);
    if (!addOutOfLineCode(ool))
        return false;

    
    
    
    

    Register tempReg = ToRegister(lir->getTempReg());
    masm.checkInterruptFlagsPar(tempReg, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineCheckInterruptPar(OutOfLineCheckInterruptPar *ool)
{
    OutOfLinePropagateAbortPar *bail = oolPropagateAbortPar(ool->lir);
    if (!bail)
        return false;

    
    
    
    LCheckInterruptPar *lir = ool->lir;
    Register tempReg = ToRegister(lir->getTempReg());
    RegisterSet saveSet(lir->safepoint()->liveRegs());
    saveSet.takeUnchecked(tempReg);

    masm.PushRegsInMask(saveSet);
    masm.movePtr(ToRegister(ool->lir->forkJoinSlice()), CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, CheckInterruptPar));
    masm.movePtr(ReturnReg, tempReg);
    masm.PopRegsInMask(saveSet);
    masm.branchIfFalseBool(tempReg, bail->entry());
    masm.jump(ool->rejoin());

    return true;
}

IonScriptCounts *
CodeGenerator::maybeCreateScriptCounts()
{
    
    
    JSContext *cx = GetIonContext()->cx;
    if (!cx || !cx->runtime()->profilingScripts)
        return nullptr;

    IonScriptCounts *counts = nullptr;

    CompileInfo *outerInfo = &gen->info();
    JSScript *script = outerInfo->script();

    if (script && !script->hasScriptCounts && !script->initScriptCounts(cx))
        return nullptr;

    counts = js_new<IonScriptCounts>();
    if (!counts || !counts->init(graph.numBlocks())) {
        js_delete(counts);
        return nullptr;
    }

    if (script)
        script->addIonCounts(counts);

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i)->mir();

        uint32_t offset = 0;
        if (script) {
            
            
            
            MResumePoint *resume = block->entryResumePoint();
            while (resume->caller())
                resume = resume->caller();
            DebugOnly<uint32_t> offset = resume->pc() - script->code;
            JS_ASSERT(offset < script->length);
        }

        if (!counts->block(i).init(block->id(), offset, block->numSuccessors()))
            return nullptr;
        for (size_t j = 0; j < block->numSuccessors(); j++)
            counts->block(i).setSuccessor(j, block->getSuccessor(j)->id());
    }

    if (!script) {
        
        
        unassociatedScriptCounts_ = counts;
    }

    return counts;
}


struct ScriptCountBlockState
{
    IonBlockCounts &block;
    MacroAssembler &masm;

    Sprinter printer;

    uint32_t instructionBytes;
    uint32_t spillBytes;

    
    
    uint32_t *last;
    uint32_t lastLength;

  public:
    ScriptCountBlockState(IonBlockCounts *block, MacroAssembler *masm)
      : block(*block), masm(*masm),
        printer(GetIonContext()->cx),
        instructionBytes(0), spillBytes(0), last(nullptr), lastLength(0)
    {
    }

    bool init()
    {
        if (!printer.init())
            return false;

        
        
        
        masm.inc64(AbsoluteAddress(block.addressOfHitCount()));

        
        masm.setPrinter(&printer);

        return true;
    }

    void visitInstruction(LInstruction *ins)
    {
        if (last)
            *last += masm.size() - lastLength;
        lastLength = masm.size();
        last = ins->isMoveGroup() ? &spillBytes : &instructionBytes;

        
        
        if (const char *extra = ins->extraName())
            printer.printf("[%s:%s]\n", ins->opName(), extra);
        else
            printer.printf("[%s]\n", ins->opName());
    }

    ~ScriptCountBlockState()
    {
        masm.setPrinter(nullptr);

        if (last)
            *last += masm.size() - lastLength;

        block.setCode(printer.string());
        block.setInstructionBytes(instructionBytes);
        block.setSpillBytes(spillBytes);
    }
};

bool
CodeGenerator::generateBody()
{
    IonScriptCounts *counts = maybeCreateScriptCounts();

#if defined(JS_ION_PERF)
    PerfSpewer *perfSpewer = &perfSpewer_;
    if (gen->compilingAsmJS())
        perfSpewer = &gen->perfSpewer();
#endif

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        current = graph.getBlock(i);

        LInstructionIterator iter = current->begin();

        
        
        if (!iter->accept(this))
            return false;
        iter++;

        mozilla::Maybe<ScriptCountBlockState> blockCounts;
        if (counts) {
            blockCounts.construct(&counts->block(i), &masm);
            if (!blockCounts.ref().init())
                return false;
        }

#if defined(JS_ION_PERF)
        perfSpewer->startBasicBlock(current->mir(), masm);
#endif

        for (; iter != current->end(); iter++) {
            IonSpew(IonSpew_Codegen, "instruction %s", iter->opName());

            if (counts)
                blockCounts.ref().visitInstruction(*iter);

            if (iter->safepoint() && pushedArgumentSlots_.length()) {
                if (!markArgumentSlots(iter->safepoint()))
                    return false;
            }

#ifdef CHECK_OSIPOINT_REGISTERS
            if (iter->safepoint())
                resetOsiPointRegs(iter->safepoint());
#endif

            if (!callTraceLIR(i, *iter))
                return false;

            if (!iter->accept(this))
                return false;
        }
        if (masm.oom())
            return false;

#if defined(JS_ION_PERF)
        perfSpewer->endBasicBlock(masm);
#endif
    }

    JS_ASSERT(pushedArgumentSlots_.empty());
    return true;
}


class OutOfLineNewParallelArray : public OutOfLineCodeBase<CodeGenerator>
{
    LNewParallelArray *lir_;

  public:
    OutOfLineNewParallelArray(LNewParallelArray *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineNewParallelArray(this);
    }

    LNewParallelArray *lir() const {
        return lir_;
    }
};

typedef JSObject *(*NewInitParallelArrayFn)(JSContext *, HandleObject);
static const VMFunction NewInitParallelArrayInfo =
    FunctionInfo<NewInitParallelArrayFn>(NewInitParallelArray);

bool
CodeGenerator::visitNewParallelArrayVMCall(LNewParallelArray *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);

    Register objReg = ToRegister(lir->output());

    JS_ASSERT(!lir->isCall());
    saveLive(lir);

    pushArg(ImmGCPtr(lir->mir()->templateObject()));
    if (!callVM(NewInitParallelArrayInfo, lir))
        return false;

    if (ReturnReg != objReg)
        masm.movePtr(ReturnReg, objReg);

    restoreLive(lir);
    return true;
}


class OutOfLineNewArray : public OutOfLineCodeBase<CodeGenerator>
{
    LNewArray *lir_;

  public:
    OutOfLineNewArray(LNewArray *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineNewArray(this);
    }

    LNewArray *lir() const {
        return lir_;
    }
};

typedef JSObject *(*NewInitArrayFn)(JSContext *, uint32_t, types::TypeObject *);
static const VMFunction NewInitArrayInfo =
    FunctionInfo<NewInitArrayFn>(NewInitArray);

bool
CodeGenerator::visitNewArrayCallVM(LNewArray *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);

    Register objReg = ToRegister(lir->output());

    JS_ASSERT(!lir->isCall());
    saveLive(lir);

    JSObject *templateObject = lir->mir()->templateObject();
    types::TypeObject *type =
        templateObject->hasSingletonType() ? nullptr : templateObject->type();

    pushArg(ImmGCPtr(type));
    pushArg(Imm32(lir->mir()->count()));

    if (!callVM(NewInitArrayInfo, lir))
        return false;

    if (ReturnReg != objReg)
        masm.movePtr(ReturnReg, objReg);

    restoreLive(lir);

    return true;
}

typedef JSObject *(*NewDerivedTypedObjectFn)(JSContext *,
                                             HandleObject type,
                                             HandleObject owner,
                                             int32_t offset);
static const VMFunction CreateDerivedTypedObjInfo =
    FunctionInfo<NewDerivedTypedObjectFn>(CreateDerivedTypedObj);

bool
CodeGenerator::visitNewDerivedTypedObject(LNewDerivedTypedObject *lir)
{
    
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);

    pushArg(ToRegister(lir->offset()));
    pushArg(ToRegister(lir->owner()));
    pushArg(ToRegister(lir->type()));
    return callVM(CreateDerivedTypedObjInfo, lir);
}

bool
CodeGenerator::visitNewSlots(LNewSlots *lir)
{
    Register temp1 = ToRegister(lir->temp1());
    Register temp2 = ToRegister(lir->temp2());
    Register temp3 = ToRegister(lir->temp3());
    Register output = ToRegister(lir->output());

    masm.mov(ImmPtr(GetIonContext()->runtime), temp1);
    masm.mov(ImmWord(lir->mir()->nslots()), temp2);

    masm.setupUnalignedABICall(2, temp3);
    masm.passABIArg(temp1);
    masm.passABIArg(temp2);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NewSlots));

    masm.testPtr(output, output);
    if (!bailoutIf(Assembler::Zero, lir->snapshot()))
        return false;

    return true;
}

bool CodeGenerator::visitAtan2D(LAtan2D *lir)
{
    Register temp = ToRegister(lir->temp());
    FloatRegister y = ToFloatRegister(lir->y());
    FloatRegister x = ToFloatRegister(lir->x());

    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(y);
    masm.passABIArg(x);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ecmaAtan2), MacroAssembler::DOUBLE);

    JS_ASSERT(ToFloatRegister(lir->output()) == ReturnFloatReg);
    return true;
}

bool
CodeGenerator::visitNewParallelArray(LNewParallelArray *lir)
{
    Register objReg = ToRegister(lir->output());
    JSObject *templateObject = lir->mir()->templateObject();

    OutOfLineNewParallelArray *ool = new OutOfLineNewParallelArray(lir);
    if (!addOutOfLineCode(ool))
        return false;

    masm.newGCThing(objReg, templateObject, ool->entry());
    masm.initGCThing(objReg, templateObject);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineNewParallelArray(OutOfLineNewParallelArray *ool)
{
    if (!visitNewParallelArrayVMCall(ool->lir()))
        return false;
    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitNewArray(LNewArray *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);
    Register objReg = ToRegister(lir->output());
    JSObject *templateObject = lir->mir()->templateObject();
    DebugOnly<uint32_t> count = lir->mir()->count();

    JS_ASSERT(count < JSObject::NELEMENTS_LIMIT);

    if (lir->mir()->shouldUseVM())
        return visitNewArrayCallVM(lir);

    OutOfLineNewArray *ool = new OutOfLineNewArray(lir);
    if (!addOutOfLineCode(ool))
        return false;

    masm.newGCThing(objReg, templateObject, ool->entry());
    masm.initGCThing(objReg, templateObject);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineNewArray(OutOfLineNewArray *ool)
{
    if (!visitNewArrayCallVM(ool->lir()))
        return false;
    masm.jump(ool->rejoin());
    return true;
}


class OutOfLineNewObject : public OutOfLineCodeBase<CodeGenerator>
{
    LNewObject *lir_;

  public:
    OutOfLineNewObject(LNewObject *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineNewObject(this);
    }

    LNewObject *lir() const {
        return lir_;
    }
};

typedef JSObject *(*NewInitObjectFn)(JSContext *, HandleObject);
static const VMFunction NewInitObjectInfo = FunctionInfo<NewInitObjectFn>(NewInitObject);

typedef JSObject *(*NewInitObjectWithClassPrototypeFn)(JSContext *, HandleObject);
static const VMFunction NewInitObjectWithClassPrototypeInfo =
    FunctionInfo<NewInitObjectWithClassPrototypeFn>(NewInitObjectWithClassPrototype);

bool
CodeGenerator::visitNewObjectVMCall(LNewObject *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);

    Register objReg = ToRegister(lir->output());

    JS_ASSERT(!lir->isCall());
    saveLive(lir);

    pushArg(ImmGCPtr(lir->mir()->templateObject()));

    
    
    
    
    if (lir->mir()->templateObjectIsClassPrototype()) {
        if (!callVM(NewInitObjectWithClassPrototypeInfo, lir))
            return false;
    } else if (!callVM(NewInitObjectInfo, lir)) {
        return false;
    }

    if (ReturnReg != objReg)
        masm.movePtr(ReturnReg, objReg);

    restoreLive(lir);
    return true;
}

bool
CodeGenerator::visitNewObject(LNewObject *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);
    Register objReg = ToRegister(lir->output());
    JSObject *templateObject = lir->mir()->templateObject();

    if (lir->mir()->shouldUseVM())
        return visitNewObjectVMCall(lir);

    OutOfLineNewObject *ool = new OutOfLineNewObject(lir);
    if (!addOutOfLineCode(ool))
        return false;

    masm.newGCThing(objReg, templateObject, ool->entry());
    masm.initGCThing(objReg, templateObject);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineNewObject(OutOfLineNewObject *ool)
{
    if (!visitNewObjectVMCall(ool->lir()))
        return false;
    masm.jump(ool->rejoin());
    return true;
}

typedef js::DeclEnvObject *(*NewDeclEnvObjectFn)(JSContext *, HandleFunction, gc::InitialHeap);
static const VMFunction NewDeclEnvObjectInfo =
    FunctionInfo<NewDeclEnvObjectFn>(DeclEnvObject::createTemplateObject);

bool
CodeGenerator::visitNewDeclEnvObject(LNewDeclEnvObject *lir)
{
    Register obj = ToRegister(lir->output());
    JSObject *templateObj = lir->mir()->templateObj();
    CompileInfo &info = lir->mir()->block()->info();

    
    OutOfLineCode *ool = oolCallVM(NewDeclEnvObjectInfo, lir,
                                   (ArgList(), ImmGCPtr(info.fun()), Imm32(gc::DefaultHeap)),
                                   StoreRegisterTo(obj));
    if (!ool)
        return false;

    masm.newGCThing(obj, templateObj, ool->entry());
    masm.initGCThing(obj, templateObj);
    masm.bind(ool->rejoin());
    return true;
}

typedef JSObject *(*NewCallObjectFn)(JSContext *, HandleScript, HandleShape,
                                     HandleTypeObject, HeapSlot *);
static const VMFunction NewCallObjectInfo =
    FunctionInfo<NewCallObjectFn>(NewCallObject);

bool
CodeGenerator::visitNewCallObject(LNewCallObject *lir)
{
    Register obj = ToRegister(lir->output());

    JSObject *templateObj = lir->mir()->templateObject();

    
    OutOfLineCode *ool;
    if (lir->slots()->isRegister()) {
        ool = oolCallVM(NewCallObjectInfo, lir,
                        (ArgList(), ImmGCPtr(lir->mir()->block()->info().script()),
                                    ImmGCPtr(templateObj->lastProperty()),
                                    ImmGCPtr(templateObj->hasLazyType() ? nullptr : templateObj->type()),
                                    ToRegister(lir->slots())),
                        StoreRegisterTo(obj));
    } else {
        ool = oolCallVM(NewCallObjectInfo, lir,
                        (ArgList(), ImmGCPtr(lir->mir()->block()->info().script()),
                                    ImmGCPtr(templateObj->lastProperty()),
                                    ImmGCPtr(templateObj->hasLazyType() ? nullptr : templateObj->type()),
                                    ImmPtr(nullptr)),
                        StoreRegisterTo(obj));
    }
    if (!ool)
        return false;

    if (lir->mir()->needsSingletonType()) {
        
        masm.jump(ool->entry());
    } else {
        masm.newGCThing(obj, templateObj, ool->entry());
        masm.initGCThing(obj, templateObj);

        if (lir->slots()->isRegister())
            masm.storePtr(ToRegister(lir->slots()), Address(obj, JSObject::offsetOfSlots()));
    }

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitNewCallObjectPar(LNewCallObjectPar *lir)
{
    Register resultReg = ToRegister(lir->output());
    Register sliceReg = ToRegister(lir->forkJoinSlice());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    JSObject *templateObj = lir->mir()->templateObj();

    emitAllocateGCThingPar(lir, resultReg, sliceReg, tempReg1, tempReg2, templateObj);

    
    
    

    if (lir->slots()->isRegister()) {
        Register slotsReg = ToRegister(lir->slots());
        JS_ASSERT(slotsReg != resultReg);
        masm.storePtr(slotsReg, Address(resultReg, JSObject::offsetOfSlots()));
    }

    return true;
}

bool
CodeGenerator::visitNewDenseArrayPar(LNewDenseArrayPar *lir)
{
    Register sliceReg = ToRegister(lir->forkJoinSlice());
    Register lengthReg = ToRegister(lir->length());
    Register tempReg0 = ToRegister(lir->getTemp0());
    Register tempReg1 = ToRegister(lir->getTemp1());
    Register tempReg2 = ToRegister(lir->getTemp2());
    JSObject *templateObj = lir->mir()->templateObject();

    
    
    emitAllocateGCThingPar(lir, tempReg2, sliceReg, tempReg0, tempReg1, templateObj);

    
    
    
    
    
    
    
    
    masm.setupUnalignedABICall(3, tempReg0);
    masm.passABIArg(sliceReg);
    masm.passABIArg(tempReg2);
    masm.passABIArg(lengthReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ExtendArrayPar));

    Register resultReg = ToRegister(lir->output());
    JS_ASSERT(resultReg == ReturnReg);
    OutOfLineAbortPar *bail = oolAbortPar(ParallelBailoutOutOfMemory, lir);
    if (!bail)
        return false;
    masm.branchTestPtr(Assembler::Zero, resultReg, resultReg, bail->entry());

    return true;
}

typedef JSObject *(*NewStringObjectFn)(JSContext *, HandleString);
static const VMFunction NewStringObjectInfo = FunctionInfo<NewStringObjectFn>(NewStringObject);

bool
CodeGenerator::visitNewStringObject(LNewStringObject *lir)
{
    Register input = ToRegister(lir->input());
    Register output = ToRegister(lir->output());
    Register temp = ToRegister(lir->temp());

    StringObject *templateObj = lir->mir()->templateObj();

    OutOfLineCode *ool = oolCallVM(NewStringObjectInfo, lir, (ArgList(), input),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    masm.newGCThing(output, templateObj, ool->entry());
    masm.initGCThing(output, templateObj);

    masm.loadStringLength(input, temp);

    masm.storeValue(JSVAL_TYPE_STRING, input, Address(output, StringObject::offsetOfPrimitiveValue()));
    masm.storeValue(JSVAL_TYPE_INT32, temp, Address(output, StringObject::offsetOfLength()));

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitNewPar(LNewPar *lir)
{
    Register objReg = ToRegister(lir->output());
    Register sliceReg = ToRegister(lir->forkJoinSlice());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    JSObject *templateObject = lir->mir()->templateObject();
    emitAllocateGCThingPar(lir, objReg, sliceReg, tempReg1, tempReg2, templateObject);
    return true;
}

class OutOfLineNewGCThingPar : public OutOfLineCodeBase<CodeGenerator>
{
public:
    LInstruction *lir;
    gc::AllocKind allocKind;
    Register objReg;
    Register sliceReg;

    OutOfLineNewGCThingPar(LInstruction *lir, gc::AllocKind allocKind, Register objReg,
                           Register sliceReg)
      : lir(lir), allocKind(allocKind), objReg(objReg), sliceReg(sliceReg)
    {}

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineNewGCThingPar(this);
    }
};

bool
CodeGenerator::emitAllocateGCThingPar(LInstruction *lir, const Register &objReg,
                                      const Register &sliceReg, const Register &tempReg1,
                                      const Register &tempReg2, JSObject *templateObj)
{
    gc::AllocKind allocKind = templateObj->tenuredGetAllocKind();
    OutOfLineNewGCThingPar *ool = new OutOfLineNewGCThingPar(lir, allocKind, objReg, sliceReg);
    if (!ool || !addOutOfLineCode(ool))
        return false;

    masm.newGCThingPar(objReg, sliceReg, tempReg1, tempReg2, templateObj, ool->entry());
    masm.bind(ool->rejoin());
    masm.initGCThing(objReg, templateObj);
    return true;
}

bool
CodeGenerator::visitOutOfLineNewGCThingPar(OutOfLineNewGCThingPar *ool)
{
    
    
    
    
    Register out = ool->objReg;

    saveVolatile(out);
    masm.setupUnalignedABICall(2, out);
    masm.passABIArg(ool->sliceReg);
    masm.move32(Imm32(ool->allocKind), out);
    masm.passABIArg(out);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NewGCThingPar));
    masm.storeCallResult(out);
    restoreVolatile(out);

    OutOfLineAbortPar *bail = oolAbortPar(ParallelBailoutOutOfMemory, ool->lir);
    if (!bail)
        return false;
    masm.branchTestPtr(Assembler::Zero, out, out, bail->entry());
    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitAbortPar(LAbortPar *lir)
{
    OutOfLineAbortPar *bail = oolAbortPar(ParallelBailoutUnsupported, lir);
    if (!bail)
        return false;
    masm.jump(bail->entry());
    return true;
}

typedef bool(*InitElemFn)(JSContext *cx, HandleObject obj,
                          HandleValue id, HandleValue value);
static const VMFunction InitElemInfo =
    FunctionInfo<InitElemFn>(InitElemOperation);

bool
CodeGenerator::visitInitElem(LInitElem *lir)
{
    Register objReg = ToRegister(lir->getObject());

    pushArg(ToValue(lir, LInitElem::ValueIndex));
    pushArg(ToValue(lir, LInitElem::IdIndex));
    pushArg(objReg);

    return callVM(InitElemInfo, lir);
}

typedef bool (*InitElemGetterSetterFn)(JSContext *, jsbytecode *, HandleObject, HandleValue,
                                       HandleObject);
static const VMFunction InitElemGetterSetterInfo =
    FunctionInfo<InitElemGetterSetterFn>(InitGetterSetterOperation);

bool
CodeGenerator::visitInitElemGetterSetter(LInitElemGetterSetter *lir)
{
    Register obj = ToRegister(lir->object());
    Register value = ToRegister(lir->value());

    pushArg(value);
    pushArg(ToValue(lir, LInitElemGetterSetter::IdIndex));
    pushArg(obj);
    pushArg(ImmPtr(lir->mir()->resumePoint()->pc()));

    return callVM(InitElemGetterSetterInfo, lir);
}

typedef bool(*InitPropFn)(JSContext *cx, HandleObject obj,
                          HandlePropertyName name, HandleValue value);
static const VMFunction InitPropInfo =
    FunctionInfo<InitPropFn>(InitProp);

bool
CodeGenerator::visitInitProp(LInitProp *lir)
{
    Register objReg = ToRegister(lir->getObject());

    pushArg(ToValue(lir, LInitProp::ValueIndex));
    pushArg(ImmGCPtr(lir->mir()->propertyName()));
    pushArg(objReg);

    return callVM(InitPropInfo, lir);
}

typedef bool(*InitPropGetterSetterFn)(JSContext *, jsbytecode *, HandleObject, HandlePropertyName,
                                      HandleObject);
static const VMFunction InitPropGetterSetterInfo =
    FunctionInfo<InitPropGetterSetterFn>(InitGetterSetterOperation);

bool
CodeGenerator::visitInitPropGetterSetter(LInitPropGetterSetter *lir)
{
    Register obj = ToRegister(lir->object());
    Register value = ToRegister(lir->value());

    pushArg(value);
    pushArg(ImmGCPtr(lir->mir()->name()));
    pushArg(obj);
    pushArg(ImmPtr(lir->mir()->resumePoint()->pc()));

    return callVM(InitPropGetterSetterInfo, lir);
}

typedef bool (*CreateThisFn)(JSContext *cx, HandleObject callee, MutableHandleValue rval);
static const VMFunction CreateThisInfo = FunctionInfo<CreateThisFn>(CreateThis);

bool
CodeGenerator::visitCreateThis(LCreateThis *lir)
{
    const LAllocation *callee = lir->getCallee();

    if (callee->isConstant())
        pushArg(ImmGCPtr(&callee->toConstant()->toObject()));
    else
        pushArg(ToRegister(callee));

    return callVM(CreateThisInfo, lir);
}

static JSObject *
CreateThisForFunctionWithProtoWrapper(JSContext *cx, js::HandleObject callee, JSObject *proto)
{
    return CreateThisForFunctionWithProto(cx, callee, proto);
}

typedef JSObject *(*CreateThisWithProtoFn)(JSContext *cx, HandleObject callee, JSObject *proto);
static const VMFunction CreateThisWithProtoInfo =
FunctionInfo<CreateThisWithProtoFn>(CreateThisForFunctionWithProtoWrapper);

bool
CodeGenerator::visitCreateThisWithProto(LCreateThisWithProto *lir)
{
    const LAllocation *callee = lir->getCallee();
    const LAllocation *proto = lir->getPrototype();

    if (proto->isConstant())
        pushArg(ImmGCPtr(&proto->toConstant()->toObject()));
    else
        pushArg(ToRegister(proto));

    if (callee->isConstant())
        pushArg(ImmGCPtr(&callee->toConstant()->toObject()));
    else
        pushArg(ToRegister(callee));

    return callVM(CreateThisWithProtoInfo, lir);
}

typedef JSObject *(*NewGCThingFn)(JSContext *cx, gc::AllocKind allocKind, size_t thingSize,
                                  gc::InitialHeap initialHeap);
static const VMFunction NewGCThingInfo =
    FunctionInfo<NewGCThingFn>(js::jit::NewGCThing);

bool
CodeGenerator::visitCreateThisWithTemplate(LCreateThisWithTemplate *lir)
{
    JSObject *templateObject = lir->mir()->templateObject();
    gc::AllocKind allocKind = templateObject->tenuredGetAllocKind();
    int thingSize = (int)gc::Arena::thingSize(allocKind);
    gc::InitialHeap initialHeap = templateObject->type()->initialHeapForJITAlloc();
    Register objReg = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(NewGCThingInfo, lir,
                                   (ArgList(), Imm32(allocKind), Imm32(thingSize), Imm32(initialHeap)),
                                   StoreRegisterTo(objReg));
    if (!ool)
        return false;

    
    masm.newGCThing(objReg, templateObject, ool->entry());

    
    masm.bind(ool->rejoin());
    masm.initGCThing(objReg, templateObject);

    return true;
}

typedef JSObject *(*NewIonArgumentsObjectFn)(JSContext *cx, IonJSFrameLayout *frame, HandleObject);
static const VMFunction NewIonArgumentsObjectInfo =
    FunctionInfo<NewIonArgumentsObjectFn>((NewIonArgumentsObjectFn) ArgumentsObject::createForIon);

bool
CodeGenerator::visitCreateArgumentsObject(LCreateArgumentsObject *lir)
{
    
    JS_ASSERT(lir->mir()->block()->id() == 0);

    const LAllocation *callObj = lir->getCallObject();
    Register temp = ToRegister(lir->getTemp(0));

    masm.movePtr(StackPointer, temp);
    masm.addPtr(Imm32(frameSize()), temp);

    pushArg(ToRegister(callObj));
    pushArg(temp);
    return callVM(NewIonArgumentsObjectInfo, lir);
}

bool
CodeGenerator::visitGetArgumentsObjectArg(LGetArgumentsObjectArg *lir)
{
    Register temp = ToRegister(lir->getTemp(0));
    Register argsObj = ToRegister(lir->getArgsObject());
    ValueOperand out = ToOutValue(lir);

    masm.loadPrivate(Address(argsObj, ArgumentsObject::getDataSlotOffset()), temp);
    Address argAddr(temp, ArgumentsData::offsetOfArgs() + lir->mir()->argno() * sizeof(Value));
    masm.loadValue(argAddr, out);
#ifdef DEBUG
    Label success;
    masm.branchTestMagic(Assembler::NotEqual, out, &success);
    masm.breakpoint();
    masm.bind(&success);
#endif
    return true;
}

bool
CodeGenerator::visitSetArgumentsObjectArg(LSetArgumentsObjectArg *lir)
{
    Register temp = ToRegister(lir->getTemp(0));
    Register argsObj = ToRegister(lir->getArgsObject());
    ValueOperand value = ToValue(lir, LSetArgumentsObjectArg::ValueIndex);

    masm.loadPrivate(Address(argsObj, ArgumentsObject::getDataSlotOffset()), temp);
    Address argAddr(temp, ArgumentsData::offsetOfArgs() + lir->mir()->argno() * sizeof(Value));
    emitPreBarrier(argAddr, MIRType_Value);
#ifdef DEBUG
    Label success;
    masm.branchTestMagic(Assembler::NotEqual, argAddr, &success);
    masm.breakpoint();
    masm.bind(&success);
#endif
    masm.storeValue(value, argAddr);
    return true;
}

bool
CodeGenerator::visitReturnFromCtor(LReturnFromCtor *lir)
{
    ValueOperand value = ToValue(lir, LReturnFromCtor::ValueIndex);
    Register obj = ToRegister(lir->getObject());
    Register output = ToRegister(lir->output());

    Label valueIsObject, end;

    masm.branchTestObject(Assembler::Equal, value, &valueIsObject);

    
    masm.movePtr(obj, output);
    masm.jump(&end);

    
    masm.bind(&valueIsObject);
    Register payload = masm.extractObject(value, output);
    if (payload != output)
        masm.movePtr(payload, output);

    masm.bind(&end);
    return true;
}

typedef JSObject *(*BoxNonStrictThisFn)(JSContext *, HandleValue);
static const VMFunction BoxNonStrictThisInfo = FunctionInfo<BoxNonStrictThisFn>(BoxNonStrictThis);

bool
CodeGenerator::visitComputeThis(LComputeThis *lir)
{
    ValueOperand value = ToValue(lir, LComputeThis::ValueIndex);
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(BoxNonStrictThisInfo, lir, (ArgList(), value),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    masm.branchTestObject(Assembler::NotEqual, value, ool->entry());
    masm.unboxObject(value, output);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitArrayLength(LArrayLength *lir)
{
    Address length(ToRegister(lir->elements()), ObjectElements::offsetOfLength());
    masm.load32(length, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitTypedArrayLength(LTypedArrayLength *lir)
{
    Register obj = ToRegister(lir->object());
    Register out = ToRegister(lir->output());
    masm.unboxInt32(Address(obj, TypedArrayObject::lengthOffset()), out);
    return true;
}

bool
CodeGenerator::visitTypedArrayElements(LTypedArrayElements *lir)
{
    Register obj = ToRegister(lir->object());
    Register out = ToRegister(lir->output());
    masm.loadPtr(Address(obj, TypedArrayObject::dataOffset()), out);
    return true;
}

bool
CodeGenerator::visitTypedObjectElements(LTypedObjectElements *lir)
{
    Register obj = ToRegister(lir->object());
    Register out = ToRegister(lir->output());
    masm.loadPtr(Address(obj, BinaryBlock::dataOffset()), out);
    return true;
}

bool
CodeGenerator::visitStringLength(LStringLength *lir)
{
    Register input = ToRegister(lir->string());
    Register output = ToRegister(lir->output());

    masm.loadStringLength(input, output);
    return true;
}

bool
CodeGenerator::visitMinMaxI(LMinMaxI *ins)
{
    Register first = ToRegister(ins->first());
    Register output = ToRegister(ins->output());

    JS_ASSERT(first == output);

    if (ins->second()->isConstant())
        masm.cmp32(first, Imm32(ToInt32(ins->second())));
    else
        masm.cmp32(first, ToRegister(ins->second()));

    Label done;
    if (ins->mir()->isMax())
        masm.j(Assembler::GreaterThan, &done);
    else
        masm.j(Assembler::LessThan, &done);

    if (ins->second()->isConstant())
        masm.move32(Imm32(ToInt32(ins->second())), output);
    else
        masm.mov(ToRegister(ins->second()), output);


    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitAbsI(LAbsI *ins)
{
    Register input = ToRegister(ins->input());
    Label positive;

    JS_ASSERT(input == ToRegister(ins->output()));
    masm.test32(input, input);
    masm.j(Assembler::GreaterThanOrEqual, &positive);
    masm.neg32(input);
    if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
        return false;
    masm.bind(&positive);

    return true;
}

bool
CodeGenerator::visitPowI(LPowI *ins)
{
    FloatRegister value = ToFloatRegister(ins->value());
    Register power = ToRegister(ins->power());
    Register temp = ToRegister(ins->temp());

    JS_ASSERT(power != temp);

    
    
    
    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(value);
    masm.passABIArg(power);

    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::powi), MacroAssembler::DOUBLE);
    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnFloatReg);

    return true;
}

bool
CodeGenerator::visitPowD(LPowD *ins)
{
    FloatRegister value = ToFloatRegister(ins->value());
    FloatRegister power = ToFloatRegister(ins->power());
    Register temp = ToRegister(ins->temp());

    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(value);
    masm.passABIArg(power);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ecmaPow), MacroAssembler::DOUBLE);

    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnFloatReg);
    return true;
}

bool
CodeGenerator::visitRandom(LRandom *ins)
{
    Register temp = ToRegister(ins->temp());
    Register temp2 = ToRegister(ins->temp2());

    masm.loadJSContext(temp);

    masm.setupUnalignedABICall(1, temp2);
    masm.passABIArg(temp);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, math_random_no_outparam), MacroAssembler::DOUBLE);

    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnFloatReg);
    return true;
}

bool
CodeGenerator::visitMathFunctionD(LMathFunctionD *ins)
{
    Register temp = ToRegister(ins->temp());
    FloatRegister input = ToFloatRegister(ins->input());
    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnFloatReg);

    MathCache *mathCache = ins->mir()->cache();

    masm.setupUnalignedABICall(mathCache ? 2 : 1, temp);
    if (mathCache) {
        masm.movePtr(ImmPtr(mathCache), temp);
        masm.passABIArg(temp);
    }
    masm.passABIArg(input);

#   define MAYBE_CACHED(fcn) (mathCache ? (void*)fcn ## _impl : (void*)fcn ## _uncached)

    void *funptr = nullptr;
    switch (ins->mir()->function()) {
      case MMathFunction::Log:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_log));
        break;
      case MMathFunction::Sin:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_sin));
        break;
      case MMathFunction::Cos:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_cos));
        break;
      case MMathFunction::Exp:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_exp));
        break;
      case MMathFunction::Tan:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_tan));
        break;
      case MMathFunction::ATan:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_atan));
        break;
      case MMathFunction::ASin:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_asin));
        break;
      case MMathFunction::ACos:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_acos));
        break;
      case MMathFunction::Log10:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_log10));
        break;
      case MMathFunction::Log2:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_log2));
        break;
      case MMathFunction::Log1P:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_log1p));
        break;
      case MMathFunction::ExpM1:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_expm1));
        break;
      case MMathFunction::CosH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_cosh));
        break;
      case MMathFunction::SinH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_sinh));
        break;
      case MMathFunction::TanH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_tanh));
        break;
      case MMathFunction::ACosH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_acosh));
        break;
      case MMathFunction::ASinH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_asinh));
        break;
      case MMathFunction::ATanH:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_atanh));
        break;
      case MMathFunction::Sign:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_sign));
        break;
      case MMathFunction::Trunc:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_trunc));
        break;
      case MMathFunction::Cbrt:
        funptr = JS_FUNC_TO_DATA_PTR(void *, MAYBE_CACHED(js::math_cbrt));
        break;
      case MMathFunction::Floor:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_floor_impl);
        break;
      case MMathFunction::Round:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_round_impl);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Unknown math function");
    }

#   undef MAYBE_CACHED

    masm.callWithABI(funptr, MacroAssembler::DOUBLE);
    return true;
}

bool
CodeGenerator::visitModD(LModD *ins)
{
    FloatRegister lhs = ToFloatRegister(ins->lhs());
    FloatRegister rhs = ToFloatRegister(ins->rhs());
    Register temp = ToRegister(ins->temp());

    JS_ASSERT(ToFloatRegister(ins->output()) == ReturnFloatReg);

    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(lhs);
    masm.passABIArg(rhs);

    if (gen->compilingAsmJS())
        masm.callWithABI(AsmJSImm_ModD, MacroAssembler::DOUBLE);
    else
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NumberMod), MacroAssembler::DOUBLE);
    return true;
}

typedef bool (*BinaryFn)(JSContext *, MutableHandleValue, MutableHandleValue, Value *);
typedef ParallelResult (*BinaryParFn)(ForkJoinSlice *, HandleValue, HandleValue, Value *);

static const VMFunction AddInfo = FunctionInfo<BinaryFn>(js::AddValues);
static const VMFunction SubInfo = FunctionInfo<BinaryFn>(js::SubValues);
static const VMFunction MulInfo = FunctionInfo<BinaryFn>(js::MulValues);
static const VMFunction DivInfo = FunctionInfo<BinaryFn>(js::DivValues);
static const VMFunction ModInfo = FunctionInfo<BinaryFn>(js::ModValues);
static const VMFunctionsModal UrshInfo = VMFunctionsModal(
    FunctionInfo<BinaryFn>(js::UrshValues),
    FunctionInfo<BinaryParFn>(UrshValuesPar));

bool
CodeGenerator::visitBinaryV(LBinaryV *lir)
{
    pushArg(ToValue(lir, LBinaryV::RhsInput));
    pushArg(ToValue(lir, LBinaryV::LhsInput));

    switch (lir->jsop()) {
      case JSOP_ADD:
        return callVM(AddInfo, lir);

      case JSOP_SUB:
        return callVM(SubInfo, lir);

      case JSOP_MUL:
        return callVM(MulInfo, lir);

      case JSOP_DIV:
        return callVM(DivInfo, lir);

      case JSOP_MOD:
        return callVM(ModInfo, lir);

      case JSOP_URSH:
        return callVM(UrshInfo, lir);

      default:
        MOZ_ASSUME_UNREACHABLE("Unexpected binary op");
    }
}

typedef bool (*StringCompareFn)(JSContext *, HandleString, HandleString, bool *);
typedef ParallelResult (*StringCompareParFn)(ForkJoinSlice *, HandleString, HandleString, bool *);
static const VMFunctionsModal StringsEqualInfo = VMFunctionsModal(
    FunctionInfo<StringCompareFn>(jit::StringsEqual<true>),
    FunctionInfo<StringCompareParFn>(jit::StringsEqualPar));
static const VMFunctionsModal StringsNotEqualInfo = VMFunctionsModal(
    FunctionInfo<StringCompareFn>(jit::StringsEqual<false>),
    FunctionInfo<StringCompareParFn>(jit::StringsUnequalPar));

bool
CodeGenerator::emitCompareS(LInstruction *lir, JSOp op, Register left, Register right,
                            Register output, Register temp)
{
    JS_ASSERT(lir->isCompareS() || lir->isCompareStrictS());

    OutOfLineCode *ool = nullptr;

    if (op == JSOP_EQ || op == JSOP_STRICTEQ) {
        ool = oolCallVM(StringsEqualInfo, lir, (ArgList(), left, right),  StoreRegisterTo(output));
    } else {
        JS_ASSERT(op == JSOP_NE || op == JSOP_STRICTNE);
        ool = oolCallVM(StringsNotEqualInfo, lir, (ArgList(), left, right), StoreRegisterTo(output));
    }
    if (!ool)
        return false;

    masm.compareStrings(op, left, right, output, temp, ool->entry());

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitCompareStrictS(LCompareStrictS *lir)
{
    JSOp op = lir->mir()->jsop();
    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    const ValueOperand leftV = ToValue(lir, LCompareStrictS::Lhs);
    Register right = ToRegister(lir->right());
    Register output = ToRegister(lir->output());
    Register temp = ToRegister(lir->temp());
    Register tempToUnbox = ToTempUnboxRegister(lir->tempToUnbox());

    Label string, done;

    masm.branchTestString(Assembler::Equal, leftV, &string);
    masm.move32(Imm32(op == JSOP_STRICTNE), output);
    masm.jump(&done);

    masm.bind(&string);
    Register left = masm.extractString(leftV, tempToUnbox);
    if (!emitCompareS(lir, op, left, right, output, temp))
        return false;

    masm.bind(&done);

    return true;
}

bool
CodeGenerator::visitCompareS(LCompareS *lir)
{
    JSOp op = lir->mir()->jsop();
    Register left = ToRegister(lir->left());
    Register right = ToRegister(lir->right());
    Register output = ToRegister(lir->output());
    Register temp = ToRegister(lir->temp());

    return emitCompareS(lir, op, left, right, output, temp);
}

typedef bool (*CompareFn)(JSContext *, MutableHandleValue, MutableHandleValue, bool *);
typedef ParallelResult (*CompareParFn)(ForkJoinSlice *, MutableHandleValue, MutableHandleValue,
                                       bool *);
static const VMFunctionsModal EqInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::LooselyEqual<true>),
    FunctionInfo<CompareParFn>(jit::LooselyEqualPar));
static const VMFunctionsModal NeInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::LooselyEqual<false>),
    FunctionInfo<CompareParFn>(jit::LooselyUnequalPar));
static const VMFunctionsModal StrictEqInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::StrictlyEqual<true>),
    FunctionInfo<CompareParFn>(jit::StrictlyEqualPar));
static const VMFunctionsModal StrictNeInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::StrictlyEqual<false>),
    FunctionInfo<CompareParFn>(jit::StrictlyUnequalPar));
static const VMFunctionsModal LtInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::LessThan),
    FunctionInfo<CompareParFn>(jit::LessThanPar));
static const VMFunctionsModal LeInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::LessThanOrEqual),
    FunctionInfo<CompareParFn>(jit::LessThanOrEqualPar));
static const VMFunctionsModal GtInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::GreaterThan),
    FunctionInfo<CompareParFn>(jit::GreaterThanPar));
static const VMFunctionsModal GeInfo = VMFunctionsModal(
    FunctionInfo<CompareFn>(jit::GreaterThanOrEqual),
    FunctionInfo<CompareParFn>(jit::GreaterThanOrEqualPar));

bool
CodeGenerator::visitCompareVM(LCompareVM *lir)
{
    pushArg(ToValue(lir, LBinaryV::RhsInput));
    pushArg(ToValue(lir, LBinaryV::LhsInput));

    switch (lir->mir()->jsop()) {
      case JSOP_EQ:
        return callVM(EqInfo, lir);

      case JSOP_NE:
        return callVM(NeInfo, lir);

      case JSOP_STRICTEQ:
        return callVM(StrictEqInfo, lir);

      case JSOP_STRICTNE:
        return callVM(StrictNeInfo, lir);

      case JSOP_LT:
        return callVM(LtInfo, lir);

      case JSOP_LE:
        return callVM(LeInfo, lir);

      case JSOP_GT:
        return callVM(GtInfo, lir);

      case JSOP_GE:
        return callVM(GeInfo, lir);

      default:
        MOZ_ASSUME_UNREACHABLE("Unexpected compare op");
    }
}

bool
CodeGenerator::visitIsNullOrLikeUndefined(LIsNullOrLikeUndefined *lir)
{
    JSOp op = lir->mir()->jsop();
    MCompare::CompareType compareType = lir->mir()->compareType();
    JS_ASSERT(compareType == MCompare::Compare_Undefined ||
              compareType == MCompare::Compare_Null);

    const ValueOperand value = ToValue(lir, LIsNullOrLikeUndefined::Value);
    Register output = ToRegister(lir->output());

    if (op == JSOP_EQ || op == JSOP_NE) {
        MOZ_ASSERT(lir->mir()->lhs()->type() != MIRType_Object ||
                   lir->mir()->operandMightEmulateUndefined(),
                   "Operands which can't emulate undefined should have been folded");

        OutOfLineTestObjectWithLabels *ool = nullptr;
        Maybe<Label> label1, label2;
        Label *nullOrLikeUndefined;
        Label *notNullOrLikeUndefined;
        if (lir->mir()->operandMightEmulateUndefined()) {
            ool = new OutOfLineTestObjectWithLabels();
            if (!addOutOfLineCode(ool))
                return false;
            nullOrLikeUndefined = ool->label1();
            notNullOrLikeUndefined = ool->label2();
        } else {
            label1.construct();
            label2.construct();
            nullOrLikeUndefined = label1.addr();
            notNullOrLikeUndefined = label2.addr();
        }

        Register tag = masm.splitTagForTest(value);

        masm.branchTestNull(Assembler::Equal, tag, nullOrLikeUndefined);
        masm.branchTestUndefined(Assembler::Equal, tag, nullOrLikeUndefined);

        if (ool) {
            
            
            masm.branchTestObject(Assembler::NotEqual, tag, notNullOrLikeUndefined);

            Register objreg = masm.extractObject(value, ToTempUnboxRegister(lir->tempToUnbox()));
            testObjectTruthy(objreg, notNullOrLikeUndefined, nullOrLikeUndefined,
                             ToRegister(lir->temp()), ool);
        }

        Label done;

        
        
        masm.bind(notNullOrLikeUndefined);
        masm.move32(Imm32(op == JSOP_NE), output);
        masm.jump(&done);

        masm.bind(nullOrLikeUndefined);
        masm.move32(Imm32(op == JSOP_EQ), output);

        
        masm.bind(&done);
        return true;
    }

    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    Assembler::Condition cond = JSOpToCondition(compareType, op);
    if (compareType == MCompare::Compare_Null)
        cond = masm.testNull(cond, value);
    else
        cond = masm.testUndefined(cond, value);

    masm.emitSet(cond, output);
    return true;
}

bool
CodeGenerator::visitIsNullOrLikeUndefinedAndBranch(LIsNullOrLikeUndefinedAndBranch *lir)
{
    JSOp op = lir->mir()->jsop();
    MCompare::CompareType compareType = lir->mir()->compareType();
    JS_ASSERT(compareType == MCompare::Compare_Undefined ||
              compareType == MCompare::Compare_Null);

    const ValueOperand value = ToValue(lir, LIsNullOrLikeUndefinedAndBranch::Value);

    if (op == JSOP_EQ || op == JSOP_NE) {
        MBasicBlock *ifTrue;
        MBasicBlock *ifFalse;

        if (op == JSOP_EQ) {
            ifTrue = lir->ifTrue();
            ifFalse = lir->ifFalse();
        } else {
            
            ifTrue = lir->ifFalse();
            ifFalse = lir->ifTrue();
            op = JSOP_EQ;
        }

        MOZ_ASSERT(lir->mir()->lhs()->type() != MIRType_Object ||
                   lir->mir()->operandMightEmulateUndefined(),
                   "Operands which can't emulate undefined should have been folded");

        OutOfLineTestObject *ool = nullptr;
        if (lir->mir()->operandMightEmulateUndefined()) {
            ool = new OutOfLineTestObject();
            if (!addOutOfLineCode(ool))
                return false;
        }

        Register tag = masm.splitTagForTest(value);

        Label *ifTrueLabel = getJumpLabelForBranch(ifTrue);
        Label *ifFalseLabel = getJumpLabelForBranch(ifFalse);

        masm.branchTestNull(Assembler::Equal, tag, ifTrueLabel);
        masm.branchTestUndefined(Assembler::Equal, tag, ifTrueLabel);

        if (ool) {
            masm.branchTestObject(Assembler::NotEqual, tag, ifFalseLabel);

            
            Register objreg = masm.extractObject(value, ToTempUnboxRegister(lir->tempToUnbox()));
            testObjectTruthy(objreg, ifFalseLabel, ifTrueLabel, ToRegister(lir->temp()), ool);
        } else {
            masm.jump(ifFalseLabel);
        }
        return true;
    }

    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    Assembler::Condition cond = JSOpToCondition(compareType, op);
    if (compareType == MCompare::Compare_Null)
        cond = masm.testNull(cond, value);
    else
        cond = masm.testUndefined(cond, value);

    emitBranch(cond, lir->ifTrue(), lir->ifFalse());
    return true;
}

bool
CodeGenerator::visitEmulatesUndefined(LEmulatesUndefined *lir)
{
    MOZ_ASSERT(lir->mir()->compareType() == MCompare::Compare_Undefined ||
               lir->mir()->compareType() == MCompare::Compare_Null);
    MOZ_ASSERT(lir->mir()->lhs()->type() == MIRType_Object);
    MOZ_ASSERT(lir->mir()->operandMightEmulateUndefined(),
               "If the object couldn't emulate undefined, this should have been folded.");

    JSOp op = lir->mir()->jsop();
    MOZ_ASSERT(op == JSOP_EQ || op == JSOP_NE, "Strict equality should have been folded");

    OutOfLineTestObjectWithLabels *ool = new OutOfLineTestObjectWithLabels();
    if (!addOutOfLineCode(ool))
        return false;

    Label *emulatesUndefined = ool->label1();
    Label *doesntEmulateUndefined = ool->label2();

    Register objreg = ToRegister(lir->input());
    Register output = ToRegister(lir->output());
    testObjectTruthy(objreg, doesntEmulateUndefined, emulatesUndefined, output, ool);

    Label done;

    masm.bind(doesntEmulateUndefined);
    masm.move32(Imm32(op == JSOP_NE), output);
    masm.jump(&done);

    masm.bind(emulatesUndefined);
    masm.move32(Imm32(op == JSOP_EQ), output);
    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitEmulatesUndefinedAndBranch(LEmulatesUndefinedAndBranch *lir)
{
    MOZ_ASSERT(lir->mir()->compareType() == MCompare::Compare_Undefined ||
               lir->mir()->compareType() == MCompare::Compare_Null);
    MOZ_ASSERT(lir->mir()->operandMightEmulateUndefined(),
               "Operands which can't emulate undefined should have been folded");

    JSOp op = lir->mir()->jsop();
    MOZ_ASSERT(op == JSOP_EQ || op == JSOP_NE, "Strict equality should have been folded");

    OutOfLineTestObject *ool = new OutOfLineTestObject();
    if (!addOutOfLineCode(ool))
        return false;

    Label *equal;
    Label *unequal;

    {
        MBasicBlock *ifTrue;
        MBasicBlock *ifFalse;

        if (op == JSOP_EQ) {
            ifTrue = lir->ifTrue();
            ifFalse = lir->ifFalse();
        } else {
            
            ifTrue = lir->ifFalse();
            ifFalse = lir->ifTrue();
            op = JSOP_EQ;
        }

        equal = getJumpLabelForBranch(ifTrue);
        unequal = getJumpLabelForBranch(ifFalse);
    }

    Register objreg = ToRegister(lir->input());

    testObjectTruthy(objreg, unequal, equal, ToRegister(lir->temp()), ool);
    return true;
}

typedef JSString *(*ConcatStringsFn)(ThreadSafeContext *, HandleString, HandleString);
typedef ParallelResult (*ConcatStringsParFn)(ForkJoinSlice *, HandleString, HandleString,
                                             MutableHandleString);
static const VMFunctionsModal ConcatStringsInfo = VMFunctionsModal(
    FunctionInfo<ConcatStringsFn>(ConcatStrings<CanGC>),
    FunctionInfo<ConcatStringsParFn>(ConcatStringsPar));

bool
CodeGenerator::emitConcat(LInstruction *lir, Register lhs, Register rhs, Register output)
{
    OutOfLineCode *ool = oolCallVM(ConcatStringsInfo, lir, (ArgList(), lhs, rhs),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    ExecutionMode mode = gen->info().executionMode();
    IonCode *stringConcatStub = gen->ionCompartment()->stringConcatStub(mode);
    masm.call(stringConcatStub);
    masm.branchTestPtr(Assembler::Zero, output, output, ool->entry());

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitConcat(LConcat *lir)
{
    Register lhs = ToRegister(lir->lhs());
    Register rhs = ToRegister(lir->rhs());

    Register output = ToRegister(lir->output());

    JS_ASSERT(lhs == CallTempReg0);
    JS_ASSERT(rhs == CallTempReg1);
    JS_ASSERT(ToRegister(lir->temp1()) == CallTempReg2);
    JS_ASSERT(ToRegister(lir->temp2()) == CallTempReg3);
    JS_ASSERT(ToRegister(lir->temp3()) == CallTempReg4);
    JS_ASSERT(ToRegister(lir->temp4()) == CallTempReg5);
    JS_ASSERT(output == CallTempReg6);

    return emitConcat(lir, lhs, rhs, output);
}

bool
CodeGenerator::visitConcatPar(LConcatPar *lir)
{
    DebugOnly<Register> slice = ToRegister(lir->forkJoinSlice());
    Register lhs = ToRegister(lir->lhs());
    Register rhs = ToRegister(lir->rhs());
    Register output = ToRegister(lir->output());

    JS_ASSERT(lhs == CallTempReg0);
    JS_ASSERT(rhs == CallTempReg1);
    JS_ASSERT((Register)slice == CallTempReg5);
    JS_ASSERT(ToRegister(lir->temp1()) == CallTempReg2);
    JS_ASSERT(ToRegister(lir->temp2()) == CallTempReg3);
    JS_ASSERT(ToRegister(lir->temp3()) == CallTempReg4);
    JS_ASSERT(output == CallTempReg6);

    return emitConcat(lir, lhs, rhs, output);
}

static void
CopyStringChars(MacroAssembler &masm, Register to, Register from, Register len, Register scratch)
{
    
    

#ifdef DEBUG
    Label ok;
    masm.branch32(Assembler::GreaterThan, len, Imm32(0), &ok);
    masm.breakpoint();
    masm.bind(&ok);
#endif

    JS_STATIC_ASSERT(sizeof(jschar) == 2);

    Label start;
    masm.bind(&start);
    masm.load16ZeroExtend(Address(from, 0), scratch);
    masm.store16(scratch, Address(to, 0));
    masm.addPtr(Imm32(2), from);
    masm.addPtr(Imm32(2), to);
    masm.sub32(Imm32(1), len);
    masm.j(Assembler::NonZero, &start);
}

IonCode *
IonCompartment::generateStringConcatStub(JSContext *cx, ExecutionMode mode)
{
    MacroAssembler masm(cx);

    Register lhs = CallTempReg0;
    Register rhs = CallTempReg1;
    Register temp1 = CallTempReg2;
    Register temp2 = CallTempReg3;
    Register temp3 = CallTempReg4;
    Register temp4 = CallTempReg5;
    Register output = CallTempReg6;

    
    
    
    Register forkJoinSlice = CallTempReg5;

    Label failure, failurePopTemps;

    
    Label leftEmpty;
    masm.loadStringLength(lhs, temp1);
    masm.branchTest32(Assembler::Zero, temp1, temp1, &leftEmpty);

    
    Label rightEmpty;
    masm.loadStringLength(rhs, temp2);
    masm.branchTest32(Assembler::Zero, temp2, temp2, &rightEmpty);

    masm.add32(temp1, temp2);

    
    Label isShort;
    masm.branch32(Assembler::BelowOrEqual, temp2, Imm32(JSShortString::MAX_SHORT_LENGTH),
                  &isShort);

    
    masm.branch32(Assembler::Above, temp2, Imm32(JSString::MAX_LENGTH), &failure);

    
    switch (mode) {
      case SequentialExecution:
        masm.newGCString(output, &failure);
        break;
      case ParallelExecution:
        masm.push(temp1);
        masm.push(temp2);
        masm.newGCStringPar(output, forkJoinSlice, temp1, temp2, &failurePopTemps);
        masm.pop(temp2);
        masm.pop(temp1);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }

    
    JS_STATIC_ASSERT(JSString::ROPE_FLAGS == 0);
    masm.lshiftPtr(Imm32(JSString::LENGTH_SHIFT), temp2);
    masm.storePtr(temp2, Address(output, JSString::offsetOfLengthAndFlags()));

    
    masm.storePtr(lhs, Address(output, JSRope::offsetOfLeft()));
    masm.storePtr(rhs, Address(output, JSRope::offsetOfRight()));
    masm.ret();

    masm.bind(&leftEmpty);
    masm.mov(rhs, output);
    masm.ret();

    masm.bind(&rightEmpty);
    masm.mov(lhs, output);
    masm.ret();

    masm.bind(&isShort);

    

    
    JS_STATIC_ASSERT(JSString::ROPE_FLAGS == 0);
    masm.branchTestPtr(Assembler::Zero, Address(lhs, JSString::offsetOfLengthAndFlags()),
                       Imm32(JSString::FLAGS_MASK), &failure);
    masm.branchTestPtr(Assembler::Zero, Address(rhs, JSString::offsetOfLengthAndFlags()),
                       Imm32(JSString::FLAGS_MASK), &failure);

    
    switch (mode) {
      case SequentialExecution:
        masm.newGCShortString(output, &failure);
        break;
      case ParallelExecution:
        masm.push(temp1);
        masm.push(temp2);
        masm.newGCShortStringPar(output, forkJoinSlice, temp1, temp2, &failurePopTemps);
        masm.pop(temp2);
        masm.pop(temp1);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }

    
    masm.lshiftPtr(Imm32(JSString::LENGTH_SHIFT), temp2);
    masm.orPtr(Imm32(JSString::FIXED_FLAGS), temp2);
    masm.storePtr(temp2, Address(output, JSString::offsetOfLengthAndFlags()));

    
    masm.computeEffectiveAddress(Address(output, JSShortString::offsetOfInlineStorage()), temp2);
    masm.storePtr(temp2, Address(output, JSShortString::offsetOfChars()));

    
    
    masm.loadPtr(Address(lhs, JSString::offsetOfChars()), temp3);
    CopyStringChars(masm, temp2, temp3, temp1, temp4);

    
    masm.loadPtr(Address(rhs, JSString::offsetOfChars()), temp3);
    masm.loadStringLength(rhs, temp1);
    CopyStringChars(masm, temp2, temp3, temp1, temp4);

    
    masm.store16(Imm32(0), Address(temp2, 0));
    masm.ret();

    masm.bind(&failurePopTemps);
    masm.pop(temp2);
    masm.pop(temp1);

    masm.bind(&failure);
    masm.movePtr(ImmPtr(nullptr), output);
    masm.ret();

    Linker linker(masm);
    IonCode *code = linker.newCode(cx, JSC::OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerIonCodeProfile(code, "StringConcatStub");
#endif

    return code;
}

typedef bool (*CharCodeAtFn)(JSContext *, HandleString, int32_t, uint32_t *);
static const VMFunction CharCodeAtInfo = FunctionInfo<CharCodeAtFn>(jit::CharCodeAt);

bool
CodeGenerator::visitCharCodeAt(LCharCodeAt *lir)
{
    Register str = ToRegister(lir->str());
    Register index = ToRegister(lir->index());
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(CharCodeAtInfo, lir, (ArgList(), str, index), StoreRegisterTo(output));
    if (!ool)
        return false;

    Address lengthAndFlagsAddr(str, JSString::offsetOfLengthAndFlags());
    masm.loadPtr(lengthAndFlagsAddr, output);

    masm.branchTest32(Assembler::Zero, output, Imm32(JSString::FLAGS_MASK), ool->entry());

    
    Address charsAddr(str, JSString::offsetOfChars());
    masm.loadPtr(charsAddr, output);
    masm.load16ZeroExtend(BaseIndex(output, index, TimesTwo, 0), output);

    masm.bind(ool->rejoin());
    return true;
}

typedef JSFlatString *(*StringFromCharCodeFn)(JSContext *, int32_t);
static const VMFunction StringFromCharCodeInfo = FunctionInfo<StringFromCharCodeFn>(jit::StringFromCharCode);

bool
CodeGenerator::visitFromCharCode(LFromCharCode *lir)
{
    Register code = ToRegister(lir->code());
    Register output = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(StringFromCharCodeInfo, lir, (ArgList(), code), StoreRegisterTo(output));
    if (!ool)
        return false;

    
    masm.branch32(Assembler::AboveOrEqual, code, Imm32(StaticStrings::UNIT_STATIC_LIMIT),
                  ool->entry());

    masm.movePtr(ImmPtr(&GetIonContext()->runtime->staticStrings.unitStaticTable), output);
    masm.loadPtr(BaseIndex(output, code, ScalePointer), output);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitInitializedLength(LInitializedLength *lir)
{
    Address initLength(ToRegister(lir->elements()), ObjectElements::offsetOfInitializedLength());
    masm.load32(initLength, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitSetInitializedLength(LSetInitializedLength *lir)
{
    Address initLength(ToRegister(lir->elements()), ObjectElements::offsetOfInitializedLength());
    Int32Key index = ToInt32Key(lir->index());

    masm.bumpKey(&index, 1);
    masm.storeKey(index, initLength);
    
    masm.bumpKey(&index, -1);
    return true;
}

bool
CodeGenerator::visitNotO(LNotO *lir)
{
    MOZ_ASSERT(lir->mir()->operandMightEmulateUndefined(),
               "This should be constant-folded if the object can't emulate undefined.");

    OutOfLineTestObjectWithLabels *ool = new OutOfLineTestObjectWithLabels();
    if (!addOutOfLineCode(ool))
        return false;

    Label *ifTruthy = ool->label1();
    Label *ifFalsy = ool->label2();

    Register objreg = ToRegister(lir->input());
    Register output = ToRegister(lir->output());
    testObjectTruthy(objreg, ifTruthy, ifFalsy, output, ool);

    Label join;

    masm.bind(ifTruthy);
    masm.move32(Imm32(0), output);
    masm.jump(&join);

    masm.bind(ifFalsy);
    masm.move32(Imm32(1), output);

    masm.bind(&join);
    return true;
}

bool
CodeGenerator::visitNotV(LNotV *lir)
{
    Maybe<Label> ifTruthyLabel, ifFalsyLabel;
    Label *ifTruthy;
    Label *ifFalsy;

    OutOfLineTestObjectWithLabels *ool = nullptr;
    if (lir->mir()->operandMightEmulateUndefined()) {
        ool = new OutOfLineTestObjectWithLabels();
        if (!addOutOfLineCode(ool))
            return false;
        ifTruthy = ool->label1();
        ifFalsy = ool->label2();
    } else {
        ifTruthyLabel.construct();
        ifFalsyLabel.construct();
        ifTruthy = ifTruthyLabel.addr();
        ifFalsy = ifFalsyLabel.addr();
    }

    testValueTruthy(ToValue(lir, LNotV::Input), lir->temp1(), lir->temp2(),
                    ToFloatRegister(lir->tempFloat()),
                    ifTruthy, ifFalsy, ool);

    Label join;
    Register output = ToRegister(lir->output());

    masm.bind(ifFalsy);
    masm.move32(Imm32(1), output);
    masm.jump(&join);

    masm.bind(ifTruthy);
    masm.move32(Imm32(0), output);

    
    masm.bind(&join);
    return true;
}

bool
CodeGenerator::visitBoundsCheck(LBoundsCheck *lir)
{
    if (lir->index()->isConstant()) {
        
        uint32_t index = ToInt32(lir->index());
        if (lir->length()->isConstant()) {
            uint32_t length = ToInt32(lir->length());
            if (index < length)
                return true;
            return bailout(lir->snapshot());
        }
        masm.cmp32(ToOperand(lir->length()), Imm32(index));
        return bailoutIf(Assembler::BelowOrEqual, lir->snapshot());
    }
    if (lir->length()->isConstant()) {
        masm.cmp32(ToRegister(lir->index()), Imm32(ToInt32(lir->length())));
        return bailoutIf(Assembler::AboveOrEqual, lir->snapshot());
    }
    masm.cmp32(ToOperand(lir->length()), ToRegister(lir->index()));
    return bailoutIf(Assembler::BelowOrEqual, lir->snapshot());
}

bool
CodeGenerator::visitBoundsCheckRange(LBoundsCheckRange *lir)
{
    int32_t min = lir->mir()->minimum();
    int32_t max = lir->mir()->maximum();
    JS_ASSERT(max >= min);

    Register temp = ToRegister(lir->getTemp(0));
    if (lir->index()->isConstant()) {
        int32_t nmin, nmax;
        int32_t index = ToInt32(lir->index());
        if (SafeAdd(index, min, &nmin) && SafeAdd(index, max, &nmax) && nmin >= 0) {
            masm.cmp32(ToOperand(lir->length()), Imm32(nmax));
            return bailoutIf(Assembler::BelowOrEqual, lir->snapshot());
        }
        masm.mov(ImmWord(index), temp);
    } else {
        masm.mov(ToRegister(lir->index()), temp);
    }

    
    
    
    if (min != max) {
        if (min != 0) {
            masm.add32(Imm32(min), temp);
            if (!bailoutIf(Assembler::Overflow, lir->snapshot()))
                return false;
            int32_t diff;
            if (SafeSub(max, min, &diff))
                max = diff;
            else
                masm.sub32(Imm32(min), temp);
        }

        masm.cmp32(temp, Imm32(0));
        if (!bailoutIf(Assembler::LessThan, lir->snapshot()))
            return false;
    }

    
    
    
    
    
    if (max != 0) {
        masm.add32(Imm32(max), temp);
        if (max < 0 && !bailoutIf(Assembler::Overflow, lir->snapshot()))
            return false;
    }

    masm.cmp32(ToOperand(lir->length()), temp);
    return bailoutIf(Assembler::BelowOrEqual, lir->snapshot());
}

bool
CodeGenerator::visitBoundsCheckLower(LBoundsCheckLower *lir)
{
    int32_t min = lir->mir()->minimum();
    masm.cmp32(ToRegister(lir->index()), Imm32(min));
    return bailoutIf(Assembler::LessThan, lir->snapshot());
}

class OutOfLineStoreElementHole : public OutOfLineCodeBase<CodeGenerator>
{
    LInstruction *ins_;
    Label rejoinStore_;

  public:
    OutOfLineStoreElementHole(LInstruction *ins)
      : ins_(ins)
    {
        JS_ASSERT(ins->isStoreElementHoleV() || ins->isStoreElementHoleT());
    }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineStoreElementHole(this);
    }
    LInstruction *ins() const {
        return ins_;
    }
    Label *rejoinStore() {
        return &rejoinStore_;
    }
};

bool
CodeGenerator::emitStoreHoleCheck(Register elements, const LAllocation *index, LSnapshot *snapshot)
{
    Assembler::Condition cond;
    if (index->isConstant())
        cond = masm.testMagic(Assembler::Equal, Address(elements, ToInt32(index) * sizeof(js::Value)));
    else
        cond = masm.testMagic(Assembler::Equal, BaseIndex(elements, ToRegister(index), TimesEight));
    return bailoutIf(cond, snapshot);
}

bool
CodeGenerator::visitStoreElementT(LStoreElementT *store)
{
    Register elements = ToRegister(store->elements());
    const LAllocation *index = store->index();

    if (store->mir()->needsBarrier())
       emitPreBarrier(elements, index, store->mir()->elementType());

    if (store->mir()->needsHoleCheck() && !emitStoreHoleCheck(elements, index, store->snapshot()))
        return false;

    storeElementTyped(store->value(), store->mir()->value()->type(), store->mir()->elementType(),
                      elements, index);
    return true;
}

bool
CodeGenerator::visitStoreElementV(LStoreElementV *lir)
{
    const ValueOperand value = ToValue(lir, LStoreElementV::Value);
    Register elements = ToRegister(lir->elements());
    const LAllocation *index = lir->index();

    if (lir->mir()->needsBarrier())
        emitPreBarrier(elements, index, MIRType_Value);

    if (lir->mir()->needsHoleCheck() && !emitStoreHoleCheck(elements, index, lir->snapshot()))
        return false;

    if (lir->index()->isConstant())
        masm.storeValue(value, Address(elements, ToInt32(lir->index()) * sizeof(js::Value)));
    else
        masm.storeValue(value, BaseIndex(elements, ToRegister(lir->index()), TimesEight));
    return true;
}

bool
CodeGenerator::visitStoreElementHoleT(LStoreElementHoleT *lir)
{
    OutOfLineStoreElementHole *ool = new OutOfLineStoreElementHole(lir);
    if (!addOutOfLineCode(ool))
        return false;

    Register elements = ToRegister(lir->elements());
    const LAllocation *index = lir->index();

    
    Address initLength(elements, ObjectElements::offsetOfInitializedLength());
    masm.branchKey(Assembler::BelowOrEqual, initLength, ToInt32Key(index), ool->entry());

    if (lir->mir()->needsBarrier())
        emitPreBarrier(elements, index, lir->mir()->elementType());

    masm.bind(ool->rejoinStore());
    storeElementTyped(lir->value(), lir->mir()->value()->type(), lir->mir()->elementType(),
                      elements, index);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitStoreElementHoleV(LStoreElementHoleV *lir)
{
    OutOfLineStoreElementHole *ool = new OutOfLineStoreElementHole(lir);
    if (!addOutOfLineCode(ool))
        return false;

    Register elements = ToRegister(lir->elements());
    const LAllocation *index = lir->index();
    const ValueOperand value = ToValue(lir, LStoreElementHoleV::Value);

    
    Address initLength(elements, ObjectElements::offsetOfInitializedLength());
    masm.branchKey(Assembler::BelowOrEqual, initLength, ToInt32Key(index), ool->entry());

    if (lir->mir()->needsBarrier())
        emitPreBarrier(elements, index, lir->mir()->elementType());

    masm.bind(ool->rejoinStore());
    if (lir->index()->isConstant())
        masm.storeValue(value, Address(elements, ToInt32(lir->index()) * sizeof(js::Value)));
    else
        masm.storeValue(value, BaseIndex(elements, ToRegister(lir->index()), TimesEight));

    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*SetObjectElementFn)(JSContext *, HandleObject, HandleValue, HandleValue,
                                   bool strict);
static const VMFunction SetObjectElementInfo = FunctionInfo<SetObjectElementFn>(SetObjectElement);

bool
CodeGenerator::visitOutOfLineStoreElementHole(OutOfLineStoreElementHole *ool)
{
    Register object, elements;
    LInstruction *ins = ool->ins();
    const LAllocation *index;
    MIRType valueType;
    ConstantOrRegister value;

    if (ins->isStoreElementHoleV()) {
        LStoreElementHoleV *store = ins->toStoreElementHoleV();
        object = ToRegister(store->object());
        elements = ToRegister(store->elements());
        index = store->index();
        valueType = store->mir()->value()->type();
        value = TypedOrValueRegister(ToValue(store, LStoreElementHoleV::Value));
    } else {
        LStoreElementHoleT *store = ins->toStoreElementHoleT();
        object = ToRegister(store->object());
        elements = ToRegister(store->elements());
        index = store->index();
        valueType = store->mir()->value()->type();
        if (store->value()->isConstant())
            value = ConstantOrRegister(*store->value()->toConstant());
        else
            value = TypedOrValueRegister(valueType, ToAnyRegister(store->value()));
    }

    
    
    
    Label callStub;
    masm.j(Assembler::NotEqual, &callStub);

    Int32Key key = ToInt32Key(index);

    
    masm.branchKey(Assembler::BelowOrEqual, Address(elements, ObjectElements::offsetOfCapacity()),
                   key, &callStub);

    
    
    masm.bumpKey(&key, 1);
    masm.storeKey(key, Address(elements, ObjectElements::offsetOfInitializedLength()));

    
    Label dontUpdate;
    masm.branchKey(Assembler::AboveOrEqual, Address(elements, ObjectElements::offsetOfLength()),
                   key, &dontUpdate);
    masm.storeKey(key, Address(elements, ObjectElements::offsetOfLength()));
    masm.bind(&dontUpdate);

    masm.bumpKey(&key, -1);

    if (ins->isStoreElementHoleT() && valueType != MIRType_Double) {
        
        
        
        storeElementTyped(ins->toStoreElementHoleT()->value(), valueType, MIRType_None, elements,
                          index);
        masm.jump(ool->rejoin());
    } else {
        
        masm.jump(ool->rejoinStore());
    }

    masm.bind(&callStub);
    saveLive(ins);

    pushArg(Imm32(current->mir()->strict()));
    pushArg(value);
    if (index->isConstant())
        pushArg(*index->toConstant());
    else
        pushArg(TypedOrValueRegister(MIRType_Int32, ToAnyRegister(index)));
    pushArg(object);
    
    if (!callVM(SetObjectElementInfo, ins))
        return false;

    restoreLive(ins);
    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*ArrayPopShiftFn)(JSContext *, HandleObject, MutableHandleValue);
static const VMFunction ArrayPopDenseInfo = FunctionInfo<ArrayPopShiftFn>(jit::ArrayPopDense);
static const VMFunction ArrayShiftDenseInfo = FunctionInfo<ArrayPopShiftFn>(jit::ArrayShiftDense);

bool
CodeGenerator::emitArrayPopShift(LInstruction *lir, const MArrayPopShift *mir, Register obj,
                                 Register elementsTemp, Register lengthTemp, TypedOrValueRegister out)
{
    OutOfLineCode *ool;

    if (mir->mode() == MArrayPopShift::Pop) {
        ool = oolCallVM(ArrayPopDenseInfo, lir, (ArgList(), obj), StoreValueTo(out));
        if (!ool)
            return false;
    } else {
        JS_ASSERT(mir->mode() == MArrayPopShift::Shift);
        ool = oolCallVM(ArrayShiftDenseInfo, lir, (ArgList(), obj), StoreValueTo(out));
        if (!ool)
            return false;
    }

    
    masm.branchTestNeedsBarrier(Assembler::NonZero, lengthTemp, ool->entry());

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), elementsTemp);
    masm.load32(Address(elementsTemp, ObjectElements::offsetOfLength()), lengthTemp);

    
    Int32Key key = Int32Key(lengthTemp);
    Address initLength(elementsTemp, ObjectElements::offsetOfInitializedLength());
    masm.branchKey(Assembler::NotEqual, initLength, key, ool->entry());

    
    
    
    Label done;
    if (mir->maybeUndefined()) {
        Label notEmpty;
        masm.branchTest32(Assembler::NonZero, lengthTemp, lengthTemp, &notEmpty);
        masm.moveValue(UndefinedValue(), out.valueReg());
        masm.jump(&done);
        masm.bind(&notEmpty);
    } else {
        masm.branchTest32(Assembler::Zero, lengthTemp, lengthTemp, ool->entry());
    }

    masm.bumpKey(&key, -1);

    if (mir->mode() == MArrayPopShift::Pop) {
        masm.loadElementTypedOrValue(BaseIndex(elementsTemp, lengthTemp, TimesEight), out,
                                     mir->needsHoleCheck(), ool->entry());
    } else {
        JS_ASSERT(mir->mode() == MArrayPopShift::Shift);
        masm.loadElementTypedOrValue(Address(elementsTemp, 0), out, mir->needsHoleCheck(),
                                     ool->entry());
    }

    
    
    
    
    Address elementFlags(elementsTemp, ObjectElements::offsetOfFlags());
    Imm32 bit(ObjectElements::NONWRITABLE_ARRAY_LENGTH);
    masm.branchTest32(Assembler::NonZero, elementFlags, bit, ool->entry());

    
    masm.store32(lengthTemp, Address(elementsTemp, ObjectElements::offsetOfLength()));
    masm.store32(lengthTemp, Address(elementsTemp, ObjectElements::offsetOfInitializedLength()));

    if (mir->mode() == MArrayPopShift::Shift) {
        
        RegisterSet temps;
        temps.add(elementsTemp);
        temps.add(lengthTemp);

        saveVolatile(temps);
        masm.setupUnalignedABICall(1, lengthTemp);
        masm.passABIArg(obj);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::ArrayShiftMoveElements));
        restoreVolatile(temps);
    }

    masm.bind(&done);
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitArrayPopShiftV(LArrayPopShiftV *lir)
{
    Register obj = ToRegister(lir->object());
    Register elements = ToRegister(lir->temp0());
    Register length = ToRegister(lir->temp1());
    TypedOrValueRegister out(ToOutValue(lir));
    return emitArrayPopShift(lir, lir->mir(), obj, elements, length, out);
}

bool
CodeGenerator::visitArrayPopShiftT(LArrayPopShiftT *lir)
{
    Register obj = ToRegister(lir->object());
    Register elements = ToRegister(lir->temp0());
    Register length = ToRegister(lir->temp1());
    TypedOrValueRegister out(lir->mir()->type(), ToAnyRegister(lir->output()));
    return emitArrayPopShift(lir, lir->mir(), obj, elements, length, out);
}

typedef bool (*ArrayPushDenseFn)(JSContext *, HandleObject, HandleValue, uint32_t *);
static const VMFunction ArrayPushDenseInfo =
    FunctionInfo<ArrayPushDenseFn>(jit::ArrayPushDense);

bool
CodeGenerator::emitArrayPush(LInstruction *lir, const MArrayPush *mir, Register obj,
                             ConstantOrRegister value, Register elementsTemp, Register length)
{
    OutOfLineCode *ool = oolCallVM(ArrayPushDenseInfo, lir, (ArgList(), obj, value), StoreRegisterTo(length));
    if (!ool)
        return false;

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), elementsTemp);
    masm.load32(Address(elementsTemp, ObjectElements::offsetOfLength()), length);

    Int32Key key = Int32Key(length);
    Address initLength(elementsTemp, ObjectElements::offsetOfInitializedLength());
    Address capacity(elementsTemp, ObjectElements::offsetOfCapacity());

    
    masm.branchKey(Assembler::NotEqual, initLength, key, ool->entry());

    
    masm.branchKey(Assembler::BelowOrEqual, capacity, key, ool->entry());

    masm.storeConstantOrRegister(value, BaseIndex(elementsTemp, length, TimesEight));

    masm.bumpKey(&key, 1);
    masm.store32(length, Address(elementsTemp, ObjectElements::offsetOfLength()));
    masm.store32(length, Address(elementsTemp, ObjectElements::offsetOfInitializedLength()));

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitArrayPushV(LArrayPushV *lir)
{
    Register obj = ToRegister(lir->object());
    Register elementsTemp = ToRegister(lir->temp());
    Register length = ToRegister(lir->output());
    ConstantOrRegister value = TypedOrValueRegister(ToValue(lir, LArrayPushV::Value));
    return emitArrayPush(lir, lir->mir(), obj, value, elementsTemp, length);
}

bool
CodeGenerator::visitArrayPushT(LArrayPushT *lir)
{
    Register obj = ToRegister(lir->object());
    Register elementsTemp = ToRegister(lir->temp());
    Register length = ToRegister(lir->output());
    ConstantOrRegister value;
    if (lir->value()->isConstant())
        value = ConstantOrRegister(*lir->value()->toConstant());
    else
        value = TypedOrValueRegister(lir->mir()->value()->type(), ToAnyRegister(lir->value()));
    return emitArrayPush(lir, lir->mir(), obj, value, elementsTemp, length);
}

typedef JSObject *(*ArrayConcatDenseFn)(JSContext *, HandleObject, HandleObject, HandleObject);
static const VMFunction ArrayConcatDenseInfo = FunctionInfo<ArrayConcatDenseFn>(ArrayConcatDense);

bool
CodeGenerator::visitArrayConcat(LArrayConcat *lir)
{
    Register lhs = ToRegister(lir->lhs());
    Register rhs = ToRegister(lir->rhs());
    Register temp1 = ToRegister(lir->temp1());
    Register temp2 = ToRegister(lir->temp2());

    
    
    
    Label fail, call;
    masm.loadPtr(Address(lhs, JSObject::offsetOfElements()), temp1);
    masm.load32(Address(temp1, ObjectElements::offsetOfInitializedLength()), temp2);
    masm.branch32(Assembler::NotEqual, Address(temp1, ObjectElements::offsetOfLength()), temp2, &fail);

    masm.loadPtr(Address(rhs, JSObject::offsetOfElements()), temp1);
    masm.load32(Address(temp1, ObjectElements::offsetOfInitializedLength()), temp2);
    masm.branch32(Assembler::NotEqual, Address(temp1, ObjectElements::offsetOfLength()), temp2, &fail);

    
    JSObject *templateObj = lir->mir()->templateObj();
    masm.newGCThing(temp1, templateObj, &fail);
    masm.initGCThing(temp1, templateObj);
    masm.jump(&call);
    {
        masm.bind(&fail);
        masm.movePtr(ImmPtr(nullptr), temp1);
    }
    masm.bind(&call);

    pushArg(temp1);
    pushArg(ToRegister(lir->rhs()));
    pushArg(ToRegister(lir->lhs()));
    return callVM(ArrayConcatDenseInfo, lir);
}

typedef JSObject *(*GetIteratorObjectFn)(JSContext *, HandleObject, uint32_t);
static const VMFunction GetIteratorObjectInfo = FunctionInfo<GetIteratorObjectFn>(GetIteratorObject);

bool
CodeGenerator::visitCallIteratorStart(LCallIteratorStart *lir)
{
    pushArg(Imm32(lir->mir()->flags()));
    pushArg(ToRegister(lir->object()));
    return callVM(GetIteratorObjectInfo, lir);
}

bool
CodeGenerator::visitIteratorStart(LIteratorStart *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register output = ToRegister(lir->output());

    uint32_t flags = lir->mir()->flags();

    OutOfLineCode *ool = oolCallVM(GetIteratorObjectInfo, lir,
                                   (ArgList(), obj, Imm32(flags)), StoreRegisterTo(output));
    if (!ool)
        return false;

    const Register temp1 = ToRegister(lir->temp1());
    const Register temp2 = ToRegister(lir->temp2());
    const Register niTemp = ToRegister(lir->temp3()); 

    
    JS_ASSERT(flags == JSITER_ENUMERATE);

    
    masm.loadPtr(AbsoluteAddress(&GetIonContext()->runtime->nativeIterCache.last), output);
    masm.branchTestPtr(Assembler::Zero, output, output, ool->entry());

    
    masm.loadObjPrivate(output, JSObject::ITER_CLASS_NFIXED_SLOTS, niTemp);

    
    masm.branchTest32(Assembler::NonZero, Address(niTemp, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_ACTIVE|JSITER_UNREUSABLE), ool->entry());

    
    masm.loadPtr(Address(niTemp, offsetof(NativeIterator, shapes_array)), temp2);

    
    masm.loadObjShape(obj, temp1);
    masm.branchPtr(Assembler::NotEqual, Address(temp2, 0), temp1, ool->entry());

    
    masm.loadObjProto(obj, temp1);
    masm.loadObjShape(temp1, temp1);
    masm.branchPtr(Assembler::NotEqual, Address(temp2, sizeof(Shape *)), temp1, ool->entry());

    
    
    
    masm.loadObjProto(obj, temp1);
    masm.loadObjProto(temp1, temp1);
    masm.branchTestPtr(Assembler::NonZero, temp1, temp1, ool->entry());

    
    
    masm.branchPtr(Assembler::NotEqual,
                   Address(obj, JSObject::offsetOfElements()),
                   ImmPtr(js::emptyObjectElements),
                   ool->entry());

    
    
    {
#ifdef JSGC_GENERATIONAL
        
        
        
        Address objAddr(niTemp, offsetof(NativeIterator, obj));
        masm.branchPtr(Assembler::NotEqual, objAddr, obj, ool->entry());
#else
        Label noBarrier;
        masm.branchTestNeedsBarrier(Assembler::Zero, temp1, &noBarrier);

        Address objAddr(niTemp, offsetof(NativeIterator, obj));
        masm.branchPtr(Assembler::NotEqual, objAddr, obj, ool->entry());

        masm.bind(&noBarrier);
#endif 
    }

    
    masm.storePtr(obj, Address(niTemp, offsetof(NativeIterator, obj)));
    masm.or32(Imm32(JSITER_ACTIVE), Address(niTemp, offsetof(NativeIterator, flags)));

    
    masm.loadPtr(AbsoluteAddress(&gen->compartment->enumerators), temp1);

    
    masm.storePtr(temp1, Address(niTemp, NativeIterator::offsetOfNext()));

    
    masm.loadPtr(Address(temp1, NativeIterator::offsetOfPrev()), temp2);
    masm.storePtr(temp2, Address(niTemp, NativeIterator::offsetOfPrev()));

    
    masm.storePtr(niTemp, Address(temp2, NativeIterator::offsetOfNext()));

    
    masm.storePtr(niTemp, Address(temp1, NativeIterator::offsetOfPrev()));

    masm.bind(ool->rejoin());
    return true;
}

static void
LoadNativeIterator(MacroAssembler &masm, Register obj, Register dest, Label *failures)
{
    JS_ASSERT(obj != dest);

    
    masm.branchTestObjClass(Assembler::NotEqual, obj, dest, &PropertyIteratorObject::class_, failures);

    
    masm.loadObjPrivate(obj, JSObject::ITER_CLASS_NFIXED_SLOTS, dest);
}

typedef bool (*IteratorNextFn)(JSContext *, HandleObject, MutableHandleValue);
static const VMFunction IteratorNextInfo = FunctionInfo<IteratorNextFn>(js_IteratorNext);

bool
CodeGenerator::visitIteratorNext(LIteratorNext *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register temp = ToRegister(lir->temp());
    const ValueOperand output = ToOutValue(lir);

    OutOfLineCode *ool = oolCallVM(IteratorNextInfo, lir, (ArgList(), obj), StoreValueTo(output));
    if (!ool)
        return false;

    LoadNativeIterator(masm, obj, temp, ool->entry());

    masm.branchTest32(Assembler::NonZero, Address(temp, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_FOREACH), ool->entry());

    
    masm.loadPtr(Address(temp, offsetof(NativeIterator, props_cursor)), output.scratchReg());
    masm.loadPtr(Address(output.scratchReg(), 0), output.scratchReg());
    masm.tagValue(JSVAL_TYPE_STRING, output.scratchReg(), output);

    
    masm.addPtr(Imm32(sizeof(JSString *)), Address(temp, offsetof(NativeIterator, props_cursor)));

    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*IteratorMoreFn)(JSContext *, HandleObject, bool *);
static const VMFunction IteratorMoreInfo = FunctionInfo<IteratorMoreFn>(jit::IteratorMore);

bool
CodeGenerator::visitIteratorMore(LIteratorMore *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register output = ToRegister(lir->output());
    const Register temp = ToRegister(lir->temp());

    OutOfLineCode *ool = oolCallVM(IteratorMoreInfo, lir,
                                   (ArgList(), obj), StoreRegisterTo(output));
    if (!ool)
        return false;

    LoadNativeIterator(masm, obj, output, ool->entry());

    masm.branchTest32(Assembler::NonZero, Address(output, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_FOREACH), ool->entry());

    
    masm.loadPtr(Address(output, offsetof(NativeIterator, props_end)), temp);
    masm.cmpPtr(Address(output, offsetof(NativeIterator, props_cursor)), temp);
    masm.emitSet(Assembler::LessThan, output);

    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*CloseIteratorFn)(JSContext *, HandleObject);
static const VMFunction CloseIteratorInfo = FunctionInfo<CloseIteratorFn>(CloseIterator);

bool
CodeGenerator::visitIteratorEnd(LIteratorEnd *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register temp1 = ToRegister(lir->temp1());
    const Register temp2 = ToRegister(lir->temp2());
    const Register temp3 = ToRegister(lir->temp3());

    OutOfLineCode *ool = oolCallVM(CloseIteratorInfo, lir, (ArgList(), obj), StoreNothing());
    if (!ool)
        return false;

    LoadNativeIterator(masm, obj, temp1, ool->entry());

    masm.branchTest32(Assembler::Zero, Address(temp1, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_ENUMERATE), ool->entry());

    
    masm.and32(Imm32(~JSITER_ACTIVE), Address(temp1, offsetof(NativeIterator, flags)));

    
    masm.loadPtr(Address(temp1, offsetof(NativeIterator, props_array)), temp2);
    masm.storePtr(temp2, Address(temp1, offsetof(NativeIterator, props_cursor)));

    
    const Register next = temp2;
    const Register prev = temp3;
    masm.loadPtr(Address(temp1, NativeIterator::offsetOfNext()), next);
    masm.loadPtr(Address(temp1, NativeIterator::offsetOfPrev()), prev);
    masm.storePtr(prev, Address(next, NativeIterator::offsetOfPrev()));
    masm.storePtr(next, Address(prev, NativeIterator::offsetOfNext()));
#ifdef DEBUG
    masm.storePtr(ImmPtr(nullptr), Address(temp1, NativeIterator::offsetOfNext()));
    masm.storePtr(ImmPtr(nullptr), Address(temp1, NativeIterator::offsetOfPrev()));
#endif

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitArgumentsLength(LArgumentsLength *lir)
{
    
    Register argc = ToRegister(lir->output());
    Address ptr(StackPointer, frameSize() + IonJSFrameLayout::offsetOfNumActualArgs());

    masm.loadPtr(ptr, argc);
    return true;
}

bool
CodeGenerator::visitGetFrameArgument(LGetFrameArgument *lir)
{
    ValueOperand result = GetValueOutput(lir);
    const LAllocation *index = lir->index();
    size_t argvOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs();

    if (index->isConstant()) {
        int32_t i = index->toConstant()->toInt32();
        Address argPtr(StackPointer, sizeof(Value) * i + argvOffset);
        masm.loadValue(argPtr, result);
    } else {
        Register i = ToRegister(index);
        BaseIndex argPtr(StackPointer, i, ScaleFromElemWidth(sizeof(Value)), argvOffset);
        masm.loadValue(argPtr, result);
    }
    return true;
}

bool
CodeGenerator::visitSetFrameArgumentT(LSetFrameArgumentT *lir)
{
    size_t argOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs() +
                       (sizeof(Value) * lir->mir()->argno());

    MIRType type = lir->mir()->value()->type();

    if (type == MIRType_Double) {
        
        FloatRegister input = ToFloatRegister(lir->input());
        masm.storeDouble(input, Address(StackPointer, argOffset));

    } else {
        Register input = ToRegister(lir->input());
        masm.storeValue(ValueTypeFromMIRType(type), input, Address(StackPointer, argOffset));
    }
    return true;
}

bool
CodeGenerator:: visitSetFrameArgumentC(LSetFrameArgumentC *lir)
{
    size_t argOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs() +
                       (sizeof(Value) * lir->mir()->argno());
    masm.storeValue(lir->val(), Address(StackPointer, argOffset));
    return true;
}

bool
CodeGenerator:: visitSetFrameArgumentV(LSetFrameArgumentV *lir)
{
    const ValueOperand val = ToValue(lir, LSetFrameArgumentV::Input);
    size_t argOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs() +
                       (sizeof(Value) * lir->mir()->argno());
    masm.storeValue(val, Address(StackPointer, argOffset));
    return true;
}

typedef bool (*RunOnceScriptPrologueFn)(JSContext *, HandleScript);
static const VMFunction RunOnceScriptPrologueInfo =
    FunctionInfo<RunOnceScriptPrologueFn>(js::RunOnceScriptPrologue);

bool
CodeGenerator::visitRunOncePrologue(LRunOncePrologue *lir)
{
    pushArg(ImmGCPtr(lir->mir()->block()->info().script()));
    return callVM(RunOnceScriptPrologueInfo, lir);
}


typedef JSObject *(*InitRestParameterFn)(JSContext *, uint32_t, Value *, HandleObject,
                                         HandleObject);
typedef ParallelResult (*InitRestParameterParFn)(ForkJoinSlice *, uint32_t, Value *,
                                                 HandleObject, HandleObject,
                                                 MutableHandleObject);
static const VMFunctionsModal InitRestParameterInfo = VMFunctionsModal(
    FunctionInfo<InitRestParameterFn>(InitRestParameter),
    FunctionInfo<InitRestParameterParFn>(InitRestParameterPar));

bool
CodeGenerator::emitRest(LInstruction *lir, Register array, Register numActuals,
                        Register temp0, Register temp1, unsigned numFormals,
                        JSObject *templateObject)
{
    
    size_t actualsOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs();
    masm.movePtr(StackPointer, temp1);
    masm.addPtr(Imm32(sizeof(Value) * numFormals + actualsOffset), temp1);

    
    Label emptyLength, joinLength;
    masm.movePtr(numActuals, temp0);
    masm.branch32(Assembler::LessThanOrEqual, temp0, Imm32(numFormals), &emptyLength);
    masm.sub32(Imm32(numFormals), temp0);
    masm.jump(&joinLength);
    {
        masm.bind(&emptyLength);
        masm.move32(Imm32(0), temp0);
    }
    masm.bind(&joinLength);

    pushArg(array);
    pushArg(ImmGCPtr(templateObject));
    pushArg(temp1);
    pushArg(temp0);

    return callVM(InitRestParameterInfo, lir);
}

bool
CodeGenerator::visitRest(LRest *lir)
{
    Register numActuals = ToRegister(lir->numActuals());
    Register temp0 = ToRegister(lir->getTemp(0));
    Register temp1 = ToRegister(lir->getTemp(1));
    Register temp2 = ToRegister(lir->getTemp(2));
    unsigned numFormals = lir->mir()->numFormals();
    JSObject *templateObject = lir->mir()->templateObject();

    Label joinAlloc, failAlloc;
    masm.newGCThing(temp2, templateObject, &failAlloc);
    masm.initGCThing(temp2, templateObject);
    masm.jump(&joinAlloc);
    {
        masm.bind(&failAlloc);
        masm.movePtr(ImmPtr(nullptr), temp2);
    }
    masm.bind(&joinAlloc);

    return emitRest(lir, temp2, numActuals, temp0, temp1, numFormals, templateObject);
}

bool
CodeGenerator::visitRestPar(LRestPar *lir)
{
    Register numActuals = ToRegister(lir->numActuals());
    Register slice = ToRegister(lir->forkJoinSlice());
    Register temp0 = ToRegister(lir->getTemp(0));
    Register temp1 = ToRegister(lir->getTemp(1));
    Register temp2 = ToRegister(lir->getTemp(2));
    unsigned numFormals = lir->mir()->numFormals();
    JSObject *templateObject = lir->mir()->templateObject();

    if (!emitAllocateGCThingPar(lir, temp2, slice, temp0, temp1, templateObject))
        return false;

    return emitRest(lir, temp2, numActuals, temp0, temp1, numFormals, templateObject);
}

bool
CodeGenerator::generateAsmJS()
{
    IonSpew(IonSpew_Codegen, "# Emitting asm.js code");

    
    
    
    
    
    
    if (!generatePrologue())
        return false;
    if (!generateBody())
        return false;
    if (!generateEpilogue())
        return false;
#if defined(JS_ION_PERF)
    
    gen->perfSpewer().noteEndInlineCode(masm);
#endif
    if (!generateOutOfLineCode())
        return false;

    
    
    
    
    
    
    
    
    JS_ASSERT(snapshots_.size() == 0);
    JS_ASSERT(bailouts_.empty());
    JS_ASSERT(graph.numConstants() == 0);
    JS_ASSERT(safepointIndices_.empty());
    JS_ASSERT(osiIndices_.empty());
    JS_ASSERT(cacheList_.empty());
    JS_ASSERT(safepoints_.size() == 0);
    return true;
}

bool
CodeGenerator::generate()
{
    IonSpew(IonSpew_Codegen, "# Emitting code for script %s:%d",
            gen->info().script()->filename(),
            gen->info().script()->lineno);

    if (!safepoints_.init(graph.totalSlotCount()))
        return false;

#if JS_TRACE_LOGGING
    masm.tracelogStart(gen->info().script());
    masm.tracelogLog(TraceLogging::INFO_ENGINE_IONMONKEY);
#endif

    
    
    
    if (!generateArgumentsChecks())
        return false;

    if (frameClass_ != FrameSizeClass::None()) {
        deoptTable_ = gen->ionRuntime()->getBailoutTable(frameClass_);
        if (!deoptTable_)
            return false;
    }

#if JS_TRACE_LOGGING
    Label skip;
    masm.jump(&skip);
#endif

    
    masm.flushBuffer();
    setSkipArgCheckEntryOffset(masm.size());

#if JS_TRACE_LOGGING
    masm.tracelogStart(gen->info().script());
    masm.tracelogLog(TraceLogging::INFO_ENGINE_IONMONKEY);
    masm.bind(&skip);
#endif

    if (!generatePrologue())
        return false;
    if (!generateBody())
        return false;
    if (!generateEpilogue())
        return false;
    if (!generateInvalidateEpilogue())
        return false;
#if defined(JS_ION_PERF)
    
    perfSpewer_.noteEndInlineCode(masm);
#endif
    if (!generateOutOfLineCode())
        return false;

    return !masm.oom();
}

bool
CodeGenerator::link(JSContext *cx, types::CompilerConstraintList *constraints)
{
    JSScript *script = gen->info().script();
    ExecutionMode executionMode = gen->info().executionMode();
    JS_ASSERT(!HasIonScript(script, executionMode));

    
    
    types::RecompileInfo recompileInfo;
    if (!types::FinishCompilation(cx, script, executionMode, constraints, &recompileInfo))
        return true;

    uint32_t scriptFrameSize = frameClass_ == FrameSizeClass::None()
                           ? frameDepth_
                           : FrameSizeClass::FromDepth(frameDepth_).frameSize();

    
    encodeSafepoints();

    
    
    CallTargetVector callTargets;
    if (executionMode == ParallelExecution)
        AddPossibleCallees(cx, graph.mir(), callTargets);

    IonScript *ionScript =
      IonScript::New(cx, recompileInfo,
                     graph.totalSlotCount(), scriptFrameSize, snapshots_.size(),
                     bailouts_.length(), graph.numConstants(),
                     safepointIndices_.length(), osiIndices_.length(),
                     cacheList_.length(), runtimeData_.length(),
                     safepoints_.size(), callTargets.length(),
                     patchableBackedges_.length());
    if (!ionScript)
        return false;

    
    
    
    
    JSRuntime::AutoLockForOperationCallback lock(cx->runtime());

    
    
    cx->runtime()->ionRuntime()->ensureIonCodeAccessible(cx->runtime());

    
    
    
    Linker linker(masm);
    IonCode *code = (executionMode == SequentialExecution)
                    ? linker.newCodeForIonScript(cx)
                    : linker.newCode(cx, JSC::ION_CODE);
    if (!code) {
        
        
        js_free(ionScript);
        return false;
    }

    ionScript->setMethod(code);
    ionScript->setSkipArgCheckEntryOffset(getSkipArgCheckEntryOffset());

    
    if (sps_.enabled())
        ionScript->setHasSPSInstrumentation();

    SetIonScript(script, executionMode, ionScript);

    
    
    
    if (callTargets.length() != 0)
        ionScript->setHasUncompiledCallTarget();

    invalidateEpilogueData_.fixup(&masm);
    Assembler::patchDataWithValueCheck(CodeLocationLabel(code, invalidateEpilogueData_),
                                       ImmPtr(ionScript),
                                       ImmPtr((void*)-1));

    IonSpew(IonSpew_Codegen, "Created IonScript %p (raw %p)",
            (void *) ionScript, (void *) code->raw());

    ionScript->setInvalidationEpilogueDataOffset(invalidateEpilogueData_.offset());
    ionScript->setOsrPc(gen->info().osrPc());
    ionScript->setOsrEntryOffset(getOsrEntryOffset());
    ptrdiff_t real_invalidate = masm.actualOffset(invalidate_.offset());
    ionScript->setInvalidationEpilogueOffset(real_invalidate);

    ionScript->setDeoptTable(deoptTable_);

#if defined(JS_ION_PERF)
    if (PerfEnabled())
        perfSpewer_.writeProfile(script, code, masm);
#endif

    
    if (runtimeData_.length())
        ionScript->copyRuntimeData(&runtimeData_[0]);
    if (cacheList_.length())
        ionScript->copyCacheEntries(&cacheList_[0], masm);

    
    if (safepointIndices_.length())
        ionScript->copySafepointIndices(&safepointIndices_[0], masm);
    if (safepoints_.size())
        ionScript->copySafepoints(&safepoints_);

    
    if (bailouts_.length())
        ionScript->copyBailoutTable(&bailouts_[0]);
    if (osiIndices_.length())
        ionScript->copyOsiIndices(&osiIndices_[0], masm);
    if (snapshots_.size())
        ionScript->copySnapshots(&snapshots_);
    if (graph.numConstants())
        ionScript->copyConstants(graph.constantPool());
    if (callTargets.length() > 0)
        ionScript->copyCallTargetEntries(callTargets.begin());
    if (patchableBackedges_.length() > 0)
        ionScript->copyPatchableBackedges(cx, code, patchableBackedges_.begin());

    switch (executionMode) {
      case SequentialExecution:
        
        
        
        if (cx->zone()->needsBarrier())
            ionScript->toggleBarriers(true);
        break;
      case ParallelExecution:
        
        
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }

    return true;
}


class OutOfLineUnboxFloatingPoint : public OutOfLineCodeBase<CodeGenerator>
{
    LUnboxFloatingPoint *unboxFloatingPoint_;

  public:
    OutOfLineUnboxFloatingPoint(LUnboxFloatingPoint *unboxFloatingPoint)
      : unboxFloatingPoint_(unboxFloatingPoint)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineUnboxFloatingPoint(this);
    }

    LUnboxFloatingPoint *unboxFloatingPoint() const {
        return unboxFloatingPoint_;
    }
};

bool
CodeGenerator::visitUnboxFloatingPoint(LUnboxFloatingPoint *lir)
{
    const ValueOperand box = ToValue(lir, LUnboxFloatingPoint::Input);
    const LDefinition *result = lir->output();

    
    
    OutOfLineUnboxFloatingPoint *ool = new OutOfLineUnboxFloatingPoint(lir);
    if (!addOutOfLineCode(ool))
        return false;

    FloatRegister resultReg = ToFloatRegister(result);
    masm.branchTestDouble(Assembler::NotEqual, box, ool->entry());
    masm.unboxDouble(box, resultReg);
    if (lir->type() == MIRType_Float32)
        masm.convertDoubleToFloat(resultReg, resultReg);
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineUnboxFloatingPoint(OutOfLineUnboxFloatingPoint *ool)
{
    LUnboxFloatingPoint *ins = ool->unboxFloatingPoint();
    const ValueOperand value = ToValue(ins, LUnboxFloatingPoint::Input);

    if (ins->mir()->fallible()) {
        Assembler::Condition cond = masm.testInt32(Assembler::NotEqual, value);
        if (!bailoutIf(cond, ins->snapshot()))
            return false;
    }
    masm.int32ValueToFloatingPoint(value, ToFloatRegister(ins->output()), ins->type());
    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*GetPropertyFn)(JSContext *, HandleValue, HandlePropertyName, MutableHandleValue);
static const VMFunction GetPropertyInfo = FunctionInfo<GetPropertyFn>(GetProperty);

bool
CodeGenerator::visitCallGetProperty(LCallGetProperty *lir)
{
    pushArg(ImmGCPtr(lir->mir()->name()));
    pushArg(ToValue(lir, LCallGetProperty::Value));
    return callVM(GetPropertyInfo, lir);
}

typedef bool (*GetOrCallElementFn)(JSContext *, MutableHandleValue, HandleValue, MutableHandleValue);
static const VMFunction GetElementInfo = FunctionInfo<GetOrCallElementFn>(js::GetElement);
static const VMFunction CallElementInfo = FunctionInfo<GetOrCallElementFn>(js::CallElement);

bool
CodeGenerator::visitCallGetElement(LCallGetElement *lir)
{
    pushArg(ToValue(lir, LCallGetElement::RhsInput));
    pushArg(ToValue(lir, LCallGetElement::LhsInput));

    JSOp op = JSOp(*lir->mir()->resumePoint()->pc());

    if (op == JSOP_GETELEM) {
        return callVM(GetElementInfo, lir);
    } else {
        JS_ASSERT(op == JSOP_CALLELEM);
        return callVM(CallElementInfo, lir);
    }
}

bool
CodeGenerator::visitCallSetElement(LCallSetElement *lir)
{
    pushArg(Imm32(current->mir()->strict()));
    pushArg(ToValue(lir, LCallSetElement::Value));
    pushArg(ToValue(lir, LCallSetElement::Index));
    pushArg(ToRegister(lir->getOperand(0)));
    return callVM(SetObjectElementInfo, lir);
}

typedef bool (*InitElementArrayFn)(JSContext *, jsbytecode *, HandleObject, uint32_t, HandleValue);
static const VMFunction InitElementArrayInfo = FunctionInfo<InitElementArrayFn>(js::InitElementArray);

bool
CodeGenerator::visitCallInitElementArray(LCallInitElementArray *lir)
{
    pushArg(ToValue(lir, LCallInitElementArray::Value));
    pushArg(Imm32(lir->mir()->index()));
    pushArg(ToRegister(lir->getOperand(0)));
    pushArg(ImmPtr(lir->mir()->resumePoint()->pc()));
    return callVM(InitElementArrayInfo, lir);
}

bool
CodeGenerator::visitLoadFixedSlotV(LLoadFixedSlotV *ins)
{
    const Register obj = ToRegister(ins->getOperand(0));
    size_t slot = ins->mir()->slot();
    ValueOperand result = GetValueOutput(ins);

    masm.loadValue(Address(obj, JSObject::getFixedSlotOffset(slot)), result);
    return true;
}

bool
CodeGenerator::visitLoadFixedSlotT(LLoadFixedSlotT *ins)
{
    const Register obj = ToRegister(ins->getOperand(0));
    size_t slot = ins->mir()->slot();
    AnyRegister result = ToAnyRegister(ins->getDef(0));
    MIRType type = ins->mir()->type();

    masm.loadUnboxedValue(Address(obj, JSObject::getFixedSlotOffset(slot)), type, result);

    return true;
}

bool
CodeGenerator::visitStoreFixedSlotV(LStoreFixedSlotV *ins)
{
    const Register obj = ToRegister(ins->getOperand(0));
    size_t slot = ins->mir()->slot();

    const ValueOperand value = ToValue(ins, LStoreFixedSlotV::Value);

    Address address(obj, JSObject::getFixedSlotOffset(slot));
    if (ins->mir()->needsBarrier())
        emitPreBarrier(address, MIRType_Value);

    masm.storeValue(value, address);

    return true;
}

bool
CodeGenerator::visitStoreFixedSlotT(LStoreFixedSlotT *ins)
{
    const Register obj = ToRegister(ins->getOperand(0));
    size_t slot = ins->mir()->slot();

    const LAllocation *value = ins->value();
    MIRType valueType = ins->mir()->value()->type();

    ConstantOrRegister nvalue = value->isConstant()
                              ? ConstantOrRegister(*value->toConstant())
                              : TypedOrValueRegister(valueType, ToAnyRegister(value));

    Address address(obj, JSObject::getFixedSlotOffset(slot));
    if (ins->mir()->needsBarrier())
        emitPreBarrier(address, MIRType_Value);

    masm.storeConstantOrRegister(nvalue, address);

    return true;
}

bool
CodeGenerator::visitCallsiteCloneCache(LCallsiteCloneCache *ins)
{
    const MCallsiteCloneCache *mir = ins->mir();
    Register callee = ToRegister(ins->callee());
    Register output = ToRegister(ins->output());

    CallsiteCloneIC cache(callee, mir->block()->info().script(), mir->callPc(), output);
    return addCache(ins, allocateCache(cache));
}

typedef JSObject *(*CallsiteCloneICFn)(JSContext *, size_t, HandleObject);
const VMFunction CallsiteCloneIC::UpdateInfo =
    FunctionInfo<CallsiteCloneICFn>(CallsiteCloneIC::update);

bool
CodeGenerator::visitCallsiteCloneIC(OutOfLineUpdateCache *ool, DataPtr<CallsiteCloneIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->calleeReg());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(CallsiteCloneIC::UpdateInfo, lir))
        return false;
    StoreRegisterTo(ic->outputReg()).generate(this);
    restoreLiveIgnore(lir, StoreRegisterTo(ic->outputReg()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitGetNameCache(LGetNameCache *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register scopeChain = ToRegister(ins->scopeObj());
    TypedOrValueRegister output(GetValueOutput(ins));
    bool isTypeOf = ins->mir()->accessKind() != MGetNameCache::NAME;

    NameIC cache(liveRegs, isTypeOf, scopeChain, ins->mir()->name(), output);
    return addCache(ins, allocateCache(cache));
}

typedef bool (*NameICFn)(JSContext *, size_t, HandleObject, MutableHandleValue);
const VMFunction NameIC::UpdateInfo = FunctionInfo<NameICFn>(NameIC::update);

bool
CodeGenerator::visitNameIC(OutOfLineUpdateCache *ool, DataPtr<NameIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->scopeChainReg());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(NameIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->outputReg()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->outputReg()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::addGetPropertyCache(LInstruction *ins, RegisterSet liveRegs, Register objReg,
                                   PropertyName *name, TypedOrValueRegister output,
                                   bool allowGetters)
{
    switch (gen->info().executionMode()) {
      case SequentialExecution: {
        GetPropertyIC cache(liveRegs, objReg, name, output, allowGetters);
        return addCache(ins, allocateCache(cache));
      }
      case ParallelExecution: {
        GetPropertyParIC cache(objReg, name, output);
        return addCache(ins, allocateCache(cache));
      }
      default:
        MOZ_ASSUME_UNREACHABLE("Bad execution mode");
    }
}

bool
CodeGenerator::visitGetPropertyCacheV(LGetPropertyCacheV *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    PropertyName *name = ins->mir()->name();
    bool allowGetters = ins->mir()->allowGetters();
    TypedOrValueRegister output = TypedOrValueRegister(GetValueOutput(ins));

    return addGetPropertyCache(ins, liveRegs, objReg, name, output, allowGetters);
}

bool
CodeGenerator::visitGetPropertyCacheT(LGetPropertyCacheT *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    PropertyName *name = ins->mir()->name();
    bool allowGetters = ins->mir()->allowGetters();
    TypedOrValueRegister output(ins->mir()->type(), ToAnyRegister(ins->getDef(0)));

    return addGetPropertyCache(ins, liveRegs, objReg, name, output, allowGetters);
}

typedef bool (*GetPropertyICFn)(JSContext *, size_t, HandleObject, MutableHandleValue);
const VMFunction GetPropertyIC::UpdateInfo =
    FunctionInfo<GetPropertyICFn>(GetPropertyIC::update);

bool
CodeGenerator::visitGetPropertyIC(OutOfLineUpdateCache *ool, DataPtr<GetPropertyIC> &ic)
{
    LInstruction *lir = ool->lir();

    if (ic->idempotent()) {
        size_t numLocs;
        CacheLocationList &cacheLocs = lir->mirRaw()->toGetPropertyCache()->location();
        size_t locationBase = addCacheLocations(cacheLocs, &numLocs);
        ic->setLocationInfo(locationBase, numLocs);
    }

    saveLive(lir);

    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(GetPropertyIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->output()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->output()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

typedef ParallelResult (*GetPropertyParICFn)(ForkJoinSlice *, size_t, HandleObject,
                                             MutableHandleValue);
const VMFunction GetPropertyParIC::UpdateInfo =
    FunctionInfo<GetPropertyParICFn>(GetPropertyParIC::update);

bool
CodeGenerator::visitGetPropertyParIC(OutOfLineUpdateCache *ool, DataPtr<GetPropertyParIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(GetPropertyParIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->output()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->output()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::addGetElementCache(LInstruction *ins, Register obj, ConstantOrRegister index,
                                  TypedOrValueRegister output, bool monitoredResult)
{
    switch (gen->info().executionMode()) {
      case SequentialExecution: {
        GetElementIC cache(obj, index, output, monitoredResult);
        return addCache(ins, allocateCache(cache));
      }
      case ParallelExecution: {
        GetElementParIC cache(obj, index, output, monitoredResult);
        return addCache(ins, allocateCache(cache));
      }
      default:
        MOZ_ASSUME_UNREACHABLE("No such execution mode");
    }
}

bool
CodeGenerator::visitGetElementCacheV(LGetElementCacheV *ins)
{
    Register obj = ToRegister(ins->object());
    ConstantOrRegister index = TypedOrValueRegister(ToValue(ins, LGetElementCacheV::Index));
    TypedOrValueRegister output = TypedOrValueRegister(GetValueOutput(ins));

    return addGetElementCache(ins, obj, index, output, ins->mir()->monitoredResult());
}

bool
CodeGenerator::visitGetElementCacheT(LGetElementCacheT *ins)
{
    Register obj = ToRegister(ins->object());
    ConstantOrRegister index = TypedOrValueRegister(MIRType_Int32, ToAnyRegister(ins->index()));
    TypedOrValueRegister output(ins->mir()->type(), ToAnyRegister(ins->output()));

    return addGetElementCache(ins, obj, index, output, ins->mir()->monitoredResult());
}

typedef bool (*GetElementICFn)(JSContext *, size_t, HandleObject, HandleValue, MutableHandleValue);
const VMFunction GetElementIC::UpdateInfo =
    FunctionInfo<GetElementICFn>(GetElementIC::update);

bool
CodeGenerator::visitGetElementIC(OutOfLineUpdateCache *ool, DataPtr<GetElementIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->index());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(GetElementIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->output()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->output()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitSetElementCacheV(LSetElementCacheV *ins)
{
    Register obj = ToRegister(ins->object());
    Register unboxIndex = ToTempUnboxRegister(ins->tempToUnboxIndex());
    Register temp = ToRegister(ins->temp());
    FloatRegister tempFloat = ToFloatRegister(ins->tempFloat());
    ValueOperand index = ToValue(ins, LSetElementCacheV::Index);
    ConstantOrRegister value = TypedOrValueRegister(ToValue(ins, LSetElementCacheV::Value));

    SetElementIC cache(obj, unboxIndex, temp, tempFloat, index, value, ins->mir()->strict());

    return addCache(ins, allocateCache(cache));
}

bool
CodeGenerator::visitSetElementCacheT(LSetElementCacheT *ins)
{
    Register obj = ToRegister(ins->object());
    Register unboxIndex = ToTempUnboxRegister(ins->tempToUnboxIndex());
    Register temp = ToRegister(ins->temp());
    FloatRegister tempFloat = ToFloatRegister(ins->tempFloat());
    ValueOperand index = ToValue(ins, LSetElementCacheT::Index);
    ConstantOrRegister value;
    const LAllocation *tmp = ins->value();
    if (tmp->isConstant())
        value = *tmp->toConstant();
    else
        value = TypedOrValueRegister(ins->mir()->value()->type(), ToAnyRegister(tmp));

    SetElementIC cache(obj, unboxIndex, temp, tempFloat, index, value, ins->mir()->strict());

    return addCache(ins, allocateCache(cache));
}

typedef bool (*SetElementICFn)(JSContext *, size_t, HandleObject, HandleValue, HandleValue);
const VMFunction SetElementIC::UpdateInfo =
    FunctionInfo<SetElementICFn>(SetElementIC::update);

bool
CodeGenerator::visitSetElementIC(OutOfLineUpdateCache *ool, DataPtr<SetElementIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->value());
    pushArg(ic->index());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(SetElementIC::UpdateInfo, lir))
        return false;
    restoreLive(lir);

    masm.jump(ool->rejoin());
    return true;
}

typedef ParallelResult (*GetElementParICFn)(ForkJoinSlice *, size_t, HandleObject,
                                            HandleValue, MutableHandleValue);
const VMFunction GetElementParIC::UpdateInfo =
    FunctionInfo<GetElementParICFn>(GetElementParIC::update);

bool
CodeGenerator::visitGetElementParIC(OutOfLineUpdateCache *ool, DataPtr<GetElementParIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->index());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(GetElementParIC::UpdateInfo, lir))
        return false;
    StoreValueTo(ic->output()).generate(this);
    restoreLiveIgnore(lir, StoreValueTo(ic->output()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitBindNameCache(LBindNameCache *ins)
{
    Register scopeChain = ToRegister(ins->scopeChain());
    Register output = ToRegister(ins->output());
    BindNameIC cache(scopeChain, ins->mir()->name(), output);

    return addCache(ins, allocateCache(cache));
}

typedef JSObject *(*BindNameICFn)(JSContext *, size_t, HandleObject);
const VMFunction BindNameIC::UpdateInfo =
    FunctionInfo<BindNameICFn>(BindNameIC::update);

bool
CodeGenerator::visitBindNameIC(OutOfLineUpdateCache *ool, DataPtr<BindNameIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->scopeChainReg());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(BindNameIC::UpdateInfo, lir))
        return false;
    StoreRegisterTo(ic->outputReg()).generate(this);
    restoreLiveIgnore(lir, StoreRegisterTo(ic->outputReg()).clobbered());

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*SetPropertyFn)(JSContext *, HandleObject,
                              HandlePropertyName, const HandleValue, bool, jsbytecode *);
static const VMFunction SetPropertyInfo =
    FunctionInfo<SetPropertyFn>(SetProperty);

bool
CodeGenerator::visitCallSetProperty(LCallSetProperty *ins)
{
    ConstantOrRegister value = TypedOrValueRegister(ToValue(ins, LCallSetProperty::Value));

    const Register objReg = ToRegister(ins->getOperand(0));

    pushArg(ImmPtr(ins->mir()->resumePoint()->pc()));
    pushArg(Imm32(ins->mir()->strict()));

    pushArg(value);
    pushArg(ImmGCPtr(ins->mir()->name()));
    pushArg(objReg);

    return callVM(SetPropertyInfo, ins);
}

typedef bool (*DeletePropertyFn)(JSContext *, HandleValue, HandlePropertyName, bool *);
static const VMFunction DeletePropertyStrictInfo =
    FunctionInfo<DeletePropertyFn>(DeleteProperty<true>);
static const VMFunction DeletePropertyNonStrictInfo =
    FunctionInfo<DeletePropertyFn>(DeleteProperty<false>);

bool
CodeGenerator::visitCallDeleteProperty(LCallDeleteProperty *lir)
{
    pushArg(ImmGCPtr(lir->mir()->name()));
    pushArg(ToValue(lir, LCallDeleteProperty::Value));

    if (lir->mir()->block()->info().script()->strict)
        return callVM(DeletePropertyStrictInfo, lir);

    return callVM(DeletePropertyNonStrictInfo, lir);
}

typedef bool (*DeleteElementFn)(JSContext *, HandleValue, HandleValue, bool *);
static const VMFunction DeleteElementStrictInfo =
    FunctionInfo<DeleteElementFn>(DeleteElement<true>);
static const VMFunction DeleteElementNonStrictInfo =
    FunctionInfo<DeleteElementFn>(DeleteElement<false>);

bool
CodeGenerator::visitCallDeleteElement(LCallDeleteElement *lir)
{
    pushArg(ToValue(lir, LCallDeleteElement::Index));
    pushArg(ToValue(lir, LCallDeleteElement::Value));

    if (lir->mir()->block()->info().script()->strict)
        return callVM(DeleteElementStrictInfo, lir);

    return callVM(DeleteElementNonStrictInfo, lir);
}

bool
CodeGenerator::visitSetPropertyCacheV(LSetPropertyCacheV *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    ConstantOrRegister value = TypedOrValueRegister(ToValue(ins, LSetPropertyCacheV::Value));

    SetPropertyIC cache(liveRegs, objReg, ins->mir()->name(), value, ins->mir()->strict(),
                        ins->mir()->needsTypeBarrier());
    return addCache(ins, allocateCache(cache));
}

bool
CodeGenerator::visitSetPropertyCacheT(LSetPropertyCacheT *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    ConstantOrRegister value;

    if (ins->getOperand(1)->isConstant())
        value = ConstantOrRegister(*ins->getOperand(1)->toConstant());
    else
        value = TypedOrValueRegister(ins->valueType(), ToAnyRegister(ins->getOperand(1)));

    SetPropertyIC cache(liveRegs, objReg, ins->mir()->name(), value, ins->mir()->strict(),
                        ins->mir()->needsTypeBarrier());
    return addCache(ins, allocateCache(cache));
}

typedef bool (*SetPropertyICFn)(JSContext *, size_t, HandleObject, HandleValue);
const VMFunction SetPropertyIC::UpdateInfo =
    FunctionInfo<SetPropertyICFn>(SetPropertyIC::update);

bool
CodeGenerator::visitSetPropertyIC(OutOfLineUpdateCache *ool, DataPtr<SetPropertyIC> &ic)
{
    LInstruction *lir = ool->lir();
    saveLive(lir);

    pushArg(ic->value());
    pushArg(ic->object());
    pushArg(Imm32(ool->getCacheIndex()));
    if (!callVM(SetPropertyIC::UpdateInfo, lir))
        return false;
    restoreLive(lir);

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*ThrowFn)(JSContext *, HandleValue);
static const VMFunction ThrowInfo = FunctionInfo<ThrowFn>(js::Throw);

bool
CodeGenerator::visitThrow(LThrow *lir)
{
    pushArg(ToValue(lir, LThrow::Value));
    return callVM(ThrowInfo, lir);
}

typedef bool (*BitNotFn)(JSContext *, HandleValue, int *p);
typedef ParallelResult (*BitNotParFn)(ForkJoinSlice *, HandleValue, int32_t *);
static const VMFunctionsModal BitNotInfo = VMFunctionsModal(
    FunctionInfo<BitNotFn>(BitNot),
    FunctionInfo<BitNotParFn>(BitNotPar));

bool
CodeGenerator::visitBitNotV(LBitNotV *lir)
{
    pushArg(ToValue(lir, LBitNotV::Input));
    return callVM(BitNotInfo, lir);
}

typedef bool (*BitopFn)(JSContext *, HandleValue, HandleValue, int *p);
typedef ParallelResult (*BitopParFn)(ForkJoinSlice *, HandleValue, HandleValue, int32_t *);
static const VMFunctionsModal BitAndInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitAnd),
    FunctionInfo<BitopParFn>(BitAndPar));
static const VMFunctionsModal BitOrInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitOr),
    FunctionInfo<BitopParFn>(BitOrPar));
static const VMFunctionsModal BitXorInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitXor),
    FunctionInfo<BitopParFn>(BitXorPar));
static const VMFunctionsModal BitLhsInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitLsh),
    FunctionInfo<BitopParFn>(BitLshPar));
static const VMFunctionsModal BitRhsInfo = VMFunctionsModal(
    FunctionInfo<BitopFn>(BitRsh),
    FunctionInfo<BitopParFn>(BitRshPar));

bool
CodeGenerator::visitBitOpV(LBitOpV *lir)
{
    pushArg(ToValue(lir, LBitOpV::RhsInput));
    pushArg(ToValue(lir, LBitOpV::LhsInput));

    switch (lir->jsop()) {
      case JSOP_BITAND:
        return callVM(BitAndInfo, lir);
      case JSOP_BITOR:
        return callVM(BitOrInfo, lir);
      case JSOP_BITXOR:
        return callVM(BitXorInfo, lir);
      case JSOP_LSH:
        return callVM(BitLhsInfo, lir);
      case JSOP_RSH:
        return callVM(BitRhsInfo, lir);
      default:
        break;
    }
    MOZ_ASSUME_UNREACHABLE("unexpected bitop");
}

class OutOfLineTypeOfV : public OutOfLineCodeBase<CodeGenerator>
{
    LTypeOfV *ins_;

  public:
    OutOfLineTypeOfV(LTypeOfV *ins)
      : ins_(ins)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineTypeOfV(this);
    }
    LTypeOfV *ins() const {
        return ins_;
    }
};

bool
CodeGenerator::visitTypeOfV(LTypeOfV *lir)
{
    const ValueOperand value = ToValue(lir, LTypeOfV::Input);
    Register output = ToRegister(lir->output());
    Register tag = masm.splitTagForTest(value);

    JSRuntime *rt = GetIonContext()->runtime;
    Label done;

    OutOfLineTypeOfV *ool = nullptr;
    if (lir->mir()->inputMaybeCallableOrEmulatesUndefined()) {
        
        
        ool = new OutOfLineTypeOfV(lir);
        if (!addOutOfLineCode(ool))
            return false;

        masm.branchTestObject(Assembler::Equal, tag, ool->entry());
    } else {
        
        
        Label notObject;
        masm.branchTestObject(Assembler::NotEqual, tag, &notObject);
        masm.movePtr(ImmGCPtr(rt->atomState.object), output);
        masm.jump(&done);
        masm.bind(&notObject);
    }

    Label notNumber;
    masm.branchTestNumber(Assembler::NotEqual, tag, &notNumber);
    masm.movePtr(ImmGCPtr(rt->atomState.number), output);
    masm.jump(&done);
    masm.bind(&notNumber);

    Label notUndefined;
    masm.branchTestUndefined(Assembler::NotEqual, tag, &notUndefined);
    masm.movePtr(ImmGCPtr(rt->atomState.undefined), output);
    masm.jump(&done);
    masm.bind(&notUndefined);

    Label notNull;
    masm.branchTestNull(Assembler::NotEqual, tag, &notNull);
    masm.movePtr(ImmGCPtr(rt->atomState.object), output);
    masm.jump(&done);
    masm.bind(&notNull);

    Label notBoolean;
    masm.branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
    masm.movePtr(ImmGCPtr(rt->atomState.boolean), output);
    masm.jump(&done);
    masm.bind(&notBoolean);

    masm.movePtr(ImmGCPtr(rt->atomState.string), output);

    masm.bind(&done);
    if (ool)
        masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineTypeOfV(OutOfLineTypeOfV *ool)
{
    LTypeOfV *ins = ool->ins();

    ValueOperand input = ToValue(ins, LTypeOfV::Input);
    Register temp = ToTempUnboxRegister(ins->tempToUnbox());
    Register output = ToRegister(ins->output());

    Register obj = masm.extractObject(input, temp);

    saveVolatile(output);
    masm.setupUnalignedABICall(2, output);
    masm.passABIArg(obj);
    masm.movePtr(ImmPtr(GetIonContext()->runtime), output);
    masm.passABIArg(output);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::TypeOfObjectOperation));
    masm.storeCallResult(output);
    restoreVolatile(output);

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*ToIdFn)(JSContext *, HandleScript, jsbytecode *, HandleValue, HandleValue,
                       MutableHandleValue);
static const VMFunction ToIdInfo = FunctionInfo<ToIdFn>(ToIdOperation);

bool
CodeGenerator::visitToIdV(LToIdV *lir)
{
    Label notInt32;
    FloatRegister temp = ToFloatRegister(lir->tempFloat());
    const ValueOperand out = ToOutValue(lir);
    ValueOperand index = ToValue(lir, LToIdV::Index);

    OutOfLineCode *ool = oolCallVM(ToIdInfo, lir,
                                   (ArgList(),
                                   ImmGCPtr(current->mir()->info().script()),
                                   ImmPtr(lir->mir()->resumePoint()->pc()),
                                   ToValue(lir, LToIdV::Object),
                                   ToValue(lir, LToIdV::Index)),
                                   StoreValueTo(out));

    Register tag = masm.splitTagForTest(index);

    masm.branchTestInt32(Assembler::NotEqual, tag, &notInt32);
    masm.moveValue(index, out);
    masm.jump(ool->rejoin());

    masm.bind(&notInt32);
    masm.branchTestDouble(Assembler::NotEqual, tag, ool->entry());
    masm.unboxDouble(index, temp);
    masm.convertDoubleToInt32(temp, out.scratchReg(), ool->entry(), true);
    masm.tagValue(JSVAL_TYPE_INT32, out.scratchReg(), out);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitLoadElementV(LLoadElementV *load)
{
    Register elements = ToRegister(load->elements());
    const ValueOperand out = ToOutValue(load);

    if (load->index()->isConstant())
        masm.loadValue(Address(elements, ToInt32(load->index()) * sizeof(Value)), out);
    else
        masm.loadValue(BaseIndex(elements, ToRegister(load->index()), TimesEight), out);

    if (load->mir()->needsHoleCheck()) {
        Assembler::Condition cond = masm.testMagic(Assembler::Equal, out);
        if (!bailoutIf(cond, load->snapshot()))
            return false;
    }

    return true;
}

bool
CodeGenerator::visitLoadElementHole(LLoadElementHole *lir)
{
    Register elements = ToRegister(lir->elements());
    Register initLength = ToRegister(lir->initLength());
    const ValueOperand out = ToOutValue(lir);

    const MLoadElementHole *mir = lir->mir();

    
    
    Label undefined, done;
    if (lir->index()->isConstant()) {
        masm.branch32(Assembler::BelowOrEqual, initLength, Imm32(ToInt32(lir->index())), &undefined);
        masm.loadValue(Address(elements, ToInt32(lir->index()) * sizeof(Value)), out);
    } else {
        masm.branch32(Assembler::BelowOrEqual, initLength, ToRegister(lir->index()), &undefined);
        masm.loadValue(BaseIndex(elements, ToRegister(lir->index()), TimesEight), out);
    }

    
    
    if (lir->mir()->needsHoleCheck())
        masm.branchTestMagic(Assembler::NotEqual, out, &done);
    else
        masm.jump(&done);

    masm.bind(&undefined);

    if (mir->needsNegativeIntCheck()) {
        if (lir->index()->isConstant()) {
            if (ToInt32(lir->index()) < 0 && !bailout(lir->snapshot()))
                return false;
        } else {
            Label negative;
            masm.branch32(Assembler::LessThan, ToRegister(lir->index()), Imm32(0), &negative);
            if (!bailoutFrom(&negative, lir->snapshot()))
                return false;
        }
    }

    masm.moveValue(UndefinedValue(), out);
    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitLoadTypedArrayElement(LLoadTypedArrayElement *lir)
{
    Register elements = ToRegister(lir->elements());
    Register temp = lir->temp()->isBogusTemp() ? InvalidReg : ToRegister(lir->temp());
    AnyRegister out = ToAnyRegister(lir->output());

    int arrayType = lir->mir()->arrayType();
    int width = TypedArrayObject::slotWidth(arrayType);

    Label fail;
    if (lir->index()->isConstant()) {
        Address source(elements, ToInt32(lir->index()) * width);
        masm.loadFromTypedArray(arrayType, source, out, temp, &fail);
    } else {
        BaseIndex source(elements, ToRegister(lir->index()), ScaleFromElemWidth(width));
        masm.loadFromTypedArray(arrayType, source, out, temp, &fail);
    }

    if (fail.used() && !bailoutFrom(&fail, lir->snapshot()))
        return false;

    return true;
}

bool
CodeGenerator::visitLoadTypedArrayElementHole(LLoadTypedArrayElementHole *lir)
{
    Register object = ToRegister(lir->object());
    const ValueOperand out = ToOutValue(lir);

    
    Register scratch = out.scratchReg();
    Int32Key key = ToInt32Key(lir->index());
    masm.unboxInt32(Address(object, TypedArrayObject::lengthOffset()), scratch);

    
    Label inbounds, done;
    masm.branchKey(Assembler::Above, scratch, key, &inbounds);
    masm.moveValue(UndefinedValue(), out);
    masm.jump(&done);

    
    masm.bind(&inbounds);
    masm.loadPtr(Address(object, TypedArrayObject::dataOffset()), scratch);

    int arrayType = lir->mir()->arrayType();
    int width = TypedArrayObject::slotWidth(arrayType);

    Label fail;
    if (key.isConstant()) {
        Address source(scratch, key.constant() * width);
        masm.loadFromTypedArray(arrayType, source, out, lir->mir()->allowDouble(),
                                out.scratchReg(), &fail);
    } else {
        BaseIndex source(scratch, key.reg(), ScaleFromElemWidth(width));
        masm.loadFromTypedArray(arrayType, source, out, lir->mir()->allowDouble(),
                                out.scratchReg(), &fail);
    }

    if (fail.used() && !bailoutFrom(&fail, lir->snapshot()))
        return false;

    masm.bind(&done);
    return true;
}

template <typename T>
static inline void
StoreToTypedArray(MacroAssembler &masm, int arrayType, const LAllocation *value, const T &dest)
{
    if (arrayType == ScalarTypeRepresentation::TYPE_FLOAT32 ||
        arrayType == ScalarTypeRepresentation::TYPE_FLOAT64)
    {
        masm.storeToTypedFloatArray(arrayType, ToFloatRegister(value), dest);
    } else {
        if (value->isConstant())
            masm.storeToTypedIntArray(arrayType, Imm32(ToInt32(value)), dest);
        else
            masm.storeToTypedIntArray(arrayType, ToRegister(value), dest);
    }
}

bool
CodeGenerator::visitStoreTypedArrayElement(LStoreTypedArrayElement *lir)
{
    Register elements = ToRegister(lir->elements());
    const LAllocation *value = lir->value();

    int arrayType = lir->mir()->arrayType();
    int width = TypedArrayObject::slotWidth(arrayType);

    if (lir->index()->isConstant()) {
        Address dest(elements, ToInt32(lir->index()) * width);
        StoreToTypedArray(masm, arrayType, value, dest);
    } else {
        BaseIndex dest(elements, ToRegister(lir->index()), ScaleFromElemWidth(width));
        StoreToTypedArray(masm, arrayType, value, dest);
    }

    return true;
}

bool
CodeGenerator::visitStoreTypedArrayElementHole(LStoreTypedArrayElementHole *lir)
{
    Register elements = ToRegister(lir->elements());
    const LAllocation *value = lir->value();

    int arrayType = lir->mir()->arrayType();
    int width = TypedArrayObject::slotWidth(arrayType);

    bool guardLength = true;
    if (lir->index()->isConstant() && lir->length()->isConstant()) {
        uint32_t idx = ToInt32(lir->index());
        uint32_t len = ToInt32(lir->length());
        if (idx >= len)
            return true;
        guardLength = false;
    }
    Label skip;
    if (lir->index()->isConstant()) {
        uint32_t idx = ToInt32(lir->index());
        if (guardLength)
            masm.branch32(Assembler::BelowOrEqual, ToOperand(lir->length()), Imm32(idx), &skip);
        Address dest(elements, idx * width);
        StoreToTypedArray(masm, arrayType, value, dest);
    } else {
        Register idxReg = ToRegister(lir->index());
        JS_ASSERT(guardLength);
        if (lir->length()->isConstant())
            masm.branch32(Assembler::AboveOrEqual, idxReg, Imm32(ToInt32(lir->length())), &skip);
        else
            masm.branch32(Assembler::BelowOrEqual, ToOperand(lir->length()), idxReg, &skip);
        BaseIndex dest(elements, ToRegister(lir->index()), ScaleFromElemWidth(width));
        StoreToTypedArray(masm, arrayType, value, dest);
    }
    if (guardLength)
        masm.bind(&skip);

    return true;
}

bool
CodeGenerator::visitClampIToUint8(LClampIToUint8 *lir)
{
    Register output = ToRegister(lir->output());
    JS_ASSERT(output == ToRegister(lir->input()));
    masm.clampIntToUint8(output);
    return true;
}

bool
CodeGenerator::visitClampDToUint8(LClampDToUint8 *lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    masm.clampDoubleToUint8(input, output);
    return true;
}

bool
CodeGenerator::visitClampVToUint8(LClampVToUint8 *lir)
{
    ValueOperand operand = ToValue(lir, LClampVToUint8::Input);
    FloatRegister tempFloat = ToFloatRegister(lir->tempFloat());
    Register output = ToRegister(lir->output());
    MDefinition *input = lir->mir()->input();

    Label *stringEntry, *stringRejoin;
    if (input->mightBeType(MIRType_String)) {
        OutOfLineCode *oolString = oolCallVM(StringToNumberInfo, lir, (ArgList(), output),
                                             StoreFloatRegisterTo(tempFloat));
        if (!oolString)
            return false;
        stringEntry = oolString->entry();
        stringRejoin = oolString->rejoin();
    } else {
        stringEntry = nullptr;
        stringRejoin = nullptr;
    }

    Label fails;
    masm.clampValueToUint8(operand, input,
                           stringEntry, stringRejoin,
                           output, tempFloat, output, &fails);

    if (!bailoutFrom(&fails, lir->snapshot()))
        return false;

    return true;
}

typedef bool (*OperatorInFn)(JSContext *, HandleValue, HandleObject, bool *);
static const VMFunction OperatorInInfo = FunctionInfo<OperatorInFn>(OperatorIn);

bool
CodeGenerator::visitIn(LIn *ins)
{
    pushArg(ToRegister(ins->rhs()));
    pushArg(ToValue(ins, LIn::LHS));

    return callVM(OperatorInInfo, ins);
}

typedef bool (*OperatorInIFn)(JSContext *, uint32_t, HandleObject, bool *);
static const VMFunction OperatorInIInfo = FunctionInfo<OperatorInIFn>(OperatorInI);

bool
CodeGenerator::visitInArray(LInArray *lir)
{
    const MInArray *mir = lir->mir();
    Register elements = ToRegister(lir->elements());
    Register initLength = ToRegister(lir->initLength());
    Register output = ToRegister(lir->output());

    
    Label falseBranch, done, trueBranch;

    OutOfLineCode *ool = nullptr;
    Label* failedInitLength = &falseBranch;

    if (lir->index()->isConstant()) {
        int32_t index = ToInt32(lir->index());

        JS_ASSERT_IF(index < 0, mir->needsNegativeIntCheck());
        if (mir->needsNegativeIntCheck()) {
            ool = oolCallVM(OperatorInIInfo, lir,
                            (ArgList(), Imm32(index), ToRegister(lir->object())),
                            StoreRegisterTo(output));
            failedInitLength = ool->entry();
        }

        masm.branch32(Assembler::BelowOrEqual, initLength, Imm32(index), failedInitLength);
        if (mir->needsHoleCheck()) {
            Address address = Address(elements, index * sizeof(Value));
            masm.branchTestMagic(Assembler::Equal, address, &falseBranch);
        }
    } else {
        Label negativeIntCheck;
        Register index = ToRegister(lir->index());

        if (mir->needsNegativeIntCheck())
            failedInitLength = &negativeIntCheck;

        masm.branch32(Assembler::BelowOrEqual, initLength, index, failedInitLength);
        if (mir->needsHoleCheck()) {
            BaseIndex address = BaseIndex(elements, ToRegister(lir->index()), TimesEight);
            masm.branchTestMagic(Assembler::Equal, address, &falseBranch);
        }
        masm.jump(&trueBranch);

        if (mir->needsNegativeIntCheck()) {
            masm.bind(&negativeIntCheck);
            ool = oolCallVM(OperatorInIInfo, lir,
                            (ArgList(), index, ToRegister(lir->object())),
                            StoreRegisterTo(output));

            masm.branch32(Assembler::LessThan, index, Imm32(0), ool->entry());
            masm.jump(&falseBranch);
        }
    }

    masm.bind(&trueBranch);
    masm.move32(Imm32(1), output);
    masm.jump(&done);

    masm.bind(&falseBranch);
    masm.move32(Imm32(0), output);
    masm.bind(&done);

    if (ool)
        masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitInstanceOfO(LInstanceOfO *ins)
{
    return emitInstanceOf(ins, ins->mir()->prototypeObject());
}

bool
CodeGenerator::visitInstanceOfV(LInstanceOfV *ins)
{
    return emitInstanceOf(ins, ins->mir()->prototypeObject());
}


static bool
IsDelegateObject(JSContext *cx, HandleObject protoObj, HandleObject obj, bool *res)
{
    return IsDelegate(cx, protoObj, ObjectValue(*obj), res);
}

typedef bool (*IsDelegateObjectFn)(JSContext *, HandleObject, HandleObject, bool *);
static const VMFunction IsDelegateObjectInfo = FunctionInfo<IsDelegateObjectFn>(IsDelegateObject);

bool
CodeGenerator::emitInstanceOf(LInstruction *ins, JSObject *prototypeObject)
{
    
    

    Label done;
    Register output = ToRegister(ins->getDef(0));

    
    Register objReg;
    if (ins->isInstanceOfV()) {
        Label isObject;
        ValueOperand lhsValue = ToValue(ins, LInstanceOfV::LHS);
        masm.branchTestObject(Assembler::Equal, lhsValue, &isObject);
        masm.mov(ImmWord(0), output);
        masm.jump(&done);
        masm.bind(&isObject);
        objReg = masm.extractObject(lhsValue, output);
    } else {
        objReg = ToRegister(ins->toInstanceOfO()->lhs());
    }

    
    
    

    
    masm.loadPtr(Address(objReg, JSObject::offsetOfType()), output);
    masm.loadPtr(Address(output, offsetof(types::TypeObject, proto)), output);

    Label testLazy;
    {
        Label loopPrototypeChain;
        masm.bind(&loopPrototypeChain);

        
        Label notPrototypeObject;
        masm.branchPtr(Assembler::NotEqual, output, ImmGCPtr(prototypeObject), &notPrototypeObject);
        masm.mov(ImmWord(1), output);
        masm.jump(&done);
        masm.bind(&notPrototypeObject);

        JS_ASSERT(uintptr_t(Proxy::LazyProto) == 1);

        
        masm.branchPtr(Assembler::BelowOrEqual, output, ImmWord(1), &testLazy);

        
        masm.loadPtr(Address(output, JSObject::offsetOfType()), output);
        masm.loadPtr(Address(output, offsetof(types::TypeObject, proto)), output);

        masm.jump(&loopPrototypeChain);
    }

    
    
    
    
    

    OutOfLineCode *ool = oolCallVM(IsDelegateObjectInfo, ins,
                                   (ArgList(), ImmGCPtr(prototypeObject), objReg),
                                   StoreRegisterTo(output));

    
    Label regenerate, *lazyEntry;
    if (objReg != output) {
        lazyEntry = ool->entry();
    } else {
        masm.bind(&regenerate);
        lazyEntry = &regenerate;
        if (ins->isInstanceOfV()) {
            ValueOperand lhsValue = ToValue(ins, LInstanceOfV::LHS);
            objReg = masm.extractObject(lhsValue, output);
        } else {
            objReg = ToRegister(ins->toInstanceOfO()->lhs());
        }
        JS_ASSERT(objReg == output);
        masm.jump(ool->entry());
    }

    masm.bind(&testLazy);
    masm.branchPtr(Assembler::Equal, output, ImmWord(1), lazyEntry);

    masm.bind(&done);
    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*HasInstanceFn)(JSContext *, HandleObject, HandleValue, bool *);
static const VMFunction HasInstanceInfo = FunctionInfo<HasInstanceFn>(js::HasInstance);

bool
CodeGenerator::visitCallInstanceOf(LCallInstanceOf *ins)
{
    ValueOperand lhs = ToValue(ins, LCallInstanceOf::LHS);
    Register rhs = ToRegister(ins->rhs());
    JS_ASSERT(ToRegister(ins->output()) == ReturnReg);

    pushArg(lhs);
    pushArg(rhs);
    return callVM(HasInstanceInfo, ins);
}

bool
CodeGenerator::visitGetDOMProperty(LGetDOMProperty *ins)
{
    const Register JSContextReg = ToRegister(ins->getJSContextReg());
    const Register ObjectReg = ToRegister(ins->getObjectReg());
    const Register PrivateReg = ToRegister(ins->getPrivReg());
    const Register ValueReg = ToRegister(ins->getValueReg());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    masm.Push(UndefinedValue());
    
    
    JS_STATIC_ASSERT(sizeof(JSJitGetterCallArgs) == sizeof(Value*));
    masm.movePtr(StackPointer, ValueReg);

    masm.Push(ObjectReg);

    
    masm.loadPrivate(Address(ObjectReg, JSObject::getFixedSlotOffset(0)), PrivateReg);

    
    masm.movePtr(StackPointer, ObjectReg);

    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(JSContextReg, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(ION_FRAME_DOMGETTER);

    if (!markSafepointAt(safepointOffset, ins))
        return false;

    masm.setupUnalignedABICall(4, JSContextReg);

    masm.loadJSContext(JSContextReg);

    masm.passABIArg(JSContextReg);
    masm.passABIArg(ObjectReg);
    masm.passABIArg(PrivateReg);
    masm.passABIArg(ValueReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ins->mir()->fun()));

    if (ins->mir()->isInfallible()) {
        masm.loadValue(Address(StackPointer, IonDOMExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    } else {
        masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

        masm.loadValue(Address(StackPointer, IonDOMExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    }
    masm.adjustStack(IonDOMExitFrameLayout::Size());

    JS_ASSERT(masm.framePushed() == initialStack);
    return true;
}

bool
CodeGenerator::visitSetDOMProperty(LSetDOMProperty *ins)
{
    const Register JSContextReg = ToRegister(ins->getJSContextReg());
    const Register ObjectReg = ToRegister(ins->getObjectReg());
    const Register PrivateReg = ToRegister(ins->getPrivReg());
    const Register ValueReg = ToRegister(ins->getValueReg());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    ValueOperand argVal = ToValue(ins, LSetDOMProperty::Value);
    masm.Push(argVal);
    
    
    JS_STATIC_ASSERT(sizeof(JSJitSetterCallArgs) == sizeof(Value*));
    masm.movePtr(StackPointer, ValueReg);

    masm.Push(ObjectReg);

    
    masm.loadPrivate(Address(ObjectReg, JSObject::getFixedSlotOffset(0)), PrivateReg);

    
    masm.movePtr(StackPointer, ObjectReg);

    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(JSContextReg, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(ION_FRAME_DOMSETTER);

    if (!markSafepointAt(safepointOffset, ins))
        return false;

    masm.setupUnalignedABICall(4, JSContextReg);

    masm.loadJSContext(JSContextReg);

    masm.passABIArg(JSContextReg);
    masm.passABIArg(ObjectReg);
    masm.passABIArg(PrivateReg);
    masm.passABIArg(ValueReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ins->mir()->fun()));

    masm.branchIfFalseBool(ReturnReg, masm.exceptionLabel());

    masm.adjustStack(IonDOMExitFrameLayout::Size());

    JS_ASSERT(masm.framePushed() == initialStack);
    return true;
}

typedef bool(*SPSFn)(JSContext *, HandleScript);
static const VMFunction SPSEnterInfo = FunctionInfo<SPSFn>(SPSEnter);
static const VMFunction SPSExitInfo = FunctionInfo<SPSFn>(SPSExit);

bool
CodeGenerator::visitFunctionBoundary(LFunctionBoundary *lir)
{
    Register temp = ToRegister(lir->temp()->output());

    switch (lir->type()) {
        case MFunctionBoundary::Inline_Enter:
            
            
            
            
            
            
            
            
            
            if (sps_.inliningDepth() == lir->inlineLevel()) {
                sps_.leaveInlineFrame();
                sps_.skipNextReenter();
                sps_.reenter(masm, temp);
            }

            sps_.leave(masm, temp);
            if (!sps_.enterInlineFrame())
                return false;
            

        case MFunctionBoundary::Enter:
            if (sps_.slowAssertions()) {
                saveLive(lir);
                pushArg(ImmGCPtr(lir->script()));
                if (!callVM(SPSEnterInfo, lir))
                    return false;
                restoreLive(lir);
                sps_.pushManual(lir->script(), masm, temp);
                return true;
            }

            return sps_.push(GetIonContext()->cx, lir->script(), masm, temp);

        case MFunctionBoundary::Inline_Exit:
            
            
            
            sps_.leaveInlineFrame();
            sps_.reenter(masm, temp);
            return true;

        case MFunctionBoundary::Exit:
            if (sps_.slowAssertions()) {
                saveLive(lir);
                pushArg(ImmGCPtr(lir->script()));
                
                
                
                sps_.skipNextReenter();
                if (!callVM(SPSExitInfo, lir))
                    return false;
                restoreLive(lir);
                return true;
            }

            sps_.pop(masm, temp);
            return true;

        default:
            MOZ_ASSUME_UNREACHABLE("invalid LFunctionBoundary type");
    }
}

bool
CodeGenerator::visitOutOfLineAbortPar(OutOfLineAbortPar *ool)
{
    ParallelBailoutCause cause = ool->cause();
    jsbytecode *bytecode = ool->bytecode();

    masm.move32(Imm32(cause), CallTempReg0);
    loadOutermostJSScript(CallTempReg1);
    loadJSScriptForBlock(ool->basicBlock(), CallTempReg2);
    masm.movePtr(ImmPtr(bytecode), CallTempReg3);

    masm.setupUnalignedABICall(4, CallTempReg4);
    masm.passABIArg(CallTempReg0);
    masm.passABIArg(CallTempReg1);
    masm.passABIArg(CallTempReg2);
    masm.passABIArg(CallTempReg3);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, AbortPar));

    masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    masm.jump(&returnLabel_);
    return true;
}

bool
CodeGenerator::visitIsCallable(LIsCallable *ins)
{
    Register object = ToRegister(ins->object());
    Register output = ToRegister(ins->output());

    masm.loadObjClass(object, output);

    
    Label notFunction, done;
    masm.branchPtr(Assembler::NotEqual, output, ImmPtr(&JSFunction::class_), &notFunction);
    masm.move32(Imm32(1), output);
    masm.jump(&done);

    masm.bind(&notFunction);
    masm.cmpPtr(Address(output, offsetof(js::Class, call)), ImmPtr(nullptr));
    masm.emitSet(Assembler::NonZero, output);
    masm.bind(&done);

    return true;
}

void
CodeGenerator::loadOutermostJSScript(Register reg)
{
    
    
    

    MIRGraph &graph = current->mir()->graph();
    MBasicBlock *entryBlock = graph.entryBlock();
    masm.movePtr(ImmGCPtr(entryBlock->info().script()), reg);
}

void
CodeGenerator::loadJSScriptForBlock(MBasicBlock *block, Register reg)
{
    
    

    JSScript *script = block->info().script();
    masm.movePtr(ImmGCPtr(script), reg);
}

bool
CodeGenerator::visitOutOfLinePropagateAbortPar(OutOfLinePropagateAbortPar *ool)
{
    loadOutermostJSScript(CallTempReg0);
    loadJSScriptForBlock(ool->lir()->mirRaw()->block(), CallTempReg1);

    masm.setupUnalignedABICall(2, CallTempReg2);
    masm.passABIArg(CallTempReg0);
    masm.passABIArg(CallTempReg1);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, PropagateAbortPar));

    masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    masm.jump(&returnLabel_);
    return true;
}

bool
CodeGenerator::visitHaveSameClass(LHaveSameClass *ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register temp = ToRegister(ins->getTemp(0));
    Register output = ToRegister(ins->output());

    masm.loadObjClass(lhs, temp);
    masm.loadObjClass(rhs, output);
    masm.cmpPtr(temp, output);
    masm.emitSet(Assembler::Equal, output);

    return true;
}

bool
CodeGenerator::visitAsmJSCall(LAsmJSCall *ins)
{
    MAsmJSCall *mir = ins->mir();

#if defined(JS_CPU_ARM) && !defined(JS_CPU_ARM_HARDFP)
    for (unsigned i = 0, e = ins->numOperands(); i < e; i++) {
        LAllocation *a = ins->getOperand(i);
        if (a->isFloatReg()) {
            FloatRegister fr = ToFloatRegister(a);
            int srcId = fr.code() * 2;
            masm.ma_vxfer(fr, Register::FromCode(srcId), Register::FromCode(srcId+1));
        }
    }
#endif
   if (mir->spIncrement())
        masm.freeStack(mir->spIncrement());

   JS_ASSERT((AlignmentAtPrologue +  masm.framePushed()) % StackAlignment == 0);

#ifdef DEBUG
    Label ok;
    JS_ASSERT(IsPowerOfTwo(StackAlignment));
    masm.branchTestPtr(Assembler::Zero, StackPointer, Imm32(StackAlignment - 1), &ok);
    masm.breakpoint();
    masm.bind(&ok);
#endif

    MAsmJSCall::Callee callee = mir->callee();
    switch (callee.which()) {
      case MAsmJSCall::Callee::Internal:
        masm.call(callee.internal());
        break;
      case MAsmJSCall::Callee::Dynamic:
        masm.call(ToRegister(ins->getOperand(mir->dynamicCalleeOperandIndex())));
        break;
      case MAsmJSCall::Callee::Builtin:
        masm.call(callee.builtin());
        break;
    }

    if (mir->spIncrement())
        masm.reserveStack(mir->spIncrement());

    postAsmJSCall(ins);
    return true;
}

bool
CodeGenerator::visitAsmJSParameter(LAsmJSParameter *lir)
{
#if defined(JS_CPU_ARM) && !defined(JS_CPU_ARM_HARDFP)
    
    
    LAllocation *a = lir->getDef(0)->output();
    if (a->isFloatReg()) {
        FloatRegister fr = ToFloatRegister(a);
        int srcId = fr.code() * 2;
        masm.ma_vxfer(Register::FromCode(srcId), Register::FromCode(srcId+1), fr);
    }
#endif
    return true;
}

bool
CodeGenerator::visitAsmJSReturn(LAsmJSReturn *lir)
{
    
#if defined(JS_CPU_ARM) && !defined(JS_CPU_ARM_HARDFP)
    if (lir->getOperand(0)->isFloatReg())
        masm.ma_vxfer(d0, r0, r1);
#endif
    if (current->mir() != *gen->graph().poBegin())
        masm.jump(&returnLabel_);
    return true;
}

bool
CodeGenerator::visitAsmJSVoidReturn(LAsmJSVoidReturn *lir)
{
    
    if (current->mir() != *gen->graph().poBegin())
        masm.jump(&returnLabel_);
    return true;
}

bool
CodeGenerator::visitAsmJSCheckOverRecursed(LAsmJSCheckOverRecursed *lir)
{
    masm.branchPtr(Assembler::AboveOrEqual,
                   AsmJSAbsoluteAddress(AsmJSImm_StackLimit),
                   StackPointer,
                   lir->mir()->onError());
    return true;
}

bool
CodeGenerator::emitAssertRangeI(const Range *r, Register input)
{
    
    if (r->hasInt32LowerBound() && r->lower() > INT32_MIN) {
        Label success;
        masm.branch32(Assembler::GreaterThanOrEqual, input, Imm32(r->lower()), &success);
        masm.breakpoint();
        masm.bind(&success);
    }

    
    if (r->hasInt32UpperBound() && r->upper() < INT32_MAX) {
        Label success;
        masm.branch32(Assembler::LessThanOrEqual, input, Imm32(r->upper()), &success);
        masm.breakpoint();
        masm.bind(&success);
    }

    
    
    

    return true;
}

bool
CodeGenerator::emitAssertRangeD(const Range *r, FloatRegister input, FloatRegister temp)
{
    
    if (r->hasInt32LowerBound()) {
        Label success;
        masm.loadConstantDouble(r->lower(), temp);
        if (!r->hasInt32UpperBound())
            masm.branchDouble(Assembler::DoubleUnordered, input, input, &success);
        masm.branchDouble(Assembler::DoubleGreaterThanOrEqual, input, temp, &success);
        masm.breakpoint();
        masm.bind(&success);
    }
    
    if (r->hasInt32UpperBound()) {
        Label success;
        masm.loadConstantDouble(r->upper(), temp);
        if (!r->hasInt32LowerBound())
            masm.branchDouble(Assembler::DoubleUnordered, input, input, &success);
        masm.branchDouble(Assembler::DoubleLessThanOrEqual, input, temp, &success);
        masm.breakpoint();
        masm.bind(&success);
    }

    
    

    if (!r->hasInt32Bounds() && !r->canBeInfiniteOrNaN() && r->exponent() < DoubleExponentBias) {
        
        Label exponentLoOk;
        masm.loadConstantDouble(pow(2.0, r->exponent() + 1), temp);
        masm.branchDouble(Assembler::DoubleUnordered, input, input, &exponentLoOk);
        masm.branchDouble(Assembler::DoubleLessThanOrEqual, input, temp, &exponentLoOk);
        masm.breakpoint();
        masm.bind(&exponentLoOk);

        Label exponentHiOk;
        masm.loadConstantDouble(-pow(2.0, r->exponent() + 1), temp);
        masm.branchDouble(Assembler::DoubleUnordered, input, input, &exponentHiOk);
        masm.branchDouble(Assembler::DoubleGreaterThanOrEqual, input, temp, &exponentHiOk);
        masm.breakpoint();
        masm.bind(&exponentHiOk);
    } else if (!r->hasInt32Bounds() && !r->canBeNaN()) {
        
        Label notnan;
        masm.branchDouble(Assembler::DoubleOrdered, input, input, &notnan);
        masm.breakpoint();
        masm.bind(&notnan);

        
        if (!r->canBeInfiniteOrNaN()) {
            Label notposinf;
            masm.loadConstantDouble(PositiveInfinity(), temp);
            masm.branchDouble(Assembler::DoubleLessThan, input, temp, &notposinf);
            masm.breakpoint();
            masm.bind(&notposinf);

            Label notneginf;
            masm.loadConstantDouble(NegativeInfinity(), temp);
            masm.branchDouble(Assembler::DoubleGreaterThan, input, temp, &notneginf);
            masm.breakpoint();
            masm.bind(&notneginf);
        }
    }

    return true;
}

bool
CodeGenerator::visitAssertRangeI(LAssertRangeI *ins)
{
    Register input = ToRegister(ins->input());
    const Range *r = ins->range();

    return emitAssertRangeI(r, input);
}

bool
CodeGenerator::visitAssertRangeD(LAssertRangeD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister temp = ToFloatRegister(ins->temp());
    const Range *r = ins->range();

    return emitAssertRangeD(r, input, temp);
}

bool
CodeGenerator::visitAssertRangeF(LAssertRangeF *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister temp = ToFloatRegister(ins->temp());
    const Range *r = ins->range();

    masm.convertFloatToDouble(input, input);
    bool success = emitAssertRangeD(r, input, temp);
    masm.convertDoubleToFloat(input, input);
    return success;
}

bool
CodeGenerator::visitAssertRangeV(LAssertRangeV *ins)
{
    const Range *r = ins->range();
    const ValueOperand value = ToValue(ins, LAssertRangeV::Input);
    Register tag = masm.splitTagForTest(value);
    Label done;

    {
        Label isNotInt32;
        masm.branchTestInt32(Assembler::NotEqual, tag, &isNotInt32);
        Register unboxInt32 = ToTempUnboxRegister(ins->temp());
        Register input = masm.extractInt32(value, unboxInt32);
        emitAssertRangeI(r, input);
        masm.jump(&done);
        masm.bind(&isNotInt32);
    }

    {
        Label isNotDouble;
        masm.branchTestDouble(Assembler::NotEqual, tag, &isNotDouble);
        FloatRegister input = ToFloatRegister(ins->floatTemp1());
        FloatRegister temp = ToFloatRegister(ins->floatTemp2());
        masm.unboxDouble(value, input);
        emitAssertRangeD(r, input, temp);
        masm.jump(&done);
        masm.bind(&isNotDouble);
    }

    masm.breakpoint();
    masm.bind(&done);
    return true;
}

} 
} 
