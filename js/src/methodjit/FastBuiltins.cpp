





































#include "jsbool.h"
#include "jslibmath.h"
#include "jsmath.h"
#include "jsnum.h"
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
mjit::Compiler::compileMathMinMaxDouble(FrameEntry *arg1, FrameEntry *arg2, 
                                        Assembler::DoubleCondition cond)
{
    FPRegisterID fpReg1;
    FPRegisterID fpReg2;
    bool allocate;

    DebugOnly<MaybeJump> notNumber = loadDouble(arg1, &fpReg1, &allocate);
    JS_ASSERT(!((MaybeJump)notNumber).isSet());

    if (!allocate) {
        FPRegisterID fpResultReg = frame.allocFPReg();
        masm.moveDouble(fpReg1, fpResultReg);
        fpReg1 = fpResultReg;
    }

    DebugOnly<MaybeJump> notNumber2 = loadDouble(arg2, &fpReg2, &allocate);
    JS_ASSERT(!((MaybeJump)notNumber2).isSet());


    
    masm.zeroDouble(Registers::FPConversionTemp);
    Jump zeroOrNan = masm.branchDouble(Assembler::DoubleEqualOrUnordered, fpReg1, 
                                       Registers::FPConversionTemp);
    stubcc.linkExit(zeroOrNan, Uses(4));
    Jump zeroOrNan2 = masm.branchDouble(Assembler::DoubleEqualOrUnordered, fpReg2, 
                                        Registers::FPConversionTemp);
    stubcc.linkExit(zeroOrNan2, Uses(4));


    Jump ifTrue = masm.branchDouble(cond, fpReg1, fpReg2);
    masm.moveDouble(fpReg2, fpReg1);

    ifTrue.linkTo(masm.label(), &masm);

    if (allocate)
        frame.freeReg(fpReg2);

    stubcc.leave();
    stubcc.masm.move(Imm32(2), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

    frame.popn(4);
    frame.pushDouble(fpReg1);

    stubcc.rejoin(Changes(1));
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileMathMinMaxInt(FrameEntry *arg1, FrameEntry *arg2, Assembler::Condition cond)
{
    
    if (arg1->isConstant() && arg2->isConstant()) {
        int32 a = arg1->getValue().toInt32();
        int32 b = arg2->getValue().toInt32();

        frame.popn(4);
        if (cond == Assembler::LessThan)
            frame.push(Int32Value(a < b ? a : b));
        else
            frame.push(Int32Value(a > b ? a : b));
        return Compile_Okay;
    }

    Jump ifTrue;
    RegisterID reg;
    if (arg1->isConstant()) {
        reg = frame.copyDataIntoReg(arg2);
        int32_t v = arg1->getValue().toInt32();

        ifTrue = masm.branch32(cond, reg, Imm32(v));
        masm.move(Imm32(v), reg);
    } else if (arg2->isConstant()) {
        reg = frame.copyDataIntoReg(arg1);
        int32_t v = arg2->getValue().toInt32();

        ifTrue = masm.branch32(cond, reg, Imm32(v));
        masm.move(Imm32(v), reg);
    } else {
        reg = frame.copyDataIntoReg(arg1);
        RegisterID regB = frame.tempRegForData(arg2);

        ifTrue = masm.branch32(cond, reg, regB);
        masm.move(regB, reg);
    }

    ifTrue.linkTo(masm.label(), &masm);
    frame.popn(4);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);
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
mjit::Compiler::compileStringFromCode(FrameEntry *arg)
{
    
    RegisterID argReg;
    if (arg->isConstant()) {
        argReg = frame.allocReg();
        masm.move(Imm32(arg->getValue().toInt32()), argReg);
    } else {
        argReg = frame.copyDataIntoReg(arg);
    }

    
    Jump notUnitString = masm.branch32(Assembler::AboveOrEqual, argReg,
                                       Imm32(StaticStrings::UNIT_STATIC_LIMIT));
    stubcc.linkExit(notUnitString, Uses(3));

    
    masm.lshiftPtr(Imm32(sizeof(JSAtom *) == 4 ? 2 : 3), argReg);
    masm.addPtr(ImmPtr(&cx->runtime->staticStrings.unitStaticTable), argReg);
    masm.loadPtr(Address(argReg), argReg);

    stubcc.leave();
    stubcc.masm.move(Imm32(1), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

    frame.popn(3);
    frame.pushTypedPayload(JSVAL_TYPE_STRING, argReg);

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
    masm.loadPtr(Address(objReg, JSObject::offsetOfElements()), slotsReg);

    RegisterID lengthReg = frame.allocReg();
    masm.load32(Address(slotsReg, ObjectElements::offsetOfLength()), lengthReg);

    frame.unpinReg(objReg);

    Int32Key key = Int32Key::FromRegister(lengthReg);

    
    Jump initlenGuard = masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                              slotsReg, key, Assembler::NotEqual);
    stubcc.linkExit(initlenGuard, Uses(3));

    
    Jump capacityGuard = masm.guardArrayExtent(ObjectElements::offsetOfCapacity(),
                                               slotsReg, key, Assembler::BelowOrEqual);
    stubcc.linkExit(capacityGuard, Uses(3));

    masm.storeValue(vr, BaseIndex(slotsReg, lengthReg, masm.JSVAL_SCALE));

    masm.bumpKey(key, 1);
    masm.store32(lengthReg, Address(slotsReg, ObjectElements::offsetOfLength()));
    masm.store32(lengthReg, Address(slotsReg, ObjectElements::offsetOfInitializedLength()));

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
mjit::Compiler::compileArrayPopShift(FrameEntry *thisValue, bool isPacked, bool isArrayPop)
{
    
    if (thisValue->isConstant())
        return Compile_InlineAbort;

#ifdef JSGC_INCREMENTAL_MJ
    
    if (cx->compartment->needsBarrier())
        return Compile_InlineAbort;
#endif

    RegisterID objReg = frame.tempRegForData(thisValue);
    frame.pinReg(objReg);

    RegisterID lengthReg = frame.allocReg();
    RegisterID slotsReg = frame.allocReg();

    JSValueType type = knownPushedType(0);

    MaybeRegisterID dataReg, typeReg;
    if (!analysis->popGuaranteed(PC)) {
        dataReg = frame.allocReg();
        if (type == JSVAL_TYPE_UNKNOWN || type == JSVAL_TYPE_DOUBLE)
            typeReg = frame.allocReg();
    }

    if (isArrayPop) {
        frame.unpinReg(objReg);
    } else {
        



        frame.syncAndKillEverything();
        frame.unpinKilledReg(objReg);
    }

    masm.loadPtr(Address(objReg, JSObject::offsetOfElements()), slotsReg);
    masm.load32(Address(slotsReg, ObjectElements::offsetOfLength()), lengthReg);

    
    Int32Key key = Int32Key::FromRegister(lengthReg);
    Jump initlenGuard = masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                              slotsReg, key, Assembler::NotEqual);
    stubcc.linkExit(initlenGuard, Uses(3));

    




    bool maybeUndefined = pushedTypeSet(0)->hasType(types::Type::UndefinedType());
    Jump emptyGuard = masm.branch32(Assembler::Equal, lengthReg, Imm32(0));
    if (!maybeUndefined)
        stubcc.linkExit(emptyGuard, Uses(3));

    masm.bumpKey(key, -1);

    if (dataReg.isSet()) {
        Jump holeCheck;
        if (isArrayPop) {
            BaseIndex slot(slotsReg, lengthReg, masm.JSVAL_SCALE);
            holeCheck = masm.fastArrayLoadSlot(slot, !isPacked, typeReg, dataReg.reg());
        } else {
            holeCheck = masm.fastArrayLoadSlot(Address(slotsReg), !isPacked, typeReg, dataReg.reg());
            Address addr = frame.addressOf(frame.peek(-2));
            if (typeReg.isSet())
                masm.storeValueFromComponents(typeReg.reg(), dataReg.reg(), addr);
            else
                masm.storeValueFromComponents(ImmType(type), dataReg.reg(), addr);
        }
        if (!isPacked)
            stubcc.linkExit(holeCheck, Uses(3));
    }

    masm.store32(lengthReg, Address(slotsReg, ObjectElements::offsetOfLength()));
    masm.store32(lengthReg, Address(slotsReg, ObjectElements::offsetOfInitializedLength()));

    if (!isArrayPop)
        INLINE_STUBCALL(stubs::ArrayShift, REJOIN_NONE);

    stubcc.leave();
    stubcc.masm.move(Imm32(0), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

    frame.freeReg(slotsReg);
    frame.freeReg(lengthReg);
    frame.popn(2);

    if (dataReg.isSet()) {
        if (isArrayPop) {
            if (typeReg.isSet())
                frame.pushRegs(typeReg.reg(), dataReg.reg(), type);
            else
                frame.pushTypedPayload(type, dataReg.reg());
        } else {
            frame.pushSynced(type);
            if (typeReg.isSet())
                frame.freeReg(typeReg.reg());
            frame.freeReg(dataReg.reg());
        }
    } else {
        frame.push(UndefinedValue());
    }

    stubcc.rejoin(Changes(1));

    if (maybeUndefined) {
        
        if (dataReg.isSet()) {
            stubcc.linkExitDirect(emptyGuard, stubcc.masm.label());
            if (isArrayPop) {
                if (typeReg.isSet()) {
                    stubcc.masm.loadValueAsComponents(UndefinedValue(), typeReg.reg(), dataReg.reg());
                } else {
                    JS_ASSERT(type == JSVAL_TYPE_UNDEFINED);
                    stubcc.masm.loadValuePayload(UndefinedValue(), dataReg.reg());
                }
            } else {
                stubcc.masm.storeValue(UndefinedValue(), frame.addressOf(frame.peek(-1)));
            }
            stubcc.crossJump(stubcc.masm.jump(), masm.label());
        } else {
            emptyGuard.linkTo(masm.label(), &masm);
        }
    }

    return Compile_Okay;
}

CompileStatus
mjit::Compiler::compileArrayConcat(types::TypeSet *thisTypes, types::TypeSet *argTypes,
                                   FrameEntry *thisValue, FrameEntry *argValue)
{
    



    if (thisTypes->getObjectCount() != 1)
        return Compile_InlineAbort;
    types::TypeObject *thisType = thisTypes->getTypeObject(0);
    if (!thisType || thisType->proto->getGlobal() != globalObj)
        return Compile_InlineAbort;

    




    thisTypes->addFreeze(cx);
    argTypes->addFreeze(cx);
    types::TypeSet *thisElemTypes = thisType->getProperty(cx, JSID_VOID, false);
    if (!thisElemTypes)
        return Compile_Error;
    if (!pushedTypeSet(0)->hasType(types::Type::ObjectType(thisType)))
        return Compile_InlineAbort;
    for (unsigned i = 0; i < argTypes->getObjectCount(); i++) {
        if (argTypes->getSingleObject(i))
            return Compile_InlineAbort;
        types::TypeObject *argType = argTypes->getTypeObject(i);
        if (!argType)
            continue;
        types::TypeSet *elemTypes = argType->getProperty(cx, JSID_VOID, false);
        if (!elemTypes)
            return Compile_Error;
        if (!elemTypes->knownSubset(cx, thisElemTypes))
            return Compile_InlineAbort;
    }

    

    RegisterID slotsReg = frame.allocReg();
    RegisterID reg = frame.allocReg();

    Int32Key key = Int32Key::FromRegister(reg);

    RegisterID objReg = frame.tempRegForData(thisValue);
    masm.loadPtr(Address(objReg, JSObject::offsetOfElements()), slotsReg);
    masm.load32(Address(slotsReg, ObjectElements::offsetOfLength()), reg);
    Jump initlenOneGuard = masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                                 slotsReg, key, Assembler::NotEqual);
    stubcc.linkExit(initlenOneGuard, Uses(3));

    objReg = frame.tempRegForData(argValue);
    masm.loadPtr(Address(objReg, JSObject::offsetOfElements()), slotsReg);
    masm.load32(Address(slotsReg, ObjectElements::offsetOfLength()), reg);
    Jump initlenTwoGuard = masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                                 slotsReg, key, Assembler::NotEqual);
    stubcc.linkExit(initlenTwoGuard, Uses(3));

    frame.freeReg(reg);
    frame.freeReg(slotsReg);
    frame.syncAndForgetEverything();

    





    JSObject *templateObject = NewDenseEmptyArray(cx, thisType->proto);
    if (!templateObject)
        return Compile_Error;
    templateObject->setType(thisType);

    RegisterID result = Registers::ReturnReg;
    Jump emptyFreeList = masm.getNewObject(cx, result, templateObject);
    stubcc.linkExit(emptyFreeList, Uses(3));

    masm.storeValueFromComponents(ImmType(JSVAL_TYPE_OBJECT), result, frame.addressOf(frame.peek(-3)));
    INLINE_STUBCALL(stubs::ArrayConcatTwoArrays, REJOIN_FALLTHROUGH);

    stubcc.leave();
    stubcc.masm.move(Imm32(1), Registers::ArgReg1);
    OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);

    frame.popn(3);
    frame.pushSynced(JSVAL_TYPE_OBJECT);

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

    size_t maxArraySlots =
        gc::GetGCKindSlots(gc::FINALIZE_OBJECT_LAST) - ObjectElements::VALUES_PER_HEADER;

    if (argc > maxArraySlots)
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

    int offset = JSObject::offsetOfFixedElements();
    masm.store32(Imm32(argc),
                 Address(result, offset + ObjectElements::offsetOfInitializedLength()));

    for (unsigned i = 0; i < argc; i++) {
        FrameEntry *arg = frame.peek(-(int)argc + i);
        frame.storeTo(arg, Address(result, offset),  true);
        offset += sizeof(Value);
    }

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

    Native native = callee->toFunction()->maybeNative();

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
        if ((native == js::array_pop || native == js::array_shift) && thisType == JSVAL_TYPE_OBJECT) {
            







            if (!thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY |
                                           types::OBJECT_FLAG_ITERATED) &&
                !arrayPrototypeHasIndexedProperty()) {
                bool packed = !thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED_ARRAY);
                return compileArrayPopShift(thisValue, packed, native == js::array_pop);
            }
        }
    } else if (argc == 1) {
        FrameEntry *arg = frame.peek(-1);
        types::TypeSet *argTypes = frame.extra(arg).types;
        if (!argTypes)
            return Compile_InlineAbort;
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
        if (native == js::str_fromCharCode && argType == JSVAL_TYPE_INT32 &&
            type == JSVAL_TYPE_STRING) {
            return compileStringFromCode(arg);
        }
        if (native == js::array_push &&
            thisType == JSVAL_TYPE_OBJECT && type == JSVAL_TYPE_INT32) {
            



            if (!thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY) &&
                !arrayPrototypeHasIndexedProperty()) {
                return compileArrayPush(thisValue, arg);
            }
        }
        if (native == js::array_concat && argType == JSVAL_TYPE_OBJECT &&
            thisType == JSVAL_TYPE_OBJECT && type == JSVAL_TYPE_OBJECT &&
            !thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY) &&
            !argTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY)) {
            return compileArrayConcat(thisTypes, argTypes, thisValue, arg);
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
        if ((native == js_math_min || native == js_math_max)) {
            if (arg1Type == JSVAL_TYPE_INT32 && arg2Type == JSVAL_TYPE_INT32 &&
                type == JSVAL_TYPE_INT32) {
                return compileMathMinMaxInt(arg1, arg2, 
                        native == js_math_min ? Assembler::LessThan : Assembler::GreaterThan);
            }
            if ((arg1Type == JSVAL_TYPE_INT32 || arg1Type == JSVAL_TYPE_DOUBLE) &&
                (arg2Type == JSVAL_TYPE_INT32 || arg2Type == JSVAL_TYPE_DOUBLE) &&
                type == JSVAL_TYPE_DOUBLE) {
                return compileMathMinMaxDouble(arg1, arg2,
                        (native == js_math_min)
                        ? Assembler::DoubleLessThan
                        : Assembler::DoubleGreaterThan);
            }
        }
    }
    return Compile_InlineAbort;
}

