






#include "CodeGenerator.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "MIRGenerator.h"
#include "shared/CodeGenerator-shared-inl.h"
#include "jsnum.h"
#include "jsmath.h"
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
        if (!emitTruncateDouble(temp, output))
            return false;
        break;
      default:
        JS_ASSERT(lir->mode() == LValueToInt32::NORMAL);
        emitDoubleToInt32(temp, output, &fails, lir->mir()->canBeNegativeZero());
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
    emitDoubleToInt32(input, output, &fail, lir->mir()->canBeNegativeZero());
    if (!bailoutFrom(&fail, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGenerator::visitTestVAndBranch(LTestVAndBranch *lir)
{
    const ValueOperand value = ToValue(lir, LTestVAndBranch::Input);
    masm.branchTestValueTruthy(value, lir->ifTrue(), ToFloatRegister(lir->tempFloat()));
    masm.jump(lir->ifFalse());
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
    } else {
        for (size_t i = 0; i < mir->numCallees(); i++) {
            JSFunction *func = mir->getFunction(i);
            LBlock *target = mir->getFunctionBlock(i)->lir();
            if (i < mir->numCallees() - 1) {
                masm.branchPtr(Assembler::Equal, inputReg, ImmGCPtr(func), target->label());
            } else {
                
                masm.jump(target->label());
            }
        }
    }
    return true;
}

bool
CodeGenerator::visitIntToString(LIntToString *lir)
{
    typedef JSFixedString *(*pf)(JSContext *, int);
    static const VMFunction IntToStringInfo = FunctionInfo<pf>(Int32ToString);

    pushArg(ToRegister(lir->input()));
    return callVM(IntToStringInfo, lir);
}

bool
CodeGenerator::visitRegExp(LRegExp *lir)
{
    JSObject *proto = lir->mir()->getRegExpPrototype();

    typedef JSObject *(*pf)(JSContext *, JSObject *, JSObject *);
    static const VMFunction CloneRegExpObjectInfo = FunctionInfo<pf>(CloneRegExpObject);

    pushArg(ImmGCPtr(proto));
    pushArg(ImmGCPtr(lir->mir()->source()));
    return callVM(CloneRegExpObjectInfo, lir);
}

bool
CodeGenerator::visitLambdaForSingleton(LLambdaForSingleton *lir)
{
    typedef JSObject *(*pf)(JSContext *, HandleFunction, HandleObject);
    static const VMFunction Info = FunctionInfo<pf>(js::Lambda);

    pushArg(ToRegister(lir->scopeChain()));
    pushArg(ImmGCPtr(lir->mir()->fun()));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitLambda(LLambda *lir)
{
    Register scopeChain = ToRegister(lir->scopeChain());
    Register output = ToRegister(lir->output());
    JSFunction *fun = lir->mir()->fun();

    typedef JSObject *(*pf)(JSContext *, HandleFunction, HandleObject);
    static const VMFunction Info = FunctionInfo<pf>(js::Lambda);

    OutOfLineCode *ool = oolCallVM(Info, lir, (ArgList(), ImmGCPtr(fun), scopeChain),
                                   StoreRegisterTo(output));
    if (!ool)
        return false;

    JS_ASSERT(gen->compartment == fun->compartment());
    JS_ASSERT(!fun->hasSingletonType());

    masm.newGCThing(output, fun, ool->entry());
    masm.initGCThing(output, fun);

    
    
    union {
        struct S {
            uint16_t nargs;
            uint16_t flags;
        } s;
        uint32_t word;
    } u;
    u.s.nargs = fun->nargs;
    u.s.flags = fun->flags & ~JSFUN_EXTENDED;

    JS_STATIC_ASSERT(offsetof(JSFunction, flags) == offsetof(JSFunction, nargs) + 2);
    masm.store32(Imm32(u.word), Address(output, offsetof(JSFunction, nargs)));
    masm.storePtr(ImmGCPtr(fun->script()), Address(output, JSFunction::offsetOfNativeOrScript()));
    masm.storePtr(scopeChain, Address(output, JSFunction::offsetOfEnvironment()));
    masm.storePtr(ImmGCPtr(fun->atom), Address(output, offsetof(JSFunction, atom)));

    masm.bind(ool->rejoin());
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

    uint32 osiCallPointOffset;
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
    int32 offset  = store->mir()->slot() * sizeof(Value);

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

    
    masm.loadJSContext(argJSContextReg);
    masm.move32(Imm32(call->numStackArgs()), argUintNReg);
    masm.movePtr(StackPointer, argVpReg);

    masm.Push(argUintNReg);

    
    uint32 safepointOffset;
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
CodeGenerator::emitCallInvokeFunction(LCallGeneric *call, uint32 unusedStack)
{
    typedef bool (*pf)(JSContext *, JSFunction *, uint32, Value *, Value *);
    static const VMFunction InvokeFunctionInfo = FunctionInfo<pf>(InvokeFunction);

    
    
    masm.freeStack(unusedStack);

    pushArg(StackPointer);                    
    pushArg(Imm32(call->numActualArgs()));    
    pushArg(ToRegister(call->getFunction())); 

    if (!callVM(InvokeFunctionInfo, call))
        return false;

    
    masm.reserveStack(unusedStack);

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

    
    if (call->hasSingleTarget() &&
        call->getSingleTarget()->script()->ion == ION_DISABLED_SCRIPT)
    {
        if (!emitCallInvokeFunction(call, unusedStack))
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

    Label end, invoke;

    
    
    
    if (!call->hasSingleTarget()) {
        Address flags(calleereg, offsetof(JSFunction, flags));
        masm.load16ZeroExtend_mask(flags, Imm32(JSFUN_INTERPRETED), nargsreg);
        masm.branch32(Assembler::NotEqual, nargsreg, Imm32(JSFUN_INTERPRETED), &invoke);
    } else {
        
        JS_ASSERT(!call->getSingleTarget()->isNative());
    }


    
    masm.movePtr(Address(calleereg, offsetof(JSFunction, u.i.script_)), objreg);
    masm.movePtr(Address(objreg, offsetof(JSScript, ion)), objreg);

    
    masm.branchPtr(Assembler::BelowOrEqual, objreg, ImmWord(ION_COMPILING_SCRIPT), &invoke);

    
    masm.freeStack(unusedStack);

    
    uint32 descriptor = MakeFrameDescriptor(masm.framePushed(), IonFrame_JS);
    masm.Push(Imm32(call->numActualArgs()));
    masm.Push(calleereg);
    masm.Push(Imm32(descriptor));

    Label thunk, rejoin;

    if (call->hasSingleTarget()) {
        
        JS_ASSERT(call->getSingleTarget()->nargs <= call->numStackArgs());
    } else {
        
        masm.load16ZeroExtend(Address(calleereg, offsetof(JSFunction, nargs)), nargsreg);
        masm.cmp32(nargsreg, Imm32(call->numStackArgs()));
        masm.j(Assembler::Above, &thunk);
    }

    
    {
        masm.movePtr(Address(objreg, offsetof(IonScript, method_)), objreg);
        masm.movePtr(Address(objreg, IonCode::OffsetOfCode()), objreg);
    }

    Label afterCall;

    
    if (!call->hasSingleTarget()) {
        
        masm.jump(&rejoin);
        masm.bind(&thunk);

        
        IonCompartment *ion = gen->ionCompartment();
        IonCode *argumentsRectifier = ion->getArgumentsRectifier(GetIonContext()->cx);
        if (!argumentsRectifier)
            return false;

        JS_ASSERT(ArgumentsRectifierReg != objreg);
        masm.move32(Imm32(call->numStackArgs()), ArgumentsRectifierReg);
        masm.call(argumentsRectifier);
        if (!markSafepoint(call))
            return false;
        masm.jump(&afterCall);
    }

    masm.bind(&rejoin);

    
    masm.callIon(objreg);
    if (!markSafepoint(call))
        return false;

    masm.bind(&afterCall);

    
    
    int prefixGarbage = sizeof(IonJSFrameLayout) - sizeof(void *);
    masm.adjustStack(prefixGarbage - unusedStack);

    masm.jump(&end);

    
    masm.bind(&invoke);
    if (!emitCallInvokeFunction(call, unusedStack))
        return false;

    masm.bind(&end);

    
    
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
CodeGenerator::visitCallConstructor(LCallConstructor *call)
{
    JS_ASSERT(call->mir()->isConstructing());

    
    const LAllocation *callee = call->getFunction();
    Register calleereg = ToRegister(callee);

    uint32 callargslot = call->argslot();
    uint32 unusedStack = StackOffsetOfPassedArg(callargslot);

    typedef bool (*pf)(JSContext *, JSObject *, uint32, Value *, Value *);
    static const VMFunction InvokeConstructorInfo = FunctionInfo<pf>(ion::InvokeConstructor);

    
    masm.freeStack(unusedStack);

    pushArg(StackPointer);                  
    pushArg(Imm32(call->numActualArgs()));  
    pushArg(calleereg);                     

    if (!callVM(InvokeConstructorInfo, call))
        return false;

    
    masm.reserveStack(unusedStack);

    dropArguments(call->numStackArgs() + 1);
    return true;
}

bool
CodeGenerator::emitCallInvokeFunction(LApplyArgsGeneric *apply, Register extraStackSize)
{
    Register objreg = ToRegister(apply->getTempObject());
    JS_ASSERT(objreg != extraStackSize);

    typedef bool (*pf)(JSContext *, JSFunction *, uint32, Value *, Value *);
    static const VMFunction InvokeFunctionInfo = FunctionInfo<pf>(InvokeFunction);

    
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

        
        
        BaseIndex disp(StackPointer, argcreg, ScaleFromShift(sizeof(Value)), argvOffset - sizeof(void*));

        
        
        masm.loadPtr(disp, copyreg);
        masm.push(copyreg);

        
        if (sizeof(Value) == 2 * sizeof(void*)) {
            masm.loadPtr(disp, copyreg);
            masm.push(copyreg);
        }

        masm.decBranchPtr(Assembler::NonZero, count, Imm32(1), &loop);
    }

    
    masm.movePtr(argcreg, extraStackSpace);
    masm.lshiftPtr(Imm32::ShiftOf(ScaleFromShift(sizeof(Value))), extraStackSpace);

    
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
        masm.cmpPtr(objreg, ImmWord(&js::FunctionClass));
        if (!bailoutIf(Assembler::NotEqual, apply->snapshot()))
            return false;
    }

    
    emitPushArguments(apply, copyreg);

    masm.checkStackAlignment();

    
    if (apply->hasSingleTarget() &&
        (!apply->getSingleTarget()->isInterpreted() ||
         apply->getSingleTarget()->script()->ion == ION_DISABLED_SCRIPT))
    {
        if (!emitCallInvokeFunction(apply, copyreg))
            return false;
        emitPopArguments(apply, copyreg);
        return true;
    }

    Label end, invoke;

    
    
    
    if (!apply->hasSingleTarget()) {
        Register kind = objreg;
        Address flags(calleereg, offsetof(JSFunction, flags));
        masm.load16ZeroExtend_mask(flags, Imm32(JSFUN_INTERPRETED), kind);
        masm.branch32(Assembler::NotEqual, kind, Imm32(JSFUN_INTERPRETED), &invoke);
    } else {
        
        JS_ASSERT(!apply->getSingleTarget()->isNative());
    }

    
    masm.movePtr(Address(calleereg, offsetof(JSFunction, u.i.script_)), objreg);
    masm.movePtr(Address(objreg, offsetof(JSScript, ion)), objreg);

    
    masm.branchPtr(Assembler::BelowOrEqual, objreg, ImmWord(ION_COMPILING_SCRIPT), &invoke);

    
    {
        
        unsigned pushed = masm.framePushed();
        masm.addPtr(Imm32(pushed), copyreg);
        masm.makeFrameDescriptor(copyreg, IonFrame_JS);

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
            masm.movePtr(Address(objreg, offsetof(IonScript, method_)), objreg);
            masm.movePtr(Address(objreg, IonCode::OffsetOfCode()), objreg);

            
            
            masm.jump(&rejoin);
        }

        
        {
            masm.bind(&underflow);

            
            IonCompartment *ion = gen->ionCompartment();
            IonCode *argumentsRectifier = ion->getArgumentsRectifier(GetIonContext()->cx);
            if (!argumentsRectifier)
                return false;

            JS_ASSERT(ArgumentsRectifierReg != objreg);
            masm.movePtr(argcreg, ArgumentsRectifierReg);
            masm.movePtr(ImmWord(argumentsRectifier->raw()), objreg);
        }

        masm.bind(&rejoin);

        
        masm.callIon(objreg);
        if (!markSafepoint(apply))
            return false;

        
        masm.movePtr(Address(StackPointer, 0), copyreg);
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
    
    
    
    
    
    
    
    

    JSRuntime *rt = gen->compartment->rt;
    Register limitReg = ToRegister(lir->limitTemp());

    
    
    uintptr_t *limitAddr = &rt->ionStackLimit;
    masm.loadPtr(AbsoluteAddress(limitAddr), limitReg);

    CheckOverRecursedFailure *ool = new CheckOverRecursedFailure(lir);
    if (!addOutOfLineCode(ool))
        return false;

    
    masm.branchPtr(Assembler::BelowOrEqual, StackPointer, limitReg, ool->entry());
    masm.bind(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitDefVar(LDefVar *lir)
{
    Register scopeChain = ToRegister(lir->getScopeChain());
    Register nameTemp   = ToRegister(lir->nameTemp());

    typedef bool (*pf)(JSContext *, HandlePropertyName, unsigned, HandleObject);
    static const VMFunction DefVarOrConstInfo = FunctionInfo<pf>(DefVarOrConst);

    masm.movePtr(ImmGCPtr(lir->mir()->name()), nameTemp);

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

    
    
    
    saveLive(ool->lir());

    if (!callVM(ReportOverRecursedInfo, ool->lir()))
        return false;

    restoreLive(ool->lir());
    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::generateBody()
{
    for (size_t i = 0; i < graph.numBlocks(); i++) {
        current = graph.getBlock(i);
        for (LInstructionIterator iter = current->begin(); iter != current->end(); iter++) {
            IonSpew(IonSpew_Codegen, "instruction %s", iter->opName());
            if (iter->safepoint() && pushedArgumentSlots_.length()) {
                if (!markArgumentSlots(iter->safepoint()))
                    return false;
            }

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

bool
CodeGenerator::visitNewArrayCallVM(LNewArray *lir)
{
    Register objReg = ToRegister(lir->output());

    typedef JSObject *(*pf)(JSContext *, uint32, types::TypeObject *);
    static const VMFunction NewInitArrayInfo = FunctionInfo<pf>(NewInitArray);

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
    Register objReg = ToRegister(lir->output());
    JSObject *templateObject = lir->mir()->templateObject();
    uint32 count = lir->mir()->count();

    JS_ASSERT(count < JSObject::NELEMENTS_LIMIT);

    size_t maxArraySlots =
        gc::GetGCKindSlots(gc::FINALIZE_OBJECT_LAST) - ObjectElements::VALUES_PER_HEADER;

    
    
    
    bool allocating = lir->mir()->isAllocating() && count > maxArraySlots;

    if (templateObject->hasSingletonType() || allocating)
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

bool
CodeGenerator::visitNewObjectVMCall(LNewObject *lir)
{
    Register objReg = ToRegister(lir->output());

    typedef JSObject *(*pf)(JSContext *, HandleObject);
    static const VMFunction Info = FunctionInfo<pf>(NewInitObject);

    JS_ASSERT(!lir->isCall());
    saveLive(lir);

    pushArg(ImmGCPtr(lir->mir()->templateObject()));
    if (!callVM(Info, lir))
        return false;

    if (ReturnReg != objReg)
        masm.movePtr(ReturnReg, objReg);

    restoreLive(lir);
    return true;
}

bool
CodeGenerator::visitNewObject(LNewObject *lir)
{
    Register objReg = ToRegister(lir->output());

    JSObject *templateObject = lir->mir()->templateObject();

    if (templateObject->hasSingletonType() || templateObject->hasDynamicSlots())
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

bool
CodeGenerator::visitNewCallObject(LNewCallObject *lir)
{
    Register obj = ToRegister(lir->output());

    typedef JSObject *(*pf)(JSContext *, HandleShape, HandleTypeObject, HeapSlot *, HandleObject);
    static const VMFunction NewCallObjectInfo = FunctionInfo<pf>(NewCallObject);

    JSObject *templateObj = lir->mir()->templateObj();
    JSObject *global = &templateObj->global();

    if (lir->isCall()) {
        pushArg(ImmGCPtr(global));
        if (lir->slots()->isRegister())
            pushArg(ToRegister(lir->slots()));
        else
            pushArg(ImmWord((void *)NULL));
        pushArg(ImmGCPtr(templateObj->type()));
        pushArg(ImmGCPtr(templateObj->lastProperty()));
        return callVM(NewCallObjectInfo, lir);
    }

    
    OutOfLineCode *ool;
    if (lir->slots()->isRegister()) {
        ool = oolCallVM(NewCallObjectInfo, lir,
                        (ArgList(), ImmGCPtr(templateObj->lastProperty()),
                                    ImmGCPtr(templateObj->type()),
                                    ToRegister(lir->slots()),
                                    ImmGCPtr(global)),
                        StoreRegisterTo(obj));
    } else {
        ool = oolCallVM(NewCallObjectInfo, lir,
                        (ArgList(), ImmGCPtr(templateObj->lastProperty()),
                                    ImmGCPtr(templateObj->type()),
                                    ImmWord((void *)NULL),
                                    ImmGCPtr(global)),
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
CodeGenerator::visitInitProp(LInitProp *lir)
{
    Register objReg = ToRegister(lir->getObject());

    typedef bool(*pf)(JSContext *cx, HandleObject obj, HandlePropertyName name, const Value &value);
    static const VMFunction InitPropInfo = FunctionInfo<pf>(InitProp);

    pushArg(ToValue(lir, LInitProp::ValueIndex));
    pushArg(ImmGCPtr(lir->mir()->propertyName()));
    pushArg(objReg);

    return callVM(InitPropInfo, lir);
}

bool
CodeGenerator::visitCreateThis(LCreateThis *lir)
{
    JS_ASSERT(lir->mir()->hasTemplateObject());

    JSObject *templateObject = lir->mir()->getTemplateObject();
    gc::AllocKind allocKind = templateObject->getAllocKind();
    int thingSize = (int)gc::Arena::thingSize(allocKind);
    Register objReg = ToRegister(lir->output());

    typedef JSObject *(*pf)(JSContext *cx, gc::AllocKind allocKind, size_t thingSize);
    static const VMFunction NewGCThingInfo = FunctionInfo<pf>(js::ion::NewGCThing);

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
CodeGenerator::visitCreateThisVM(LCreateThisVM *lir)
{
    const LAllocation *proto = lir->getPrototype();
    const LAllocation *callee = lir->getCallee();

    typedef JSObject *(*pf)(JSContext *cx, HandleObject callee, JSObject *proto);
    static const VMFunction CreateThisInfo =
        FunctionInfo<pf>(js_CreateThisForFunctionWithProto);

    
    if (proto->isConstant())
        pushArg(ImmGCPtr(&proto->toConstant()->toObject()));
    else
        pushArg(ToRegister(proto));

    if (callee->isConstant())
        pushArg(ImmGCPtr(&callee->toConstant()->toObject()));
    else
        pushArg(ToRegister(callee));

    if (!callVM(CreateThisInfo, lir))
        return false;
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
CodeGenerator::visitPowI(LPowI *ins)
{
    FloatRegister value = ToFloatRegister(ins->value());
    Register temp = ToRegister(ins->temp());

    
    
    
    masm.setupUnalignedABICall(2, temp);
    masm.passABIArg(value);

    const LAllocation *power = ins->power();
    if (power->isRegister()) {
        JS_ASSERT(ToRegister(power) != temp);
        masm.passABIArg(ToRegister(power));
    } else {
        masm.move32(Imm32(ToInt32(power)), temp);
        masm.passABIArg(temp);
    }

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

bool
CodeGenerator::visitBinaryV(LBinaryV *lir)
{
    typedef bool (*pf)(JSContext *, HandleScript, jsbytecode *, HandleValue, HandleValue, Value *);
    static const VMFunction AddInfo = FunctionInfo<pf>(js::AddValues);
    static const VMFunction SubInfo = FunctionInfo<pf>(js::SubValues);
    static const VMFunction MulInfo = FunctionInfo<pf>(js::MulValues);
    static const VMFunction DivInfo = FunctionInfo<pf>(js::DivValues);
    static const VMFunction ModInfo = FunctionInfo<pf>(js::ModValues);
    static const VMFunction UrshInfo = FunctionInfo<pf>(js::UrshValues);

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
CodeGenerator::visitCompareS(LCompareS *lir)
{
    JSOp op = lir->mir()->jsop();
    Register left = ToRegister(lir->left());
    Register right = ToRegister(lir->right());
    Register output = ToRegister(lir->output());

    typedef bool (*pf)(JSContext *, HandleString, HandleString, JSBool *);
    static const VMFunction stringsEqualInfo = FunctionInfo<pf>(ion::StringsEqual<true>);
    static const VMFunction stringsNotEqualInfo = FunctionInfo<pf>(ion::StringsEqual<false>);

    OutOfLineCode *ool = NULL;
    if (op == JSOP_EQ || op == JSOP_STRICTEQ) {
        ool = oolCallVM(stringsEqualInfo, lir, (ArgList(), left, right),  StoreRegisterTo(output));
    } else {
        JS_ASSERT(op == JSOP_NE || op == JSOP_STRICTNE);
        ool = oolCallVM(stringsNotEqualInfo, lir, (ArgList(), left, right), StoreRegisterTo(output));
    }

    if (!ool)
        return false;

    Label notPointerEqual;
    
    masm.branchPtr(Assembler::NotEqual, left, right, &notPointerEqual);
    masm.move32(Imm32(op == JSOP_EQ || op == JSOP_STRICTEQ), output);
    masm.jump(ool->rejoin());

    masm.bind(&notPointerEqual);

    
    Imm32 atomMask(JSString::ATOM_MASK);

    
    
    masm.branchTest32(Assembler::NonZero, Address(left, JSString::offsetOfLengthAndFlags()), 
                      atomMask, ool->entry());

    masm.branchTest32(Assembler::NonZero, Address(right, JSString::offsetOfLengthAndFlags()), 
                      atomMask, ool->entry());

    masm.cmpPtr(left, right);
    emitSet(JSOpToCondition(op), output);

    masm.bind(ool->rejoin());
    return true;
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
    typedef JSString *(*pf)(JSContext *, HandleString, HandleString);
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

    masm.branchTest32(Assembler::NonZero, output, Imm32(JSString::ROPE_BIT), ool->entry());
    masm.bind(ool->rejoin());

    
    Address charsAddr(str, JSString::offsetOfChars());
    masm.loadPtr(charsAddr, output);
    masm.load16ZeroExtend(BaseIndex(output, index, TimesTwo, 0), output);

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
CodeGenerator::visitNotV(LNotV *lir)
{
    Label setFalse;
    Label join;
    masm.branchTestValueTruthy(ToValue(lir, LNotV::Input), &setFalse, ToFloatRegister(lir->tempFloat()));

    
    masm.move32(Imm32(1), ToRegister(lir->getDef(0)));
    masm.jump(&join);

    
    masm.bind(&setFalse);
    masm.move32(Imm32(0), ToRegister(lir->getDef(0)));

    
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
        masm.cmp32(ToRegister(lir->length()), Imm32(index));
        return bailoutIf(Assembler::BelowOrEqual, lir->snapshot());
    }
    if (lir->length()->isConstant()) {
        masm.cmp32(ToRegister(lir->index()), Imm32(ToInt32(lir->length())));
        return bailoutIf(Assembler::AboveOrEqual, lir->snapshot());
    }
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

    typedef bool (*pf)(JSContext *, HandleObject, const Value &, const Value &, JSBool strict);
    static const VMFunction Info = FunctionInfo<pf>(SetObjectElement);

    pushArg(Imm32(current->mir()->strictModeCode()));
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
CodeGenerator::emitArrayPopShift(LInstruction *lir, const MArrayPopShift *mir, Register obj,
                                 Register elementsTemp, Register lengthTemp, TypedOrValueRegister out)
{
    OutOfLineCode *ool;
    typedef bool (*pf)(JSContext *, JSObject *, Value *);

    if (mir->mode() == MArrayPopShift::Pop) {
        static const VMFunction Info = FunctionInfo<pf>(ion::ArrayPopDense);
        ool = oolCallVM(Info, lir, (ArgList(), obj), StoreValueTo(out));
        if (!ool)
            return false;
    } else {
        JS_ASSERT(mir->mode() == MArrayPopShift::Shift);
        static const VMFunction Info = FunctionInfo<pf>(ion::ArrayShiftDense);
        ool = oolCallVM(Info, lir, (ArgList(), obj), StoreValueTo(out));
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

bool
CodeGenerator::emitArrayPush(LInstruction *lir, const MArrayPush *mir, Register obj,
                             ConstantOrRegister value, Register elementsTemp, Register length)
{
    typedef bool (*pf)(JSContext *, JSObject *, const Value &, uint32_t *);
    static const VMFunction Info = FunctionInfo<pf>(ion::ArrayPushDense);
    OutOfLineCode *ool = oolCallVM(Info, lir, (ArgList(), obj, value), StoreRegisterTo(length));
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

bool
CodeGenerator::visitCallIteratorStart(LCallIteratorStart *lir)
{
    typedef JSObject *(*pf)(JSContext *, HandleObject, uint32_t);
    static const VMFunction Info = FunctionInfo<pf>(GetIteratorObject);

    pushArg(Imm32(lir->mir()->flags()));
    pushArg(ToRegister(lir->object()));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitIteratorStart(LIteratorStart *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register output = ToRegister(lir->output());

    uint32_t flags = lir->mir()->flags();

    typedef JSObject *(*pf)(JSContext *, HandleObject, uint32_t);
    static const VMFunction Info = FunctionInfo<pf>(GetIteratorObject);

    OutOfLineCode *ool = oolCallVM(Info, lir, (ArgList(), obj, Imm32(flags)), StoreRegisterTo(output));
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

    
    
    {
        Label noBarrier;
        masm.branchTestNeedsBarrier(Assembler::Zero, temp1, &noBarrier);

        Address objAddr(niTemp, offsetof(NativeIterator, obj));
        masm.branchPtr(Assembler::NotEqual, objAddr, obj, ool->entry());

        masm.bind(&noBarrier);
    }

    
    masm.storePtr(obj, Address(niTemp, offsetof(NativeIterator, obj)));
    masm.or32(Imm32(JSITER_ACTIVE), Address(niTemp, offsetof(NativeIterator, flags)));

    
    masm.loadJSContext(temp1);
    masm.loadPtr(Address(temp1, offsetof(JSContext, enumerators)), temp2);
    masm.storePtr(temp2, Address(niTemp, offsetof(NativeIterator, next)));
    masm.storePtr(output, Address(temp1, offsetof(JSContext, enumerators)));

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

bool
CodeGenerator::visitIteratorNext(LIteratorNext *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register temp = ToRegister(lir->temp());
    const ValueOperand output = ToOutValue(lir);

    typedef bool (*pf)(JSContext *, JSObject *, MutableHandleValue);
    static const VMFunction Info = FunctionInfo<pf>(js_IteratorNext);

    OutOfLineCode *ool = oolCallVM(Info, lir, (ArgList(), obj), StoreValueTo(output));
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

bool
CodeGenerator::visitIteratorMore(LIteratorMore *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register output = ToRegister(lir->output());
    const Register temp = ToRegister(lir->temp());

    typedef bool (*pf)(JSContext *, HandleObject, JSBool *);
    static const VMFunction Info = FunctionInfo<pf>(ion::IteratorMore);
    OutOfLineCode *ool = oolCallVM(Info, lir, (ArgList(), obj), StoreRegisterTo(output));
    if (!ool)
        return false;

    LoadNativeIterator(masm, obj, output, ool->entry());

    masm.branchTest32(Assembler::NonZero, Address(output, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_FOREACH), ool->entry());

    
    masm.loadPtr(Address(output, offsetof(NativeIterator, props_end)), temp);
    masm.cmpPtr(Address(output, offsetof(NativeIterator, props_cursor)), temp);
    emitSet(Assembler::LessThan, output);

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitIteratorEnd(LIteratorEnd *lir)
{
    const Register obj = ToRegister(lir->object());
    const Register temp1 = ToRegister(lir->temp1());
    const Register temp2 = ToRegister(lir->temp2());

    typedef bool (*pf)(JSContext *, JSObject *);
    static const VMFunction Info = FunctionInfo<pf>(CloseIterator);

    OutOfLineCode *ool = oolCallVM(Info, lir, (ArgList(), obj), StoreNothing());
    if (!ool)
        return false;

    LoadNativeIterator(masm, obj, temp1, ool->entry());

    masm.branchTest32(Assembler::Zero, Address(temp1, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_ENUMERATE), ool->entry());

    
    masm.and32(Imm32(~JSITER_ACTIVE), Address(temp1, offsetof(NativeIterator, flags)));

    
    masm.loadPtr(Address(temp1, offsetof(NativeIterator, props_array)), temp2);
    masm.storePtr(temp2, Address(temp1, offsetof(NativeIterator, props_cursor)));

    
    masm.loadJSContext(temp2);
    masm.loadPtr(Address(temp1, offsetof(NativeIterator, next)), temp1);
    masm.storePtr(temp1, Address(temp2, offsetof(JSContext, enumerators)));

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitArgumentsLength(LArgumentsLength *lir)
{
    
    Register argc = ToRegister(lir->output());
    Address ptr(StackPointer, frameSize() + IonJSFrameLayout::offsetOfNumActualArgs());

    masm.movePtr(ptr, argc);
    return true;
}

bool
CodeGenerator::visitGetArgument(LGetArgument *lir)
{
    ValueOperand result = GetValueOutput(lir);
    const LAllocation *index = lir->index();
    size_t argvOffset = frameSize() + IonJSFrameLayout::offsetOfActualArgs();

    if (index->isConstant()) {
        int32 i = index->toConstant()->toInt32();
        Address argPtr(StackPointer, sizeof(Value) * i + argvOffset);
        masm.loadValue(argPtr, result);
    } else {
        Register i = ToRegister(index);
        BaseIndex argPtr(StackPointer, i, ScaleFromShift(sizeof(Value)), argvOffset);
        masm.loadValue(argPtr, result);
    }
    return true;
}

bool
CodeGenerator::generate()
{
    JSContext *cx = GetIonContext()->cx;

    unsigned slots = graph.localSlotCount() +
                     (graph.argumentSlotCount() * sizeof(Value) / STACK_SLOT_SIZE);
    if (!safepoints_.init(slots))
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

    script->ion = IonScript::New(cx, slots, scriptFrameSize, snapshots_.size(),
                                 bailouts_.length(), graph.numConstants(),
                                 safepointIndices_.length(), osiIndices_.length(),
                                 cacheList_.length(), barrierOffsets_.length(),
                                 safepoints_.size());
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
    if (barrierOffsets_.length())
        script->ion->copyPrebarrierEntries(&barrierOffsets_[0], masm);
    if (safepoints_.size())
        script->ion->copySafepoints(&safepoints_);

    linkAbsoluteLabels();

    
    
    
    if (cx->compartment->needsBarrier())
        script->ion->toggleBarriers(true);

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

typedef bool (*GetPropertyOrNameFn)(JSContext *, HandleObject, HandlePropertyName, Value *);

bool
CodeGenerator::visitCallGetProperty(LCallGetProperty *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, PropertyName *, MutableHandleValue);
    static const VMFunction Info = FunctionInfo<pf>(GetProperty);

    pushArg(ImmGCPtr(lir->mir()->name()));
    pushArg(ToValue(lir, LCallGetProperty::Value));
    return callVM(Info, lir);
}

bool
CodeGenerator::visitCallGetElement(LCallGetElement *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, const Value &, MutableHandleValue);
    static const VMFunction GetElementInfo = FunctionInfo<pf>(js::GetElement);
    static const VMFunction CallElementInfo = FunctionInfo<pf>(js::CallElement);

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
    typedef bool (*pf)(JSContext *, HandleObject, const Value &, const Value &, JSBool strict);
    static const VMFunction SetObjectElementInfo = FunctionInfo<pf>(js::SetObjectElement);

    pushArg(Imm32(current->mir()->strictModeCode()));
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


class OutOfLineCache : public OutOfLineCodeBase<CodeGenerator>
{
    LInstruction *ins;
    RepatchLabel repatchEntry_;
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
          case LInstruction::LOp_InstanceOfO:
          case LInstruction::LOp_InstanceOfV:
          case LInstruction::LOp_GetPropertyCacheT:
          case LInstruction::LOp_GetPropertyCacheV:
            return codegen->visitOutOfLineCacheGetProperty(this);
          case LInstruction::LOp_GetElementCacheV:
            return codegen->visitOutOfLineGetElementCache(this);
          case LInstruction::LOp_SetPropertyCacheT:
          case LInstruction::LOp_SetPropertyCacheV:
            return codegen->visitOutOfLineSetPropertyCache(this);
          case LInstruction::LOp_BindNameCache:
            return codegen->visitOutOfLineBindNameCache(this);
          case LInstruction::LOp_GetNameCache:
            return codegen->visitOutOfLineGetNameCache(this);
          default:
            JS_NOT_REACHED("Bad instruction");
            return false;
        }
    }

    LInstruction *cache() {
        return ins;
    }
    void bind(MacroAssembler *masm) {
        masm->bind(&repatchEntry_);
    }
    RepatchLabel *repatchEntry() {
        return &repatchEntry_;
    }
};

bool
CodeGenerator::visitCache(LInstruction *ins)
{
    OutOfLineCache *ool = new OutOfLineCache(ins);
    if (!addOutOfLineCode(ool))
        return false;

    CodeOffsetJump jump = masm.jumpWithPatch(ool->repatchEntry());
    CodeOffsetLabel label = masm.labelForPatch();
    masm.bind(ool->rejoin());

    ool->setInlineJump(jump, label);
    return true;
}

bool
CodeGenerator::visitOutOfLineGetNameCache(OutOfLineCache *ool)
{
    LGetNameCache *lir = ool->cache()->toGetNameCache();
    const MGetNameCache *mir = lir->mir();
    Register scopeChain = ToRegister(lir->scopeObj());
    RegisterSet liveRegs = lir->safepoint()->liveRegs();
    TypedOrValueRegister output(GetValueOutput(lir));

    IonCache::Kind kind = (mir->accessKind() == MGetNameCache::NAME)
                          ? IonCache::Name
                          : IonCache::NameTypeOf;
    IonCacheName cache(kind, ool->getInlineJump(), ool->getInlineLabel(),
                       masm.labelForPatch(), liveRegs,
                       scopeChain, mir->name(), output);

    cache.setScriptedLocation(mir->block()->info().script(), mir->resumePoint()->pc());
    size_t cacheIndex = allocateCache(cache);

    saveLive(lir);

    typedef bool (*pf)(JSContext *, size_t, HandleObject, MutableHandleValue);
    static const VMFunction GetNameCacheInfo = FunctionInfo<pf>(GetNameCache);

    pushArg(scopeChain);
    pushArg(Imm32(cacheIndex));
    if (!callVM(GetNameCacheInfo, lir))
        return false;

    masm.storeCallResultValue(output);
    restoreLive(lir);

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineCacheGetProperty(OutOfLineCache *ool)
{
    RegisterSet liveRegs = ool->cache()->safepoint()->liveRegs();

    LInstruction *ins = ool->cache();
    const MInstruction *mir = ins->mirRaw()->toInstruction();

    TypedOrValueRegister output;

    Register objReg;

    
    
    
    
    PropertyName *name = NULL;
    switch (ins->op()) {
      case LInstruction::LOp_InstanceOfO:
      case LInstruction::LOp_InstanceOfV:
        name = gen->compartment->rt->atomState.classPrototypeAtom;
        objReg = ToRegister(ins->getTemp(1));
        output = TypedOrValueRegister(MIRType_Object, ToAnyRegister(ins->getDef(0)));
        break;
      case LInstruction::LOp_GetPropertyCacheT:
        name = ((LGetPropertyCacheT *) ins)->mir()->name();
        objReg = ToRegister(ins->getOperand(0));
        output = TypedOrValueRegister(mir->type(), ToAnyRegister(ins->getDef(0)));
        break;
      case LInstruction::LOp_GetPropertyCacheV:
        name = ((LGetPropertyCacheV *) ins)->mir()->name();
        objReg = ToRegister(ins->getOperand(0));
        output = TypedOrValueRegister(GetValueOutput(ins));
        break;
      default:
        JS_NOT_REACHED("Bad instruction");
        return false;
    }

    IonCacheGetProperty cache(ool->getInlineJump(), ool->getInlineLabel(),
                              masm.labelForPatch(), liveRegs,
                              objReg, name, output);

    if (mir->resumePoint())
        cache.setScriptedLocation(mir->block()->info().script(), mir->resumePoint()->pc());
    else
        cache.setIdempotent();
    size_t cacheIndex = allocateCache(cache);

    saveLive(ins);

    typedef bool (*pf)(JSContext *, size_t, HandleObject, MutableHandleValue);
    static const VMFunction GetPropertyCacheInfo = FunctionInfo<pf>(GetPropertyCache);

    pushArg(objReg);
    pushArg(Imm32(cacheIndex));
    if (!callVM(GetPropertyCacheInfo, ins))
        return false;

    masm.storeCallResultValue(output);
    restoreLive(ins);

    masm.jump(ool->rejoin());

    return true;
}

bool
CodeGenerator::visitOutOfLineGetElementCache(OutOfLineCache *ool)
{
    LGetElementCacheV *ins = ool->cache()->toGetElementCacheV();
    const MGetElementCache *mir = ins->mir();

    Register obj = ToRegister(ins->object());
    ConstantOrRegister index = TypedOrValueRegister(ToValue(ins, LGetElementCacheV::Index));
    TypedOrValueRegister output = TypedOrValueRegister(GetValueOutput(ins));

    RegisterSet liveRegs = ins->safepoint()->liveRegs();

    IonCacheGetElement cache(ool->getInlineJump(), ool->getInlineLabel(),
                             masm.labelForPatch(), liveRegs,
                             obj, index, output,
                             mir->monitoredResult());

    cache.setScriptedLocation(mir->block()->info().script(), mir->resumePoint()->pc());
    size_t cacheIndex = allocateCache(cache);

    saveLive(ins);

    typedef bool (*pf)(JSContext *, size_t, JSObject *, const Value &, MutableHandleValue);
    static const VMFunction Info = FunctionInfo<pf>(GetElementCache);

    pushArg(index);
    pushArg(obj);
    pushArg(Imm32(cacheIndex));
    if (!callVM(Info, ins))
        return false;

    masm.storeCallResultValue(output);
    restoreLive(ins);

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

    typedef JSObject *(*pf)(JSContext *, size_t, HandleObject);
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
    bool isSetName = JSOp(*ins->mir()->resumePoint()->pc()) == JSOP_SETNAME;

    pushArg(Imm32(isSetName));
    pushArg(Imm32(ins->mir()->strict()));

    pushArg(value);
    pushArg(ImmGCPtr(ins->mir()->name()));
    pushArg(objReg);

    typedef bool (*pf)(JSContext *, HandleObject, HandlePropertyName, const HandleValue, bool, bool);
    static const VMFunction info = FunctionInfo<pf>(SetProperty);

    return callVM(info, ins);
}

bool
CodeGenerator::visitCallDeleteProperty(LCallDeleteProperty *lir)
{
    typedef bool (*pf)(JSContext *, const Value &, HandlePropertyName, JSBool *);

    pushArg(ImmGCPtr(lir->mir()->atom()));
    pushArg(ToValue(lir, LCallDeleteProperty::Value));

    if (lir->mir()->block()->info().script()->strictModeCode) {
        static const VMFunction Info = FunctionInfo<pf>(DeleteProperty<true>);
        return callVM(Info, lir);
    } else {
        static const VMFunction Info = FunctionInfo<pf>(DeleteProperty<false>);
        return callVM(Info, lir);
    }
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
                              objReg, mir->name(), value,
                              mir->strict());

    size_t cacheIndex = allocateCache(cache);
    bool isSetName = JSOp(*mir->resumePoint()->pc()) == JSOP_SETNAME;

    saveLive(ins);

    pushArg(Imm32(isSetName));
    pushArg(value);
    pushArg(objReg);
    pushArg(Imm32(cacheIndex));

    typedef bool (*pf)(JSContext *, size_t, HandleObject, HandleValue, bool);
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
    static const VMFunction BitLhsInfo = FunctionInfo<pf>(BitLsh);
    static const VMFunction BitRhsInfo = FunctionInfo<pf>(BitRsh);

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

    PropertyName **typeAtoms = gen->compartment->rt->atomState.typeAtoms;

    
    
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
    typedef bool (*pf)(JSContext *, const Value &, const Value &, MutableHandleValue);
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

bool
CodeGenerator::visitLoadTypedArrayElement(LLoadTypedArrayElement *lir)
{
    Register elements = ToRegister(lir->elements());
    Register temp = lir->temp()->isBogusTemp() ? InvalidReg : ToRegister(lir->temp());
    AnyRegister out = ToAnyRegister(lir->output());

    int arrayType = lir->mir()->arrayType();
    int shift = TypedArray::slotWidth(arrayType);

    Label fail;
    if (lir->index()->isConstant()) {
        Address source(elements, ToInt32(lir->index()) * shift);
        masm.loadFromTypedArray(arrayType, source, out, temp, &fail);
    } else {
        BaseIndex source(elements, ToRegister(lir->index()), ScaleFromShift(shift));
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
    int shift = TypedArray::slotWidth(arrayType);

    Label fail;
    if (key.isConstant()) {
        Address source(scratch, key.constant() * shift);
        masm.loadFromTypedArray(arrayType, source, out, lir->mir()->allowDouble(), &fail);
    } else {
        BaseIndex source(scratch, key.reg(), ScaleFromShift(shift));
        masm.loadFromTypedArray(arrayType, source, out, lir->mir()->allowDouble(), &fail);
    }

    if (fail.used() && !bailoutFrom(&fail, lir->snapshot()))
        return false;

    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGenerator::visitOutOfLineLoadTypedArray(OutOfLineLoadTypedArray *ool)
{
    LLoadTypedArrayElementHole *ins = ool->ins();
    saveLive(ins);

    Register object = ToRegister(ins->object());
    ValueOperand out = ToOutValue(ins);

    typedef bool (*pf)(JSContext *, const Value &, const Value &, MutableHandleValue);
    static const VMFunction Info = FunctionInfo<pf>(js::GetElementMonitored);

    if (ins->index()->isConstant())
        pushArg(*ins->index()->toConstant());
    else
        pushArg(TypedOrValueRegister(MIRType_Int32, ToAnyRegister(ins->index())));
    pushArg(TypedOrValueRegister(MIRType_Object, AnyRegister(object)));
    if (!callVM(Info, ins))
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
    int shift = TypedArray::slotWidth(arrayType);

    if (lir->index()->isConstant()) {
        Address dest(elements, ToInt32(lir->index()) * shift);
        StoreToTypedArray(masm, arrayType, value, dest);
    } else {
        BaseIndex dest(elements, ToRegister(lir->index()), ScaleFromShift(shift));
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

bool
CodeGenerator::visitInstanceOfO(LInstanceOfO *ins)
{
    Register rhs = ToRegister(ins->getOperand(1));
    return emitInstanceOf(ins, rhs);
}

bool
CodeGenerator::visitInstanceOfV(LInstanceOfV *ins)
{
    Register rhs = ToRegister(ins->getOperand(LInstanceOfV::RHS));
    return emitInstanceOf(ins, rhs);
}

bool
CodeGenerator::emitInstanceOf(LInstruction *ins, Register rhs)
{
    Register rhsTmp = ToRegister(ins->getTemp(1));
    Register output = ToRegister(ins->getDef(0));

    
    
    Register rhsFlags = ToRegister(ins->getTemp(0));
    Register lhsTmp = ToRegister(ins->getTemp(0));

    Label callHasInstance;
    Label boundFunctionCheck;
    Label boundFunctionDone;
    Label done;
    Label loopPrototypeChain;

    typedef bool (*pf)(JSContext *, HandleObject, const Value &, JSBool *);
    static const VMFunction HasInstanceInfo = FunctionInfo<pf>(js::HasInstance);

    OutOfLineCode *call = oolCallVM(HasInstanceInfo, ins, (ArgList(), rhs, ToValue(ins, 0)),
                                   StoreRegisterTo(output));
    if (!call)
        return false;

    

    
    
    
    
    
    
    
    

    masm.mov(rhs, rhsTmp);

    
    masm.bind(&boundFunctionCheck);

    masm.loadBaseShape(rhsTmp, output);
    masm.cmpPtr(Address(output, BaseShape::offsetOfClass()), ImmWord(&js::FunctionClass));
    masm.j(Assembler::NotEqual, call->entry());

    
    masm.loadPtr(Address(output, BaseShape::offsetOfFlags()), rhsFlags);
    masm.and32(Imm32(BaseShape::BOUND_FUNCTION), rhsFlags);
    masm.j(Assembler::Zero, &boundFunctionDone);

    
    masm.loadPtr(Address(output, BaseShape::offsetOfParent()), rhsTmp);
    masm.jump(&boundFunctionCheck);

    
    masm.bind(&boundFunctionDone);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    if (ins->isInstanceOfV()) {
        Label isObject;
        ValueOperand lhsValue = ToValue(ins, LInstanceOfV::LHS);
        masm.branchTestObject(Assembler::Equal, lhsValue, &isObject);
        masm.mov(Imm32(0), output);
        masm.jump(&done);

        masm.bind(&isObject);
        Register tmp = masm.extractObject(lhsValue, lhsTmp);
        masm.mov(tmp, lhsTmp);
    } else {
        masm.mov(ToRegister(ins->getOperand(0)), lhsTmp);
    }

    
    
    OutOfLineCache *ool = new OutOfLineCache(ins);
    if (!addOutOfLineCode(ool))
        return false;

    CodeOffsetJump jump = masm.jumpWithPatch(ool->repatchEntry());
    CodeOffsetLabel label = masm.labelForPatch();
    masm.bind(ool->rejoin());
    ool->setInlineJump(jump, label);

    
    masm.mov(output, rhsTmp);
    masm.mov(Imm32(0), output);

    
    masm.bind(&loopPrototypeChain);
    masm.loadPtr(Address(lhsTmp, JSObject::offsetOfType()), lhsTmp);
    masm.loadPtr(Address(lhsTmp, offsetof(types::TypeObject, proto)), lhsTmp);

    masm.test32(lhsTmp, lhsTmp);
    masm.j(Assembler::Zero, &done);

    
    masm.cmp32(lhsTmp, rhsTmp);
    masm.j(Assembler::NotEqual, &loopPrototypeChain);

    
    masm.mov(Imm32(1), output);

    masm.bind(call->rejoin());
    masm.bind(&done);
    return true;
}

bool
CodeGenerator::visitGetDOMProperty(LGetDOMProperty *ins)
{
    const Register JSContextReg = ToRegister(ins->getJSContextReg());
    const Register ObjectReg = ToRegister(ins->getObjectReg());
    const Register PrivateReg = ToRegister(ins->getPrivReg());
    const Register ValueReg = ToRegister(ins->getValueReg());

    DebugOnly<uint32> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    masm.adjustStack((int)-sizeof(Value));
    masm.movePtr(StackPointer, ValueReg);

    masm.Push(ObjectReg);

    
    masm.loadPrivate(Address(ObjectReg, JSObject::getFixedSlotOffset(0)), PrivateReg);

    
    masm.movePtr(StackPointer, ObjectReg);

    uint32 safepointOffset;
    if (!masm.buildFakeExitFrame(JSContextReg, &safepointOffset))
        return false;
    masm.enterFakeDOMFrame(ION_FRAME_DOMGETTER);

    if (!markSafepointAt(safepointOffset, ins))
        return false;

    masm.setupUnalignedABICall(4, JSContextReg);

    masm.loadJSContext(JSContextReg);

    masm.passABIArg(JSContextReg);
    masm.passABIArg(ObjectReg);
    masm.passABIArg(PrivateReg);
    masm.passABIArg(ValueReg);
    masm.callWithABI((void *)ins->mir()->fun());

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

    DebugOnly<uint32> initialStack = masm.framePushed();

    masm.checkStackAlignment();

    
    ValueOperand argVal = ToValue(ins, LSetDOMProperty::Value);
    masm.Push(argVal);
    masm.movePtr(StackPointer, ValueReg);

    masm.Push(ObjectReg);

    
    masm.loadPrivate(Address(ObjectReg, JSObject::getFixedSlotOffset(0)), PrivateReg);

    
    masm.movePtr(StackPointer, ObjectReg);

    uint32 safepointOffset;
    if (!masm.buildFakeExitFrame(JSContextReg, &safepointOffset))
        return false;
    masm.enterFakeDOMFrame(ION_FRAME_DOMSETTER);

    if (!markSafepointAt(safepointOffset, ins))
        return false;

    masm.setupUnalignedABICall(4, JSContextReg);

    masm.loadJSContext(JSContextReg);

    masm.passABIArg(JSContextReg);
    masm.passABIArg(ObjectReg);
    masm.passABIArg(PrivateReg);
    masm.passABIArg(ValueReg);
    masm.callWithABI((void *)ins->mir()->fun());

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

bool
CodeGenerator::visitProfilingEnter(LProfilingEnter *lir)
{
    SPSProfiler *profiler = &gen->compartment->rt->spsProfiler;
    JS_ASSERT(profiler->enabled());

    const char *string = lir->profileString();

    Register size = ToRegister(lir->temp1()->output());
    Register base = ToRegister(lir->temp2()->output());

    
    masm.movePtr(ImmWord(profiler->sizePointer()), size);
    masm.load32(Address(size, 0), size);
    Label stackFull;
    masm.branch32(Assembler::GreaterThanOrEqual, size, Imm32(profiler->maxSize()),
                  &stackFull);

    
    masm.movePtr(ImmWord(profiler->stack()), base);
    JS_STATIC_ASSERT(sizeof(ProfileEntry) == 2 * sizeof(void*));
    masm.lshiftPtr(Imm32(sizeof(void*) == 4 ? 3 : 4), size);
    masm.addPtr(size, base);

    masm.storePtr(ImmWord(string), Address(base, offsetof(ProfileEntry, string)));
    masm.storePtr(ImmWord((uintptr_t) 0), Address(base, offsetof(ProfileEntry, sp)));

    
    masm.bind(&stackFull);
    masm.movePtr(ImmWord(profiler->sizePointer()), size);
    Address addr(size, 0);
    masm.add32(Imm32(1), addr);
    return true;
}

bool
CodeGenerator::visitProfilingExit(LProfilingExit *exit)
{
    SPSProfiler *profiler = &gen->compartment->rt->spsProfiler;
    JS_ASSERT(profiler->enabled());
    Register temp = ToRegister(exit->temp());
    masm.movePtr(ImmWord(profiler->sizePointer()), temp);
    Address addr(temp, 0);
    masm.add32(Imm32(-1), addr);
    return true;
}

} 
} 

