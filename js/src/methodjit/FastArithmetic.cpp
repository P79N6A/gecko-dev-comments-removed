







































#include "jsbool.h"
#include "jslibmath.h"
#include "jsnum.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"
#include "methodjit/StubCalls.h"
#include "methodjit/FrameState-inl.h"

using namespace js;
using namespace js::mjit;
using namespace JSC;

typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;

static inline bool
JSOpBinaryTryConstantFold(JSContext *cx, FrameState &frame, JSOp op, FrameEntry *lhs, FrameEntry *rhs)
{
    if (!lhs->isConstant() || !rhs->isConstant())
        return false;

    const Value &L = lhs->getValue();
    const Value &R = rhs->getValue();

    if (!L.isPrimitive() || !R.isPrimitive() ||
        (op == JSOP_ADD && (L.isString() || R.isString()))) {
        return false;
    }

    double dL, dR;
    ValueToNumber(cx, L, &dL);
    ValueToNumber(cx, R, &dR);

    switch (op) {
      case JSOP_ADD:
        dL += dR;
        break;
      case JSOP_SUB:
        dL -= dR;
        break;
      case JSOP_MUL:
        dL *= dR;
        break;
      case JSOP_DIV:
        if (dR == 0) {
#ifdef XP_WIN
            if (JSDOUBLE_IS_NaN(dR))
                dL = js_NaN;
            else
#endif
            if (dL == 0 || JSDOUBLE_IS_NaN(dL))
                dL = js_NaN;
            else if (JSDOUBLE_IS_NEG(dL) != JSDOUBLE_IS_NEG(dR))
                dL = cx->runtime->negativeInfinityValue.toDouble();
            else
                dL = cx->runtime->positiveInfinityValue.toDouble();
        } else {
            dL /= dR;
        }
        break;
      case JSOP_MOD:
        if (dL == 0)
            dL = js_NaN;
        else
            dL = js_fmod(dR, dL);
        break;

      default:
        JS_NOT_REACHED("NYI");
        break;
    }

    Value v;
    v.setNumber(dL);
    frame.popn(2);
    frame.push(v);

    return true;
}

void
mjit::Compiler::slowLoadConstantDouble(Assembler &masm,
                                       FrameEntry *fe, FPRegisterID fpreg)
{
    DoublePatch patch;
    if (fe->getKnownType() == JSVAL_TYPE_INT32)
        patch.d = (double)fe->getValue().toInt32();
    else
        patch.d = fe->getValue().toDouble();
    patch.label = masm.loadDouble(NULL, fpreg);
    patch.ool = &masm != &this->masm;
    JS_ASSERT_IF(patch.ool, &masm == &stubcc.masm);
    doubleList.append(patch);
}

void
mjit::Compiler::maybeJumpIfNotInt32(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                                    MaybeRegisterID &mreg)
{
    if (!fe->isTypeKnown()) {
        if (mreg.isSet())
            mj.setJump(masm.testInt32(Assembler::NotEqual, mreg.reg()));
        else
            mj.setJump(masm.testInt32(Assembler::NotEqual, frame.addressOf(fe)));
    } else if (fe->getKnownType() != JSVAL_TYPE_INT32) {
        mj.setJump(masm.jump());
    }
}

void
mjit::Compiler::maybeJumpIfNotDouble(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                                    MaybeRegisterID &mreg)
{
    if (!fe->isTypeKnown()) {
        if (mreg.isSet())
            mj.setJump(masm.testDouble(Assembler::NotEqual, mreg.reg()));
        else
            mj.setJump(masm.testDouble(Assembler::NotEqual, frame.addressOf(fe)));
    } else if (fe->getKnownType() != JSVAL_TYPE_DOUBLE) {
        mj.setJump(masm.jump());
    }
}

void
mjit::Compiler::jsop_binary(JSOp op, VoidStub stub)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    if (JSOpBinaryTryConstantFold(cx, frame, op, lhs, rhs))
        return;

    



    if ((op == JSOP_MOD) ||
        (lhs->isTypeKnown() && (lhs->getKnownType() > JSVAL_UPPER_INCL_TYPE_OF_NUMBER_SET)) ||
        (rhs->isTypeKnown() && (rhs->getKnownType() > JSVAL_UPPER_INCL_TYPE_OF_NUMBER_SET)) 
#if defined(JS_CPU_ARM)
        
        || op == JSOP_MUL
#endif 
    ) {
        bool isStringResult = (op == JSOP_ADD) &&
                              ((lhs->isTypeKnown() && lhs->getKnownType() == JSVAL_TYPE_STRING) ||
                               (rhs->isTypeKnown() && rhs->getKnownType() == JSVAL_TYPE_STRING));
        prepareStubCall(Uses(2));
        stubCall(stub);
        frame.popn(2);
        if (isStringResult)
            frame.pushSyncedType(JSVAL_TYPE_STRING);
        else
            frame.pushSynced();
        return;
    }

    
    bool canDoIntMath = op != JSOP_DIV &&
                        !((rhs->isTypeKnown() && rhs->getKnownType() == JSVAL_TYPE_DOUBLE) ||
                          (lhs->isTypeKnown() && lhs->getKnownType() == JSVAL_TYPE_DOUBLE));

    if (canDoIntMath)
        jsop_binary_full(lhs, rhs, op, stub);
    else
        jsop_binary_double(lhs, rhs, op, stub);
}

static void
EmitDoubleOp(JSOp op, FPRegisterID fpRight, FPRegisterID fpLeft, Assembler &masm)
{
    switch (op) {
      case JSOP_ADD:
        masm.addDouble(fpRight, fpLeft);
        break;

      case JSOP_SUB:
        masm.subDouble(fpRight, fpLeft);
        break;

      case JSOP_MUL:
        masm.mulDouble(fpRight, fpLeft);
        break;

      case JSOP_DIV:
        masm.divDouble(fpRight, fpLeft);
        break;

      default:
        JS_NOT_REACHED("unrecognized binary op");
    }
}

mjit::Compiler::MaybeJump
mjit::Compiler::loadDouble(FrameEntry *fe, FPRegisterID fpReg)
{
    MaybeJump notNumber;

    if (fe->isConstant()) {
        slowLoadConstantDouble(masm, fe, fpReg);
    } else if (!fe->isTypeKnown()) {
        frame.tempRegForType(fe);
        Jump j = frame.testDouble(Assembler::Equal, fe);
        notNumber = frame.testInt32(Assembler::NotEqual, fe);
        frame.convertInt32ToDouble(masm, fe, fpReg);
        Jump converted = masm.jump();
        j.linkTo(masm.label(), &masm);
        frame.loadDouble(fe, fpReg, masm);
        converted.linkTo(masm.label(), &masm);
    } else if (fe->getKnownType() == JSVAL_TYPE_INT32) {
        frame.tempRegForData(fe);
        frame.convertInt32ToDouble(masm, fe, fpReg);
    } else {
        JS_ASSERT(fe->getKnownType() == JSVAL_TYPE_DOUBLE);
        frame.loadDouble(fe, fpReg, masm);
    }

    return notNumber;
}





void
mjit::Compiler::jsop_binary_double(FrameEntry *lhs, FrameEntry *rhs, JSOp op, VoidStub stub)
{
    FPRegisterID fpLeft = FPRegisters::First;
    FPRegisterID fpRight = FPRegisters::Second;

    MaybeJump lhsNotNumber = loadDouble(lhs, fpLeft);

    MaybeJump rhsNotNumber;
    if (frame.haveSameBacking(lhs, rhs))
        masm.moveDouble(fpLeft, fpRight);
    else
        rhsNotNumber = loadDouble(rhs, fpRight);

    EmitDoubleOp(op, fpRight, fpLeft, masm);
    masm.storeDouble(fpLeft, frame.addressOf(lhs));

    if (lhsNotNumber.isSet())
        stubcc.linkExit(lhsNotNumber.get(), Uses(2));
    if (rhsNotNumber.isSet())
        stubcc.linkExit(rhsNotNumber.get(), Uses(2));

    stubcc.leave();
    stubcc.call(stub);

    frame.popn(2);
    frame.pushNumber(MaybeRegisterID());

    stubcc.rejoin(Changes(1));
}




void
mjit::Compiler::jsop_binary_full_simple(FrameEntry *fe, JSOp op, VoidStub stub)
{
    FrameEntry *lhs = frame.peek(-2);

    
    if (fe->isTypeKnown() && fe->getKnownType() == JSVAL_TYPE_DOUBLE) {
        loadDouble(fe, FPRegisters::First);
        EmitDoubleOp(op, FPRegisters::First, FPRegisters::First, masm);
        frame.popn(2);
        frame.pushNumber(MaybeRegisterID());
        return;
    }

    
    FrameState::BinaryAlloc regs;
    frame.allocForSameBinary(fe, op, regs);

    MaybeJump notNumber;
    MaybeJump doublePathDone;
    if (!fe->isTypeKnown()) {
        Jump notInt = masm.testInt32(Assembler::NotEqual, regs.lhsType.reg());
        stubcc.linkExitDirect(notInt, stubcc.masm.label());

        notNumber = stubcc.masm.testDouble(Assembler::NotEqual, regs.lhsType.reg());
        frame.loadDouble(fe, FPRegisters::First, stubcc.masm);
        EmitDoubleOp(op, FPRegisters::First, FPRegisters::First, stubcc.masm);

        
        Address result = frame.addressOf(lhs);
        stubcc.masm.storeDouble(FPRegisters::First, result);

        
        stubcc.masm.loadPayload(result, regs.result);

        doublePathDone = stubcc.masm.jump();
    }

    
    MaybeJump overflow;
    switch (op) {
      case JSOP_ADD:
        overflow = masm.branchAdd32(Assembler::Overflow, regs.result, regs.result);
        break;

      case JSOP_SUB:
        overflow = masm.branchSub32(Assembler::Overflow, regs.result, regs.result);
        break;

#if !defined(JS_CPU_ARM)
      case JSOP_MUL:
        overflow = masm.branchMul32(Assembler::Overflow, regs.result, regs.result);
        break;
#endif

      default:
        JS_NOT_REACHED("unrecognized op");
    }
    
    JS_ASSERT(overflow.isSet());

    



    MaybeJump overflowDone;
    stubcc.linkExitDirect(overflow.get(), stubcc.masm.label());
    {
        if (regs.lhsNeedsRemat) {
            Address address = masm.payloadOf(frame.addressOf(lhs));
            stubcc.masm.convertInt32ToDouble(address, FPRegisters::First);
        } else if (!lhs->isConstant()) {
            stubcc.masm.convertInt32ToDouble(regs.lhsData.reg(), FPRegisters::First);
        } else {
            slowLoadConstantDouble(stubcc.masm, lhs, FPRegisters::First);
        }

        EmitDoubleOp(op, FPRegisters::First, FPRegisters::First, stubcc.masm);

        Address address = frame.addressOf(lhs);
        stubcc.masm.storeDouble(FPRegisters::First, address);
        stubcc.masm.loadPayload(address, regs.result);

        overflowDone = stubcc.masm.jump();
    }

    
    if (notNumber.isSet())
        notNumber.get().linkTo(stubcc.masm.label(), &stubcc.masm);
    overflowDone.get().linkTo(stubcc.masm.label(), &stubcc.masm);

    
    stubcc.syncExit(Uses(2));
    stubcc.leave();
    stubcc.call(stub);

    
    frame.popn(2);
    frame.pushNumber(regs.result, true);

    
    if (doublePathDone.isSet())
        stubcc.linkRejoin(doublePathDone.get());
    stubcc.linkRejoin(overflowDone.get());

    stubcc.rejoin(Changes(1));
}





































void
mjit::Compiler::jsop_binary_full(FrameEntry *lhs, FrameEntry *rhs, JSOp op, VoidStub stub)
{
    if (frame.haveSameBacking(lhs, rhs)) {
        jsop_binary_full_simple(lhs, op, stub);
        return;
    }

    
    FrameState::BinaryAlloc regs;
    frame.allocForBinary(lhs, rhs, op, regs);

    
    JS_ASSERT_IF(lhs->isTypeKnown(), lhs->getKnownType() == JSVAL_TYPE_INT32);
    JS_ASSERT_IF(rhs->isTypeKnown(), rhs->getKnownType() == JSVAL_TYPE_INT32);

    FPRegisterID fpLeft = FPRegisters::First;
    FPRegisterID fpRight = FPRegisters::Second;

    MaybeJump lhsNotDouble;
    MaybeJump rhsNotNumber;
    MaybeJump lhsUnknownDone;
    if (!lhs->isTypeKnown()) {
        
        Jump lhsNotInt32 = masm.testInt32(Assembler::NotEqual, regs.lhsType.reg());
        stubcc.linkExitDirect(lhsNotInt32, stubcc.masm.label());

        
        lhsNotDouble = stubcc.masm.testDouble(Assembler::NotEqual, regs.lhsType.reg());

        
        MaybeJump rhsIsDouble;
        if (!rhs->isTypeKnown()) {
            rhsIsDouble = stubcc.masm.testDouble(Assembler::Equal, regs.rhsType.reg());
            rhsNotNumber = stubcc.masm.testInt32(Assembler::NotEqual, regs.rhsType.reg());
        }

        
        if (rhs->isConstant())
            slowLoadConstantDouble(stubcc.masm, rhs, fpRight);
        else
            stubcc.masm.convertInt32ToDouble(regs.rhsData.reg(), fpRight);

        if (!rhs->isTypeKnown()) {
            
            Jump converted = stubcc.masm.jump();
            rhsIsDouble.get().linkTo(stubcc.masm.label(), &stubcc.masm);

            
            frame.loadDouble(rhs, fpRight, stubcc.masm);

            converted.linkTo(stubcc.masm.label(), &stubcc.masm);
        }

        
        frame.loadDouble(lhs, fpLeft, stubcc.masm);
        lhsUnknownDone = stubcc.masm.jump();
    }

    MaybeJump rhsNotNumber2;
    if (!rhs->isTypeKnown()) {
        
        Jump notInt32 = masm.testInt32(Assembler::NotEqual, regs.rhsType.reg());
        stubcc.linkExitDirect(notInt32, stubcc.masm.label());

        
        rhsNotNumber2 = stubcc.masm.testDouble(Assembler::NotEqual, regs.rhsType.reg());

        
        if (lhs->isConstant())
            slowLoadConstantDouble(stubcc.masm, lhs, fpLeft);
        else
            stubcc.masm.convertInt32ToDouble(regs.lhsData.reg(), fpLeft);

        
        frame.loadDouble(rhs, fpRight, stubcc.masm);
    }

    
    MaybeJump doublePathDone;
    if (!rhs->isTypeKnown() || lhsUnknownDone.isSet()) {
        
        if (lhsUnknownDone.isSet())
            lhsUnknownDone.get().linkTo(stubcc.masm.label(), &stubcc.masm);
        
        
        EmitDoubleOp(op, fpRight, fpLeft, stubcc.masm);

        
        Address result = frame.addressOf(lhs);
        stubcc.masm.storeDouble(fpLeft, result);

        
        stubcc.masm.loadPayload(result, regs.result);

        
        doublePathDone = stubcc.masm.jump();
    }

    
    int32 value = 0;
    JSOp origOp = op;
    MaybeRegisterID reg;
    if (!regs.resultHasRhs) {
        if (!regs.rhsData.isSet())
            value = rhs->getValue().toInt32();
        else
            reg = regs.rhsData.reg();
    } else {
        if (!regs.lhsData.isSet())
            value = lhs->getValue().toInt32();
        else
            reg = regs.lhsData.reg();
        if (op == JSOP_SUB) {
            masm.neg32(regs.result);
            op = JSOP_ADD;
        }
    }

    
    MaybeJump overflow;
    switch (op) {
      case JSOP_ADD:
        if (reg.isSet())
            overflow = masm.branchAdd32(Assembler::Overflow, reg.reg(), regs.result);
        else
            overflow = masm.branchAdd32(Assembler::Overflow, Imm32(value), regs.result);
        break;

      case JSOP_SUB:
        if (reg.isSet())
            overflow = masm.branchSub32(Assembler::Overflow, reg.reg(), regs.result);
        else
            overflow = masm.branchSub32(Assembler::Overflow, Imm32(value), regs.result);
        break;

#if !defined(JS_CPU_ARM)
      case JSOP_MUL:
        JS_ASSERT(reg.isSet());
        overflow = masm.branchMul32(Assembler::Overflow, reg.reg(), regs.result);
        break;
#endif

      default:
        JS_NOT_REACHED("unrecognized op");
    }
    op = origOp;
    
    JS_ASSERT(overflow.isSet());

    



    MaybeJump overflowDone;
    stubcc.linkExitDirect(overflow.get(), stubcc.masm.label());
    {
        if (regs.lhsNeedsRemat) {
            Address address = masm.payloadOf(frame.addressOf(lhs));
            stubcc.masm.convertInt32ToDouble(address, fpLeft);
        } else if (!lhs->isConstant()) {
            stubcc.masm.convertInt32ToDouble(regs.lhsData.reg(), fpLeft);
        } else {
            slowLoadConstantDouble(stubcc.masm, lhs, fpLeft);
        }

        if (regs.rhsNeedsRemat) {
            Address address = masm.payloadOf(frame.addressOf(rhs));
            stubcc.masm.convertInt32ToDouble(address, fpRight);
        } else if (!rhs->isConstant()) {
            stubcc.masm.convertInt32ToDouble(regs.rhsData.reg(), fpRight);
        } else {
            slowLoadConstantDouble(stubcc.masm, rhs, fpRight);
        }

        EmitDoubleOp(op, fpRight, fpLeft, stubcc.masm);

        Address address = frame.addressOf(lhs);
        stubcc.masm.storeDouble(fpLeft, address);
        stubcc.masm.loadPayload(address, regs.result);

        overflowDone = stubcc.masm.jump();
    }

    
    if (regs.extraFree.isSet())
        frame.freeReg(regs.extraFree.reg());

    
    if (lhsNotDouble.isSet()) {
        lhsNotDouble.get().linkTo(stubcc.masm.label(), &stubcc.masm);
        if (rhsNotNumber.isSet())
            rhsNotNumber.get().linkTo(stubcc.masm.label(), &stubcc.masm);
    }
    if (rhsNotNumber2.isSet())
        rhsNotNumber2.get().linkTo(stubcc.masm.label(), &stubcc.masm);

    
    stubcc.syncExit(Uses(2));
    stubcc.leave();
    stubcc.call(stub);

    
    frame.popn(2);
    frame.pushNumber(regs.result, true);

    
    if (doublePathDone.isSet())
        stubcc.linkRejoin(doublePathDone.get());
    stubcc.linkRejoin(overflowDone.get());

    stubcc.rejoin(Changes(1));
}

static const uint64 DoubleNegMask = 0x8000000000000000ULL;

void
mjit::Compiler::jsop_neg()
{
    FrameEntry *fe = frame.peek(-1);

    if (fe->isTypeKnown() && fe->getKnownType() > JSVAL_UPPER_INCL_TYPE_OF_NUMBER_SET) {
        prepareStubCall(Uses(1));
        stubCall(stubs::Neg);
        frame.pop();
        frame.pushSynced();
        return;
    }

    JS_ASSERT(!fe->isConstant());

    


    MaybeRegisterID feTypeReg;
    if (!fe->isTypeKnown() && !frame.shouldAvoidTypeRemat(fe)) {
        
        feTypeReg.setReg(frame.tempRegForType(fe));

        
        frame.pinReg(feTypeReg.reg());
    }

    








    RegisterID reg = frame.copyDataIntoReg(masm, fe);
    Label feSyncTarget = stubcc.syncExitAndJump(Uses(1));

    
    MaybeJump jmpNotDbl;
    {
        maybeJumpIfNotDouble(masm, jmpNotDbl, fe, feTypeReg);

        FPRegisterID fpreg = frame.copyEntryIntoFPReg(fe, FPRegisters::First);

#if defined JS_CPU_X86 or defined JS_CPU_X64
        masm.loadDouble(&DoubleNegMask, FPRegisters::Second);
        masm.xorDouble(FPRegisters::Second, fpreg);
#elif defined JS_CPU_ARM
        masm.negDouble(fpreg);
#endif

        
        masm.storeDouble(fpreg, frame.addressOf(fe));
    }

    
    MaybeJump jmpNotInt;
    MaybeJump jmpIntZero;
    MaybeJump jmpIntRejoin;
    Label lblIntPath = stubcc.masm.label();
    {
        maybeJumpIfNotInt32(stubcc.masm, jmpNotInt, fe, feTypeReg);

        
        jmpIntZero.setJump(stubcc.masm.branch32(Assembler::Equal, reg, Imm32(0)));

        stubcc.masm.neg32(reg);

        
        stubcc.masm.storePayload(reg, frame.addressOf(fe));
        stubcc.masm.storeTypeTag(ImmType(JSVAL_TYPE_INT32), frame.addressOf(fe));

        jmpIntRejoin.setJump(stubcc.masm.jump());
    }

    frame.freeReg(reg);
    if (feTypeReg.isSet())
        frame.unpinReg(feTypeReg.reg());

    stubcc.leave();
    stubcc.call(stubs::Neg);

    frame.pop();
    frame.pushSynced();

    
    if (jmpNotDbl.isSet())
        stubcc.linkExitDirect(jmpNotDbl.getJump(), lblIntPath);

    if (jmpNotInt.isSet())
        jmpNotInt.getJump().linkTo(feSyncTarget, &stubcc.masm);
    if (jmpIntZero.isSet())
        jmpIntZero.getJump().linkTo(feSyncTarget, &stubcc.masm);
    if (jmpIntRejoin.isSet())
        stubcc.crossJump(jmpIntRejoin.getJump(), masm.label());

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_mod()
{
#if defined(JS_CPU_X86)
    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);
    if ((lhs->isTypeKnown() && lhs->getKnownType() != JSVAL_TYPE_INT32) ||
        (rhs->isTypeKnown() && rhs->getKnownType() != JSVAL_TYPE_INT32))
#endif
    {
        prepareStubCall(Uses(2));
        stubCall(stubs::Mod);
        frame.popn(2);
        frame.pushSynced();
        return;
    }

#if defined(JS_CPU_X86)
    if (!lhs->isTypeKnown()) {
        Jump j = frame.testInt32(Assembler::NotEqual, lhs);
        stubcc.linkExit(j, Uses(2));
    }
    if (!rhs->isTypeKnown()) {
        Jump j = frame.testInt32(Assembler::NotEqual, rhs);
        stubcc.linkExit(j, Uses(2));
    }

    
    if (!lhs->isConstant()) {
        frame.copyDataIntoReg(lhs, X86Registers::eax);
    } else {
        frame.takeReg(X86Registers::eax);
        masm.move(Imm32(lhs->getValue().toInt32()), X86Registers::eax);
    }

    
    MaybeRegisterID temp;
    RegisterID rhsReg;
    if (!rhs->isConstant()) {
        uint32 mask = Registers::AvailRegs & ~Registers::maskReg(X86Registers::edx);
        rhsReg = frame.tempRegInMaskForData(rhs, mask);
        JS_ASSERT(rhsReg != X86Registers::edx);
    } else {
        rhsReg = frame.allocReg(Registers::AvailRegs & ~Registers::maskReg(X86Registers::edx));
        JS_ASSERT(rhsReg != X86Registers::edx);
        masm.move(Imm32(rhs->getValue().toInt32()), rhsReg);
        temp = rhsReg;
    }
    frame.takeReg(X86Registers::edx);
    frame.freeReg(X86Registers::eax);

    if (temp.isSet())
        frame.freeReg(temp.reg());

    bool slowPath = !(lhs->isTypeKnown() && rhs->isTypeKnown());
    if (rhs->isConstant() && rhs->getValue().toInt32() != 0) {
        if (rhs->getValue().toInt32() == -1) {
            
            Jump checkDivExc = masm.branch32(Assembler::Equal, X86Registers::eax,
                                             Imm32(0x80000000));
            stubcc.linkExit(checkDivExc, Uses(2));
            slowPath = true;
        }
    } else {
        Jump checkDivExc = masm.branch32(Assembler::Equal, X86Registers::eax, Imm32(0x80000000));
        stubcc.linkExit(checkDivExc, Uses(2));
        Jump checkZero = masm.branchTest32(Assembler::Zero, rhsReg, rhsReg);
        stubcc.linkExit(checkZero, Uses(2));
        slowPath = true;
    }

    
    masm.idiv(rhsReg);

    
    RegisterID lhsData = frame.tempRegForData(lhs);
    Jump negZero1 = masm.branchTest32(Assembler::NonZero, X86Registers::edx);
    Jump negZero2 = masm.branchTest32(Assembler::Zero, lhsData, Imm32(0x80000000));

    
    masm.storeValue(DoubleValue(-0.0), frame.addressOf(lhs));

    

    Jump done = masm.jump();
    negZero1.linkTo(masm.label(), &masm);
    negZero2.linkTo(masm.label(), &masm);

    
    masm.storeTypeTag(ImmType(JSVAL_TYPE_INT32), frame.addressOf(lhs));

    done.linkTo(masm.label(), &masm);

    if (slowPath) {
        stubcc.leave();
        stubcc.call(stubs::Mod);
    }

    frame.popn(2);
    frame.pushNumber(X86Registers::edx);

    if (slowPath)
        stubcc.rejoin(Changes(1));
#endif
}

