






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

    IonCode *wrapper = generateVMWrapper(fun);
    if (!wrapper)
        return false;

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
    EmitTailCall(wrapper, masm);

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

    IonCode *wrapper = generateVMWrapper(fun);
    if (!wrapper)
        return false;

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
    EmitTailCall(wrapper, masm);

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

    IonCode *wrapper = generateVMWrapper(fun);
    if (!wrapper)
        return false;

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
    EmitTailCall(wrapper, masm);

    Linker linker(masm);
    return linker.newCode(cx);
}

} 
} 
