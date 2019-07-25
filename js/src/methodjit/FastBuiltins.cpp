





































#include "jsbool.h"
#include "jslibmath.h"
#include "jsmath.h"
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
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

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

    DebugOnly<MaybeJump> notNumber = loadDouble(arg, &fpReg, &allocate);
    JS_ASSERT(!((MaybeJump)notNumber).isSet());

    masm.absDouble(fpReg, fpResultReg);

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

    DebugOnly<MaybeJump> notNumber = loadDouble(arg, &fpReg, &allocate);
    JS_ASSERT(!((MaybeJump)notNumber).isSet());

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
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

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

    DebugOnly<MaybeJump> notNumber = loadDouble(arg, &fpReg, &allocate);
    JS_ASSERT(!((MaybeJump)notNumber).isSet());

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

    DebugOnly<MaybeJump> notNumber = loadDouble(arg1, &fpReg, &allocate);
    JS_ASSERT(!((MaybeJump)notNumber).isSet());

    
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
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

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
                                           Imm32(StaticStrings::UNIT_STATIC_LIMIT));
        stubcc.linkExit(notUnitString, Uses(3));

        
        masm.lshiftPtr(Imm32(sizeof(JSAtom *) == 4 ? 2 : 3), reg2);
        masm.addPtr(ImmPtr(&cx->runtime->staticStrings.unitStaticTable), reg2);
        masm.loadPtr(Address(reg2), reg2);
    }

    if (thisValue->isConstant())
        frame.freeReg(strReg);
    if (arg->isConstant())
        frame.freeReg(argReg);
    frame.freeReg(reg1);

    stubcc.leave();
    stubcc.masm.move(Imm32(1), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

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
mjit::Compiler::compileArrayPush(FrameEntry *thisValue, FrameEntry *arg)
{
    

    
    if (frame.haveSameBacking(thisValue, arg) || thisValue->isConstant())
        return Compile_InlineAbort;

    
    ValueRemat vr;
    frame.pinEntry(arg, vr,  false);

    RegisterID objReg = frame.tempRegForData(thisValue);
    frame.pinReg(objReg);

    RegisterID slotsReg = frame.allocReg();

    RegisterID lengthReg = frame.allocReg();
    masm.load32(Address(objReg, offsetof(JSObject, privateData)), lengthReg);

    frame.unpinReg(objReg);

    Int32Key key = Int32Key::FromRegister(lengthReg);

    
    Jump initlenGuard = masm.guardArrayExtent(offsetof(JSObject, initializedLength),
                                              objReg, key, Assembler::NotEqual);
    stubcc.linkExit(initlenGuard, Uses(3));

    
    Jump capacityGuard = masm.guardArrayExtent(offsetof(JSObject, capacity),
                                               objReg, key, Assembler::BelowOrEqual);
    stubcc.linkExit(capacityGuard, Uses(3));

    masm.loadPtr(Address(objReg, JSObject::offsetOfSlots()), slotsReg);
    masm.storeValue(vr, BaseIndex(slotsReg, lengthReg, masm.JSVAL_SCALE));

    masm.bumpKey(key, 1);
    masm.store32(lengthReg, Address(objReg, offsetof(JSObject, privateData)));
    masm.store32(lengthReg, Address(objReg, offsetof(JSObject, initializedLength)));

    stubcc.leave();
    stubcc.masm.move(Imm32(1), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

    frame.unpinEntry(vr);
    frame.freeReg(slotsReg);
    frame.popn(3);

    frame.pushTypedPayload(JSVAL_TYPE_INT32, lengthReg);

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileArrayPop(FrameEntry *thisValue, bool isPacked)
{
    
    if (thisValue->isConstant())
        return Compile_InlineAbort;

    RegisterID objReg = frame.tempRegForData(thisValue);
    frame.pinReg(objReg);

    RegisterID lengthReg = frame.allocReg();
    masm.load32(Address(objReg, offsetof(JSObject, privateData)), lengthReg);

    JSValueType type = knownPushedType(0);

    MaybeRegisterID slotsReg, dataReg, typeReg;
    if (!analysis->popGuaranteed(PC)) {
        slotsReg = frame.allocReg();
        dataReg = frame.allocReg();
        if (type == JSVAL_TYPE_UNKNOWN || type == JSVAL_TYPE_DOUBLE)
            typeReg = frame.allocReg();
    }

    frame.unpinReg(objReg);

    
    Int32Key key = Int32Key::FromRegister(lengthReg);
    Jump initlenGuard = masm.guardArrayExtent(offsetof(JSObject, initializedLength),
                                              objReg, key, Assembler::NotEqual);
    stubcc.linkExit(initlenGuard, Uses(3));

    
    Jump emptyGuard = masm.branch32(Assembler::Equal, lengthReg, Imm32(0));
    stubcc.linkExit(emptyGuard, Uses(3));

    masm.bumpKey(key, -1);

    if (dataReg.isSet()) {
        masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), slotsReg.reg());
        BaseIndex slot(slotsReg.reg(), lengthReg, masm.JSVAL_SCALE);
        Jump holeCheck = masm.fastArrayLoadSlot(slot, !isPacked, typeReg, dataReg.reg());
        if (!isPacked)
            stubcc.linkExit(holeCheck, Uses(3));
        frame.freeReg(slotsReg.reg());
    }

    masm.store32(lengthReg, Address(objReg, offsetof(JSObject, privateData)));
    masm.store32(lengthReg, Address(objReg, offsetof(JSObject, initializedLength)));

    stubcc.leave();
    stubcc.masm.move(Imm32(0), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

    frame.freeReg(lengthReg);
    frame.popn(2);

    if (dataReg.isSet()) {
        if (type == JSVAL_TYPE_UNKNOWN || type == JSVAL_TYPE_DOUBLE)
            frame.pushRegs(typeReg.reg(), dataReg.reg(), type);
        else
            frame.pushTypedPayload(type, dataReg.reg());
    } else {
        frame.push(UndefinedValue());
    }

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileArrayWithLength(uint32 argc)
{
    
    JS_ASSERT(argc == 0 || argc == 1);

    int32 length = 0;
    if (argc == 1) {
        FrameEntry *arg = frame.peek(-1);
        if (!arg->isConstant() || !arg->getValue().isInt32())
            return Compile_InlineAbort;
        length = arg->getValue().toInt32();
        if (length < 0)
            return Compile_InlineAbort;
    }

    types::TypeObject *type = types::TypeScript::InitObject(cx, script, PC, JSProto_Array);
    if (!type)
        return Compile_Error;

    JSObject *templateObject = NewDenseUnallocatedArray(cx, length, type->proto);
    if (!templateObject)
        return Compile_Error;
    templateObject->setType(type);

    RegisterID result = frame.allocReg();
    Jump emptyFreeList = masm.getNewObject(cx, result, templateObject);

    stubcc.linkExit(emptyFreeList, Uses(0));
    stubcc.leave();

    stubcc.masm.move(Imm32(argc), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

    frame.popn(argc + 2);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, result);

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileArrayWithArgs(uint32 argc)
{
    




    JS_ASSERT(argc >= 2);

    if (argc >= gc::GetGCKindSlots(gc::FINALIZE_OBJECT_LAST))
        return Compile_InlineAbort;

    types::TypeObject *type = types::TypeScript::InitObject(cx, script, PC, JSProto_Array);
    if (!type)
        return Compile_Error;

    JSObject *templateObject = NewDenseUnallocatedArray(cx, argc, type->proto);
    if (!templateObject)
        return Compile_Error;
    templateObject->setType(type);

    JS_ASSERT(templateObject->getDenseArrayCapacity() >= argc);

    RegisterID result = frame.allocReg();
    Jump emptyFreeList = masm.getNewObject(cx, result, templateObject);
    stubcc.linkExit(emptyFreeList, Uses(0));

    for (unsigned i = 0; i < argc; i++) {
        FrameEntry *arg = frame.peek(-(int)argc + i);
        frame.storeTo(arg, Address(result, JSObject::getFixedSlotOffset(i)),  true);
    }

    masm.storePtr(ImmPtr((void *) argc), Address(result, offsetof(JSObject, initializedLength)));

    stubcc.leave();

    stubcc.masm.move(Imm32(argc), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

    frame.popn(argc + 2);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, result);

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::inlineNativeFunction(uint32 argc, bool callingNew)
{
    if (!cx->typeInferenceEnabled())
        return Compile_InlineAbort;

    if (applyTricks == LazyArgsObj)
        return Compile_InlineAbort;

    FrameEntry *origCallee = frame.peek(-((int)argc + 2));
    FrameEntry *thisValue = frame.peek(-((int)argc + 1));
    types::TypeSet *thisTypes = analysis->poppedTypes(PC, argc);

    if (!origCallee->isConstant() || !origCallee->isType(JSVAL_TYPE_OBJECT))
        return Compile_InlineAbort;

    JSObject *callee = &origCallee->getValue().toObject();
    if (!callee->isFunction())
        return Compile_InlineAbort;

    



    if (!globalObj || globalObj != callee->getGlobal())
        return Compile_InlineAbort;

    JSFunction *fun = callee->getFunctionPrivate();
    Native native = fun->maybeNative();

    if (!native)
        return Compile_InlineAbort;

    JSValueType type = knownPushedType(0);
    JSValueType thisType = thisValue->isTypeKnown()
                           ? thisValue->getKnownType()
                           : JSVAL_TYPE_UNKNOWN;

    




    

    if (native == js_Array && type == JSVAL_TYPE_OBJECT && globalObj) {
        if (argc == 0 || argc == 1)
            return compileArrayWithLength(argc);
        return compileArrayWithArgs(argc);
    }

    
    if (callingNew)
        return Compile_InlineAbort;

    if (argc == 0) {
        if (native == js::array_pop && thisType == JSVAL_TYPE_OBJECT) {
            







            if (!thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY |
                                           types::OBJECT_FLAG_ITERATED) &&
                !arrayPrototypeHasIndexedProperty()) {
                bool packed = !thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED_ARRAY);
                return compileArrayPop(thisValue, packed);
            }
        }
    } else if (argc == 1) {
        FrameEntry *arg = frame.peek(-1);
        JSValueType argType = arg->isTypeKnown() ? arg->getKnownType() : JSVAL_TYPE_UNKNOWN;

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
        if (native == js::array_push &&
            thisType == JSVAL_TYPE_OBJECT && type == JSVAL_TYPE_INT32) {
            



            if (!thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY) &&
                !arrayPrototypeHasIndexedProperty()) {
                return compileArrayPush(thisValue, arg);
            }
        }
    } else if (argc == 2) {
        FrameEntry *arg1 = frame.peek(-2);
        FrameEntry *arg2 = frame.peek(-1);

        JSValueType arg1Type = arg1->isTypeKnown() ? arg1->getKnownType() : JSVAL_TYPE_UNKNOWN;
        JSValueType arg2Type = arg2->isTypeKnown() ? arg2->getKnownType() : JSVAL_TYPE_UNKNOWN;

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

