








































#include "CodeGenerator.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "MIRGenerator.h"
#include "shared/CodeGenerator-shared-inl.h"
#include "jsnum.h"
#include "jsinterpinlines.h"

using namespace js;
using namespace js::ion;

namespace js {
namespace ion {

CodeGenerator::CodeGenerator(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorSpecific(gen, graph)
{
}

bool
CodeGenerator::visitValueToInt32(LValueToInt32 *lir)
{
    ValueOperand operand = ToValue(lir, LValueToInt32::Input);
    Register output = ToRegister(lir->output());

    Label done, simple, isInt32, isBool, notDouble;

    
    masm.branchTestInt32(Assembler::Equal, operand, &isInt32);
    masm.branchTestBoolean(Assembler::Equal, operand, &isBool);
    masm.branchTestDouble(Assembler::NotEqual, operand, &notDouble);

    
    
    FloatRegister temp = ToFloatRegister(lir->tempFloat());
    masm.unboxDouble(operand, temp);

    Label fails;
    switch (lir->mode()) {
      case LValueToInt32::TRUNCATE:
        emitTruncateDouble(temp, output, &fails);
        break;
      default:
        JS_ASSERT(lir->mode() == LValueToInt32::NORMAL);
        emitDoubleToInt32(temp, output, &fails);
        break;
    }
    masm.jump(&done);

    masm.bind(&notDouble);

    if (lir->mode() == LValueToInt32::NORMAL) {
        
        
        masm.branchTestNull(Assembler::NotEqual, operand, &fails);
    } else {
        
        
        masm.branchTestObject(Assembler::Equal, operand, &fails);
        masm.branchTestString(Assembler::Equal, operand, &fails);
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

    Label isDouble, isInt32, isBool, isNull, done;

    
    masm.branchTestDouble(Assembler::Equal, operand, &isDouble);
    masm.branchTestInt32(Assembler::Equal, operand, &isInt32);
    masm.branchTestBoolean(Assembler::Equal, operand, &isBool);
    masm.branchTestNull(Assembler::Equal, operand, &isNull);

    Assembler::Condition cond = masm.testUndefined(Assembler::NotEqual, operand);
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
    emitDoubleToInt32(input, output, &fail);
    if (!bailoutFrom(&fail, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitTestVAndBranch(LTestVAndBranch *lir)
{
    const ValueOperand value = ToValue(lir, LTestVAndBranch::Input);

    Register tag = masm.splitTagForTest(value);

    Assembler::Condition cond;

    
    
    
    
    masm.branchTestUndefined(Assembler::Equal, tag, lir->ifFalse());

    masm.branchTestNull(Assembler::Equal, tag, lir->ifFalse());
    masm.branchTestObject(Assembler::Equal, tag, lir->ifTrue());

    Label notBoolean;
    masm.branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
    masm.branchTestBooleanTruthy(false, value, lir->ifFalse());
    masm.jump(lir->ifTrue());
    masm.bind(&notBoolean);

    Label notInt32;
    masm.branchTestInt32(Assembler::NotEqual, tag, &notInt32);
    cond = masm.testInt32Truthy(false, value);
    masm.j(cond, lir->ifFalse());
    masm.jump(lir->ifTrue());
    masm.bind(&notInt32);

    
    Label notString;
    masm.branchTestString(Assembler::NotEqual, tag, &notString);
    cond = masm.testStringTruthy(false, value);
    masm.j(cond, lir->ifFalse());
    masm.jump(lir->ifTrue());
    masm.bind(&notString);

    
    masm.unboxDouble(value, ToFloatRegister(lir->tempFloat()));
    cond = masm.testDoubleTruthy(false, ToFloatRegister(lir->tempFloat()));
    masm.j(cond, lir->ifFalse());
    masm.jump(lir->ifTrue());

    return true;
}

bool
CodeGenerator::visitTruncateDToInt32(LTruncateDToInt32 *lir)
{
    Label fails;

    emitTruncateDouble(ToFloatRegister(lir->input()), ToRegister(lir->output()), &fails);
    if (!bailoutFrom(&fails, lir->snapshot()))
        return false;

    return true;
}

bool
CodeGenerator::visitIntToString(LIntToString *lir)
{
    typedef JSString *(*pf)(JSContext *, jsint);
    static const VMFunction js_IntToStringInfo = FunctionInfo<pf>(js_IntToString);

    pushArg(ToRegister(lir->input()));
    return callVM(js_IntToStringInfo, lir);
}

bool
CodeGenerator::visitRegExp(LRegExp *lir)
{
    GlobalObject *global = gen->info().script()->global();
    JSObject *proto = global->getOrCreateRegExpPrototype(gen->cx);

    typedef JSObject *(*pf)(JSContext *, JSObject *, JSObject *);
    static const VMFunction CloneRegExpObjectInfo = FunctionInfo<pf>(CloneRegExpObject);

    pushArg(ImmGCPtr(proto));
    pushArg(ImmGCPtr(lir->mir()->source()));
    return callVM(CloneRegExpObjectInfo, lir);
}

bool
CodeGenerator::visitLambda(LLambda *lir)
{
    typedef JSObject *(*pf)(JSContext *, JSFunction *, JSObject *);
    static const VMFunction Info = FunctionInfo<pf>(js::Lambda);

    pushArg(ToRegister(lir->scopeChain()));
    pushArg(ImmGCPtr(lir->mir()->fun()));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitLambdaJoinableForCall(LLambdaJoinableForCall *lir)
{
    typedef JSObject *(*pf)(JSContext *, JSFunction *, JSObject *, JSObject *, uint32_t);
    static const VMFunction Info = FunctionInfo<pf>(js::LambdaJoinableForCall);

    pushArg(Imm32(lir->mir()->argc()));
    pushArg(ToRegister(lir->callee()));
    pushArg(ToRegister(lir->scopeChain()));
    pushArg(ImmGCPtr(lir->mir()->fun()));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitLambdaJoinableForSet(LLambdaJoinableForSet *lir)
{
    typedef JSObject *(*pf)(JSContext *, JSFunction *, JSObject *, JSObject *);
    static const VMFunction Info = FunctionInfo<pf>(js::LambdaJoinableForSet);

    pushArg(ToRegister(lir->target()));
    pushArg(ToRegister(lir->scopeChain()));
    pushArg(ImmGCPtr(lir->mir()->fun()));
    return callVM(Info, lir);
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

    uint32 osiReturnPointOffset;
    if (!markOsiPoint(lir, &osiReturnPointOffset))
        return false;

    LSafepoint *safepoint = lir->associatedSafepoint();
    JS_ASSERT(!safepoint->osiReturnPointOffset());
    safepoint->setOsiReturnPointOffset(osiReturnPointOffset);
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
CodeGenerator::visitParameter(LParameter *lir)
{
    return true;
}

bool
CodeGenerator::visitCallee(LCallee *lir)
{
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
CodeGenerator::visitStackArg(LStackArg *lir)
{
    ValueOperand val = ToValue(lir, 0);
    uint32 argslot = lir->argslot();
    int32 stack_offset = StackOffsetOfPassedArg(argslot);

    masm.storeValue(val, Address(StackPointer, stack_offset));
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
    masm.movePtr(ImmGCPtr(lir->ptr()), ToRegister(lir->output()));
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
    int32 offset  = store->mir()->slot() * sizeof(Value);

    const ValueOperand value = ToValue(store, LStoreSlotV::Value);

    if (store->mir()->needsBarrier())
       masm.emitPreBarrier(Address(base, offset), JSVAL_TYPE_UNKNOWN);

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

bool
CodeGenerator::visitFlatClosureUpvars(LFlatClosureUpvars *lir)
{
    Register callee = ToRegister(lir->callee());
    Register output = ToRegister(lir->output());

    Address upvars(callee, JSFunction::getFlatClosureUpvarsOffset());
    masm.loadPrivate(upvars, output);
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
CodeGenerator::visitTypeBarrier(LTypeBarrier *lir)
{
    ValueOperand operand = ToValue(lir, LTypeBarrier::Input);
    Register scratch = ToRegister(lir->temp());

    Label mismatched;
    masm.guardTypeSet(operand, lir->mir()->typeSet(), scratch, &mismatched);
    if (!bailoutFrom(&mismatched, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitMonitorTypes(LMonitorTypes *lir)
{
    ValueOperand operand = ToValue(lir, LMonitorTypes::Input);
    Register scratch = ToRegister(lir->temp());

    Label mismatched;
    masm.guardTypeSet(operand, lir->mir()->typeSet(), scratch, &mismatched);
    if (!bailoutFrom(&mismatched, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitCallNative(LCallNative *call)
{
    JSFunction *target = call->function();
    JS_ASSERT(target);
    JS_ASSERT(target->isNative());

    int callargslot = call->argslot();
    int unusedStack = StackOffsetOfPassedArg(callargslot);

    
    const Register argJSContextReg = ToRegister(call->getArgJSContextReg());
    const Register argUintNReg     = ToRegister(call->getArgUintNReg());
    const Register argVpReg        = ToRegister(call->getArgVpReg());

    
    const Register tempReg = ToRegister(call->getTempReg());

    DebugOnly<uint32> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    
    
    

    
    masm.adjustStack(unusedStack);

    
    
    masm.Push(ObjectValue(*target));

    
    masm.loadJSContext(gen->cx->runtime, argJSContextReg);
    masm.move32(Imm32(call->nargs()), argUintNReg);
    masm.movePtr(StackPointer, argVpReg);

    
    uint32 safepointOffset = masm.buildFakeExitFrame(tempReg);
    masm.linkExitFrame();
    if (!markSafepointAt(safepointOffset, call))
        return false;

    
    masm.setupUnalignedABICall(3, tempReg);
    masm.setABIArg(0, argJSContextReg);
    masm.setABIArg(1, argUintNReg);
    masm.setABIArg(2, argVpReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, target->native()));

    
    Label success, exception;
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

    
    masm.loadValue(Address(StackPointer, IonExitFrameLayout::Size()), JSReturnOperand);
    masm.jump(&success);

    
    {
        masm.bind(&exception);
        masm.handleException();
    }
    masm.bind(&success);

    
    masm.adjustStack(IonExitFrameLayout::Size() - unusedStack + sizeof(Value));
    JS_ASSERT(masm.framePushed() == initialStack);

    return true;
}

bool
CodeGenerator::visitCallGeneric(LCallGeneric *call)
{
    
    const LAllocation *callee = call->getFunction();
    Register calleereg = ToRegister(callee);

    
    const LAllocation *obj = call->getTempObject();
    Register objreg = ToRegister(obj);

    
    const LAllocation *nargs = call->getNargsReg();
    Register nargsreg = ToRegister(nargs);

    uint32 callargslot = call->argslot();
    uint32 unusedStack = StackOffsetOfPassedArg(callargslot);


    masm.checkStackAlignment();

    
    if (!call->hasSingleTarget()) {
        masm.loadObjClass(calleereg, nargsreg);
        masm.cmpPtr(nargsreg, ImmWord(&js::FunctionClass));
        if (!bailoutIf(Assembler::NotEqual, call->snapshot()))
            return false;
    }

    
    
    
    if (call->mir()->isConstruct()) {
        typedef bool (*pf)(JSContext *, JSFunction *, uint32, Value *, Value *);
        static const VMFunction InvokeConstructorFunctionInfo =
            FunctionInfo<pf>(InvokeConstructorFunction);

        
        
        masm.freeStack(unusedStack);

        pushArg(StackPointer);          
        pushArg(Imm32(call->nargs()));  
        pushArg(calleereg);             

        if (!callVM(InvokeConstructorFunctionInfo, call))
            return false;

        
        masm.reserveStack(unusedStack);

        return true;
    }

    Label end, invoke;

    
    
    
    if (!call->hasSingleTarget()) {
        masm.branchTest32(Assembler::Zero, Address(calleereg, offsetof(JSFunction, flags)),
                          Imm32(JSFUN_INTERPRETED), &invoke);
    } else {
        
        JS_ASSERT(!call->getSingleTarget()->isNative());
    }


    
    masm.movePtr(Address(calleereg, offsetof(JSFunction, u.i.script_)), objreg);
    masm.movePtr(Address(objreg, offsetof(JSScript, ion)), objreg);

    
    masm.branchPtr(Assembler::BelowOrEqual, objreg, ImmWord(ION_DISABLED_SCRIPT), &invoke);

    
    masm.freeStack(unusedStack);

    
    uint32 descriptor = MakeFrameDescriptor(masm.framePushed(), IonFrame_JS);
    masm.Push(calleereg);
    masm.Push(Imm32(descriptor));

    Label thunk, rejoin;

    if (call->hasSingleTarget()) {
        
        JS_ASSERT(call->getSingleTarget()->nargs <= call->nargs());
    } else {
        
        masm.load16(Address(calleereg, offsetof(JSFunction, nargs)), nargsreg);
        masm.cmp32(nargsreg, Imm32(call->nargs()));
        masm.j(Assembler::Above, &thunk);
    }

    
    {
        masm.movePtr(Address(objreg, offsetof(IonScript, method_)), objreg);
        masm.movePtr(Address(objreg, IonCode::OffsetOfCode()), objreg);
    }

    
    if (!call->hasSingleTarget()) {
        
        masm.jump(&rejoin);
        masm.bind(&thunk);

        
        IonCompartment *ion = gen->ionCompartment();
        IonCode *argumentsRectifier = ion->getArgumentsRectifier(gen->cx);
        if (!argumentsRectifier)
            return false;

        JS_ASSERT(ArgumentsRectifierReg != objreg);
        masm.move32(Imm32(call->nargs()), ArgumentsRectifierReg);
        masm.movePtr(ImmWord(argumentsRectifier->raw()), objreg);
    }

    masm.bind(&rejoin);

    masm.checkStackAlignment();

    
    masm.callIon(objreg);
    if (!markSafepoint(call))
        return false;


    
    
    int prefixGarbage = sizeof(IonJSFrameLayout) - sizeof(void *);
    masm.adjustStack(prefixGarbage - unusedStack);

    masm.jump(&end);

    
    {
        masm.bind(&invoke);

        typedef bool (*pf)(JSContext *, JSFunction *, uint32, Value *, Value *);
        static const VMFunction InvokeFunctionInfo = FunctionInfo<pf>(InvokeFunction);

        
        
        masm.freeStack(unusedStack);

        pushArg(StackPointer);          
        pushArg(Imm32(call->nargs()));  
        pushArg(calleereg);             

        if (!callVM(InvokeFunctionInfo, call))
            return false;

        
        masm.reserveStack(unusedStack);
    }

    masm.bind(&end);

    return true;
}


static const uint32 EntryTempMask = Registers::TempMask & ~(1 << OsrFrameReg.code());

bool
CodeGenerator::generateArgumentsChecks()
{
    MIRGraph &mir = gen->graph();
    MResumePoint *rp = mir.entryResumePoint();

    
    
    
    masm.reserveStack(frameSize());

    
    Register temp = GeneralRegisterSet(EntryTempMask).getAny();

    CompileInfo &info = gen->info();

    
    JS_ASSERT(info.scopeChainSlot() == 0);
    static const uint32 START_SLOT = 1;

    Label mismatched;
    for (uint32 i = START_SLOT; i < CountArgSlots(info.fun()); i++) {
        
        MParameter *param = rp->getOperand(i)->toParameter();
        types::TypeSet *types = param->typeSet();
        if (!types || types->unknown())
            continue;

        
        
        int32 offset = ArgToStackOffset((i - START_SLOT) * sizeof(Value));
        masm.guardTypeSet(Address(StackPointer, offset), types, temp, &mismatched);
    }

    if (mismatched.used() && !bailoutFrom(&mismatched, graph.entrySnapshot()))
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
    
    
    
    
    
    
    
    

    JSRuntime *rt = gen->cx->runtime;

    
    const LAllocation *limit = lir->limitTemp();
    Register limitReg = ToRegister(limit);

    
    
    uintptr_t *limitAddr = &rt->ionStackLimit;
    masm.loadPtr(ImmWord(limitAddr), limitReg);

    CheckOverRecursedFailure *ool = new CheckOverRecursedFailure(lir);
    if (!addOutOfLineCode(ool))
        return false;

    
    masm.branchPtr(Assembler::BelowOrEqual, StackPointer, limitReg, ool->entry());

    return true;
}

bool
CodeGenerator::visitDefVar(LDefVar *lir)
{
    Register scopeChain = ToRegister(lir->getScopeChain());
    Register nameTemp   = ToRegister(lir->nameTemp());

    typedef bool (*pf)(JSContext *, PropertyName *, uintN, JSObject *);
    static const VMFunction DefVarOrConstInfo =
        FunctionInfo<pf>(DefVarOrConst);

    masm.movePtr(ImmWord(lir->mir()->name()), nameTemp);

    pushArg(scopeChain); 
    pushArg(Imm32(lir->mir()->attrs())); 
    pushArg(nameTemp); 

    if (!callVM(DefVarOrConstInfo, lir))
        return false;

    return true;
}

bool
CodeGenerator::visitCheckOverRecursedFailure(CheckOverRecursedFailure *ool)
{
    
    

    typedef bool (*pf)(JSContext *);
    static const VMFunction ReportOverRecursedInfo =
        FunctionInfo<pf>(ReportOverRecursed);

    if (!callVM(ReportOverRecursedInfo, ool->lir()))
        return false;

#ifdef DEBUG
    
    masm.breakpoint();
#endif
    return true;
}

bool
CodeGenerator::generateBody()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        current = graph.getBlock(i);
        for (LInstructionIterator iter = current->begin(); iter != current->end(); iter++) {
            IonSpew(IonSpew_Codegen, "instruction %s", iter->opName());
            if (!iter->accept(this))
                return false;
        }
        if (masm.oom())
            return false;
    }
    return true;
}

bool
CodeGenerator::visitNewArray(LNewArray *lir)
{
    typedef JSObject *(*pf)(JSContext *, uint32, types::TypeObject *);
    static const VMFunction NewInitArrayInfo = FunctionInfo<pf>(NewInitArray);

    pushArg(ImmGCPtr(lir->mir()->type()));
    pushArg(Imm32(lir->mir()->count()));
    if (!callVM(NewInitArrayInfo, lir))
        return false;
    return true;
}

bool
CodeGenerator::visitNewObject(LNewObject *lir)
{
    typedef JSObject *(*pf)(JSContext *, JSObject *, types::TypeObject *);
    static const VMFunction Info = FunctionInfo<pf>(CopyInitializerObject);

    pushArg(ImmGCPtr(lir->mir()->type()));
    pushArg(ImmGCPtr(lir->mir()->baseObj()));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitArrayLength(LArrayLength *lir)
{
    Address length(ToRegister(lir->elements()), ObjectElements::offsetOfLength());
    masm.load32(length, ToRegister(lir->output()));
    return true;
}

bool
CodeGenerator::visitStringLength(LStringLength *lir)
{
    Address lengthAndFlags(ToRegister(lir->string()), JSString::offsetOfLengthAndFlags());
    Register output = ToRegister(lir->output());

    masm.loadPtr(lengthAndFlags, output);
    masm.rshiftPtr(Imm32(JSString::LENGTH_SHIFT), output);
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
    if (!ins->snapshot() || !bailoutIf(Assembler::Overflow, ins->snapshot()))
        return false;
    masm.bind(&positive);

    return true;
}

bool
CodeGenerator::visitBinaryV(LBinaryV *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, const Value &, Value *);
    static const VMFunction AddInfo = FunctionInfo<pf>(js::AddValues);
    static const VMFunction SubInfo = FunctionInfo<pf>(js::SubValues);
    static const VMFunction MulInfo = FunctionInfo<pf>(js::MulValues);
    static const VMFunction DivInfo = FunctionInfo<pf>(js::DivValues);
    static const VMFunction ModInfo = FunctionInfo<pf>(js::ModValues);

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

      default:
        JS_NOT_REACHED("Unexpected binary op");
        return false;
    }
}

bool
CodeGenerator::visitCompareV(LCompareV *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, const Value &, JSBool *);
    static const VMFunction EqInfo = FunctionInfo<pf>(ion::LooselyEqual<true>);
    static const VMFunction NeInfo = FunctionInfo<pf>(ion::LooselyEqual<false>);
    static const VMFunction StrictEqInfo = FunctionInfo<pf>(ion::StrictlyEqual<true>);
    static const VMFunction StrictNeInfo = FunctionInfo<pf>(ion::StrictlyEqual<false>);
    static const VMFunction LtInfo = FunctionInfo<pf>(ion::LessThan);
    static const VMFunction LeInfo = FunctionInfo<pf>(ion::LessThanOrEqual);
    static const VMFunction GtInfo = FunctionInfo<pf>(ion::GreaterThan);
    static const VMFunction GeInfo = FunctionInfo<pf>(ion::GreaterThanOrEqual);

    pushArg(ToValue(lir, LBinaryV::RhsInput));
    pushArg(ToValue(lir, LBinaryV::LhsInput));

    switch (lir->jsop()) {
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
CodeGenerator::visitIsNullOrUndefined(LIsNullOrUndefined *lir)
{
    JSOp op = lir->mir()->jsop();
    MIRType specialization = lir->mir()->specialization();
    JS_ASSERT(IsNullOrUndefined(specialization));

    const ValueOperand value = ToValue(lir, LIsNullOrUndefined::Value);
    Register output = ToRegister(lir->output());

    if (op == JSOP_EQ || op == JSOP_NE) {
        Register tag = masm.splitTagForTest(value);

        Label nullOrUndefined, done;
        masm.branchTestNull(Assembler::Equal, tag, &nullOrUndefined);
        masm.branchTestUndefined(Assembler::Equal, tag, &nullOrUndefined);

        masm.move32(Imm32(op == JSOP_NE), output);
        masm.jump(&done);

        masm.bind(&nullOrUndefined);
        masm.move32(Imm32(op == JSOP_EQ), output);
        masm.bind(&done);
        return true;
    }

    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    Assembler::Condition cond = JSOpToCondition(op);
    if (specialization == MIRType_Null)
        cond = masm.testNull(cond, value);
    else
        cond = masm.testUndefined(cond, value);

    emitSet(cond, output);
    return true;
}

bool
CodeGenerator::visitIsNullOrUndefinedAndBranch(LIsNullOrUndefinedAndBranch *lir)
{
    JSOp op = lir->mir()->jsop();
    MIRType specialization = lir->mir()->specialization();
    JS_ASSERT(IsNullOrUndefined(specialization));

    const ValueOperand value = ToValue(lir, LIsNullOrUndefinedAndBranch::Value);

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

        Register tag = masm.splitTagForTest(value);
        masm.branchTestNull(Assembler::Equal, tag, ifTrue->lir()->label());

        Assembler::Condition cond = masm.testUndefined(Assembler::Equal, tag);
        emitBranch(cond, ifTrue, ifFalse);
        return true;
    }

    JS_ASSERT(op == JSOP_STRICTEQ || op == JSOP_STRICTNE);

    Assembler::Condition cond = JSOpToCondition(op);
    if (specialization == MIRType_Null)
        cond = masm.testNull(cond, value);
    else
        cond = masm.testUndefined(cond, value);

    emitBranch(cond, lir->ifTrue(), lir->ifFalse());
    return true;
}

bool
CodeGenerator::visitConcat(LConcat *lir)
{
    typedef JSString *(*pf)(JSContext *, JSString *, JSString *);
    static const VMFunction js_ConcatStringsInfo = FunctionInfo<pf>(js_ConcatStrings);

    pushArg(ToRegister(lir->rhs()));
    pushArg(ToRegister(lir->lhs()));
    if (!callVM(js_ConcatStringsInfo, lir))
        return false;
    return true;
}

bool
CodeGenerator::visitCharCodeAt(LCharCodeAt *lir)
{
    Register str = ToRegister(lir->str());
    Register index = ToRegister(lir->index());
    Register output = ToRegister(lir->output());

    typedef bool (*pf)(JSContext *, JSString *);
    static const VMFunction ensureLinearInfo = FunctionInfo<pf>(JSString::ensureLinear);
    OutOfLineCode *ool = oolCallVM(ensureLinearInfo, lir, (ArgList(), str), StoreNothing());
    if (!ool)
        return false;

    Address lengthAndFlagsAddr(str, JSString::offsetOfLengthAndFlags());
    masm.loadPtr(lengthAndFlagsAddr, output);

    JS_ASSERT(JSString::LINEAR_FLAGS == 0);
    masm.branchTest32(Assembler::NonZero, output, Imm32(JSString::LINEAR_MASK), ool->entry());
    masm.bind(ool->rejoin());

    
    Address charsAddr(str, JSString::offsetOfChars());
    masm.loadPtr(charsAddr, output);
    masm.load16(BaseIndex(output, index, TimesTwo, 0), output);

    return true;
}

bool
CodeGenerator::visitFromCharCode(LFromCharCode *lir)
{
    Register code = ToRegister(lir->code());
    Register output = ToRegister(lir->output());

    
    Label fast;
    masm.cmpPtr(code, ImmWord(StaticStrings::UNIT_STATIC_LIMIT));
    masm.j(Assembler::Below, &fast);

    
    
    static jschar tmpString[2] = {0, 0};
    Register tmpStringAddr = output;
    masm.movePtr(ImmWord(tmpString), tmpStringAddr);
    masm.store16(code, Address(tmpStringAddr, 0));

    
    typedef JSFixedString *(*pf)(JSContext *, const jschar *, size_t);
    static const VMFunction newStringCopyNInfo = FunctionInfo<pf>(js_NewStringCopyN);
    OutOfLineCode *ool = oolCallVM(newStringCopyNInfo, lir, (ArgList(), tmpStringAddr, Imm32(1)),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    masm.jump(ool->entry());
    masm.bind(&fast);
    masm.movePtr(ImmWord(&gen->cx->runtime->staticStrings.unitStaticTable), output);
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
CodeGenerator::visitNotV(LNotV *ins)
{
    typedef bool (*pf)(JSContext *, const Value &, JSBool *);
    static const VMFunction FValueToBooleanComplement = FunctionInfo<pf>(ValueToBooleanComplement);

    pushArg(ToValue(ins, LNotV::Input));
    return callVM(FValueToBooleanComplement, ins);
}

bool
CodeGenerator::visitBoundsCheck(LBoundsCheck *lir)
{
    if (lir->index()->isConstant())
        masm.cmp32(ToRegister(lir->length()), Imm32(ToInt32(lir->index())));
    else
        masm.cmp32(ToRegister(lir->length()), ToRegister(lir->index()));
    return bailoutIf(Assembler::BelowOrEqual, lir->snapshot());
}

bool
CodeGenerator::visitBoundsCheckRange(LBoundsCheckRange *lir)
{
    int32 min = lir->mir()->minimum();
    int32 max = lir->mir()->maximum();
    JS_ASSERT(max >= min);

    Register temp = ToRegister(lir->getTemp(0));
    if (lir->index()->isConstant()) {
        int32 nmin, nmax;
        int32 index = ToInt32(lir->index());
        if (SafeAdd(index, min, &nmin) && SafeAdd(index, max, &nmax) && nmin >= 0) {
            masm.cmp32(ToRegister(lir->length()), Imm32(nmax));
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
            int32 diff;
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

    masm.cmp32(ToRegister(lir->length()), temp);
    return bailoutIf(Assembler::BelowOrEqual, lir->snapshot());
}

bool
CodeGenerator::visitBoundsCheckLower(LBoundsCheckLower *lir)
{
    int32 min = lir->mir()->minimum();
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
CodeGenerator::visitStoreElementT(LStoreElementT *store)
{
    if (store->mir()->needsBarrier())
       emitPreBarrier(ToRegister(store->elements()), store->index(), store->mir()->elementType());

    storeElementTyped(store->value(), store->mir()->value()->type(), store->mir()->elementType(),
                      ToRegister(store->elements()), store->index());
    return true;
}

bool
CodeGenerator::visitStoreElementV(LStoreElementV *lir)
{
    const ValueOperand value = ToValue(lir, LStoreElementV::Value);
    Register elements = ToRegister(lir->elements());

    if (lir->mir()->needsBarrier())
        emitPreBarrier(elements, lir->index(), MIRType_Value);

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

    typedef bool (*pf)(JSContext *, JSObject *, const Value &, const Value &);
    static const VMFunction Info = FunctionInfo<pf>(SetObjectElement);

    pushArg(value);
    if (index->isConstant())
        pushArg(*index->toConstant());
    else
        pushArg(TypedOrValueRegister(MIRType_Int32, ToAnyRegister(index)));
    pushArg(object);
    if (!callVM(Info, ins))
        return false;

    restoreLive(ins);
    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitCallIteratorStart(LCallIteratorStart *lir)
{
    typedef JSObject *(*pf)(JSContext *, JSObject *, uint32_t);
    static const VMFunction Info = FunctionInfo<pf>(GetIteratorObject);

    const Register objReg = ToRegister(lir->getOperand(0));

    pushArg(Imm32(lir->mir()->flags()));
    pushArg(objReg);
    return callVM(Info, lir);
}

bool
CodeGenerator::visitCallIteratorNext(LCallIteratorNext *lir)
{
    typedef bool (*pf)(JSContext *, JSObject *, Value *);
    static const VMFunction Info = FunctionInfo<pf>(js_IteratorNext);

    const Register objReg = ToRegister(lir->getOperand(0));

    pushArg(objReg);
    return callVM(Info, lir);
}

bool
CodeGenerator::visitCallIteratorMore(LCallIteratorMore *lir)
{
    typedef bool (*pf)(JSContext *, JSObject *, Value *);
    static const VMFunction Info = FunctionInfo<pf>(js_IteratorMore);

    const Register objReg = ToRegister(lir->getOperand(0));

    pushArg(objReg);
    if (!callVM(Info, lir))
        return false;

    
    Register output = ToRegister(lir->getDef(0));
    masm.unboxValue(JSReturnOperand, AnyRegister(output));

    return true;
}

bool
CodeGenerator::visitCallIteratorEnd(LCallIteratorEnd *lir)
{
    typedef bool (*pf)(JSContext *, JSObject *);
    static const VMFunction Info = FunctionInfo<pf>(CloseIterator);

    const Register objReg = ToRegister(lir->getOperand(0));

    pushArg(objReg);
    return callVM(Info, lir);
}

bool
CodeGenerator::generate()
{
    JSContext *cx = gen->cx;

    if (!safepoints_.init(graph.localSlotCount()))
        return false;

    
    
    
    if (!generateArgumentsChecks())
        return false;

    if (frameClass_ != FrameSizeClass::None()) {
        deoptTable_ = cx->compartment->ionCompartment()->getBailoutTable(cx, frameClass_);
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

    if (masm.oom())
        return false;

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return false;

    
    encodeSafepoints();

    JSScript *script = gen->info().script();
    JS_ASSERT(!script->ion);

    uint32 scriptFrameSize = frameClass_ == FrameSizeClass::None()
                           ? frameDepth_
                           : FrameSizeClass::FromDepth(frameDepth_).frameSize();

    script->ion = IonScript::New(cx, graph.localSlotCount(), scriptFrameSize, snapshots_.size(),
                                 bailouts_.length(), graph.numConstants(),
                                 safepointIndices_.length(), osiIndices_.length(),
                                 cacheList_.length(), safepoints_.size());
    if (!script->ion)
        return false;
    invalidateEpilogueData_.fixup(&masm);
    Assembler::patchDataWithValueCheck(CodeLocationLabel(code, invalidateEpilogueData_),
                                       ImmWord(uintptr_t(script->ion)),
                                       ImmWord(uintptr_t(-1)));

    IonSpew(IonSpew_Codegen, "Created IonScript %p (raw %p)",
            (void *) script->ion, (void *) code->raw());

    script->ion->setInvalidationEpilogueDataOffset(invalidateEpilogueData_.offset());
    script->ion->setOsrPc(gen->info().osrPc());
    script->ion->setOsrEntryOffset(getOsrEntryOffset());
    ptrdiff_t real_invalidate = masm.actualOffset(invalidate_.offset());
    script->ion->setInvalidationEpilogueOffset(real_invalidate);

    script->ion->setMethod(code);
    script->ion->setDeoptTable(deoptTable_);
    if (snapshots_.size())
        script->ion->copySnapshots(&snapshots_);
    if (bailouts_.length())
        script->ion->copyBailoutTable(&bailouts_[0]);
    if (graph.numConstants())
        script->ion->copyConstants(graph.constantPool());
    if (safepointIndices_.length())
        script->ion->copySafepointIndices(&safepointIndices_[0], masm);
    if (osiIndices_.length())
        script->ion->copyOsiIndices(&osiIndices_[0], masm);
    if (cacheList_.length())
        script->ion->copyCacheEntries(&cacheList_[0], masm);
    if (safepoints_.size())
        script->ion->copySafepoints(&safepoints_);

    linkAbsoluteLabels();

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

typedef bool (*GetPropertyOrNameFn)(JSContext *, JSObject *, PropertyName *, Value *);

bool
CodeGenerator::visitCallGetProperty(LCallGetProperty *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, PropertyName *, Value *);
    static const VMFunction Info = FunctionInfo<pf>(GetProperty);

    pushArg(ImmGCPtr(lir->mir()->atom()));
    pushArg(ToValue(lir, LCallGetProperty::Value));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitCallGetName(LCallGetName *lir)
{
    static const VMFunction Info = FunctionInfo<GetPropertyOrNameFn>(GetScopeName);

    pushArg(ImmGCPtr(lir->mir()->atom()));
    pushArg(ToRegister(lir->getOperand(0)));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitCallGetNameTypeOf(LCallGetNameTypeOf *lir)
{
    static const VMFunction Info = FunctionInfo<GetPropertyOrNameFn>(GetScopeNameForTypeOf);

    pushArg(ImmGCPtr(lir->mir()->atom()));
    pushArg(ToRegister(lir->getOperand(0)));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitCallGetElement(LCallGetElement *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, const Value &, Value *);
    static const VMFunction GetElementInfo = FunctionInfo<pf>(js::GetElement);

    pushArg(ToValue(lir, LCallGetElement::RhsInput));
    pushArg(ToValue(lir, LCallGetElement::LhsInput));
    return callVM(GetElementInfo, lir);
}

bool
CodeGenerator::visitCallSetElement(LCallSetElement *lir)
{
    typedef bool (*pf)(JSContext *, JSObject *, const Value &, const Value &);
    static const VMFunction SetObjectElementInfo = FunctionInfo<pf>(js::SetObjectElement);

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

    masm.loadUnboxedValue(Address(obj, JSObject::getFixedSlotOffset(slot)), result);
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
        masm.emitPreBarrier(address, JSVAL_TYPE_UNKNOWN);

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
        masm.emitPreBarrier(address, JSVAL_TYPE_UNKNOWN);

    masm.storeConstantOrRegister(nvalue, address);

    return true;
}


class OutOfLineCache : public OutOfLineCodeBase<CodeGenerator>
{
    LInstruction *ins;

    CodeOffsetJump inlineJump;
    CodeOffsetLabel inlineLabel;

  public:
    OutOfLineCache(LInstruction *ins)
      : ins(ins)
    {}

    void setInlineJump(CodeOffsetJump jump, CodeOffsetLabel label) {
        inlineJump = jump;
        inlineLabel = label;
    }

    CodeOffsetJump getInlineJump() const {
        return inlineJump;
    }

    CodeOffsetLabel getInlineLabel() const {
        return inlineLabel;
    }

    bool accept(CodeGenerator *codegen) {
        switch (ins->op()) {
          case LInstruction::LOp_GetPropertyCacheT:
          case LInstruction::LOp_GetPropertyCacheV:
            return codegen->visitOutOfLineCacheGetProperty(this);
          case LInstruction::LOp_SetPropertyCacheT:
          case LInstruction::LOp_SetPropertyCacheV:
            return codegen->visitOutOfLineSetPropertyCache(this);
          case LInstruction::LOp_BindNameCache:
            return codegen->visitOutOfLineBindNameCache(this);
          default:
            JS_NOT_REACHED("Bad instruction");
            return false;
        }
    }

    LInstruction *cache() {
        return ins;
    }
};

bool
CodeGenerator::visitCache(LInstruction *ins)
{
    OutOfLineCache *ool = new OutOfLineCache(ins);
    if (!addOutOfLineCode(ool))
        return false;

    CodeOffsetJump jump = masm.jumpWithPatch(ool->entry());
    CodeOffsetLabel label = masm.labelForPatch();
    masm.bind(ool->rejoin());

    ool->setInlineJump(jump, label);
    return true;
}

bool
CodeGenerator::visitOutOfLineCacheGetProperty(OutOfLineCache *ool)
{
    Register objReg = ToRegister(ool->cache()->getOperand(0));
    RegisterSet liveRegs = ool->cache()->safepoint()->liveRegs();

    LInstruction *ins_ = ool->cache();
    const MGetPropertyCache *mir;

    TypedOrValueRegister output;

    if (ins_->op() == LInstruction::LOp_GetPropertyCacheT) {
        LGetPropertyCacheT *ins = (LGetPropertyCacheT *) ins_;
        output = TypedOrValueRegister(ins->mir()->type(), ToAnyRegister(ins->getDef(0)));
        mir = ins->mir();
    } else {
        LGetPropertyCacheV *ins = (LGetPropertyCacheV *) ins_;
        output = TypedOrValueRegister(GetValueOutput(ins));
        mir = ins->mir();
    }

    IonCacheGetProperty cache(ool->getInlineJump(), ool->getInlineLabel(),
                              masm.labelForPatch(), liveRegs,
                              objReg, mir->atom(), output);

    cache.setScriptedLocation(mir->block()->info().script(), mir->resumePoint()->pc());
    size_t cacheIndex = allocateCache(cache);

    saveLive(ins_);

    typedef bool (*pf)(JSContext *, size_t, JSObject *, Value *);
    static const VMFunction GetPropertyCacheInfo = FunctionInfo<pf>(GetPropertyCache);

    pushArg(objReg);
    pushArg(Imm32(cacheIndex));
    if (!callVM(GetPropertyCacheInfo, ins_))
        return false;

    masm.storeCallResultValue(output);
    restoreLive(ins_);

    masm.jump(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitOutOfLineBindNameCache(OutOfLineCache *ool)
{
    LBindNameCache *ins = ool->cache()->toBindNameCache();
    Register scopeChain = ToRegister(ins->scopeChain());
    Register output = ToRegister(ins->output());

    RegisterSet liveRegs = ins->safepoint()->liveRegs();

    const MBindNameCache *mir = ins->mir();
    IonCacheBindName cache(ool->getInlineJump(), ool->getInlineLabel(),
                           masm.labelForPatch(), liveRegs,
                           scopeChain, mir->name(), output);
    cache.setScriptedLocation(mir->script(), mir->pc());
    size_t cacheIndex = allocateCache(cache);

    saveLive(ins);

    typedef JSObject *(*pf)(JSContext *, size_t, JSObject *);
    static const VMFunction BindNameCacheInfo = FunctionInfo<pf>(BindNameCache);

    pushArg(scopeChain);
    pushArg(Imm32(cacheIndex));
    if (!callVM(BindNameCacheInfo, ins))
        return false;

    masm.storeCallResult(output);
    restoreLive(ins);

    masm.jump(ool->rejoin());
    return true;
}

ConstantOrRegister
CodeGenerator::getSetPropertyValue(LInstruction *ins)
{
    if (ins->getOperand(1)->isConstant()) {
        JS_ASSERT(ins->isSetPropertyCacheT());
        return ConstantOrRegister(*ins->getOperand(1)->toConstant());
    }

    switch (ins->op()) {
      case LInstruction::LOp_CallSetProperty:
        return TypedOrValueRegister(ToValue(ins, LCallSetProperty::Value));
      case LInstruction::LOp_SetPropertyCacheV:
        return TypedOrValueRegister(ToValue(ins, LSetPropertyCacheV::Value));
      case LInstruction::LOp_SetPropertyCacheT: {
        LSetPropertyCacheT *ins_ = ins->toSetPropertyCacheT();
        return TypedOrValueRegister(ins_->valueType(), ToAnyRegister(ins->getOperand(1)));
      }
      default:
        JS_NOT_REACHED("Bad opcode");
        return ConstantOrRegister(UndefinedValue());
    }
}

bool
CodeGenerator::visitCallSetProperty(LCallSetProperty *ins)
{
    ConstantOrRegister value = getSetPropertyValue(ins);

    const Register objReg = ToRegister(ins->getOperand(0));

    pushArg(value);
    pushArg(ImmGCPtr(ins->mir()->atom()));
    pushArg(objReg);

    typedef bool (*pf)(JSContext *, JSObject *, JSAtom *, Value);
    if (ins->mir()->strict()) {
        static const VMFunction info = FunctionInfo<pf>(SetProperty<true>);
        if (!callVM(info, ins))
            return false;
    } else {
        static const VMFunction info = FunctionInfo<pf>(SetProperty<false>);
        if (!callVM(info, ins))
            return false;
    }

    return true;
}

bool
CodeGenerator::visitOutOfLineSetPropertyCache(OutOfLineCache *ool)
{
    LInstruction *ins = ool->cache();

    Register objReg = ToRegister(ins->getOperand(0));
    RegisterSet liveRegs = ins->safepoint()->liveRegs();

    ConstantOrRegister value = getSetPropertyValue(ins);
    const MSetPropertyCache *mir = ins->mirRaw()->toSetPropertyCache();

    IonCacheSetProperty cache(ool->getInlineJump(), ool->getInlineLabel(),
                              masm.labelForPatch(), liveRegs,
                              objReg, mir->atom(), value,
                              mir->strict());

    size_t cacheIndex = allocateCache(cache);

    saveLive(ins);

    pushArg(value);
    pushArg(objReg);
    pushArg(Imm32(cacheIndex));

    typedef bool (*pf)(JSContext *, size_t, JSObject *, Value);
    static const VMFunction info = FunctionInfo<pf>(ion::SetPropertyCache);

    if (!callVM(info, ool->cache()))
        return false;

    restoreLive(ins);

    masm.jump(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitThrow(LThrow *lir)
{
    typedef bool (*pf)(JSContext *, const Value &);
    static const VMFunction ThrowInfo = FunctionInfo<pf>(js::Throw);

    pushArg(ToValue(lir, LThrow::Value));
    return callVM(ThrowInfo, lir);
}

bool
CodeGenerator::visitBitNotV(LBitNotV *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, int *p);
    static const VMFunction info = FunctionInfo<pf>(BitNot);

    pushArg(ToValue(lir, LBitNotV::Input));
    return callVM(info, lir);
}

bool
CodeGenerator::visitBitOpV(LBitOpV *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, const Value &, int *p);
    static const VMFunction BitAndInfo = FunctionInfo<pf>(BitAnd);
    static const VMFunction BitOrInfo = FunctionInfo<pf>(BitOr);
    static const VMFunction BitXorInfo = FunctionInfo<pf>(BitXor);

    pushArg(ToValue(lir, LBitOpV::LhsInput));
    pushArg(ToValue(lir, LBitOpV::RhsInput));

    switch (lir->jsop()) {
      case JSOP_BITAND:
        return callVM(BitAndInfo, lir);
      case JSOP_BITOR:
        return callVM(BitOrInfo, lir);
      case JSOP_BITXOR:
        return callVM(BitXorInfo, lir);
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

    PropertyName **typeAtoms = GetIonContext()->cx->runtime->atomState.typeAtoms;

    
    
    masm.branchTestObject(Assembler::Equal, tag, ool->entry());

    Label done;

    Label notNumber;
    masm.branchTestNumber(Assembler::NotEqual, tag, &notNumber);
    masm.movePtr(ImmGCPtr(typeAtoms[JSTYPE_NUMBER]), output);
    masm.jump(&done);
    masm.bind(&notNumber);

    Label notUndefined;
    masm.branchTestUndefined(Assembler::NotEqual, tag, &notUndefined);
    masm.movePtr(ImmGCPtr(typeAtoms[JSTYPE_VOID]), output);
    masm.jump(&done);
    masm.bind(&notUndefined);

    Label notNull;
    masm.branchTestNull(Assembler::NotEqual, tag, &notNull);
    masm.movePtr(ImmGCPtr(typeAtoms[JSTYPE_OBJECT]), output);
    masm.jump(&done);
    masm.bind(&notNull);

    Label notBoolean;
    masm.branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
    masm.movePtr(ImmGCPtr(typeAtoms[JSTYPE_BOOLEAN]), output);
    masm.jump(&done);
    masm.bind(&notBoolean);

    masm.movePtr(ImmGCPtr(typeAtoms[JSTYPE_STRING]), output);

    masm.bind(&done);
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineTypeOfV(OutOfLineTypeOfV *ool)
{
    typedef JSString *(*pf)(JSContext *, const Value &);
    static const VMFunction Info = FunctionInfo<pf>(TypeOfOperation);

    LTypeOfV *ins = ool->ins();
    saveLive(ins);

    pushArg(ToValue(ins, LTypeOfV::Input));
    if (!callVM(Info, ins))
        return false;

    masm.storeCallResult(ToRegister(ins->output()));
    restoreLive(ins);

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitToIdV(LToIdV *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, const Value &, Value *);
    static const VMFunction Info = FunctionInfo<pf>(ToIdOperation);

    pushArg(ToValue(lir, LToIdV::Index));
    pushArg(ToValue(lir, LToIdV::Object));
    return callVM(Info, lir);
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

} 
} 

