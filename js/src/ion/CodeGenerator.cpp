






#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Util.h"

#include "CodeGenerator.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "MIRGenerator.h"
#include "shared/CodeGenerator-shared-inl.h"
#include "jsnum.h"
#include "jsmath.h"
#include "jsinterpinlines.h"
#include "ParallelFunctions.h"
#include "ExecutionModeInlines.h"
#include "builtin/Eval.h"
#include "vm/ForkJoin.h"

#include "vm/StringObject-inl.h"

using namespace js;
using namespace js::ion;

using mozilla::DebugOnly;
using mozilla::Maybe;

namespace js {
namespace ion {



class OutOfLineUpdateCache :
  public OutOfLineCodeBase<CodeGenerator>,
  public IonCacheVisitor
{
  private:
    LInstruction *lir_;
    RepatchLabel repatchEntry_;
    size_t cacheIndex_;

  public:
    OutOfLineUpdateCache(LInstruction *lir, size_t cacheIndex)
      : lir_(lir),
        cacheIndex_(cacheIndex)
    { }

    void bind(MacroAssembler *masm) {
        masm->bind(&repatchEntry_);
    }

    size_t getCacheIndex() const {
        return cacheIndex_;
    }
    LInstruction *lir() const {
        return lir_;
    }
    RepatchLabel *repatchEntry() {
        return &repatchEntry_;
    }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineCache(this);
    }

    
#define VISIT_CACHE_FUNCTION(op)                                \
    bool visit##op##IC(CodeGenerator *codegen, op##IC *ic) {    \
        return codegen->visit##op##IC(this, ic);                \
    }

    IONCACHE_KIND_LIST(VISIT_CACHE_FUNCTION)
#undef VISIT_CACHE_FUNCTION
};





bool
CodeGeneratorShared::addCache(LInstruction *lir, size_t cacheIndex)
{
    IonCache *cache = static_cast<IonCache *>(getCache(cacheIndex));
    MInstruction *mir = lir->mirRaw()->toInstruction();
    if (mir->resumePoint())
        cache->setScriptedLocation(mir->block()->info().script(),
                                   mir->resumePoint()->pc());
    else
        cache->setIdempotent();

    OutOfLineUpdateCache *ool = new OutOfLineUpdateCache(lir, cacheIndex);
    if (!addOutOfLineCode(ool))
        return false;

    CodeOffsetJump jump = masm.jumpWithPatch(ool->repatchEntry());
    CodeOffsetLabel label = masm.labelForPatch();
    masm.bind(ool->rejoin());

    cache->setInlineJump(jump, label);
    return true;
}

bool
CodeGenerator::visitOutOfLineCache(OutOfLineUpdateCache *ool)
{
    size_t cacheIndex = ool->getCacheIndex();
    IonCache *cache = static_cast<IonCache *>(getCache(cacheIndex));

    
    cache->setFallbackLabel(masm.labelForPatch());

    
    return cache->accept(this, ool);
}

StringObject *
MNewStringObject::templateObj() const {
    return &templateObj_->asString();
}

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph *graph)
  : CodeGeneratorSpecific(gen, graph)
{
}

bool
CodeGenerator::visitValueToInt32(LValueToInt32 *lir)
{
    ValueOperand operand = ToValue(lir, LValueToInt32::Input);
    Register output = ToRegister(lir->output());

    Register tag = masm.splitTagForTest(operand);

    Label done, simple, isInt32, isBool, notDouble;
    
    masm.branchTestInt32(Assembler::Equal, tag, &isInt32);
    masm.branchTestBoolean(Assembler::Equal, tag, &isBool);
    masm.branchTestDouble(Assembler::NotEqual, tag, &notDouble);

    
    
    FloatRegister temp = ToFloatRegister(lir->tempFloat());
    masm.unboxDouble(operand, temp);

    Label fails;
    switch (lir->mode()) {
      case LValueToInt32::TRUNCATE:
        if (!emitTruncateDouble(temp, output))
            return false;
        break;
      default:
        JS_ASSERT(lir->mode() == LValueToInt32::NORMAL);
        masm.convertDoubleToInt32(temp, output, &fails, lir->mir()->canBeNegativeZero());
        break;
    }
    masm.jump(&done);

    masm.bind(&notDouble);

    if (lir->mode() == LValueToInt32::NORMAL) {
        
        
        masm.branchTestNull(Assembler::NotEqual, tag, &fails);
    } else {
        
        
        masm.branchTestObject(Assembler::Equal, tag, &fails);
        masm.branchTestString(Assembler::Equal, tag, &fails);
    }

    if (fails.used() && !bailoutFrom(&fails, lir->snapshot()))
        return false;

    
    masm.mov(Imm32(0), output);
    masm.jump(&done);

    
    masm.bind(&isBool);
    masm.unboxBoolean(operand, output);
    masm.jump(&done);

    
    masm.bind(&isInt32);
    masm.unboxInt32(operand, output);

    masm.bind(&done);

    return true;
}

static const double DoubleZero = 0.0;

bool
CodeGenerator::visitValueToDouble(LValueToDouble *lir)
{
    ValueOperand operand = ToValue(lir, LValueToDouble::Input);
    FloatRegister output = ToFloatRegister(lir->output());

    Register tag = masm.splitTagForTest(operand);

    Label isDouble, isInt32, isBool, isNull, done;
    
    masm.branchTestDouble(Assembler::Equal, tag, &isDouble);
    masm.branchTestInt32(Assembler::Equal, tag, &isInt32);
    masm.branchTestBoolean(Assembler::Equal, tag, &isBool);
    masm.branchTestNull(Assembler::Equal, tag, &isNull);

    Assembler::Condition cond = masm.testUndefined(Assembler::NotEqual, tag);
    if (!bailoutIf(cond, lir->snapshot()))
        return false;
    masm.loadStaticDouble(&js_NaN, output);
    masm.jump(&done);

    masm.bind(&isNull);
    masm.loadStaticDouble(&DoubleZero, output);
    masm.jump(&done);

    masm.bind(&isBool);
    masm.boolValueToDouble(operand, output);
    masm.jump(&done);

    masm.bind(&isInt32);
    masm.int32ValueToDouble(operand, output);
    masm.jump(&done);

    masm.bind(&isDouble);
    masm.unboxDouble(operand, output);
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

void
CodeGenerator::emitOOLTestObject(Register objreg, Label *ifTruthy, Label *ifFalsy, Register scratch)
{
    saveVolatile(scratch);
    masm.setupUnalignedABICall(1, scratch);
    masm.passABIArg(objreg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ObjectEmulatesUndefined));
    masm.storeCallResult(scratch);
    restoreVolatile(scratch);

    masm.branchTest32(Assembler::NonZero, scratch, scratch, ifFalsy);
    masm.jump(ifTruthy);
}









class OutOfLineTestObject : public OutOfLineCodeBase<CodeGenerator>
{
    Register objreg_;
    Register scratch_;

    Label *ifTruthy_;
    Label *ifFalsy_;

#ifdef DEBUG
    bool initialized() { return ifTruthy_ != NULL; }
#endif

  public:
    OutOfLineTestObject()
#ifdef DEBUG
      : ifTruthy_(NULL), ifFalsy_(NULL)
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

    
    
    
    
    
    
    
    masm.loadObjClass(objreg, scratch);

    Label *outOfLineTest = ool->entry();
    masm.branchPtr(Assembler::Equal, scratch, ImmWord(&ObjectProxyClass), outOfLineTest);
    masm.branchPtr(Assembler::Equal, scratch, ImmWord(&OuterWindowProxyClass), outOfLineTest);
    masm.branchPtr(Assembler::Equal, scratch, ImmWord(&FunctionProxyClass), outOfLineTest);

    masm.branchTest32(Assembler::Zero, Address(scratch, Class::offsetOfFlags()),
                      Imm32(JSCLASS_EMULATES_UNDEFINED), ifTruthy);
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

bool
CodeGenerator::visitTestOAndBranch(LTestOAndBranch *lir)
{
    MOZ_ASSERT(lir->mir()->operandMightEmulateUndefined(),
               "Objects which can't emulate undefined should have been constant-folded");

    OutOfLineTestObject *ool = new OutOfLineTestObject();
    if (!addOutOfLineCode(ool))
        return false;

    testObjectTruthy(ToRegister(lir->input()), lir->ifTruthy(), lir->ifFalsy(),
                     ToRegister(lir->temp()), ool);
    return true;

}

bool
CodeGenerator::visitTestVAndBranch(LTestVAndBranch *lir)
{
    OutOfLineTestObject *ool = NULL;
    if (lir->mir()->operandMightEmulateUndefined()) {
        ool = new OutOfLineTestObject();
        if (!addOutOfLineCode(ool))
            return false;
    }

    testValueTruthy(ToValue(lir, LTestVAndBranch::Input),
                    lir->temp1(), lir->temp2(),
                    ToFloatRegister(lir->tempFloat()),
                    lir->ifTruthy(), lir->ifFalsy(), ool);
    return true;
}

bool
CodeGenerator::visitPolyInlineDispatch(LPolyInlineDispatch *lir)
{
    MPolyInlineDispatch *mir = lir->mir();
    Register inputReg = ToRegister(lir->input());

    InlinePropertyTable *inlinePropTable = mir->inlinePropertyTable();
    if (inlinePropTable) {
        
        Register tempReg = ToRegister(lir->temp());
        masm.loadPtr(Address(inputReg, JSObject::offsetOfType()), tempReg);

        
        for (size_t i = 0; i < inlinePropTable->numEntries(); i++) {
            types::TypeObject *typeObj = inlinePropTable->getTypeObject(i);
            JSFunction *func = inlinePropTable->getFunction(i);
            LBlock *target = mir->getFunctionBlock(func)->lir();
            masm.branchPtr(Assembler::Equal, tempReg, ImmGCPtr(typeObj), target->label());
        }

        
        LBlock *fallback = mir->fallbackPrepBlock()->lir();
        masm.jump(fallback->label());
        return true;
    }

    
    for (size_t i = 0; i < mir->numCallees() - 1; i++) {
        JSFunction *func = mir->getFunction(i);
        LBlock *target = mir->getFunctionBlock(i)->lir();
        masm.branchPtr(Assembler::Equal, inputReg, ImmGCPtr(func), target->label());
    }

    
    LBlock *target = mir->getFunctionBlock(mir->numCallees() - 1)->lir();
    masm.jump(target->label());
    return true;
}

typedef JSFlatString *(*IntToStringFn)(JSContext *, int);
static const VMFunction IntToStringInfo =
    FunctionInfo<IntToStringFn>(Int32ToString<CanGC>);

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

    masm.movePtr(ImmWord(&gen->compartment->rt->staticStrings.intStaticTable), output);
    masm.loadPtr(BaseIndex(output, input, ScalePointer), output);

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
                                HandleString input, JSBool *result);
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
    pushArg(ImmGCPtr(lir->mir()->fun()));
    return callVM(LambdaInfo, lir);
}

bool
CodeGenerator::visitLambda(LLambda *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());
    Register output = ToRegister(lir->output());
    JSFunction *fun = lir->mir()->fun();

    OutOfLineCode *ool = oolCallVM(LambdaInfo, lir, (ArgList(), ImmGCPtr(fun), scopeChain),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    JS_ASSERT(gen->compartment == fun->compartment());
    JS_ASSERT(!fun->hasSingletonType());

    masm.newGCThing(output, fun, ool->entry());
    masm.initGCThing(output, fun);

    emitLambdaInit(output, scopeChain, fun);

    masm.bind(ool->rejoin());
    return true;
}

void
CodeGenerator::emitLambdaInit(const Register &output,
                              const Register &scopeChain,
                              JSFunction *fun)
{
    
    
    union {
        struct S {
            uint16_t nargs;
            uint16_t flags;
        } s;
        uint32_t word;
    } u;
    u.s.nargs = fun->nargs;
    u.s.flags = fun->flags & ~JSFunction::EXTENDED;

    JS_STATIC_ASSERT(offsetof(JSFunction, flags) == offsetof(JSFunction, nargs) + 2);
    masm.store32(Imm32(u.word), Address(output, offsetof(JSFunction, nargs)));
    masm.storePtr(ImmGCPtr(fun->nonLazyScript()),
                  Address(output, JSFunction::offsetOfNativeOrScript()));
    masm.storePtr(scopeChain, Address(output, JSFunction::offsetOfEnvironment()));
    masm.storePtr(ImmGCPtr(fun->displayAtom()), Address(output, JSFunction::offsetOfAtom()));
}

bool
CodeGenerator::visitParLambda(LParLambda *lir)
{
    Register resultReg = ToRegister(lir->output());
    Register parSliceReg = ToRegister(lir->parSlice());
    Register scopeChainReg    = ToRegister(lir->scopeChain());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    JSFunction *fun = lir->mir()->fun();

    JS_ASSERT(scopeChainReg != resultReg);

    emitParAllocateGCThing(resultReg, parSliceReg, tempReg1, tempReg2, fun);
    emitLambdaInit(resultReg, scopeChainReg, fun);
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
    return true;
}

bool
CodeGenerator::visitGoto(LGoto *lir)
{
    LBlock *target = lir->target()->lir();

    
    if (isNextBlock(target))
        return true;

    masm.jump(target->label());
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
        masm.jump(returnLabel_);
    return true;
}

bool
CodeGenerator::visitOsrEntry(LOsrEntry *lir)
{
    
    masm.flushBuffer();
    setOsrEntryOffset(masm.size());

    
    masm.subPtr(Imm32(frameSize()), StackPointer);
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
        masm.movePtr(ImmWord(lir->ptr()), ToRegister(lir->output()));
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

    Address convertedAddress(elements, ObjectElements::offsetOfConvertDoubleElements());
    masm.branch32(Assembler::Equal, convertedAddress, Imm32(0), ool->entry());
    masm.bind(ool->rejoin());
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
CodeGenerator::visitParSlice(LParSlice *lir)
{
    const Register tempReg = ToRegister(lir->getTempReg());

    masm.setupUnalignedABICall(0, tempReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParForkJoinSlice));
    JS_ASSERT(ToRegister(lir->output()) == ReturnReg);
    return true;
}

bool
CodeGenerator::visitParWriteGuard(LParWriteGuard *lir)
{
    JS_ASSERT(gen->info().executionMode() == ParallelExecution);

    const Register tempReg = ToRegister(lir->getTempReg());
    masm.setupUnalignedABICall(2, tempReg);
    masm.passABIArg(ToRegister(lir->parSlice()));
    masm.passABIArg(ToRegister(lir->object()));
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParWriteGuard));

    Label *bail;
    if (!ensureOutOfLineParallelAbort(&bail))
        return false;

    
    masm.branchTestBool(Assembler::Zero, ReturnReg, ReturnReg, bail);

    return true;
}

bool
CodeGenerator::visitParDump(LParDump *lir)
{
    ValueOperand value = ToValue(lir, 0);
    masm.reserveStack(sizeof(Value));
    masm.storeValue(value, Address(StackPointer, 0));
    masm.movePtr(StackPointer, CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParDumpValue));
    masm.freeStack(sizeof(Value));
    return true;
}

bool
CodeGenerator::visitTypeBarrier(LTypeBarrier *lir)
{
    ValueOperand operand = ToValue(lir, LTypeBarrier::Input);
    Register scratch = ToRegister(lir->temp());

    Label matched, miss;
    masm.guardTypeSet(operand, lir->mir()->typeSet(), scratch, &matched, &miss);
    masm.jump(&miss);
    if (!bailoutFrom(&miss, lir->snapshot()))
        return false;
    masm.bind(&matched);
    return true;
}

bool
CodeGenerator::visitMonitorTypes(LMonitorTypes *lir)
{
    ValueOperand operand = ToValue(lir, LMonitorTypes::Input);
    Register scratch = ToRegister(lir->temp());

    Label matched, miss;
    masm.guardTypeSet(operand, lir->mir()->typeSet(), scratch, &matched, &miss);
    masm.jump(&miss);
    if (!bailoutFrom(&miss, lir->snapshot()))
        return false;
    masm.bind(&matched);
    return true;
}

bool
CodeGenerator::visitExcludeType(LExcludeType *lir)
{
    ValueOperand operand = ToValue(lir, LExcludeType::Input);
    Register scratch = ToRegister(lir->temp());

    Label matched, miss;
    masm.guardType(operand, lir->mir()->type(), scratch, &matched, &miss);
    if (matched.used() && !bailoutFrom(&matched, lir->snapshot()))
        return false;
    masm.bind(&miss);
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

    
    const Register argJSContextReg = ToRegister(call->getArgJSContextReg());
    const Register argUintNReg     = ToRegister(call->getArgUintNReg());
    const Register argVpReg        = ToRegister(call->getArgVpReg());

    
    const Register tempReg = ToRegister(call->getTempReg());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    
    

    
    masm.adjustStack(unusedStack);

    
    
    masm.Push(ObjectValue(*target));

    
    masm.loadJSContext(argJSContextReg);
    masm.move32(Imm32(call->numStackArgs()), argUintNReg);
    masm.movePtr(StackPointer, argVpReg);

    masm.Push(argUintNReg);

    
    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(tempReg, &safepointOffset))
        return false;
    masm.enterFakeExitFrame();

    if (!markSafepointAt(safepointOffset, call))
        return false;

    
    masm.setupUnalignedABICall(3, tempReg);
    masm.passABIArg(argJSContextReg);
    masm.passABIArg(argUintNReg);
    masm.passABIArg(argVpReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->native()));

    
    Label success, exception;
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

    
    masm.loadValue(Address(StackPointer, IonNativeExitFrameLayout::offsetOfResult()), JSReturnOperand);
    masm.jump(&success);

    
    {
        masm.bind(&exception);
        masm.handleException();
    }
    masm.bind(&success);

    
    

    
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
    const Register argArgc      = ToRegister(call->getArgArgc());
    const Register argVp        = ToRegister(call->getArgVp());

    DebugOnly<uint32_t> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    
    

    
    
    masm.adjustStack(unusedStack);
    
    Register obj = masm.extractObject(Address(StackPointer, 0), argObj);
    JS_ASSERT(obj == argObj);

    
    
    masm.Push(ObjectValue(*target));
    masm.movePtr(StackPointer, argVp);

    
    masm.loadPrivate(Address(obj, JSObject::getFixedSlotOffset(0)), argPrivate);

    
    masm.move32(Imm32(call->numStackArgs()), argArgc);
    
    masm.Push(argArgc);

    
    
    
    masm.Push(argObj);
    masm.movePtr(StackPointer, argObj);

    
    uint32_t safepointOffset;
    if (!masm.buildFakeExitFrame(argJSContext, &safepointOffset))
        return false;
    masm.enterFakeExitFrame(ION_FRAME_DOMMETHOD);

    if (!markSafepointAt(safepointOffset, call))
        return false;

    
    masm.setupUnalignedABICall(5, argJSContext);

    masm.loadJSContext(argJSContext);

    masm.passABIArg(argJSContext);
    masm.passABIArg(argObj);
    masm.passABIArg(argPrivate);
    masm.passABIArg(argArgc);
    masm.passABIArg(argVp);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->jitInfo()->op));

    if (target->jitInfo()->isInfallible) {
        masm.loadValue(Address(StackPointer, IonDOMMethodExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
    } else {
        
        Label success, exception;
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

        
        masm.loadValue(Address(StackPointer, IonDOMMethodExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
        masm.jump(&success);

        
        {
            masm.bind(&exception);
            masm.handleException();
        }
        masm.bind(&success);
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
    
    switch (gen->info().executionMode()) {
      case SequentialExecution: {
        pushArg(ImmGCPtr(lir->mir()->name()));
        return callVM(GetIntrinsicValueInfo, lir);
      }

      case ParallelExecution: {
        Label *bail;
        if (!ensureOutOfLineParallelAbort(&bail))
            return false;

        masm.jump(bail);
        return true;
      }

      default:
        JS_NOT_REACHED("Bad execution mode");
        return false;
    }
}

typedef bool (*InvokeFunctionFn)(JSContext *, HandleFunction, uint32_t, Value *, Value *);
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

static inline int32_t ionOffset(ExecutionMode executionMode)
{
    switch (executionMode) {
      case SequentialExecution: return offsetof(JSScript, ion);
      case ParallelExecution: return offsetof(JSScript, parallelIon);
    }

    JS_ASSERT(false);
    return offsetof(JSScript, ion);
}

bool
CodeGenerator::visitCallGeneric(LCallGeneric *call)
{
    Register calleereg = ToRegister(call->getFunction());
    Register objreg    = ToRegister(call->getTempObject());
    Register nargsreg  = ToRegister(call->getNargsReg());
    uint32_t unusedStack = StackOffsetOfPassedArg(call->argslot());
    ExecutionMode executionMode = gen->info().executionMode();
    Label uncompiled, thunk, makeCall, end;

    
    JS_ASSERT(!call->hasSingleTarget());

    
    IonCompartment *ion = gen->ionCompartment();
    IonCode *argumentsRectifier = ion->getArgumentsRectifier();

    masm.checkStackAlignment();

    
    masm.loadObjClass(calleereg, nargsreg);
    masm.cmpPtr(nargsreg, ImmWord(&js::FunctionClass));
    if (!bailoutIf(Assembler::NotEqual, call->snapshot()))
        return false;

    
    masm.branchIfFunctionHasNoScript(calleereg, &uncompiled);

    
    masm.loadPtr(Address(calleereg, offsetof(JSFunction, u.i.script_)), objreg);
    masm.loadPtr(Address(objreg, ionOffset(executionMode)), objreg);

    
    masm.branchPtr(Assembler::BelowOrEqual, objreg, ImmWord(ION_COMPILING_SCRIPT), &uncompiled);

    
    masm.freeStack(unusedStack);

    
    uint32_t descriptor = MakeFrameDescriptor(masm.framePushed(), IonFrame_OptimizedJS);
    masm.Push(Imm32(call->numActualArgs()));
    masm.Push(calleereg);
    masm.Push(Imm32(descriptor));

    
    masm.load16ZeroExtend(Address(calleereg, offsetof(JSFunction, nargs)), nargsreg);
    masm.cmp32(nargsreg, Imm32(call->numStackArgs()));
    masm.j(Assembler::Above, &thunk);

    
    masm.loadPtr(Address(objreg, IonScript::offsetOfMethod()), objreg);
    masm.loadPtr(Address(objreg, IonCode::offsetOfCode()), objreg);
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

    
    masm.bind(&uncompiled);
    switch (executionMode) {
      case SequentialExecution:
        if (!emitCallInvokeFunction(call, calleereg, call->numActualArgs(), unusedStack))
            return false;
        break;

      case ParallelExecution:
        if (!emitParCallToUncompiledScript(calleereg))
            return false;
        break;
    }

    masm.bind(&end);

    
    
    if (call->mir()->isConstructing()) {
        Label notPrimitive;
        masm.branchTestPrimitive(Assembler::NotEqual, JSReturnOperand, &notPrimitive);
        masm.loadValue(Address(StackPointer, unusedStack), JSReturnOperand);
        masm.bind(&notPrimitive);
    }

    if (!checkForParallelBailout())
        return false;

    dropArguments(call->numStackArgs() + 1);
    return true;
}



bool
CodeGenerator::emitParCallToUncompiledScript(Register calleeReg)
{
    Label *bail;
    if (!ensureOutOfLineParallelAbort(&bail))
        return false;

    masm.movePtr(calleeReg, CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParCallToUncompiledScript));
    masm.jump(bail);
    return true;
}

bool
CodeGenerator::visitCallKnown(LCallKnown *call)
{
    JSContext *cx = GetIonContext()->cx;
    Register calleereg = ToRegister(call->getFunction());
    Register objreg    = ToRegister(call->getTempObject());
    uint32_t unusedStack = StackOffsetOfPassedArg(call->argslot());
    RootedFunction target(cx, call->getSingleTarget());
    ExecutionMode executionMode = gen->info().executionMode();
    Label end, uncompiled;

    
    JS_ASSERT(!target->isNative());
    
    JS_ASSERT(target->nargs <= call->numStackArgs());

    masm.checkStackAlignment();

    
    if (target->isInterpretedLazy() && !target->getOrCreateScript(cx))
        return false;

    
    
    RootedScript targetScript(cx, target->nonLazyScript());
    if (GetIonScript(targetScript, executionMode) == ION_DISABLED_SCRIPT) {
        if (executionMode == ParallelExecution)
            return false;

        if (!emitCallInvokeFunction(call, calleereg, call->numActualArgs(), unusedStack))
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

    
    masm.loadPtr(Address(calleereg, offsetof(JSFunction, u.i.script_)), objreg);
    masm.loadPtr(Address(objreg, ionOffset(executionMode)), objreg);

    
    masm.branchPtr(Assembler::BelowOrEqual, objreg, ImmWord(ION_COMPILING_SCRIPT), &uncompiled);

    
    masm.loadPtr(Address(objreg, IonScript::offsetOfMethod()), objreg);
    masm.loadPtr(Address(objreg, IonCode::offsetOfCode()), objreg);

    
    masm.freeStack(unusedStack);

    
    uint32_t descriptor = MakeFrameDescriptor(masm.framePushed(), IonFrame_OptimizedJS);
    masm.Push(Imm32(call->numActualArgs()));
    masm.Push(calleereg);
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
        if (!emitParCallToUncompiledScript(calleereg))
            return false;
        break;
    }

    masm.bind(&end);

    if (!checkForParallelBailout())
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
CodeGenerator::checkForParallelBailout()
{
    
    
    
    ExecutionMode executionMode = gen->info().executionMode();
    if (executionMode == ParallelExecution) {
        Label *bail;
        if (!ensureOutOfLineParallelAbort(&bail))
            return false;
        masm.branchTestMagic(Assembler::Equal, JSReturnOperand, bail);
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
    JSContext *cx = GetIonContext()->cx;

    
    Register calleereg = ToRegister(apply->getFunction());

    
    Register objreg = ToRegister(apply->getTempObject());
    Register copyreg = ToRegister(apply->getTempCopy());

    
    Register argcreg = ToRegister(apply->getArgc());

    
    if (!apply->hasSingleTarget()) {
        masm.loadObjClass(calleereg, objreg);
        masm.cmpPtr(objreg, ImmWord(&js::FunctionClass));
        if (!bailoutIf(Assembler::NotEqual, apply->snapshot()))
            return false;
    }

    
    emitPushArguments(apply, copyreg);

    masm.checkStackAlignment();

    
    ExecutionMode executionMode = gen->info().executionMode();
    if (apply->hasSingleTarget()) {
        RootedFunction target(cx, apply->getSingleTarget());
        if (!CanIonCompile(cx, target, executionMode)) {
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

    
    masm.loadPtr(Address(calleereg, offsetof(JSFunction, u.i.script_)), objreg);
    masm.loadPtr(Address(objreg, ionOffset(executionMode)), objreg);

    
    masm.branchPtr(Assembler::BelowOrEqual, objreg, ImmWord(ION_COMPILING_SCRIPT), &invoke);

    
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

        
        {
            masm.loadPtr(Address(objreg, IonScript::offsetOfMethod()), objreg);
            masm.loadPtr(Address(objreg, IonCode::offsetOfCode()), objreg);

            
            
            masm.jump(&rejoin);
        }

        
        {
            masm.bind(&underflow);

            
            IonCompartment *ion = gen->ionCompartment();
            IonCode *argumentsRectifier = ion->getArgumentsRectifier();

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

typedef bool (*DirectEvalFn)(JSContext *, HandleObject, HandleScript, HandleValue, HandleString,
                             MutableHandleValue);
static const VMFunction DirectEvalInfo =
    FunctionInfo<DirectEvalFn>(DirectEvalFromIon);

bool
CodeGenerator::visitCallDirectEval(LCallDirectEval *lir)
{
    Register scopeChain = ToRegister(lir->getScopeChain());
    Register string = ToRegister(lir->getString());

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

    
    JS_ASSERT(info.scopeChainSlot() == 0);
    static const uint32_t START_SLOT = 1;

    Label miss;
    for (uint32_t i = START_SLOT; i < CountArgSlots(info.fun()); i++) {
        
        MParameter *param = rp->getOperand(i)->toParameter();
        const types::TypeSet *types = param->typeSet();
        if (!types || types->unknown())
            continue;

        
        
        int32_t offset = ArgToStackOffset((i - START_SLOT) * sizeof(Value));
        Label matched;
        masm.guardTypeSet(Address(StackPointer, offset), types, temp, &matched, &miss);
        masm.jump(&miss);
        masm.bind(&matched);
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
    
    
    
    
    
    
    
    

    JSRuntime *rt = gen->compartment->rt;
    Register limitReg = ToRegister(lir->limitTemp());

    
    
    uintptr_t *limitAddr = &rt->mainThread.ionStackLimit;
    masm.loadPtr(AbsoluteAddress(limitAddr), limitReg);

    CheckOverRecursedFailure *ool = new CheckOverRecursedFailure(lir);
    if (!addOutOfLineCode(ool))
        return false;

    
    masm.branchPtr(Assembler::BelowOrEqual, StackPointer, limitReg, ool->entry());
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


class ParCheckOverRecursedFailure : public OutOfLineCodeBase<CodeGenerator>
{
    LParCheckOverRecursed *lir_;

  public:
    ParCheckOverRecursedFailure(LParCheckOverRecursed *lir)
      : lir_(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitParCheckOverRecursedFailure(this);
    }

    LParCheckOverRecursed *lir() const {
        return lir_;
    }
};

bool
CodeGenerator::visitParCheckOverRecursed(LParCheckOverRecursed *lir)
{
    
    
    
    
    
    

    Register parSliceReg = ToRegister(lir->parSlice());
    Register tempReg = ToRegister(lir->getTempReg());

    masm.loadPtr(Address(parSliceReg, offsetof(ForkJoinSlice, perThreadData)), tempReg);
    masm.loadPtr(Address(tempReg, offsetof(PerThreadData, ionStackLimit)), tempReg);

    
    ParCheckOverRecursedFailure *ool = new ParCheckOverRecursedFailure(lir);
    if (!addOutOfLineCode(ool))
        return false;
    masm.branchPtr(Assembler::BelowOrEqual, StackPointer, tempReg, ool->entry());
    masm.parCheckInterruptFlags(tempReg, ool->entry());
    masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitParCheckOverRecursedFailure(ParCheckOverRecursedFailure *ool)
{
    Label *bail;
    if (!ensureOutOfLineParallelAbort(&bail))
        return false;

    
    
    
    LParCheckOverRecursed *lir = ool->lir();
    Register tempReg = ToRegister(lir->getTempReg());
    RegisterSet saveSet(lir->safepoint()->liveRegs());
    saveSet.maybeTake(tempReg);

    masm.PushRegsInMask(saveSet);
    masm.movePtr(ToRegister(lir->parSlice()), CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParCheckOverRecursed));
    masm.movePtr(ReturnReg, tempReg);
    masm.PopRegsInMask(saveSet);
    masm.branchTestBool(Assembler::Zero, tempReg, tempReg, bail);
    masm.jump(ool->rejoin());

    return true;
}


class OutOfLineParCheckInterrupt : public OutOfLineCodeBase<CodeGenerator>
{
  public:
    LParCheckInterrupt *const lir;

    OutOfLineParCheckInterrupt(LParCheckInterrupt *lir)
      : lir(lir)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineParCheckInterrupt(this);
    }
};

bool
CodeGenerator::visitParCheckInterrupt(LParCheckInterrupt *lir)
{
    
    OutOfLineParCheckInterrupt *ool = new OutOfLineParCheckInterrupt(lir);
    if (!addOutOfLineCode(ool))
        return false;

    
    
    
    

    Register tempReg = ToRegister(lir->getTempReg());
    masm.parCheckInterruptFlags(tempReg, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineParCheckInterrupt(OutOfLineParCheckInterrupt *ool)
{
    Label *bail;
    if (!ensureOutOfLineParallelAbort(&bail))
        return false;

    
    
    
    LParCheckInterrupt *lir = ool->lir;
    Register tempReg = ToRegister(lir->getTempReg());
    RegisterSet saveSet(lir->safepoint()->liveRegs());
    saveSet.maybeTake(tempReg);

    masm.PushRegsInMask(saveSet);
    masm.movePtr(ToRegister(ool->lir->parSlice()), CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParCheckInterrupt));
    masm.movePtr(ReturnReg, tempReg);
    masm.PopRegsInMask(saveSet);
    masm.branchTestBool(Assembler::Zero, tempReg, tempReg, bail);
    masm.jump(ool->rejoin());

    return true;
}

IonScriptCounts *
CodeGenerator::maybeCreateScriptCounts()
{
    
    
    JSContext *cx = GetIonContext()->cx;
    if (!cx)
        return NULL;

    IonScriptCounts *counts = NULL;

    CompileInfo *outerInfo = &gen->info();
    RawScript script = outerInfo->script();

    if (cx->runtime->profilingScripts && !script->hasScriptCounts) {
        if (!script->initScriptCounts(cx))
            return NULL;
    }

    if (!script->hasScriptCounts)
        return NULL;

    counts = js_new<IonScriptCounts>();
    if (!counts || !counts->init(graph.numBlocks())) {
        js_delete(counts);
        return NULL;
    }

    script->addIonCounts(counts);

    for (size_t i = 0; i < graph.numBlocks(); i++) {
        MBasicBlock *block = graph.getBlock(i)->mir();

        
        
        
        MResumePoint *resume = block->entryResumePoint();
        while (resume->caller())
            resume = resume->caller();
        uint32_t offset = resume->pc() - script->code;
        JS_ASSERT(offset < script->length);

        if (!counts->block(i).init(block->id(), offset, block->numSuccessors()))
            return NULL;
        for (size_t j = 0; j < block->numSuccessors(); j++)
            counts->block(i).setSuccessor(j, block->getSuccessor(j)->id());
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
        instructionBytes(0), spillBytes(0), last(NULL), lastLength(0)
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

        
        printer.printf("[%s]\n", ins->opName());
    }

    ~ScriptCountBlockState()
    {
        masm.setPrinter(NULL);

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

        for (; iter != current->end(); iter++) {
            IonSpew(IonSpew_Codegen, "instruction %s", iter->opName());

            if (counts)
                blockCounts.ref().visitInstruction(*iter);

            if (iter->safepoint() && pushedArgumentSlots_.length()) {
                if (!markArgumentSlots(iter->safepoint()))
                    return false;
            }

            if (!callTraceLIR(i, *iter))
                return false;

            if (!iter->accept(this))
                return false;
        }
        if (masm.oom())
            return false;
    }

    JS_ASSERT(pushedArgumentSlots_.empty());
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
    types::TypeObject *type = templateObject->hasSingletonType() ? NULL : templateObject->type();

    pushArg(ImmGCPtr(type));
    pushArg(Imm32(lir->mir()->count()));

    if (!callVM(NewInitArrayInfo, lir))
        return false;

    if (ReturnReg != objReg)
        masm.movePtr(ReturnReg, objReg);

    restoreLive(lir);

    return true;
}

bool
CodeGenerator::visitNewSlots(LNewSlots *lir)
{
    Register temp1 = ToRegister(lir->temp1());
    Register temp2 = ToRegister(lir->temp2());
    Register temp3 = ToRegister(lir->temp3());
    Register output = ToRegister(lir->output());

    masm.mov(ImmWord(gen->compartment->rt), temp1);
    masm.mov(Imm32(lir->mir()->nslots()), temp2);

    masm.setupUnalignedABICall(2, temp3);
    masm.passABIArg(temp1);
    masm.passABIArg(temp2);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NewSlots));

    masm.testPtr(output, output);
    if (!bailoutIf(Assembler::Zero, lir->snapshot()))
        return false;

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

bool
CodeGenerator::visitNewObjectVMCall(LNewObject *lir)
{
    JS_ASSERT(gen->info().executionMode() == SequentialExecution);

    Register objReg = ToRegister(lir->output());

    JS_ASSERT(!lir->isCall());
    saveLive(lir);

    pushArg(ImmGCPtr(lir->mir()->templateObject()));
    if (!callVM(NewInitObjectInfo, lir))
        return false;

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

typedef js::DeclEnvObject *(*NewDeclEnvObjectFn)(JSContext *, HandleFunction);
static const VMFunction NewDeclEnvObjectInfo =
    FunctionInfo<NewDeclEnvObjectFn>(DeclEnvObject::createTemplateObject);

bool
CodeGenerator::visitNewDeclEnvObject(LNewDeclEnvObject *lir)
{
    Register obj = ToRegister(lir->output());
    JSObject *templateObj = lir->mir()->templateObj();
    CompileInfo &info = lir->mir()->block()->info();

    
    OutOfLineCode *ool = oolCallVM(NewDeclEnvObjectInfo, lir,
                                   (ArgList(), ImmGCPtr(info.fun())),
                                   StoreRegisterTo(obj));
    if (!ool)
        return false;

    masm.newGCThing(obj, templateObj, ool->entry());
    masm.initGCThing(obj, templateObj);
    masm.bind(ool->rejoin());
    return true;
}

typedef JSObject *(*NewCallObjectFn)(JSContext *, HandleShape,
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
                        (ArgList(), ImmGCPtr(templateObj->lastProperty()),
                                    ImmGCPtr(templateObj->type()),
                                    ToRegister(lir->slots())),
                        StoreRegisterTo(obj));
    } else {
        ool = oolCallVM(NewCallObjectInfo, lir,
                        (ArgList(), ImmGCPtr(templateObj->lastProperty()),
                                    ImmGCPtr(templateObj->type()),
                                    ImmWord((void *)NULL)),
                        StoreRegisterTo(obj));
    }
    if (!ool)
        return false;

    masm.newGCThing(obj, templateObj, ool->entry());
    masm.initGCThing(obj, templateObj);

    if (lir->slots()->isRegister())
        masm.storePtr(ToRegister(lir->slots()), Address(obj, JSObject::offsetOfSlots()));
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitParNewCallObject(LParNewCallObject *lir)
{
    Register resultReg = ToRegister(lir->output());
    Register parSliceReg = ToRegister(lir->parSlice());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    JSObject *templateObj = lir->mir()->templateObj();

    emitParAllocateGCThing(resultReg, parSliceReg, tempReg1, tempReg2, templateObj);

    
    
    

    if (lir->slots()->isRegister()) {
        Register slotsReg = ToRegister(lir->slots());
        JS_ASSERT(slotsReg != resultReg);
        masm.storePtr(slotsReg, Address(resultReg, JSObject::offsetOfSlots()));
    }

    return true;
}

bool
CodeGenerator::visitParNewDenseArray(LParNewDenseArray *lir)
{
    Register parSliceReg = ToRegister(lir->parSlice());
    Register lengthReg = ToRegister(lir->length());
    Register tempReg0 = ToRegister(lir->getTemp0());
    Register tempReg1 = ToRegister(lir->getTemp1());
    Register tempReg2 = ToRegister(lir->getTemp2());
    JSObject *templateObj = lir->mir()->templateObject();

    
    
    emitParAllocateGCThing(tempReg2, parSliceReg, tempReg0, tempReg1, templateObj);

    
    
    
    
    
    
    
    
    masm.setupUnalignedABICall(3, CallTempReg3);
    masm.passABIArg(parSliceReg);
    masm.passABIArg(tempReg2);
    masm.passABIArg(lengthReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParExtendArray));

    Register resultReg = ToRegister(lir->output());
    JS_ASSERT(resultReg == ReturnReg);
    Label *bail;
    if (!ensureOutOfLineParallelAbort(&bail))
        return false;
    masm.branchTestPtr(Assembler::Zero, resultReg, resultReg, bail);

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

typedef bool(*InitPropFn)(JSContext *cx, HandleObject obj,
                          HandlePropertyName name, HandleValue value);
static const VMFunction InitPropInfo =
    FunctionInfo<InitPropFn>(InitProp);

bool
CodeGenerator::visitParNew(LParNew *lir)
{
    Register objReg = ToRegister(lir->output());
    Register parSliceReg = ToRegister(lir->parSlice());
    Register tempReg1 = ToRegister(lir->getTemp0());
    Register tempReg2 = ToRegister(lir->getTemp1());
    JSObject *templateObject = lir->mir()->templateObject();
    emitParAllocateGCThing(objReg, parSliceReg, tempReg1, tempReg2,
                           templateObject);
    return true;
}

class OutOfLineParNewGCThing : public OutOfLineCodeBase<CodeGenerator>
{
public:
    gc::AllocKind allocKind;
    Register objReg;

    OutOfLineParNewGCThing(gc::AllocKind allocKind, Register objReg)
        : allocKind(allocKind), objReg(objReg)
    {}

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineParNewGCThing(this);
    }
};

bool
CodeGenerator::emitParAllocateGCThing(const Register &objReg,
                                      const Register &parSliceReg,
                                      const Register &tempReg1,
                                      const Register &tempReg2,
                                      JSObject *templateObj)
{
    gc::AllocKind allocKind = templateObj->getAllocKind();
    OutOfLineParNewGCThing *ool = new OutOfLineParNewGCThing(allocKind, objReg);
    if (!ool || !addOutOfLineCode(ool))
        return false;

    masm.parNewGCThing(objReg, parSliceReg, tempReg1, tempReg2,
                       templateObj, ool->entry());
    masm.bind(ool->rejoin());
    masm.initGCThing(objReg, templateObj);
    return true;
}

bool
CodeGenerator::visitOutOfLineParNewGCThing(OutOfLineParNewGCThing *ool)
{
    
    
    
    

    
    
    
    
    
    
    RegisterSet saveSet(RegisterSet::Volatile());

    
    
    saveSet.addUnchecked(CallTempReg0);
    saveSet.addUnchecked(CallTempReg1);
    saveSet.maybeTake(AnyRegister(ool->objReg));

    masm.PushRegsInMask(saveSet);
    masm.move32(Imm32(ool->allocKind), CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParNewGCThing));
    masm.movePtr(ReturnReg, ool->objReg);
    masm.PopRegsInMask(saveSet);
    Label *bail;
    if (!ensureOutOfLineParallelAbort(&bail))
        return false;
    masm.branchTestPtr(Assembler::Zero, ool->objReg, ool->objReg, bail);
    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitParBailout(LParBailout *lir)
{
    Label *bail;
    if (!ensureOutOfLineParallelAbort(&bail))
        return false;
    masm.jump(bail);
    return true;
}

bool
CodeGenerator::visitInitProp(LInitProp *lir)
{
    Register objReg = ToRegister(lir->getObject());

    pushArg(ToValue(lir, LInitProp::ValueIndex));
    pushArg(ImmGCPtr(lir->mir()->propertyName()));
    pushArg(objReg);

    return callVM(InitPropInfo, lir);
}

typedef bool (*CreateThisFn)(JSContext *cx, HandleObject callee, MutableHandleValue rval);
static const VMFunction CreateThisInfo =
FunctionInfo<CreateThisFn>(CreateThis);

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

typedef JSObject *(*NewGCThingFn)(JSContext *cx, gc::AllocKind allocKind, size_t thingSize);
static const VMFunction NewGCThingInfo =
    FunctionInfo<NewGCThingFn>(js::ion::NewGCThing);

bool
CodeGenerator::visitCreateThisWithTemplate(LCreateThisWithTemplate *lir)
{
    JSObject *templateObject = lir->mir()->getTemplateObject();
    gc::AllocKind allocKind = templateObject->getAllocKind();
    int thingSize = (int)gc::Arena::thingSize(allocKind);
    Register objReg = ToRegister(lir->output());

    OutOfLineCode *ool = oolCallVM(NewGCThingInfo, lir,
                                   (ArgList(), Imm32(allocKind), Imm32(thingSize)),
                                   StoreRegisterTo(objReg));
    if (!ool)
        return false;

    
    masm.newGCThing(objReg, templateObject, ool->entry());

    
    masm.bind(ool->rejoin());
    masm.initGCThing(objReg, templateObject);

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
    masm.unboxInt32(Address(obj, TypedArray::lengthOffset()), out);
    return true;
}

bool
CodeGenerator::visitTypedArrayElements(LTypedArrayElements *lir)
{
    Register obj = ToRegister(lir->object());
    Register out = ToRegister(lir->output());
    masm.loadPtr(Address(obj, TypedArray::dataOffset()), out);
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
CodeGenerator::visitNegD(LNegD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    JS_ASSERT(input == ToFloatRegister(ins->output()));

    masm.negateDouble(input);
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

    masm.setupUnalignedABICall(2, temp);
    masm.movePtr(ImmWord(mathCache), temp);
    masm.passABIArg(temp);
    masm.passABIArg(input);

    void *funptr = NULL;
    switch (ins->mir()->function()) {
      case MMathFunction::Log:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_log_impl);
        break;
      case MMathFunction::Sin:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_sin_impl);
        break;
      case MMathFunction::Cos:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_cos_impl);
        break;
      case MMathFunction::Tan:
        funptr = JS_FUNC_TO_DATA_PTR(void *, js::math_tan_impl);
        break;
      default:
        JS_NOT_REACHED("Unknown math function");
    }

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

    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NumberMod), MacroAssembler::DOUBLE);
    return true;
}

typedef bool (*BinaryFn)(JSContext *, HandleScript, jsbytecode *,
                         MutableHandleValue, MutableHandleValue, Value *);

static const VMFunction AddInfo = FunctionInfo<BinaryFn>(js::AddValues);
static const VMFunction SubInfo = FunctionInfo<BinaryFn>(js::SubValues);
static const VMFunction MulInfo = FunctionInfo<BinaryFn>(js::MulValues);
static const VMFunction DivInfo = FunctionInfo<BinaryFn>(js::DivValues);
static const VMFunction ModInfo = FunctionInfo<BinaryFn>(js::ModValues);
static const VMFunction UrshInfo = FunctionInfo<BinaryFn>(js::UrshValues);

bool
CodeGenerator::visitBinaryV(LBinaryV *lir)
{
    pushArg(ToValue(lir, LBinaryV::RhsInput));
    pushArg(ToValue(lir, LBinaryV::LhsInput));
    pushArg(ImmWord(lir->mirRaw()->toInstruction()->resumePoint()->pc()));
    pushArg(ImmGCPtr(current->mir()->info().script()));

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
        JS_NOT_REACHED("Unexpected binary op");
        return false;
    }
}

bool
CodeGenerator::visitParCompareS(LParCompareS *lir)
{
    JSOp op = lir->mir()->jsop();
    Register left = ToRegister(lir->left());
    Register right = ToRegister(lir->right());

    JS_ASSERT((op == JSOP_EQ || op == JSOP_STRICTEQ) ||
              (op == JSOP_NE || op == JSOP_STRICTNE));

    masm.setupUnalignedABICall(2, CallTempReg2);
    masm.passABIArg(left);
    masm.passABIArg(right);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParCompareStrings));
    masm.and32(Imm32(0xF), ReturnReg); 

    
    Label *bail;
    if (!ensureOutOfLineParallelAbort(&bail))
        return false;
    masm.branch32(Assembler::Equal, ReturnReg, Imm32(ParCompareUnknown), bail);

    if (op == JSOP_NE || op == JSOP_STRICTNE)
        masm.xor32(Imm32(1), ReturnReg);

    return true;
}

typedef bool (*StringCompareFn)(JSContext *, HandleString, HandleString, JSBool *);
static const VMFunction stringsEqualInfo =
    FunctionInfo<StringCompareFn>(ion::StringsEqual<true>);
static const VMFunction stringsNotEqualInfo =
    FunctionInfo<StringCompareFn>(ion::StringsEqual<false>);

bool
CodeGenerator::emitCompareS(LInstruction *lir, JSOp op, Register left, Register right,
                            Register output, Register temp)
{
    JS_ASSERT(lir->isCompareS() || lir->isCompareStrictS());

    OutOfLineCode *ool = NULL;
    if (op == JSOP_EQ || op == JSOP_STRICTEQ) {
        ool = oolCallVM(stringsEqualInfo, lir, (ArgList(), left, right),  StoreRegisterTo(output));
    } else {
        JS_ASSERT(op == JSOP_NE || op == JSOP_STRICTNE);
        ool = oolCallVM(stringsNotEqualInfo, lir, (ArgList(), left, right), StoreRegisterTo(output));
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
    Register temp = ToRegister(lir->temp0());

    Label string, done;

    masm.branchTestString(Assembler::Equal, leftV, &string);
    masm.move32(Imm32(op == JSOP_STRICTNE), output);
    masm.jump(&done);

    masm.bind(&string);
    Register left = masm.extractString(leftV, ToRegister(lir->temp1()));
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

typedef bool (*CompareFn)(JSContext *, MutableHandleValue, MutableHandleValue, JSBool *);
static const VMFunction EqInfo = FunctionInfo<CompareFn>(ion::LooselyEqual<true>);
static const VMFunction NeInfo = FunctionInfo<CompareFn>(ion::LooselyEqual<false>);
static const VMFunction StrictEqInfo = FunctionInfo<CompareFn>(ion::StrictlyEqual<true>);
static const VMFunction StrictNeInfo = FunctionInfo<CompareFn>(ion::StrictlyEqual<false>);
static const VMFunction LtInfo = FunctionInfo<CompareFn>(ion::LessThan);
static const VMFunction LeInfo = FunctionInfo<CompareFn>(ion::LessThanOrEqual);
static const VMFunction GtInfo = FunctionInfo<CompareFn>(ion::GreaterThan);
static const VMFunction GeInfo = FunctionInfo<CompareFn>(ion::GreaterThanOrEqual);

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
        JS_NOT_REACHED("Unexpected compare op");
        return false;
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

        OutOfLineTestObjectWithLabels *ool = NULL;
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

            Register objreg = masm.extractObject(value, ToRegister(lir->temp0()));
            testObjectTruthy(objreg, notNullOrLikeUndefined, nullOrLikeUndefined,
                             ToRegister(lir->temp1()), ool);
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

    Assembler::Condition cond = JSOpToCondition(op);
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

        OutOfLineTestObject *ool = NULL;
        if (lir->mir()->operandMightEmulateUndefined()) {
            ool = new OutOfLineTestObject();
            if (!addOutOfLineCode(ool))
                return false;
        }

        Register tag = masm.splitTagForTest(value);
        Label *ifTrueLabel = ifTrue->lir()->label();
        Label *ifFalseLabel = ifFalse->lir()->label();

        masm.branchTestNull(Assembler::Equal, tag, ifTrueLabel);
        masm.branchTestUndefined(Assembler::Equal, tag, ifTrueLabel);

        if (ool) {
            masm.branchTestObject(Assembler::NotEqual, tag, ifFalseLabel);

            
            Register objreg = masm.extractObject(value, ToRegister(lir->temp0()));
            testObjectTruthy(objreg, ifFalseLabel, ifTrueLabel, ToRegister(lir->temp1()), ool);
        } else {
            masm.jump(ifFalseLabel);
        }
        return true;
    }

    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    Assembler::Condition cond = JSOpToCondition(op);
    if (compareType == MCompare::Compare_Null)
        cond = masm.testNull(cond, value);
    else
        cond = masm.testUndefined(cond, value);

    emitBranch(cond, lir->ifTrue(), lir->ifFalse());
    return true;
}

typedef JSString *(*ConcatStringsFn)(JSContext *, HandleString, HandleString);
static const VMFunction ConcatStringsInfo = FunctionInfo<ConcatStringsFn>(ConcatStrings<CanGC>);

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

        equal = ifTrue->lir()->label();
        unequal = ifFalse->lir()->label();
    }

    Register objreg = ToRegister(lir->input());

    testObjectTruthy(objreg, unequal, equal, ToRegister(lir->temp()), ool);
    return true;
}

bool
CodeGenerator::visitConcat(LConcat *lir)
{
    pushArg(ToRegister(lir->rhs()));
    pushArg(ToRegister(lir->lhs()));
    if (!callVM(ConcatStringsInfo, lir))
        return false;
    return true;
}

typedef bool (*CharCodeAtFn)(JSContext *, HandleString, int32_t, uint32_t *);
static const VMFunction CharCodeAtInfo = FunctionInfo<CharCodeAtFn>(ion::CharCodeAt);

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
static const VMFunction StringFromCharCodeInfo = FunctionInfo<StringFromCharCodeFn>(ion::StringFromCharCode);

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

    masm.movePtr(ImmWord(&gen->compartment->rt->staticStrings.unitStaticTable), output);
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

    OutOfLineTestObjectWithLabels *ool = NULL;
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
        masm.mov(Imm32(index), temp);
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

typedef bool (*SetObjectElementFn)(JSContext *, HandleObject,
                                   HandleValue, HandleValue, JSBool strict);
static const VMFunction SetObjectElementInfo =
    FunctionInfo<SetObjectElementFn>(SetObjectElement);

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

    
    
    
    
    
    
    Label indexNotInitLen;
    Label indexWouldExceedCapacity;

    
    
    
    masm.j(Assembler::NotEqual, &indexNotInitLen);

    Int32Key key = ToInt32Key(index);

    
    masm.branchKey(Assembler::BelowOrEqual, Address(elements, ObjectElements::offsetOfCapacity()),
                   key, &indexWouldExceedCapacity);

    
    
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

    switch (gen->info().executionMode()) {
      case SequentialExecution:
        masm.bind(&indexNotInitLen);
        masm.bind(&indexWouldExceedCapacity);
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

      case ParallelExecution:
        Label *bail;
        if (!ensureOutOfLineParallelAbort(&bail))
            return false;

        
        
        
        
        
        
        
        
        masm.bind(&indexWouldExceedCapacity);

        
        
        
        
        
        
        
        
        
        
        
        RegisterSet saveSet(ins->safepoint()->liveRegs());
        saveSet.maybeTake(object);

        masm.PushRegsInMask(saveSet);
        masm.reserveStack(sizeof(ParPushArgs));
        masm.storePtr(object, Address(StackPointer, offsetof(ParPushArgs, object)));
        masm.storeConstantOrRegister(value, Address(StackPointer,
                                                    offsetof(ParPushArgs, value)));
        masm.movePtr(StackPointer, CallTempReg0);
        masm.setupUnalignedABICall(1, CallTempReg1);
        masm.passABIArg(CallTempReg0);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParPush));
        masm.freeStack(sizeof(ParPushArgs));
        masm.movePtr(ReturnReg, object);
        masm.PopRegsInMask(saveSet);
        masm.branchTestPtr(Assembler::Zero, object, object, bail);
        masm.jump(ool->rejoin());

        
        
        
        
        
        masm.bind(&indexNotInitLen);
        masm.jump(bail);
        return true;
    }

    JS_ASSERT(false);
    return false;
}

typedef bool (*ArrayPopShiftFn)(JSContext *, HandleObject, MutableHandleValue);
static const VMFunction ArrayPopDenseInfo = FunctionInfo<ArrayPopShiftFn>(ion::ArrayPopDense);
static const VMFunction ArrayShiftDenseInfo = FunctionInfo<ArrayPopShiftFn>(ion::ArrayShiftDense);

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
    FunctionInfo<ArrayPushDenseFn>(ion::ArrayPushDense);

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
        masm.movePtr(ImmWord((void *)NULL), temp1);
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

    
    masm.loadPtr(AbsoluteAddress(&gen->compartment->rt->nativeIterCache.last), output);
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
                   ImmWord(js::emptyObjectElements),
                   ool->entry());

    
    
    {
        Label noBarrier;
        masm.branchTestNeedsBarrier(Assembler::Zero, temp1, &noBarrier);

        Address objAddr(niTemp, offsetof(NativeIterator, obj));
        masm.branchPtr(Assembler::NotEqual, objAddr, obj, ool->entry());

        masm.bind(&noBarrier);
    }

    
    masm.storePtr(obj, Address(niTemp, offsetof(NativeIterator, obj)));
    masm.or32(Imm32(JSITER_ACTIVE), Address(niTemp, offsetof(NativeIterator, flags)));

    
    masm.movePtr(ImmWord(GetIonContext()->compartment), temp1);
    masm.loadPtr(Address(temp1, offsetof(JSCompartment, enumerators)), temp1);

    
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

typedef bool (*IteratorMoreFn)(JSContext *, HandleObject, JSBool *);
static const VMFunction IteratorMoreInfo = FunctionInfo<IteratorMoreFn>(ion::IteratorMore);

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
    masm.storePtr(ImmWord(uintptr_t(0)), Address(temp1, NativeIterator::offsetOfNext()));
    masm.storePtr(ImmWord(uintptr_t(0)), Address(temp1, NativeIterator::offsetOfPrev()));
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
CodeGenerator::visitGetArgument(LGetArgument *lir)
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
CodeGenerator::generate()
{
    if (!safepoints_.init(graph.totalSlotCount()))
        return false;

    
    
    
    if (!generateArgumentsChecks())
        return false;

    if (frameClass_ != FrameSizeClass::None()) {
        deoptTable_ = GetIonContext()->compartment->ionCompartment()->getBailoutTable(frameClass_);
        if (!deoptTable_)
            return false;
    }

    if (!generatePrologue())
        return false;
    if (!generateBody())
        return false;
    if (!generateEpilogue())
        return false;
    if (!generateInvalidateEpilogue())
        return false;
    if (!generateOutOfLineCode())
        return false;

    return !masm.oom();
}

bool
CodeGenerator::link()
{
    AssertCanGC();
    JSContext *cx = GetIonContext()->cx;

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    
    encodeSafepoints();

    RootedScript script(cx, gen->info().script());
    ExecutionMode executionMode = gen->info().executionMode();
    JS_ASSERT(!HasIonScript(script, executionMode));

    uint32_t scriptFrameSize = frameClass_ == FrameSizeClass::None()
                           ? frameDepth_
                           : FrameSizeClass::FromDepth(frameDepth_).frameSize();

    
    
    if (cx->compartment->types.compiledInfo.compilerOutput(cx)->isInvalidated())
        return true;

    IonScript *ionScript =
      IonScript::New(cx, graph.totalSlotCount(), scriptFrameSize, snapshots_.size(),
                     bailouts_.length(), graph.numConstants(),
                     safepointIndices_.length(), osiIndices_.length(),
                     cacheList_.length(), runtimeData_.length(),
                     safepoints_.size(), graph.mir().numScripts(),
					 executionMode == ParallelExecution ? ForkJoinSlices(cx) : 0);
    SetIonScript(script, executionMode, ionScript);

    if (!ionScript)
        return false;
    invalidateEpilogueData_.fixup(&masm);
    Assembler::patchDataWithValueCheck(CodeLocationLabel(code, invalidateEpilogueData_),
                                       ImmWord(uintptr_t(ionScript)),
                                       ImmWord(uintptr_t(-1)));

    IonSpew(IonSpew_Codegen, "Created IonScript %p (raw %p)",
            (void *) ionScript, (void *) code->raw());

    ionScript->setInvalidationEpilogueDataOffset(invalidateEpilogueData_.offset());
    ionScript->setOsrPc(gen->info().osrPc());
    ionScript->setOsrEntryOffset(getOsrEntryOffset());
    ptrdiff_t real_invalidate = masm.actualOffset(invalidate_.offset());
    ionScript->setInvalidationEpilogueOffset(real_invalidate);

    ionScript->setMethod(code);
    ionScript->setDeoptTable(deoptTable_);

    
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
    JS_ASSERT(graph.mir().numScripts() > 0);
    ionScript->copyScriptEntries(graph.mir().scripts());

    if (executionMode == ParallelExecution)
        ionScript->zeroParallelInvalidatedScripts();

    linkAbsoluteLabels();

    
    
    
    if (cx->zone()->needsBarrier())
        ionScript->toggleBarriers(true);

    return true;
}


class OutOfLineUnboxDouble : public OutOfLineCodeBase<CodeGenerator>
{
    LUnboxDouble *unboxDouble_;

  public:
    OutOfLineUnboxDouble(LUnboxDouble *unboxDouble)
      : unboxDouble_(unboxDouble)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineUnboxDouble(this);
    }

    LUnboxDouble *unboxDouble() const {
        return unboxDouble_;
    }
};

bool
CodeGenerator::visitUnboxDouble(LUnboxDouble *lir)
{
    const ValueOperand box = ToValue(lir, LUnboxDouble::Input);
    const LDefinition *result = lir->output();

    
    
    OutOfLineUnboxDouble *ool = new OutOfLineUnboxDouble(lir);
    if (!addOutOfLineCode(ool))
        return false;

    masm.branchTestDouble(Assembler::NotEqual, box, ool->entry());
    masm.unboxDouble(box, ToFloatRegister(result));
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineUnboxDouble(OutOfLineUnboxDouble *ool)
{
    LUnboxDouble *ins = ool->unboxDouble();
    const ValueOperand value = ToValue(ins, LUnboxDouble::Input);

    if (ins->mir()->fallible()) {
        Assembler::Condition cond = masm.testInt32(Assembler::NotEqual, value);
        if (!bailoutIf(cond, ins->snapshot()))
            return false;
    }
    masm.int32ValueToDouble(value, ToFloatRegister(ins->output()));
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
CodeGenerator::visitCallsiteCloneIC(OutOfLineUpdateCache *ool, CallsiteCloneIC *ic)
{
    AssertCanGC();
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
    Register scopeChain = ToRegister(ins->scopeObj());
    TypedOrValueRegister output(GetValueOutput(ins));
    bool isTypeOf = ins->mir()->accessKind() != MGetNameCache::NAME;

    NameIC cache(isTypeOf, scopeChain, ins->mir()->name(), output);
    return addCache(ins, allocateCache(cache));
}

typedef bool (*NameICFn)(JSContext *, size_t, HandleObject, MutableHandleValue);
const VMFunction NameIC::UpdateInfo = FunctionInfo<NameICFn>(NameIC::update);

bool
CodeGenerator::visitNameIC(OutOfLineUpdateCache *ool, NameIC *ic)
{
    AssertCanGC();
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
CodeGenerator::visitGetPropertyCacheV(LGetPropertyCacheV *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    PropertyName *name = ins->mir()->name();
    bool allowGetters = ins->mir()->allowGetters();
    TypedOrValueRegister output = TypedOrValueRegister(GetValueOutput(ins));

    GetPropertyIC cache(liveRegs, objReg, name, output, allowGetters);
    return addCache(ins, allocateCache(cache));
}

bool
CodeGenerator::visitGetPropertyCacheT(LGetPropertyCacheT *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    PropertyName *name = ins->mir()->name();
    bool allowGetters = ins->mir()->allowGetters();
    TypedOrValueRegister output(ins->mir()->type(), ToAnyRegister(ins->getDef(0)));

    GetPropertyIC cache(liveRegs, objReg, name, output, allowGetters);
    return addCache(ins, allocateCache(cache));
}

typedef bool (*GetPropertyICFn)(JSContext *, size_t, HandleObject, MutableHandleValue);
const VMFunction GetPropertyIC::UpdateInfo =
    FunctionInfo<GetPropertyICFn>(GetPropertyIC::update);

bool
CodeGenerator::visitGetPropertyIC(OutOfLineUpdateCache *ool, GetPropertyIC *ic)
{
    AssertCanGC();
    LInstruction *lir = ool->lir();
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

bool
CodeGenerator::visitGetElementCacheV(LGetElementCacheV *ins)
{
    Register obj = ToRegister(ins->object());
    ConstantOrRegister index = TypedOrValueRegister(ToValue(ins, LGetElementCacheV::Index));
    TypedOrValueRegister output = TypedOrValueRegister(GetValueOutput(ins));

    GetElementIC cache(obj, index, output, ins->mir()->monitoredResult());

    return addCache(ins, allocateCache(cache));
}

bool
CodeGenerator::visitGetElementCacheT(LGetElementCacheT *ins)
{
    Register obj = ToRegister(ins->object());
    ConstantOrRegister index = TypedOrValueRegister(MIRType_Int32, ToAnyRegister(ins->index()));
    TypedOrValueRegister output(ins->mir()->type(), ToAnyRegister(ins->output()));

    GetElementIC cache(obj, index, output, ins->mir()->monitoredResult());

    return addCache(ins, allocateCache(cache));
}

typedef bool (*GetElementICFn)(JSContext *, size_t, HandleObject, HandleValue, MutableHandleValue);
const VMFunction GetElementIC::UpdateInfo =
    FunctionInfo<GetElementICFn>(GetElementIC::update);

bool
CodeGenerator::visitGetElementIC(OutOfLineUpdateCache *ool, GetElementIC *ic)
{
    AssertCanGC();
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
CodeGenerator::visitBindNameIC(OutOfLineUpdateCache *ool, BindNameIC *ic)
{
    AssertCanGC();
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
                              HandlePropertyName, const HandleValue, bool, bool);
static const VMFunction SetPropertyInfo =
    FunctionInfo<SetPropertyFn>(SetProperty);

bool
CodeGenerator::visitCallSetProperty(LCallSetProperty *ins)
{
    ConstantOrRegister value = TypedOrValueRegister(ToValue(ins, LCallSetProperty::Value));

    const Register objReg = ToRegister(ins->getOperand(0));
    bool isSetName = JSOp(*ins->mir()->resumePoint()->pc()) == JSOP_SETNAME;

    pushArg(Imm32(isSetName));
    pushArg(Imm32(ins->mir()->strict()));

    pushArg(value);
    pushArg(ImmGCPtr(ins->mir()->name()));
    pushArg(objReg);

    return callVM(SetPropertyInfo, ins);
}

typedef bool (*DeletePropertyFn)(JSContext *, HandleValue, HandlePropertyName, JSBool *);
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
    else
        return callVM(DeletePropertyNonStrictInfo, lir);
}

bool
CodeGenerator::visitSetPropertyCacheV(LSetPropertyCacheV *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    ConstantOrRegister value = TypedOrValueRegister(ToValue(ins, LSetPropertyCacheV::Value));
    bool isSetName = JSOp(*ins->mir()->resumePoint()->pc()) == JSOP_SETNAME;

    SetPropertyIC cache(liveRegs, objReg, ins->mir()->name(), value,
                        isSetName, ins->mir()->strict());
    return addCache(ins, allocateCache(cache));
}

bool
CodeGenerator::visitSetPropertyCacheT(LSetPropertyCacheT *ins)
{
    RegisterSet liveRegs = ins->safepoint()->liveRegs();
    Register objReg = ToRegister(ins->getOperand(0));
    ConstantOrRegister value;
    bool isSetName = JSOp(*ins->mir()->resumePoint()->pc()) == JSOP_SETNAME;

    if (ins->getOperand(1)->isConstant())
        value = ConstantOrRegister(*ins->getOperand(1)->toConstant());
    else
        value = TypedOrValueRegister(ins->valueType(), ToAnyRegister(ins->getOperand(1)));

    SetPropertyIC cache(liveRegs, objReg, ins->mir()->name(), value,
                        isSetName, ins->mir()->strict());
    return addCache(ins, allocateCache(cache));
}

typedef bool (*SetPropertyICFn)(JSContext *, size_t, HandleObject, HandleValue);
const VMFunction SetPropertyIC::UpdateInfo =
    FunctionInfo<SetPropertyICFn>(SetPropertyIC::update);

bool
CodeGenerator::visitSetPropertyIC(OutOfLineUpdateCache *ool, SetPropertyIC *ic)
{
    AssertCanGC();
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
static const VMFunction BitNotInfo = FunctionInfo<BitNotFn>(BitNot);

bool
CodeGenerator::visitBitNotV(LBitNotV *lir)
{
    pushArg(ToValue(lir, LBitNotV::Input));
    return callVM(BitNotInfo, lir);
}

typedef bool (*BitopFn)(JSContext *, HandleValue, HandleValue, int *p);
static const VMFunction BitAndInfo = FunctionInfo<BitopFn>(BitAnd);
static const VMFunction BitOrInfo = FunctionInfo<BitopFn>(BitOr);
static const VMFunction BitXorInfo = FunctionInfo<BitopFn>(BitXor);
static const VMFunction BitLhsInfo = FunctionInfo<BitopFn>(BitLsh);
static const VMFunction BitRhsInfo = FunctionInfo<BitopFn>(BitRsh);

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
    JS_NOT_REACHED("unexpected bitop");
    return false;
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

    OutOfLineTypeOfV *ool = new OutOfLineTypeOfV(lir);
    if (!addOutOfLineCode(ool))
        return false;

    JSRuntime *rt = gen->compartment->rt;

    
    
    masm.branchTestObject(Assembler::Equal, tag, ool->entry());

    Label done;

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
    masm.bind(ool->rejoin());
    return true;
}

typedef JSString *(*TypeOfFn)(JSContext *, HandleValue);
static const VMFunction TypeOfInfo = FunctionInfo<TypeOfFn>(TypeOfOperation);

bool
CodeGenerator::visitOutOfLineTypeOfV(OutOfLineTypeOfV *ool)
{
    LTypeOfV *ins = ool->ins();
    saveLive(ins);

    pushArg(ToValue(ins, LTypeOfV::Input));
    if (!callVM(TypeOfInfo, ins))
        return false;

    masm.storeCallResult(ToRegister(ins->output()));
    restoreLive(ins);

    masm.jump(ool->rejoin());
    return true;
}

typedef bool (*ToIdFn)(JSContext *, HandleScript, jsbytecode *, HandleValue, HandleValue,
                       MutableHandleValue);
static const VMFunction ToIdInfo = FunctionInfo<ToIdFn>(ToIdOperation);

bool
CodeGenerator::visitToIdV(LToIdV *lir)
{
    pushArg(ToValue(lir, LToIdV::Index));
    pushArg(ToValue(lir, LToIdV::Object));
    pushArg(ImmWord(lir->mir()->resumePoint()->pc()));
    pushArg(ImmGCPtr(current->mir()->info().script()));
    return callVM(ToIdInfo, lir);
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
    int width = TypedArray::slotWidth(arrayType);

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

class OutOfLineLoadTypedArray : public OutOfLineCodeBase<CodeGenerator>
{
    LLoadTypedArrayElementHole *ins_;

  public:
    OutOfLineLoadTypedArray(LLoadTypedArrayElementHole *ins)
      : ins_(ins)
    { }

    bool accept(CodeGenerator *codegen) {
        return codegen->visitOutOfLineLoadTypedArray(this);
    }

    LLoadTypedArrayElementHole *ins() const {
        return ins_;
    }
};

bool
CodeGenerator::visitLoadTypedArrayElementHole(LLoadTypedArrayElementHole *lir)
{
    Register object = ToRegister(lir->object());
    const ValueOperand out = ToOutValue(lir);

    OutOfLineLoadTypedArray *ool = new OutOfLineLoadTypedArray(lir);
    if (!addOutOfLineCode(ool))
        return false;

    
    Register scratch = out.scratchReg();
    Int32Key key = ToInt32Key(lir->index());
    masm.unboxInt32(Address(object, TypedArray::lengthOffset()), scratch);

    
    masm.branchKey(Assembler::BelowOrEqual, scratch, key, ool->entry());

    
    masm.loadPtr(Address(object, TypedArray::dataOffset()), scratch);

    int arrayType = lir->mir()->arrayType();
    int width = TypedArray::slotWidth(arrayType);

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

    masm.bind(ool->rejoin());
    return true;
}

typedef bool (*GetElementMonitoredFn)(JSContext *, MutableHandleValue, HandleValue, MutableHandleValue);
static const VMFunction GetElementMonitoredInfo =
    FunctionInfo<GetElementMonitoredFn>(js::GetElementMonitored);

bool
CodeGenerator::visitOutOfLineLoadTypedArray(OutOfLineLoadTypedArray *ool)
{
    LLoadTypedArrayElementHole *ins = ool->ins();
    saveLive(ins);

    Register object = ToRegister(ins->object());
    ValueOperand out = ToOutValue(ins);

    if (ins->index()->isConstant())
        pushArg(*ins->index()->toConstant());
    else
        pushArg(TypedOrValueRegister(MIRType_Int32, ToAnyRegister(ins->index())));
    pushArg(TypedOrValueRegister(MIRType_Object, AnyRegister(object)));
    if (!callVM(GetElementMonitoredInfo, ins))
        return false;

    masm.storeCallResultValue(out);
    restoreLive(ins);

    masm.jump(ool->rejoin());
    return true;
}

template <typename T>
static inline void
StoreToTypedArray(MacroAssembler &masm, int arrayType, const LAllocation *value, const T &dest)
{
    if (arrayType == TypedArray::TYPE_FLOAT32 || arrayType == TypedArray::TYPE_FLOAT64) {
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
    int width = TypedArray::slotWidth(arrayType);

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
CodeGenerator::visitClampIToUint8(LClampIToUint8 *lir)
{
    Register input = ToRegister(lir->input());
    Register output = ToRegister(lir->output());
    masm.clampIntToUint8(input, output);
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
    ValueOperand input = ToValue(lir, LClampVToUint8::Input);
    FloatRegister tempFloat = ToFloatRegister(lir->tempFloat());
    Register output = ToRegister(lir->output());

    Register tag = masm.splitTagForTest(input);

    Label done;
    Label isInt32, isDouble, isBoolean;
    masm.branchTestInt32(Assembler::Equal, tag, &isInt32);
    masm.branchTestDouble(Assembler::Equal, tag, &isDouble);
    masm.branchTestBoolean(Assembler::Equal, tag, &isBoolean);

    
    Label isZero;
    masm.branchTestUndefined(Assembler::Equal, tag, &isZero);
    masm.branchTestNull(Assembler::Equal, tag, &isZero);
    masm.branchTestObject(Assembler::Equal, tag, &isZero);

    
    if (!bailout(lir->snapshot()))
        return false;

    masm.bind(&isInt32);
    masm.unboxInt32(input, output);
    masm.clampIntToUint8(output, output);
    masm.jump(&done);

    masm.bind(&isDouble);
    masm.unboxDouble(input, tempFloat);
    masm.clampDoubleToUint8(tempFloat, output);
    masm.jump(&done);

    masm.bind(&isBoolean);
    masm.unboxBoolean(input, output);
    masm.jump(&done);

    masm.bind(&isZero);
    masm.move32(Imm32(0), output);

    masm.bind(&done);
    return true;
}

typedef bool (*OperatorInFn)(JSContext *, HandleValue, HandleObject, JSBool *);
static const VMFunction OperatorInInfo = FunctionInfo<OperatorInFn>(OperatorIn);

bool
CodeGenerator::visitIn(LIn *ins)
{
    pushArg(ToRegister(ins->rhs()));
    pushArg(ToValue(ins, LIn::LHS));

    return callVM(OperatorInInfo, ins);
}

bool
CodeGenerator::visitInArray(LInArray *lir)
{
    Register elements = ToRegister(lir->elements());
    Register initLength = ToRegister(lir->initLength());
    Register output = ToRegister(lir->output());

    
    Label falseBranch, done;
    if (lir->index()->isConstant()) {
        masm.branch32(Assembler::BelowOrEqual, initLength, Imm32(ToInt32(lir->index())), &falseBranch);
        if (lir->mir()->needsHoleCheck()) {
            masm.branchTestMagic(Assembler::Equal, Address(elements, ToInt32(lir->index()) * sizeof(Value)),
                                 &falseBranch);
        }
    } else {
        masm.branch32(Assembler::BelowOrEqual, initLength, ToRegister(lir->index()), &falseBranch);
        if (lir->mir()->needsHoleCheck()) {
            masm.branchTestMagic(Assembler::Equal, BaseIndex(elements, ToRegister(lir->index()), TimesEight),
                                 &falseBranch);
        }
    }

    masm.move32(Imm32(1), output);
    masm.jump(&done);

    masm.bind(&falseBranch);
    masm.move32(Imm32(0), output);
    masm.bind(&done);
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
IsDelegateObject(JSContext *cx, HandleObject protoObj, HandleObject obj, JSBool *res)
{
    bool nres;
    if (!IsDelegate(cx, protoObj, ObjectValue(*obj), &nres))
        return false;
    *res = nres;
    return true;
}

typedef bool (*IsDelegateObjectFn)(JSContext *, HandleObject, HandleObject, JSBool *);
static const VMFunction IsDelegateObjectInfo = FunctionInfo<IsDelegateObjectFn>(IsDelegateObject);

bool
CodeGenerator::emitInstanceOf(LInstruction *ins, RawObject prototypeObject)
{
    
    

    Label done;
    Register output = ToRegister(ins->getDef(0));

    
    Register objReg;
    if (ins->isInstanceOfV()) {
        Label isObject;
        ValueOperand lhsValue = ToValue(ins, LInstanceOfV::LHS);
        masm.branchTestObject(Assembler::Equal, lhsValue, &isObject);
        masm.mov(Imm32(0), output);
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
        masm.mov(Imm32(1), output);
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

typedef bool (*HasInstanceFn)(JSContext *, HandleObject, HandleValue, JSBool *);
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

    
    masm.adjustStack(-int32_t(sizeof(Value)));
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
        Label success, exception;
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

        masm.loadValue(Address(StackPointer, IonDOMExitFrameLayout::offsetOfResult()),
                       JSReturnOperand);
        masm.jump(&success);

        {
            masm.bind(&exception);
            masm.handleException();
        }
        masm.bind(&success);
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

    Label success, exception;
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

    masm.jump(&success);

    {
        masm.bind(&exception);
        masm.handleException();
    }
    masm.bind(&success);
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
    AssertCanGC();
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
            JS_NOT_REACHED("invalid LFunctionBoundary type");
    }
}

bool
CodeGenerator::visitOutOfLineParallelAbort(OutOfLineParallelAbort *ool)
{
    masm.movePtr(ImmWord((void *) current->mir()->info().script()), CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ParallelAbort));

    masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    masm.jump(returnLabel_);
    return true;
}

} 
} 

