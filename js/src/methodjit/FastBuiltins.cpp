





































#include "jsbool.h"
#include "jslibmath.h"
#include "jsnum.h"
#include "jstracer.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"
#include "methodjit/StubCalls.h"
#include "methodjit/FrameState-inl.h"

using namespace js;
using namespace js::mjit;
using namespace JSC;

typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;

CompileStatus
mjit::Compiler::compileMathAbsInt(FrameEntry *arg)
{
    RegisterID reg;
    if (arg->isConstant()) {
        reg = frame.allocReg();
        masm.move(Imm32(arg->getValue().toInt32()), reg);
    } else {
        reg = frame.copyDataIntoReg(arg);
    }

    Jump isPositive = masm.branch32(Assembler::GreaterThanOrEqual, reg, Imm32(0));

    
    Jump isMinInt = masm.branch32(Assembler::Equal, reg, Imm32(INT32_MIN));
    stubcc.linkExit(isMinInt, Uses(3));

    masm.neg32(reg);

    isPositive.linkTo(masm.label(), &masm);

    stubcc.leave();
    stubcc.masm.move(Imm32(1), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall);

    frame.popn(3);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileMathAbsDouble(FrameEntry *arg)
{
    FPRegisterID fpResultReg = frame.allocFPReg();

    FPRegisterID fpReg;
    bool allocate;

    MaybeJump notNumber = loadDouble(arg, &fpReg, &allocate);
    JS_ASSERT(!notNumber.isSet());

    
    masm.zeroDouble(fpResultReg);
    masm.subDouble(fpReg, fpResultReg);
    masm.andDouble(fpReg, fpResultReg);

    if (allocate)
        frame.freeReg(fpReg);

    frame.popn(3);
    frame.pushDouble(fpResultReg);

    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileRound(FrameEntry *arg, RoundingMode mode)
{
    FPRegisterID fpScratchReg = frame.allocFPReg();

    FPRegisterID fpReg;
    bool allocate;

    MaybeJump notNumber = loadDouble(arg, &fpReg, &allocate);
    JS_ASSERT(!notNumber.isSet());

    masm.zeroDouble(fpScratchReg);

    
    Jump negOrNan = masm.branchDouble(Assembler::DoubleLessThanOrEqualOrUnordered, fpReg, fpScratchReg);
    stubcc.linkExit(negOrNan, Uses(3));

    
    FPRegisterID fpSourceReg;
    if (mode == Round) {
        masm.slowLoadConstantDouble(0.5, fpScratchReg);
        masm.addDouble(fpReg, fpScratchReg);
        fpSourceReg = fpScratchReg;
    } else {
        fpSourceReg = fpReg;
    }

    
    RegisterID reg = frame.allocReg();
    Jump overflow = masm.branchTruncateDoubleToInt32(fpSourceReg, reg);
    stubcc.linkExit(overflow, Uses(3));

    if (allocate)
        frame.freeReg(fpReg);
    frame.freeReg(fpScratchReg);

    stubcc.leave();
    stubcc.masm.move(Imm32(1), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall);

    frame.popn(3);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileMathSqrt(FrameEntry *arg)
{
    FPRegisterID fpResultReg = frame.allocFPReg();

    FPRegisterID fpReg;
    bool allocate;

    MaybeJump notNumber = loadDouble(arg, &fpReg, &allocate);
    JS_ASSERT(!notNumber.isSet());

    masm.sqrtDouble(fpReg, fpResultReg);

    if (allocate)
        frame.freeReg(fpReg);

    frame.popn(3);
    frame.pushDouble(fpResultReg);

    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileMathPowSimple(FrameEntry *arg1, FrameEntry *arg2)
{
    FPRegisterID fpScratchReg = frame.allocFPReg();
    FPRegisterID fpResultReg = frame.allocFPReg();

    FPRegisterID fpReg;
    bool allocate;

    MaybeJump notNumber = loadDouble(arg1, &fpReg, &allocate);
    JS_ASSERT(!notNumber.isSet());

    
    masm.slowLoadConstantDouble(js_NegativeInfinity, fpResultReg);
    Jump isNegInfinity = masm.branchDouble(Assembler::DoubleEqual, fpReg, fpResultReg);
    stubcc.linkExit(isNegInfinity, Uses(4));

    
    masm.zeroDouble(fpResultReg);
    masm.moveDouble(fpReg, fpScratchReg);
    masm.addDouble(fpResultReg, fpScratchReg);

    double y = arg2->getValue().toDouble();
    if (y == 0.5) {
        
        masm.sqrtDouble(fpScratchReg, fpResultReg);

    } else if (y == -0.5) {
        
        masm.sqrtDouble(fpScratchReg, fpScratchReg);
        masm.slowLoadConstantDouble(1, fpResultReg);
        masm.divDouble(fpScratchReg, fpResultReg);
    }

    frame.freeReg(fpScratchReg);

    if (allocate)
        frame.freeReg(fpReg);

    stubcc.leave();
    stubcc.masm.move(Imm32(2), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall);

    frame.popn(4);
    frame.pushDouble(fpResultReg);

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileGetChar(FrameEntry *thisValue, FrameEntry *arg, GetCharMode mode)
{
    RegisterID reg1 = frame.allocReg();
    RegisterID reg2 = frame.allocReg();

    
    RegisterID strReg;
    if (thisValue->isConstant()) {
        strReg = frame.allocReg();
        masm.move(ImmPtr(thisValue->getValue().toString()), strReg);
    } else {
        strReg = frame.tempRegForData(thisValue);
        frame.pinReg(strReg);
    }

    
    RegisterID argReg;
    if (arg->isConstant()) {
        argReg = frame.allocReg();
        masm.move(Imm32(arg->getValue().toInt32()), argReg);
    } else {
        argReg = frame.tempRegForData(arg);
    }
    if (!thisValue->isConstant())
        frame.unpinReg(strReg);

    Address lengthAndFlagsAddr(strReg, JSString::offsetOfLengthAndFlags());

    
    masm.loadPtr(lengthAndFlagsAddr, reg1);
    masm.move(reg1, reg2);

    
    masm.andPtr(ImmPtr((void *)JSString::ROPE_BIT), reg1);
    Jump isRope = masm.branchTestPtr(Assembler::NonZero, reg1);
    stubcc.linkExit(isRope, Uses(3));

    
    masm.rshiftPtr(Imm32(JSString::LENGTH_SHIFT), reg2);
    Jump outOfRange = masm.branchPtr(Assembler::AboveOrEqual, argReg, reg2);
    stubcc.linkExit(outOfRange, Uses(3));

    
    masm.move(argReg, reg1);
    masm.loadPtr(Address(strReg, JSString::offsetOfChars()), reg2);
    masm.lshiftPtr(Imm32(1), reg1);
    masm.addPtr(reg1, reg2);
    masm.load16(Address(reg2), reg2);

    
    if (mode == GetChar) {
        
        Jump notUnitString = masm.branch32(Assembler::AboveOrEqual, reg2,
                                           Imm32(JSAtom::UNIT_STATIC_LIMIT));
        stubcc.linkExit(notUnitString, Uses(3));

        
        masm.lshiftPtr(Imm32(sizeof(JSString) == 16 ? 4 : 5), reg2);
        masm.addPtr(ImmPtr(JSAtom::unitStaticTable), reg2);
    }

    if (thisValue->isConstant())
        frame.freeReg(strReg);
    if (arg->isConstant())
        frame.freeReg(argReg);
    frame.freeReg(reg1);

    stubcc.leave();
    stubcc.masm.move(Imm32(1), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall);

    frame.popn(3);
    switch(mode) {
      case GetCharCode:
        frame.pushTypedPayload(JSVAL_TYPE_INT32, reg2);
        break;
      case GetChar:
        frame.pushTypedPayload(JSVAL_TYPE_STRING, reg2);
        break;
      default:
        JS_NOT_REACHED("unknown getchar mode");
    }

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}


CompileStatus
mjit::Compiler::inlineNativeFunction(uint32 argc, bool callingNew)
{
    JS_ASSERT(!callingNew);

    if (!cx->typeInferenceEnabled())
        return Compile_InlineAbort;

    if (applyTricks == LazyArgsObj)
        return Compile_InlineAbort;

    FrameEntry *origCallee = frame.peek(-((int)argc + 2));
    FrameEntry *thisValue = frame.peek(-((int)argc + 1));

    if (!origCallee->isConstant() || !origCallee->isType(JSVAL_TYPE_OBJECT))
        return Compile_InlineAbort;

    JSObject *callee = &origCallee->getValue().toObject();
    if (!callee->isFunction())
        return Compile_InlineAbort;

    JSFunction *fun = callee->getFunctionPrivate();
    Native native = fun->maybeNative();

    if (!native)
        return Compile_InlineAbort;

    JSValueType type = knownPushedType(0);
    JSValueType thisType = thisValue->isTypeKnown()
                           ? thisValue->getKnownType()
                           : JSVAL_TYPE_UNKNOWN;

    
    for (int i=0; i<argc; i++) {
        FrameEntry * arg = frame.peek(-(i+1));

        if (!arg->isTypeKnown())
            return Compile_InlineAbort;
    }

    if (argc == 1) {
        FrameEntry *arg = frame.peek(-1);
        JSValueType argType = arg->getKnownType();

        if (native == js_math_abs) {
            if (argType == JSVAL_TYPE_INT32 && type == JSVAL_TYPE_INT32)
                return compileMathAbsInt(arg);

            if (argType == JSVAL_TYPE_DOUBLE && type == JSVAL_TYPE_DOUBLE)
                return compileMathAbsDouble(arg);
        }
        if (native == js_math_floor && argType == JSVAL_TYPE_DOUBLE &&
            type == JSVAL_TYPE_INT32) {
            return compileRound(arg, Floor);
        }
        if (native == js_math_round && argType == JSVAL_TYPE_DOUBLE &&
            type == JSVAL_TYPE_INT32) {
            return compileRound(arg, Round);
        }
        if (native == js_math_sqrt && type == JSVAL_TYPE_DOUBLE &&
            (argType == JSVAL_TYPE_INT32 || argType == JSVAL_TYPE_DOUBLE)) {
            return compileMathSqrt(arg);
        }
        if (native == js_str_charCodeAt && argType == JSVAL_TYPE_INT32 &&
            thisType == JSVAL_TYPE_STRING && type == JSVAL_TYPE_INT32) {
            return compileGetChar(thisValue, arg, GetCharCode);
        }
        if (native == js_str_charAt && argType == JSVAL_TYPE_INT32 &&
            thisType == JSVAL_TYPE_STRING && type == JSVAL_TYPE_STRING) {
            return compileGetChar(thisValue, arg, GetChar);
        }
    } else if (argc == 2) {
        FrameEntry *arg1 = frame.peek(-2);
        FrameEntry *arg2 = frame.peek(-1);

        JSValueType arg1Type = arg1->getKnownType();
        JSValueType arg2Type = arg2->getKnownType();

        if (native == js_math_pow && type == JSVAL_TYPE_DOUBLE &&
            (arg1Type == JSVAL_TYPE_DOUBLE || arg1Type == JSVAL_TYPE_INT32) &&
            arg2Type == JSVAL_TYPE_DOUBLE && arg2->isConstant())
        {
            Value arg2Value = arg2->getValue();
            if (arg2Value.toDouble() == -0.5 || arg2Value.toDouble() == 0.5)
                return compileMathPowSimple(arg1, arg2);
        }
    }
    return Compile_InlineAbort;
}

