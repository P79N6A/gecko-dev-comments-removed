






#include "BaselineJIT.h"
#include "BaselineCompiler.h"
#include "BaselineHelpers.h"
#include "BaselineIC.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "VMFunctions.h"
#include "IonFrames-inl.h"

namespace js {
namespace ion {

bool
ICStubCompiler::callVM(const VMFunction &fun, MacroAssembler &masm)
{
    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->generateVMWrapper(cx, fun);
    if (!code)
        return false;

    uint32_t argSize = fun.explicitStackSlots() * sizeof(void *);
    EmitTailCall(code, masm, argSize);
    return true;
}





static bool
DoCompareFallback(JSContext *cx, ICCompare_Fallback *stub, HandleValue lhs, HandleValue rhs,
                  MutableHandleValue ret)
{
    uint8_t *returnAddr;
    RootedScript script(cx, GetTopIonJSScript(cx, NULL, (void **)&returnAddr));

    
    JSOp op = JSOp(*stub->icEntry()->pc(script));
    switch(op) {
      case JSOP_LT: {
        
        JSBool out;
        if (!LessThan(cx, lhs, rhs, &out))
            return false;
        ret.setBoolean(out);
        break;
      }
      case JSOP_GT: {
        
        JSBool out;
        if (!GreaterThan(cx, lhs, rhs, &out))
            return false;
        ret.setBoolean(out);
        break;
      }
      default:
        JS_ASSERT(!"Unhandled baseline compare op");
        return false;
    }


    
    if (stub->numOptimizedStubs() >= ICCompare_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (lhs.isInt32()) {
        if (rhs.isInt32()) {
            ICCompare_Int32::Compiler compilerInt32(cx, op);
            ICStub *int32Stub = compilerInt32.getStub();
            if (!int32Stub)
                return false;

            stub->addNewStub(int32Stub);
        }
    }

    return true;
}

IonCode *
ICCompare_Fallback::Compiler::generateStubCode()
{
    MacroAssembler masm;
    JS_ASSERT(R0 == JSReturnOperand);

    
    masm.pop(BaselineTailCallReg);

    
    typedef bool (*pf)(JSContext *, ICCompare_Fallback *, HandleValue, HandleValue,
                       MutableHandleValue);
    static const VMFunction fun = FunctionInfo<pf>(DoCompareFallback);

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
    if (!callVM(fun, masm))
        return NULL;

    Linker linker(masm);
    return linker.newCode(cx);
}





static bool
DoToBoolFallback(JSContext *cx, ICToBool_Fallback *stub, HandleValue arg, MutableHandleValue ret)
{
    bool cond = ToBoolean(arg);
    ret.setBoolean(cond);

    
    if (stub->numOptimizedStubs() >= ICToBool_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (arg.isBoolean()) {
        
        ICToBool_Bool::Compiler compilerBool(cx);
        ICStub *boolStub = compilerBool.getStub();
        if (!boolStub)
            return false;

        stub->addNewStub(boolStub);
    }

    return true;
}

IonCode *
ICToBool_Fallback::Compiler::generateStubCode()
{
    MacroAssembler masm;
    JS_ASSERT(R0 == JSReturnOperand);

    
    masm.pop(BaselineTailCallReg);

    
    typedef bool (*pf)(JSContext *, ICToBool_Fallback *, HandleValue, MutableHandleValue);
    static const VMFunction fun = FunctionInfo<pf>(DoToBoolFallback);

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
    if (!callVM(fun, masm))
        return NULL;

    Linker linker(masm);
    return linker.newCode(cx);
}





IonCode *
ICToBool_Bool::Compiler::generateStubCode()
{
    MacroAssembler masm;

    
    Label failure;
    masm.branchTestBoolean(Assembler::NotEqual, R0, &failure);
    masm.ret();

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    Linker linker(masm);
    return linker.newCode(cx);
}

static bool
DoToNumberFallback(JSContext *cx, ICToNumber_Fallback *stub, HandleValue arg, MutableHandleValue ret)
{
    ret.set(arg);
    return ToNumber(cx, ret.address());
}

IonCode *
ICToNumber_Fallback::Compiler::generateStubCode()
{
    MacroAssembler masm;
    JS_ASSERT(R0 == JSReturnOperand);

    
    masm.pop(BaselineTailCallReg);

    
    typedef bool (*pf)(JSContext *, ICToNumber_Fallback *, HandleValue, MutableHandleValue);
    static const VMFunction fun = FunctionInfo<pf>(DoToNumberFallback);

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
    if (!callVM(fun, masm))
        return NULL;

    Linker linker(masm);
    return linker.newCode(cx);
}





static bool
DoBinaryArithFallback(JSContext *cx, ICBinaryArith_Fallback *stub, HandleValue lhs,
                      HandleValue rhs, MutableHandleValue ret)
{
    uint8_t *returnAddr;
    RootedScript script(cx, GetTopIonJSScript(cx, NULL, (void **)&returnAddr));

    
    JSOp op = JSOp(*stub->icEntry()->pc(script));
    switch(op) {
      case JSOP_ADD: {
        
        if (!AddValues(cx, script, stub->icEntry()->pc(script), lhs, rhs, ret.address()))
            return false;
        break;
      }
      default:
        JS_ASSERT(!"Unhandled baseline compare op");
        return false;
    }

    
    if (stub->numOptimizedStubs() >= ICBinaryArith_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (lhs.isInt32()) {
        if (rhs.isInt32()) {
            ICBinaryArith_Int32::Compiler compilerInt32(cx, op);
            ICStub *int32Stub = compilerInt32.getStub();
            if (!int32Stub)
                return false;

            stub->addNewStub(int32Stub);
        }
    }

    return true;
}

IonCode *
ICBinaryArith_Fallback::Compiler::generateStubCode()
{
    MacroAssembler masm;
    JS_ASSERT(R0 == JSReturnOperand);

    
    masm.pop(BaselineTailCallReg);

    
    typedef bool (*pf)(JSContext *, ICBinaryArith_Fallback *, HandleValue, HandleValue,
                       MutableHandleValue);
    static const VMFunction fun = FunctionInfo<pf>(DoBinaryArithFallback);

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
    if (!callVM(fun, masm))
        return NULL;

    Linker linker(masm);
    return linker.newCode(cx);
}

} 
} 
