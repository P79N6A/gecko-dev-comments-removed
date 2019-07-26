






#include "BaselineJIT.h"
#include "BaselineCompiler.h"
#include "BaselineHelpers.h"
#include "BaselineIC.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "VMFunctions.h"
#include "IonFrames-inl.h"

#include "jsinterpinlines.h"

namespace js {
namespace ion {

void
ICStub::markCode(JSTracer *trc, const char *name)
{
    IonCode *stubIonCode = ionCode();
    MarkIonCodeUnbarriered(trc, &stubIonCode, name);
}

 void
ICStub::trace(JSTracer *trc)
{
    markCode(trc, "baseline-stub-ioncode");

    
    
    
    
    if (isMonitoredFallback()) {
        ICTypeMonitor_Fallback *lastMonStub = toMonitoredFallbackStub()->fallbackMonitorStub();
        for (ICStub *monStub = lastMonStub->firstMonitorStub();
             monStub != NULL;
             monStub = monStub->next())
        {
            JS_ASSERT_IF(monStub->next() == NULL, monStub == lastMonStub);
            monStub->markCode(trc, "baseline-monitor-stub-ioncode");
        }
    }

    if (isUpdated()) {
        for (ICStub *updateStub = toUpdatedStub()->firstUpdateStub();
             updateStub != NULL;
             updateStub = updateStub->next())
        {
            JS_ASSERT_IF(updateStub->next() == NULL, updateStub->isTypeUpdate_Fallback());
            updateStub->markCode(trc, "baseline-update-stub-ioncode");
        }
    }

    switch (kind()) {
      case ICStub::Call_Scripted: {
        ICCall_Scripted *callStub = toCall_Scripted();
        MarkObject(trc, &callStub->callee(), "baseline-callstub-callee");
        break;
      }
      case ICStub::SetElem_Dense: {
        ICSetElem_Dense *setElemStub = toSetElem_Dense();
        MarkTypeObject(trc, &setElemStub->type(), "baseline-setelem-dense-stub-type");
        break;
      }
      case ICStub::TypeMonitor_TypeObject: {
        ICTypeMonitor_TypeObject *monitorStub = toTypeMonitor_TypeObject();
        MarkTypeObject(trc, &monitorStub->type(), "baseline-monitor-typeobject");
        break;
      }
      case ICStub::GetName_Global: {
        ICGetName_Global *globalStub = toGetName_Global();
        MarkShape(trc, &globalStub->shape(), "baseline-global-stub-shape");
        break;
      }
      default:
        break;
    }
}

void
ICFallbackStub::unlinkStubsWithKind(ICStub::Kind kind)
{
    ICStub *stub = icEntry_->firstStub();
    ICStub *last = NULL;
    do {
        if (stub->kind() == kind) {
            JS_ASSERT(stub->next());

            
            if (stub->next() == this) {
                JS_ASSERT(lastStubPtrAddr_ == stub->addressOfNext());
                if (last)
                    lastStubPtrAddr_ = last->addressOfNext();
                else
                    lastStubPtrAddr_ = icEntry()->addressOfFirstStub();
                *lastStubPtrAddr_ = this;
            }

            JS_ASSERT(numOptimizedStubs_ > 0);
            numOptimizedStubs_--;

            stub = stub->next();
            continue;
        }

        last = stub;
        stub = stub->next();
    } while (stub);
}

ICMonitoredStub::ICMonitoredStub(Kind kind, IonCode *stubCode, ICStub *firstMonitorStub)
  : ICStub(kind, ICStub::Monitored, stubCode),
    firstMonitorStub_(firstMonitorStub)
{
    
    
    JS_ASSERT_IF(firstMonitorStub_->isTypeMonitor_Fallback(),
                 firstMonitorStub_->toTypeMonitor_Fallback()->firstMonitorStub() ==
                    firstMonitorStub_);
}

bool
ICMonitoredFallbackStub::initMonitoringChain(JSContext *cx, ICStubSpace *space)
{
    JS_ASSERT(fallbackMonitorStub_ == NULL);

    ICTypeMonitor_Fallback::Compiler compiler(cx, this);
    ICTypeMonitor_Fallback *stub = compiler.getStub(space);
    if (!stub)
        return false;
    fallbackMonitorStub_ = stub;
    return true;
}

bool
ICUpdatedStub::initUpdatingChain(JSContext *cx, ICStubSpace *space)
{
    JS_ASSERT(firstUpdateStub_ == NULL);

    ICTypeUpdate_Fallback::Compiler compiler(cx);
    ICTypeUpdate_Fallback *stub = compiler.getStub(space);
    if (!stub)
        return false;

    firstUpdateStub_ = stub;
    return true;
}

IonCode *
ICStubCompiler::getStubCode()
{
    IonCompartment *ion = cx->compartment->ionCompartment();

    
    uint32_t stubKey = getKey();
    IonCode *stubCode = ion->getStubCode(stubKey);
    if (stubCode)
        return stubCode;

    
    MacroAssembler masm;
#ifdef JS_CPU_ARM
    masm.setSecondScratchReg(BaselineSecondScratchReg);
#endif

    AutoFlushCache afc("ICStubCompiler::getStubCode", ion);
    if (!generateStubCode(masm))
        return NULL;
    Linker linker(masm);
    Rooted<IonCode *> newStubCode(cx, linker.newCode(cx));
    if (!newStubCode)
        return NULL;

    
    if (!ion->putStubCode(stubKey, newStubCode))
        return NULL;

    return newStubCode;
}

bool
ICStubCompiler::tailCallVM(const VMFunction &fun, MacroAssembler &masm)
{
    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->getVMWrapper(fun);
    if (!code)
        return false;

    uint32_t argSize = fun.explicitStackSlots() * sizeof(void *);
    EmitTailCallVM(code, masm, argSize);
    return true;
}

bool
ICStubCompiler::callVM(const VMFunction &fun, MacroAssembler &masm)
{
    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->getVMWrapper(fun);
    if (!code)
        return false;

    EmitCallVM(code, masm);
    return true;
}

bool
ICStubCompiler::callTypeUpdateIC(MacroAssembler &masm)
{
    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->getVMWrapper(DoTypeUpdateFallbackInfo);
    if (!code)
        return false;

    EmitCallTypeUpdateIC(masm, code);
    return true;
}





static bool
DoStackCheckFallback(JSContext *cx, ICStackCheck_Fallback *stub)
{
    JS_CHECK_RECURSION(cx, return false);
    return true;
}

typedef bool (*DoStackCheckFallbackFn)(JSContext *, ICStackCheck_Fallback *);
static const VMFunction DoStackCheckFallbackInfo =
    FunctionInfo<DoStackCheckFallbackFn>(DoStackCheckFallback);

bool
ICStackCheck_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.push(BaselineStubReg);

    return tailCallVM(DoStackCheckFallbackInfo, masm);
}





bool
ICTypeMonitor_Fallback::addMonitorStubForValue(JSContext *cx, ICStubSpace *space, HandleValue val)
{
    bool wasEmptyMonitorChain = (numOptimizedMonitorStubs_ == 0);

    if (numOptimizedMonitorStubs_ >= MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    if (val.isPrimitive()) {
        JSValueType type = val.isDouble() ? JSVAL_TYPE_DOUBLE : val.extractNonDoubleType();
        ICTypeMonitor_Type::Compiler compiler(cx, type);
        ICStub *stub = compiler.getStub(space);
        if (!stub)
            return false;
        addOptimizedMonitorStub(stub);
    } else {
        RootedTypeObject type(cx, val.toObject().getType(cx));
        if (!type)
            return false;
        ICTypeMonitor_TypeObject::Compiler compiler(cx, type);
        ICStub *stub = compiler.getStub(space);
        if (!stub)
            return false;
        addOptimizedMonitorStub(stub);
    }

    bool firstMonitorStubAdded = wasEmptyMonitorChain && (numOptimizedMonitorStubs_ > 0);

    if (firstMonitorStubAdded) {
        
        
        
        ICEntry *ent = mainFallbackStub_->icEntry();
        for (ICStub *mainStub = ent->firstStub();
             mainStub != mainFallbackStub_;
             mainStub = mainStub->next())
        {
            
            JS_ASSERT(mainStub->next() != NULL);

            
            
            
            JS_ASSERT(mainStub->toMonitoredStub()->firstMonitorStub() == this);
            mainStub->toMonitoredStub()->updateFirstMonitorStub(firstMonitorStub_);
        }
    }

    return true;
}

static bool
DoTypeMonitorFallback(JSContext *cx, ICTypeMonitor_Fallback *stub, HandleValue value,
                      MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));

    
    jsbytecode *pc = stub->mainFallbackStub()->icEntry()->pc(script);
    types::TypeScript::Monitor(cx, script, pc, value);

    if (!stub->addMonitorStubForValue(cx, ICStubSpace::StubSpaceFor(script), value))
        return false;

    
    res.set(value);
    return true;
}

typedef bool (*DoTypeMonitorFallbackFn)(JSContext *, ICTypeMonitor_Fallback *, HandleValue,
                                        MutableHandleValue);
static const VMFunction DoTypeMonitorFallbackInfo =
    FunctionInfo<DoTypeMonitorFallbackFn>(DoTypeMonitorFallback);

bool
ICTypeMonitor_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoTypeMonitorFallbackInfo, masm);
}

bool
ICTypeMonitor_Type::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    switch (type_) {
      case JSVAL_TYPE_INT32:
        masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_DOUBLE:
        
        masm.branchTestNumber(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_BOOLEAN:
        masm.branchTestBoolean(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_STRING:
        masm.branchTestString(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_NULL:
        masm.branchTestNull(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_UNDEFINED:
        masm.branchTestUndefined(Assembler::NotEqual, R0, &failure);
        break;
      default:
        JS_NOT_REACHED("Unexpected type");
    }

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICTypeMonitor_TypeObject::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(obj, JSObject::offsetOfType()), R1.scratchReg());

    Address expectedType(BaselineStubReg, ICTypeMonitor_TypeObject::offsetOfType());
    masm.branchPtr(Assembler::NotEqual, expectedType, R1.scratchReg(), &failure);

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
DoTypeUpdateFallback(JSContext *cx, ICUpdatedStub *stub, HandleValue value)
{
    










    return true;
}

typedef bool (*DoTypeUpdateFallbackFn)(JSContext *, ICUpdatedStub *, HandleValue);
const VMFunction DoTypeUpdateFallbackInfo =
    FunctionInfo<DoTypeUpdateFallbackFn>(DoTypeUpdateFallback);

bool
ICTypeUpdate_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    masm.move32(Imm32(0), R1.scratchReg());
    EmitReturnFromIC(masm);
    return true;
}





static bool
DoThisFallback(JSContext *cx, ICThis_Fallback *stub, HandleValue thisv, MutableHandleValue ret)
{
    ret.set(thisv);
    bool modified;
    if (!BoxNonStrictThis(cx, ret, &modified))
        return false;
    return true;
}

typedef bool (*DoThisFallbackFn)(JSContext *, ICThis_Fallback *, HandleValue, MutableHandleValue);
static const VMFunction DoThisFallbackInfo = FunctionInfo<DoThisFallbackFn>(DoThisFallback);

bool
ICThis_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoThisFallbackInfo, masm);
}





static bool
DoCompareFallback(JSContext *cx, ICCompare_Fallback *stub, HandleValue lhs, HandleValue rhs,
                  MutableHandleValue ret)
{
    RootedScript script(cx, GetTopIonJSScript(cx));

    
    JSOp op = JSOp(*stub->icEntry()->pc(script));
    JSBool out;

    switch(op) {
      case JSOP_LT:
        if (!LessThan(cx, lhs, rhs, &out))
            return false;
        break;
      case JSOP_LE:
        if (!LessThanOrEqual(cx, lhs, rhs, &out))
            return false;
        break;
      case JSOP_GT:
        if (!GreaterThan(cx, lhs, rhs, &out))
            return false;
        break;
      case JSOP_GE:
        if (!GreaterThanOrEqual(cx, lhs, rhs, &out))
            return false;
        break;
      case JSOP_EQ:
        if (!LooselyEqual<true>(cx, lhs, rhs, &out))
            return false;
        break;
      case JSOP_NE:
        if (!LooselyEqual<false>(cx, lhs, rhs, &out))
            return false;
        break;
      default:
        JS_ASSERT(!"Unhandled baseline compare op");
        return false;
    }

    ret.setBoolean(out);

    
    if (stub->numOptimizedStubs() >= ICCompare_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (lhs.isInt32()) {
        if (rhs.isInt32()) {
            ICCompare_Int32::Compiler compilerInt32(cx, op);
            ICStub *int32Stub = compilerInt32.getStub(ICStubSpace::StubSpaceFor(script));
            if (!int32Stub)
                return false;

            stub->addNewStub(int32Stub);
        }
    }

    return true;
}

typedef bool (*DoCompareFallbackFn)(JSContext *, ICCompare_Fallback *, HandleValue, HandleValue,
                                    MutableHandleValue);
static const VMFunction DoCompareFallbackInfo = FunctionInfo<DoCompareFallbackFn>(DoCompareFallback);

bool
ICCompare_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoCompareFallbackInfo, masm);
}





static bool
DoToBoolFallback(JSContext *cx, ICToBool_Fallback *stub, HandleValue arg, MutableHandleValue ret)
{
    RootedScript script(cx, GetTopIonJSScript(cx));

    bool cond = ToBoolean(arg);
    ret.setBoolean(cond);

    
    if (stub->numOptimizedStubs() >= ICToBool_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    JS_ASSERT(!arg.isBoolean());

    
    if (arg.isInt32()) {
        ICToBool_Int32::Compiler compiler(cx);
        ICStub *int32Stub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!int32Stub)
            return false;

        stub->addNewStub(int32Stub);
        return true;
    }

    return true;
}

typedef bool (*pf)(JSContext *, ICToBool_Fallback *, HandleValue, MutableHandleValue);
static const VMFunction fun = FunctionInfo<pf>(DoToBoolFallback);

bool
ICToBool_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
    return tailCallVM(fun, masm);
}





bool
ICToBool_Int32::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);

    Label ifFalse;
    Assembler::Condition cond = masm.testInt32Truthy(false, R0);
    masm.j(cond, &ifFalse);

    masm.moveValue(BooleanValue(true), R0);
    EmitReturnFromIC(masm);

    masm.bind(&ifFalse);
    masm.moveValue(BooleanValue(false), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoToNumberFallback(JSContext *cx, ICToNumber_Fallback *stub, HandleValue arg, MutableHandleValue ret)
{
    ret.set(arg);
    return ToNumber(cx, ret.address());
}

typedef bool (*DoToNumberFallbackFn)(JSContext *, ICToNumber_Fallback *, HandleValue, MutableHandleValue);
static const VMFunction DoToNumberFallbackInfo = FunctionInfo<DoToNumberFallbackFn>(DoToNumberFallback);

bool
ICToNumber_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoToNumberFallbackInfo, masm);
}





static bool
DoBinaryArithFallback(JSContext *cx, ICBinaryArith_Fallback *stub, HandleValue lhs,
                      HandleValue rhs, MutableHandleValue ret)
{
    RootedScript script(cx, GetTopIonJSScript(cx));

    
    JSOp op = JSOp(*stub->icEntry()->pc(script));
    switch(op) {
      case JSOP_ADD:
        
        if (!AddValues(cx, script, stub->icEntry()->pc(script), lhs, rhs, ret.address()))
            return false;
        break;
      case JSOP_SUB:
        if (!SubValues(cx, script, stub->icEntry()->pc(script), lhs, rhs, ret.address()))
            return false;
        break;
      case JSOP_MUL:
        if (!MulValues(cx, script, stub->icEntry()->pc(script), lhs, rhs, ret.address()))
            return false;
        break;
      case JSOP_DIV:
        if (!DivValues(cx, script, stub->icEntry()->pc(script), lhs, rhs, ret.address()))
            return false;
        break;
      case JSOP_MOD:
        if (!ModValues(cx, script, stub->icEntry()->pc(script), lhs, rhs, ret.address()))
            return false;
        break;
      case JSOP_BITOR: {
        int32_t result;
        if (!BitOr(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_BITXOR: {
        int32_t result;
        if (!BitXor(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_BITAND: {
        int32_t result;
        if (!BitAnd(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_LSH: {
        int32_t result;
        if (!BitLsh(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_RSH: {
        int32_t result;
        if (!BitRsh(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_URSH: {
        if (!UrshOperation(cx, script, stub->icEntry()->pc(script), lhs, rhs, ret.address()))
            return false;
        break;
      }
      default:
        JS_NOT_REACHED("Unhandled baseline arith op");
        return false;
    }

    
    if (stub->numOptimizedStubs() >= ICBinaryArith_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (!lhs.isNumber() || !rhs.isNumber())
        return true;

    JS_ASSERT(ret.isNumber());

    if (lhs.isDouble() || rhs.isDouble() || ret.isDouble()) {
        switch (op) {
          case JSOP_ADD:
          case JSOP_SUB:
          case JSOP_MUL:
          case JSOP_DIV:
          case JSOP_MOD: {
            
            stub->unlinkStubsWithKind(ICStub::BinaryArith_Int32);

            ICBinaryArith_Double::Compiler compiler(cx, op);
            ICStub *doubleStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
            if (!doubleStub)
                return false;
            stub->addNewStub(doubleStub);
            return true;
          }
          case JSOP_URSH:
            
            break;
          default:
            
            return true;
        }
    }

    
    if (lhs.isInt32() && rhs.isInt32()) {
        bool allowDouble = ret.isDouble();
        ICBinaryArith_Int32::Compiler compilerInt32(cx, op, allowDouble);
        ICStub *int32Stub = compilerInt32.getStub(ICStubSpace::StubSpaceFor(script));
        if (!int32Stub)
            return false;
        stub->addNewStub(int32Stub);
    }

    return true;
}

typedef bool (*DoBinaryArithFallbackFn)(JSContext *, ICBinaryArith_Fallback *, HandleValue, HandleValue,
                                        MutableHandleValue);
static const VMFunction DoBinaryArithFallbackInfo =
    FunctionInfo<DoBinaryArithFallbackFn>(DoBinaryArithFallback);

bool
ICBinaryArith_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoBinaryArithFallbackInfo, masm);
}

void
ICBinaryArith_Double::Compiler::ensureDouble(MacroAssembler &masm, const ValueOperand &source,
                                             FloatRegister dest, Label *failure)
{
    
    

    Label isDouble, done;
    Register tag = masm.splitTagForTest(source);
    masm.branchTestDouble(Assembler::Equal, tag, &isDouble);
    masm.branchTestInt32(Assembler::NotEqual, tag, failure);

    Register payload = masm.extractInt32(source, ExtractTemp0);
    masm.convertInt32ToDouble(payload, dest);
    masm.jump(&done);

    masm.bind(&isDouble);
    masm.unboxDouble(source, dest);

    masm.bind(&done);
}

bool
ICBinaryArith_Double::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    ensureDouble(masm, R0, FloatReg0, &failure);
    ensureDouble(masm, R1, FloatReg1, &failure);

    switch (op) {
      case JSOP_ADD:
        masm.addDouble(FloatReg1, FloatReg0);
        break;
      case JSOP_SUB:
        masm.subDouble(FloatReg1, FloatReg0);
        break;
      case JSOP_MUL:
        masm.mulDouble(FloatReg1, FloatReg0);
        break;
      case JSOP_DIV:
        masm.divDouble(FloatReg1, FloatReg0);
        break;
      case JSOP_MOD:
        masm.setupUnalignedABICall(2, R0.scratchReg());
        masm.passABIArg(FloatReg0);
        masm.passABIArg(FloatReg1);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NumberMod), MacroAssembler::DOUBLE);
        JS_ASSERT(ReturnFloatReg == FloatReg0);
        break;
      default:
        JS_NOT_REACHED("Unexpected op");
        return false;
    }

    masm.boxDouble(FloatReg0, R0);

    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoUnaryArithFallback(JSContext *cx, ICUnaryArith_Fallback *stub, HandleValue val,
                     MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);

    JSOp op = JSOp(*pc);

    switch (op) {
      case JSOP_BITNOT: {
        int32_t result;
        if (!BitNot(cx, val, &result))
            return false;
        res.setInt32(result);
        break;
      }
      case JSOP_NEG:
        if (!NegOperation(cx, script, pc, val, res))
            return false;
        break;
      default:
        JS_NOT_REACHED("Unexpected op");
        return false;
    }

    if (stub->numOptimizedStubs() >= ICUnaryArith_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    if (!val.isInt32() || !res.isInt32())
        return true;

    ICUnaryArith_Int32::Compiler compiler(cx, op);
    ICStub *int32Stub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
    if (!int32Stub)
        return false;
    stub->addNewStub(int32Stub);
    return true;
}

typedef bool (*DoUnaryArithFallbackFn)(JSContext *, ICUnaryArith_Fallback *, HandleValue,
                                       MutableHandleValue);
static const VMFunction DoUnaryArithFallbackInfo =
    FunctionInfo<DoUnaryArithFallbackFn>(DoUnaryArithFallback);

bool
ICUnaryArith_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoUnaryArithFallbackInfo, masm);
}





static bool
DoGetElemFallback(JSContext *cx, ICGetElem_Fallback *stub, HandleValue lhs, HandleValue rhs, MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));

    if (!GetElementMonitored(cx, lhs, rhs, res))
        return false;

    if (stub->numOptimizedStubs() >= ICGetElem_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (!lhs.isObject())
        return true;

    RootedObject obj(cx, &lhs.toObject());
    if (obj->isDenseArray() && rhs.isInt32()) {
        
        if (stub->hasStub(ICStub::GetElem_Dense))
            return true;

        ICGetElem_Dense::Compiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub());
        ICStub *denseStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!denseStub)
            return false;

        stub->addNewStub(denseStub);
    }

    return true;
}

typedef bool (*DoGetElemFallbackFn)(JSContext *, ICGetElem_Fallback *, HandleValue, HandleValue,
                                    MutableHandleValue);
static const VMFunction DoGetElemFallbackInfo = FunctionInfo<DoGetElemFallbackFn>(DoGetElemFallback);

bool
ICGetElem_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoGetElemFallbackInfo, masm);
}





bool
ICGetElem_Dense::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    RootedShape shape(cx, GetDenseArrayShape(cx, cx->global()));
    if (!shape)
        return false;

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.branchTestObjShape(Assembler::NotEqual, obj, shape, &failure);

    
    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), scratchReg);

    
    Register key = masm.extractInt32(R1, ExtractTemp1);

    
    Address initLength(scratchReg, ObjectElements::offsetOfInitializedLength());
    masm.branch32(Assembler::BelowOrEqual, initLength, key, &failure);

    
    BaseIndex element(scratchReg, key, TimesEight);
    masm.branchTestMagic(Assembler::Equal, element, &failure);
    masm.loadValue(element, R0);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoSetElemFallback(JSContext *cx, ICSetElem_Fallback *stub, HandleValue rhs, HandleValue objv,
                  HandleValue index)
{
    RootedObject obj(cx, ToObject(cx, objv));
    if (!obj)
        return false;

    RootedScript script(cx, GetTopIonJSScript(cx));
    if (!SetObjectElement(cx, obj, index, rhs, script->strictModeCode))
        return false;

    if (stub->numOptimizedStubs() >= ICSetElem_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (obj->isDenseArray() && index.isInt32()) {
        RootedTypeObject type(cx, obj->getType(cx));
        ICSetElem_Dense::Compiler compiler(cx, type);
        ICStub *denseStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!denseStub)
            return false;

        stub->addNewStub(denseStub);
    }

    return true;
}

typedef bool (*DoSetElemFallbackFn)(JSContext *, ICSetElem_Fallback *, HandleValue, HandleValue,
                                    HandleValue);
static const VMFunction DoSetElemFallbackInfo = FunctionInfo<DoSetElemFallbackFn>(DoSetElemFallback);

bool
ICSetElem_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R1);
    masm.pushValue(R0);

    
    
    masm.mov(BaselineStackReg, R0.scratchReg());
    masm.pushValue(Address(R0.scratchReg(), 2 * sizeof(Value)));

    masm.push(BaselineStubReg);

    return tailCallVM(DoSetElemFallbackInfo, masm);
}





bool
ICSetElem_Dense::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    
    
    Label failure;
    Label failureUnstow;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    RootedShape shape(cx, GetDenseArrayShape(cx, cx->global()));
    if (!shape)
        return false;

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.branchTestObjShape(Assembler::NotEqual, obj, shape, &failure);

    
    
    EmitStowICValues(masm, 2);

    
    GeneralRegisterSet regs(availableGeneralRegs(0));
    regs.take(R0);

    
    Register typeReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_Dense::offsetOfType()), typeReg);
    masm.branchPtr(Assembler::NotEqual, Address(obj, JSObject::offsetOfType()), typeReg,
                   &failureUnstow);

    
    
    masm.loadValue(Address(BaselineStackReg, 2 * sizeof(Value) + ICStackValueOffset), R0);

    
    if (!callTypeUpdateIC(masm))
        return false;

    
    EmitUnstowICValues(masm, 2);

    
    regs = availableGeneralRegs(2);
    Register scratchReg = regs.takeAny();
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), scratchReg);

    
    Register key = masm.extractInt32(R1, ExtractTemp1);

    
    Address initLength(scratchReg, ObjectElements::offsetOfInitializedLength());
    masm.branch32(Assembler::BelowOrEqual, initLength, key, &failure);

    
    BaseIndex element(scratchReg, key, TimesEight);
    masm.branchTestMagic(Assembler::Equal, element, &failure);

    
    masm.loadValue(Address(BaselineStackReg, ICStackValueOffset), R0);
    masm.storeValue(R0, element);
    EmitReturnFromIC(masm);

    
    masm.bind(&failureUnstow);
    EmitUnstowICValues(masm, 2);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}


static bool
TryAttachGlobalNameStub(JSContext *cx, HandleScript script, ICGetName_Fallback *stub,
                        HandleObject global, HandlePropertyName name)
{
    JS_ASSERT(global->isGlobal());

    RootedId id(cx, NameToId(name));

    
    RootedShape shape(cx, global->nativeLookup(cx, id));
    if (!shape || !shape->hasDefaultGetter() || !shape->hasSlot())
        return true;

    JS_ASSERT(shape->slot() >= global->numFixedSlots());
    uint32_t slot = shape->slot() - global->numFixedSlots();

    

    ICStub *monitorStub = stub->fallbackMonitorStub()->firstMonitorStub();
    ICGetName_Global::Compiler compiler(cx, monitorStub, global->lastProperty(), slot);
    ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    return true;
}

static bool
DoGetNameFallback(JSContext *cx, ICGetName_Fallback *stub, HandleObject scopeChain, MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);

    JS_ASSERT(JSOp(*pc) == JSOP_GETGNAME || JSOp(*pc) == JSOP_CALLGNAME);

    RootedPropertyName name(cx, script->getName(pc));

    if (JSOp(pc[JSOP_GETGNAME_LENGTH]) == JSOP_TYPEOF) {
        if (!GetScopeNameForTypeOf(cx, scopeChain, name, res))
            return false;
    } else {
        if (!GetScopeName(cx, scopeChain, name, res))
            return false;
    }

    types::TypeScript::Monitor(cx, script, pc, res);

    
    if (stub->numOptimizedStubs() >= ICGetName_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    if (js_CodeSpec[*pc].format & JOF_GNAME) {
        if (!TryAttachGlobalNameStub(cx, script, stub, scopeChain, name))
            return false;
    }

    return true;
}

typedef bool (*DoGetNameFallbackFn)(JSContext *, ICGetName_Fallback *, HandleObject, MutableHandleValue);
static const VMFunction DoGetNameFallbackInfo = FunctionInfo<DoGetNameFallbackFn>(DoGetNameFallback);

bool
ICGetName_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    masm.push(R0.scratchReg());
    masm.push(BaselineStubReg);

    return tailCallVM(DoGetNameFallbackInfo, masm);
}

bool
ICGetName_Global::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    Register obj = R0.scratchReg();
    Register scratch = R1.scratchReg();

    
    masm.loadPtr(Address(BaselineStubReg, ICGetName_Global::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, obj, scratch, &failure);

    
    masm.loadPtr(Address(obj, JSObject::offsetOfSlots()), obj);
    masm.load32(Address(BaselineStubReg, ICGetName_Global::offsetOfSlot()), scratch);
    masm.loadValue(BaseIndex(obj, scratch, TimesEight), R0);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
TryAttachLengthStub(JSContext *cx, HandleScript script, ICGetProp_Fallback *stub, HandleValue val,
                    HandleValue res)
{
    if (!val.isObject())
        return true;

    RootedObject obj(cx, &val.toObject());

    if (obj->isDenseArray()) {
        JS_ASSERT(res.isInt32());
        ICGetProp_DenseLength::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        return true;
    }

    return true;
}

static bool
DoGetPropFallback(JSContext *cx, ICGetProp_Fallback *stub, HandleValue val, MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);

    JS_ASSERT(JSOp(*pc) == JSOP_GETPROP ||
              JSOp(*pc) == JSOP_CALLPROP ||
              JSOp(*pc) == JSOP_LENGTH);

    RootedPropertyName name(cx, script->getName(pc));
    RootedId id(cx, NameToId(name));

    RootedObject obj(cx, ToObjectFromStack(cx, val));
    if (!obj)
        return false;

    if (obj->getOps()->getProperty) {
        if (!GetPropertyGenericMaybeCallXML(cx, JSOp(*pc), obj, id, res))
            return false;
    } else {
        if (!GetPropertyHelper(cx, obj, id, 0, res))
            return false;
    }

#if JS_HAS_NO_SUCH_METHOD
    
    if (JSOp(*pc) == JSOP_CALLPROP && JS_UNLIKELY(res.isPrimitive())) {
        if (!OnUnknownMethod(cx, obj, IdToValue(id), res))
            return false;
    }
#endif

    types::TypeScript::Monitor(cx, script, pc, res);

    if (stub->numOptimizedStubs() >= ICGetProp_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    if (JSOp(*pc) == JSOP_LENGTH) {
        if (!TryAttachLengthStub(cx, script, stub, val, res))
            return false;
    }

    return true;
}

typedef bool (*DoGetPropFallbackFn)(JSContext *, ICGetProp_Fallback *, HandleValue, MutableHandleValue);
static const VMFunction DoGetPropFallbackInfo = FunctionInfo<DoGetPropFallbackFn>(DoGetPropFallback);

bool
ICGetProp_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoGetPropFallbackInfo, masm);
}

bool
ICGetProp_DenseLength::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    RootedShape shape(cx, GetDenseArrayShape(cx, cx->global()));
    if (!shape)
        return false;

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.branchTestObjShape(Assembler::NotEqual, obj, shape, &failure);

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), obj);
    masm.load32(Address(obj, ObjectElements::offsetOfLength()), obj);

    masm.tagValue(JSVAL_TYPE_INT32, obj, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}




static bool
TryAttachCallStub(JSContext *cx, ICCall_Fallback *stub, HandleScript script, JSOp op,
                  uint32_t argc, Value *vp, MutableHandleValue res, bool *attachedStub)
{
    *attachedStub = false;

    RootedValue callee(cx, vp[0]);
    RootedValue thisv(cx, vp[1]);

    if (stub->numOptimizedStubs() >= ICCall_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    if (op == JSOP_NEW || !callee.isObject())
        return true;

    RootedObject obj(cx, &callee.toObject());
    if (!obj->isFunction())
        return true;

    RootedFunction fun(cx, obj->toFunction());
    if (obj->toFunction()->hasScript()) {
        RootedScript calleeScript(cx, fun->nonLazyScript());
        if (!calleeScript->hasBaselineScript() && !calleeScript->hasIonScript())
            return true;

        ICCall_Scripted::Compiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(), fun);
        ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        *attachedStub = true;
        return true;
    }

    return true;
}

static bool
DoCallFallback(JSContext *cx, ICCall_Fallback *stub, uint32_t argc, Value *vp, MutableHandleValue res)
{
    RootedValue callee(cx, vp[0]);
    RootedValue thisv(cx, vp[1]);

    Value *args = vp + 2;

    RootedScript script(cx, GetTopIonJSScript(cx));
    JSOp op = JSOp(*stub->icEntry()->pc(script));

    bool attachedStub;
    if (!TryAttachCallStub(cx, stub, script, op, argc, vp, res, &attachedStub))
        return false;
    

    if (op == JSOP_NEW) {
        if (!InvokeConstructor(cx, callee, argc, args, res.address()))
            return false;
    } else {
        JS_ASSERT(op == JSOP_CALL || op == JSOP_FUNCALL || op == JSOP_FUNAPPLY);
        if (!Invoke(cx, thisv, callee, argc, args, res.address()))
            return false;
    }

    types::TypeScript::Monitor(cx, res);

    
    ICTypeMonitor_Fallback *typeMonFbStub = stub->fallbackMonitorStub();
    if (!typeMonFbStub->addMonitorStubForValue(cx, ICStubSpace::StubSpaceFor(script), res))
        return false;

    return true;
}

void
ICCallStubCompiler::pushCallArguments(MacroAssembler &masm, GeneralRegisterSet regs, Register argcReg)
{
    JS_ASSERT(!regs.has(argcReg));

    
    Register count = regs.takeAny();
    masm.mov(argcReg, count);
    masm.add32(Imm32(2), count);

    
    Register argPtr = regs.takeAny();
    masm.mov(BaselineStackReg, argPtr);

    
    
    masm.addPtr(Imm32(sizeof(void *) * 4), argPtr);

    
    Label loop, done;
    masm.bind(&loop);
    masm.branchTest32(Assembler::Zero, count, count, &done);
    {
        masm.pushValue(Address(argPtr, 0));
        masm.addPtr(Imm32(sizeof(Value)), argPtr);

        masm.sub32(Imm32(1), count);
        masm.jump(&loop);
    }
    masm.bind(&done);
}

typedef bool (*DoCallFallbackFn)(JSContext *, ICCall_Fallback *, uint32_t, Value *, MutableHandleValue);
static const VMFunction DoCallFallbackInfo = FunctionInfo<DoCallFallbackFn>(DoCallFallback);

bool
ICCall_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitEnterStubFrame(masm, R1.scratchReg());

    
    
    

    GeneralRegisterSet regs(availableGeneralRegs(0));
    regs.take(R0.scratchReg()); 

    pushCallArguments(masm, regs, R0.scratchReg());

    masm.push(BaselineStackReg);
    masm.push(R0.scratchReg());
    masm.push(BaselineStubReg);

    if (!callVM(DoCallFallbackInfo, masm))
        return false;

    EmitLeaveStubFrame(masm);

    EmitReturnFromIC(masm);
    return true;
}

bool
ICCall_Scripted::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    GeneralRegisterSet regs(availableGeneralRegs(0));
    bool canUseTailCallReg = regs.has(BaselineTailCallReg);

    Register argcReg = R0.scratchReg();
    JS_ASSERT(argcReg != ArgumentsRectifierReg);

    regs.take(argcReg);
    regs.take(ArgumentsRectifierReg);
    if (regs.has(BaselineTailCallReg))
        regs.take(BaselineTailCallReg);

    
    BaseIndex calleeSlot(BaselineStackReg, argcReg, TimesEight, ICStackValueOffset + sizeof(Value));
    masm.loadValue(calleeSlot, R1);
    regs.take(R1);

    masm.branchTestObject(Assembler::NotEqual, R1, &failure);

    
    Register callee = masm.extractObject(R1, ExtractTemp0);
    Address expectedCallee(BaselineStubReg, ICCall_Scripted::offsetOfCallee());
    masm.branchPtr(Assembler::NotEqual, expectedCallee, callee, &failure);

    
    
    masm.loadPtr(Address(callee, offsetof(JSFunction, u.i.script_)), callee);

    
    masm.loadBaselineOrIonCode(callee);

    
    Register code = regs.takeAny();
    masm.loadPtr(Address(callee, IonCode::offsetOfCode()), code);

    
    regs.add(R1);

    
    Register scratch = regs.takeAny();
    EmitEnterStubFrame(masm, scratch);
    if (canUseTailCallReg)
        regs.add(BaselineTailCallReg);

    
    
    
    pushCallArguments(masm, regs, argcReg);

    
    ValueOperand val = regs.takeAnyValue();
    masm.popValue(val);
    callee = masm.extractObject(val, ExtractTemp0);

    EmitCreateStubFrameDescriptor(masm, scratch);

    
    
    masm.Push(argcReg);
    masm.Push(callee);
    masm.Push(scratch);

    
    Label noUnderflow;
    masm.load16ZeroExtend(Address(callee, offsetof(JSFunction, nargs)), callee);
    masm.branch32(Assembler::AboveOrEqual, argcReg, callee, &noUnderflow);
    {
        
        JS_ASSERT(ArgumentsRectifierReg != code);
        JS_ASSERT(ArgumentsRectifierReg != argcReg);

        IonCode *argumentsRectifier = cx->compartment->ionCompartment()->getArgumentsRectifier();

        masm.movePtr(ImmGCPtr(argumentsRectifier), code);
        masm.loadPtr(Address(code, IonCode::offsetOfCode()), code);
        masm.mov(argcReg, ArgumentsRectifierReg);
    }

    masm.bind(&noUnderflow);
    masm.callIon(code);

    EmitLeaveStubFrame(masm, true);

    
    EmitEnterTypeMonitorIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

} 
} 
