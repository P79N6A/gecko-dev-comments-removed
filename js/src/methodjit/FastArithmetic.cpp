







































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

bool
mjit::Compiler::tryBinaryConstantFold(JSContext *cx, FrameState &frame, JSOp op,
                                      FrameEntry *lhs, FrameEntry *rhs, Value *vp)
{
    if (!lhs->isConstant() || !rhs->isConstant())
        return false;

    const Value &L = lhs->getValue();
    const Value &R = rhs->getValue();

    if (!L.isPrimitive() || !R.isPrimitive() ||
        (op == JSOP_ADD && (L.isString() || R.isString()))) {
        return false;
    }

    bool needInt;
    switch (op) {
      case JSOP_ADD:
      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_DIV:
        needInt = false;
        break;

      case JSOP_MOD:
        needInt = (L.isInt32() && R.isInt32() &&
                   L.toInt32() >= 0 && R.toInt32() > 0);
        break;

      case JSOP_RSH:
        needInt = true;
        break;

      default:
        JS_NOT_REACHED("NYI");
        needInt = false; 
        break;
    }

    double dL = 0, dR = 0;
    int32_t nL = 0, nR = 0;
    



    if (needInt) {
        ValueToECMAInt32(cx, L, &nL);
        ValueToECMAInt32(cx, R, &nR);
    } else {
        ValueToNumber(cx, L, &dL);
        ValueToNumber(cx, R, &dR);
    }

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
        if (needInt)
            nL %= nR;
        else if (dR == 0)
            dL = js_NaN;
        else
            dL = js_fmod(dL, dR);
        break;

      case JSOP_RSH:
        nL >>= (nR & 31);
        break;

      default:
        JS_NOT_REACHED("NYI");
        break;
    }

    if (needInt)
        vp->setInt32(nL);
    else
        vp->setNumber(dL);

    return true;
}

void
mjit::Compiler::slowLoadConstantDouble(Assembler &masm,
                                       FrameEntry *fe, FPRegisterID fpreg)
{
    if (fe->getValue().isInt32())
        masm.slowLoadConstantDouble((double) fe->getValue().toInt32(), fpreg);
    else
        masm.slowLoadConstantDouble(fe->getValue().toDouble(), fpreg);
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

bool
mjit::Compiler::jsop_binary(JSOp op, VoidStub stub, JSValueType type)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    Value v;
    if (tryBinaryConstantFold(cx, frame, op, lhs, rhs, &v)) {
        if (type == JSVAL_TYPE_INT32 && !v.isInt32()) {
            
            JaegerSpew(JSpew_Abort, "overflow in binary (%u)\n", PC - script->code);
            return false;
        }
        frame.popn(2);
        frame.push(v);
        return true;
    }

    



    if ((lhs->isConstant() && rhs->isConstant()) ||
        (lhs->isTypeKnown() && (lhs->getKnownType() > JSVAL_UPPER_INCL_TYPE_OF_NUMBER_SET)) ||
        (rhs->isTypeKnown() && (rhs->getKnownType() > JSVAL_UPPER_INCL_TYPE_OF_NUMBER_SET)) 
#if defined(JS_CPU_ARM)
        
        || op == JSOP_MUL
#endif 
    ) {
        bool isStringResult = (op == JSOP_ADD) &&
                              (lhs->isType(JSVAL_TYPE_STRING) ||
                               rhs->isType(JSVAL_TYPE_STRING));
        JS_ASSERT_IF(isStringResult && type != JSVAL_TYPE_UNKNOWN, type == JSVAL_TYPE_STRING);

        prepareStubCall(Uses(2));
        INLINE_STUBCALL(stub);
        frame.popn(2);
        frame.pushSynced(isStringResult ? JSVAL_TYPE_STRING : type);
        return true;
    }

    
    bool canDoIntMath = op != JSOP_DIV && type != JSVAL_TYPE_DOUBLE &&
                        !((rhs->isTypeKnown() && rhs->getKnownType() == JSVAL_TYPE_DOUBLE) ||
                          (lhs->isTypeKnown() && lhs->getKnownType() == JSVAL_TYPE_DOUBLE));

    if (canDoIntMath)
        jsop_binary_full(lhs, rhs, op, stub, type);
    else
        jsop_binary_double(lhs, rhs, op, stub, type);

    return true;
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

mjit::MaybeJump
mjit::Compiler::loadDouble(FrameEntry *fe, FPRegisterID *fpReg, bool *allocated)
{
    MaybeJump notNumber;

    if (!fe->isConstant() && fe->isType(JSVAL_TYPE_DOUBLE)) {
        *fpReg = frame.tempFPRegForData(fe);
        *allocated = false;
        return notNumber;
    }

    *fpReg = frame.allocFPReg();
    *allocated = true;

    if (fe->isConstant()) {
        slowLoadConstantDouble(masm, fe, *fpReg);
    } else if (!fe->isTypeKnown()) {
        frame.tempRegForType(fe);
        Jump j = frame.testDouble(Assembler::Equal, fe);
        notNumber = frame.testInt32(Assembler::NotEqual, fe);
        frame.convertInt32ToDouble(masm, fe, *fpReg);
        Jump converted = masm.jump();
        j.linkTo(masm.label(), &masm);
        
        frame.loadDouble(fe, *fpReg, masm);
        converted.linkTo(masm.label(), &masm);
    } else {
        JS_ASSERT(fe->isType(JSVAL_TYPE_INT32));
        frame.tempRegForData(fe);
        frame.convertInt32ToDouble(masm, fe, *fpReg);
    }

    return notNumber;
}





void
mjit::Compiler::jsop_binary_double(FrameEntry *lhs, FrameEntry *rhs, JSOp op,
                                   VoidStub stub, JSValueType type)
{
    FPRegisterID fpLeft, fpRight;
    bool allocateLeft, allocateRight;

    MaybeJump lhsNotNumber = loadDouble(lhs, &fpLeft, &allocateLeft);
    if (lhsNotNumber.isSet())
        stubcc.linkExit(lhsNotNumber.get(), Uses(2));

    
    if (!allocateLeft) {
        FPRegisterID res = frame.allocFPReg();
        masm.moveDouble(fpLeft, res);
        fpLeft = res;
        allocateLeft = true;
    }

    MaybeJump rhsNotNumber;
    if (frame.haveSameBacking(lhs, rhs)) {
        fpRight = fpLeft;
        allocateRight = false;
    } else {
        rhsNotNumber = loadDouble(rhs, &fpRight, &allocateRight);
        if (rhsNotNumber.isSet())
            stubcc.linkExit(rhsNotNumber.get(), Uses(2));
    }

    EmitDoubleOp(op, fpRight, fpLeft, masm);
    
    MaybeJump done;

    



    if (op == JSOP_DIV &&
        (type == JSVAL_TYPE_INT32 ||
         (type == JSVAL_TYPE_UNKNOWN &&
          !(lhs->isConstant() && lhs->isType(JSVAL_TYPE_INT32) &&
            abs(lhs->getValue().toInt32()) == 1)))) {
        RegisterID reg = frame.allocReg();
        FPRegisterID fpReg = frame.allocFPReg();
        JumpList isDouble;
        masm.branchConvertDoubleToInt32(fpLeft, reg, isDouble, fpReg);
        
        masm.storeValueFromComponents(ImmType(JSVAL_TYPE_INT32), reg,
                                      frame.addressOf(lhs));
        
        frame.freeReg(reg);
        frame.freeReg(fpReg);
        done.setJump(masm.jump());

        isDouble.linkTo(masm.label(), &masm);
    }

    if (type == JSVAL_TYPE_INT32) {
        




        stubcc.linkExit(masm.jump(), Uses(2));
    } else if (type != JSVAL_TYPE_DOUBLE) {
        masm.storeDouble(fpLeft, frame.addressOf(lhs));
    }

    if (done.isSet())
        done.getJump().linkTo(masm.label(), &masm);

    stubcc.leave();
    OOL_STUBCALL(stub);

    if (allocateRight)
        frame.freeReg(fpRight);

    frame.popn(2);

    if (type == JSVAL_TYPE_DOUBLE) {
        frame.pushDouble(fpLeft);
    } else {
        frame.freeReg(fpLeft);
        frame.pushSynced(type);
    }

    stubcc.rejoin(Changes(1));
}




void
mjit::Compiler::jsop_binary_full_simple(FrameEntry *fe, JSOp op, VoidStub stub, JSValueType type)
{
    FrameEntry *lhs = frame.peek(-2);

    
    if (fe->isType(JSVAL_TYPE_DOUBLE)) {
        FPRegisterID fpreg = frame.allocFPReg();
        FPRegisterID lhs = frame.tempFPRegForData(fe);
        masm.moveDouble(lhs, fpreg);
        EmitDoubleOp(op, fpreg, fpreg, masm);
        frame.popn(2);

        JS_ASSERT(type == JSVAL_TYPE_DOUBLE);  
        frame.pushDouble(fpreg);
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
        frame.loadDouble(fe, regs.lhsFP, stubcc.masm);
        EmitDoubleOp(op, regs.lhsFP, regs.lhsFP, stubcc.masm);

        
        Address result = frame.addressOf(lhs);
        stubcc.masm.storeDouble(regs.lhsFP, result);

        
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

    



    stubcc.linkExitDirect(overflow.get(), stubcc.masm.label());
    frame.rematBinary(fe, NULL, regs, stubcc.masm);
    stubcc.syncExitAndJump(Uses(2));

    
    if (notNumber.isSet())
        notNumber.get().linkTo(stubcc.masm.label(), &stubcc.masm);

    
    frame.sync(stubcc.masm, Uses(2));
    stubcc.leave();
    OOL_STUBCALL(stub);

    
    frame.popn(2);

    if (type == JSVAL_TYPE_INT32)
        frame.pushTypedPayload(type, regs.result);
    else
        frame.pushNumber(regs.result, true);

    frame.freeReg(regs.lhsFP);

    
    if (doublePathDone.isSet())
        stubcc.linkRejoin(doublePathDone.get());

    stubcc.rejoin(Changes(1));
}





































void
mjit::Compiler::jsop_binary_full(FrameEntry *lhs, FrameEntry *rhs, JSOp op,
                                 VoidStub stub, JSValueType type)
{
    if (frame.haveSameBacking(lhs, rhs)) {
        jsop_binary_full_simple(lhs, op, stub, type);
        return;
    }

    
    FrameState::BinaryAlloc regs;
    frame.allocForBinary(lhs, rhs, op, regs);

    
    JS_ASSERT_IF(lhs->isTypeKnown(), lhs->getKnownType() == JSVAL_TYPE_INT32);
    JS_ASSERT_IF(rhs->isTypeKnown(), rhs->getKnownType() == JSVAL_TYPE_INT32);

    MaybeJump lhsNotDouble, rhsNotNumber, lhsUnknownDone;
    if (!lhs->isTypeKnown())
        emitLeftDoublePath(lhs, rhs, regs, lhsNotDouble, rhsNotNumber, lhsUnknownDone);

    MaybeJump rhsNotNumber2;
    if (!rhs->isTypeKnown())
        emitRightDoublePath(lhs, rhs, regs, rhsNotNumber2);

    
    MaybeJump doublePathDone;
    if (!rhs->isTypeKnown() || lhsUnknownDone.isSet()) {
        
        if (lhsUnknownDone.isSet())
            lhsUnknownDone.get().linkTo(stubcc.masm.label(), &stubcc.masm);
        
        
        EmitDoubleOp(op, regs.rhsFP, regs.lhsFP, stubcc.masm);

        
        Address result = frame.addressOf(lhs);
        stubcc.masm.storeDouble(regs.lhsFP, result);

        
        stubcc.masm.loadPayload(result, regs.result);

        
        doublePathDone = stubcc.masm.jump();
    }

    
    int32 value = 0;
    JSOp origOp = op;
    MaybeRegisterID reg;
    MaybeJump preOverflow;
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
            
            
            preOverflow = masm.branch32(Assembler::Equal, regs.result, Imm32(0x80000000));
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
      {
        JS_ASSERT(reg.isSet());
        
        MaybeJump storeNegZero;
        bool maybeNegZero = true;
        bool hasConstant = (lhs->isConstant() || rhs->isConstant());
        
        if (hasConstant) {
            value = (lhs->isConstant() ? lhs : rhs)->getValue().toInt32();
            RegisterID nonConstReg = lhs->isConstant() ? regs.rhsData.reg() : regs.lhsData.reg();

            if (value > 0)
                maybeNegZero = false;
            else if (value < 0)
                storeNegZero = masm.branchTest32(Assembler::Zero, nonConstReg);
            else
                storeNegZero = masm.branch32(Assembler::LessThan, nonConstReg, Imm32(0));
        }
        overflow = masm.branchMul32(Assembler::Overflow, reg.reg(), regs.result);

        if (maybeNegZero) {
            if (hasConstant) {
                stubcc.linkExit(storeNegZero.get(), Uses(2));
            } else {
                Jump isZero = masm.branchTest32(Assembler::Zero, regs.result);
                stubcc.linkExitDirect(isZero, stubcc.masm.label());

                
                if (regs.resultHasRhs) {
                    if (regs.rhsNeedsRemat)
                        stubcc.masm.loadPayload(frame.addressForDataRemat(rhs), regs.result);
                    else
                        stubcc.masm.move(regs.rhsData.reg(), regs.result);
                } else {
                    if (regs.lhsNeedsRemat)
                        stubcc.masm.loadPayload(frame.addressForDataRemat(lhs), regs.result);
                    else
                        stubcc.masm.move(regs.lhsData.reg(), regs.result);
                }
                storeNegZero = stubcc.masm.branchOr32(Assembler::Signed, reg.reg(), regs.result);
                stubcc.masm.xor32(regs.result, regs.result);
                stubcc.crossJump(stubcc.masm.jump(), masm.label());
                storeNegZero.getJump().linkTo(stubcc.masm.label(), &stubcc.masm);
                frame.rematBinary(lhs, rhs, regs, stubcc.masm);
            }
            stubcc.syncExitAndJump(Uses(2));
        }
        break;
      }
#endif

      default:
        JS_NOT_REACHED("unrecognized op");
    }
    op = origOp;
    
    JS_ASSERT(overflow.isSet());

    



    MaybeJump overflowDone;
    if (preOverflow.isSet())
        stubcc.linkExitDirect(preOverflow.get(), stubcc.masm.label());
    stubcc.linkExitDirect(overflow.get(), stubcc.masm.label());

    
    if (regs.undoResult) {
        if (reg.isSet()) {
            JS_ASSERT(op == JSOP_ADD);
            stubcc.masm.neg32(reg.reg());
            stubcc.masm.add32(reg.reg(), regs.result);
            stubcc.masm.neg32(reg.reg());
        } else {
            JS_ASSERT(op == JSOP_ADD || op == JSOP_SUB);
            int32 fixValue = (op == JSOP_ADD) ? -value : value;
            stubcc.masm.add32(Imm32(fixValue), regs.result);
        }
    }

    frame.rematBinary(lhs, rhs, regs, stubcc.masm);
    stubcc.syncExitAndJump(Uses(2));

    
    if (regs.extraFree.isSet())
        frame.freeReg(regs.extraFree.reg());

    
    if (lhsNotDouble.isSet()) {
        lhsNotDouble.get().linkTo(stubcc.masm.label(), &stubcc.masm);
        if (rhsNotNumber.isSet())
            rhsNotNumber.get().linkTo(stubcc.masm.label(), &stubcc.masm);
    }
    if (rhsNotNumber2.isSet())
        rhsNotNumber2.get().linkTo(stubcc.masm.label(), &stubcc.masm);

    
    frame.sync(stubcc.masm, Uses(2));
    stubcc.leave();
    OOL_STUBCALL(stub);

    
    frame.popn(2);

    




    if (regs.undoResult)
        frame.takeReg(regs.result);

    if (type == JSVAL_TYPE_INT32)
        frame.pushTypedPayload(type, regs.result);
    else
        frame.pushNumber(regs.result, true);

    frame.freeReg(regs.lhsFP);
    frame.freeReg(regs.rhsFP);

    
    if (doublePathDone.isSet())
        stubcc.linkRejoin(doublePathDone.get());

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_neg()
{
    FrameEntry *fe = frame.peek(-1);
    JSValueType type = knownPushedType(0);

    if (fe->isTypeKnown() && fe->getKnownType() > JSVAL_UPPER_INCL_TYPE_OF_NUMBER_SET) {
        prepareStubCall(Uses(1));
        INLINE_STUBCALL(stubs::Neg);
        frame.pop();
        frame.pushSynced(type);
        return;
    }

    JS_ASSERT(!fe->isConstant());

    
    if (fe->isType(JSVAL_TYPE_DOUBLE) ||
        (fe->isType(JSVAL_TYPE_INT32) && type == JSVAL_TYPE_DOUBLE)) {
        FPRegisterID fpreg;
        if (fe->isType(JSVAL_TYPE_DOUBLE)) {
            fpreg = frame.tempFPRegForData(fe);
        } else {
            fpreg = frame.allocFPReg();
            frame.convertInt32ToDouble(masm, fe, fpreg);
        }

        FPRegisterID res = frame.allocFPReg();
        masm.moveDouble(fpreg, res);
        masm.negateDouble(res);

        if (!fe->isType(JSVAL_TYPE_DOUBLE))
            frame.freeReg(fpreg);

        frame.pop();
        frame.pushDouble(res);

        if (recompiling) {
            OOL_STUBCALL(stubs::Neg);
            stubcc.rejoin(Changes(1));
        }

        return;
    }

    
    if (fe->isType(JSVAL_TYPE_INT32) && type == JSVAL_TYPE_INT32) {
        RegisterID reg = frame.copyDataIntoReg(fe);

        
        Jump zeroOrMinInt = masm.branchTest32(Assembler::Zero, reg, Imm32(0x7fffffff));
        stubcc.linkExit(zeroOrMinInt, Uses(1));

        masm.neg32(reg);

        stubcc.leave();
        OOL_STUBCALL(stubs::Neg);

        frame.pop();
        frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

        stubcc.rejoin(Changes(1));
        return;
    }

    
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

        FPRegisterID fpreg = frame.allocFPReg();
        frame.loadDouble(fe, fpreg, masm);
        masm.negateDouble(fpreg);

        
        masm.storeDouble(fpreg, frame.addressOf(fe));
        frame.freeReg(fpreg);
    }

    
    MaybeJump jmpNotInt;
    MaybeJump jmpMinIntOrIntZero;
    MaybeJump jmpIntRejoin;
    Label lblIntPath = stubcc.masm.label();
    {
        maybeJumpIfNotInt32(stubcc.masm, jmpNotInt, fe, feTypeReg);

        
        jmpMinIntOrIntZero = stubcc.masm.branchTest32(Assembler::Zero, reg, Imm32(0x7fffffff));

        stubcc.masm.neg32(reg);

        
        stubcc.masm.storeValueFromComponents(ImmType(JSVAL_TYPE_INT32), reg,
                                             frame.addressOf(fe));

        jmpIntRejoin.setJump(stubcc.masm.jump());
    }

    frame.freeReg(reg);
    if (feTypeReg.isSet())
        frame.unpinReg(feTypeReg.reg());

    stubcc.leave();
    OOL_STUBCALL(stubs::Neg);

    frame.pop();
    frame.pushSynced(type);

    
    if (jmpNotDbl.isSet())
        stubcc.linkExitDirect(jmpNotDbl.getJump(), lblIntPath);

    if (jmpNotInt.isSet())
        jmpNotInt.getJump().linkTo(feSyncTarget, &stubcc.masm);
    if (jmpMinIntOrIntZero.isSet())
        jmpMinIntOrIntZero.getJump().linkTo(feSyncTarget, &stubcc.masm);
    if (jmpIntRejoin.isSet())
        stubcc.crossJump(jmpIntRejoin.getJump(), masm.label());

    stubcc.rejoin(Changes(1));
}

bool
mjit::Compiler::jsop_mod()
{
#if defined(JS_CPU_X86)
    JSValueType type = knownPushedType(0);

    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);

    Value v;
    if (tryBinaryConstantFold(cx, frame, JSOP_MOD, lhs, rhs, &v)) {
        if (type == JSVAL_TYPE_INT32 && !v.isInt32()) {
            JaegerSpew(JSpew_Abort, "overflow in mod (%u)\n", PC - script->code);
            markPushedOverflow();
            return false;
        }
        frame.popn(2);
        frame.push(v);
        return true;
    }

    if ((lhs->isConstant() && rhs->isConstant()) ||
        (lhs->isTypeKnown() && lhs->getKnownType() != JSVAL_TYPE_INT32) ||
        (rhs->isTypeKnown() && rhs->getKnownType() != JSVAL_TYPE_INT32) ||
        (type != JSVAL_TYPE_INT32 && type != JSVAL_TYPE_UNKNOWN))
#endif
    {
        prepareStubCall(Uses(2));
        INLINE_STUBCALL(stubs::Mod);
        frame.popn(2);
        frame.pushSynced(knownPushedType(0));

        if (recompiling) {
            OOL_STUBCALL(stubs::NegZeroHelper);
            stubcc.rejoin(Changes(1));
        }
        return true;
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
    uint32 mask = Registers::AvailRegs & ~Registers::maskReg(X86Registers::edx);
    if (!rhs->isConstant()) {
        rhsReg = frame.tempRegInMaskForData(rhs, mask);
        JS_ASSERT(rhsReg != X86Registers::edx);
    } else {
        rhsReg = frame.allocReg(mask).reg();
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

    



    bool lhsMaybeNeg = true;
    bool lhsIsNeg = false;
    if (lhs->isConstant()) {
        
        JS_ASSERT(lhs->getValue().isInt32());
        lhsMaybeNeg = lhsIsNeg = (lhs->getValue().toInt32() < 0);
    }

    MaybeJump gotNegZero;
    MaybeJump done;
    if (lhsMaybeNeg) {
        MaybeRegisterID lhsData;
        if (!lhsIsNeg)
            lhsData = frame.tempRegForData(lhs);
        Jump negZero1 = masm.branchTest32(Assembler::NonZero, X86Registers::edx);
        MaybeJump negZero2;
        if (!lhsIsNeg)
            negZero2 = masm.branchTest32(Assembler::Zero, lhsData.reg(), Imm32(0x80000000));
        



        gotNegZero = masm.jump();

        

        done = masm.jump();
        negZero1.linkTo(masm.label(), &masm);
        if (negZero2.isSet())
            negZero2.getJump().linkTo(masm.label(), &masm);
    }

    
    masm.storeTypeTag(ImmType(JSVAL_TYPE_INT32), frame.addressOf(lhs));

    if (done.isSet())
        done.getJump().linkTo(masm.label(), &masm);

    if (slowPath) {
        stubcc.leave();
        OOL_STUBCALL(stubs::Mod);
    }

    frame.popn(2);

    if (type == JSVAL_TYPE_INT32)
        frame.pushTypedPayload(type, X86Registers::edx);
    else
        frame.pushNumber(X86Registers::edx);

    if (slowPath)
        stubcc.rejoin(Changes(1));

    if (gotNegZero.isSet()) {
        stubcc.linkExit(gotNegZero.getJump(), Uses(2));
        OOL_STUBCALL(stubs::NegZeroHelper);
        stubcc.rejoin(Changes(1));
    }
#endif

    return true;
}

bool
mjit::Compiler::jsop_equality_int_string(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    if (lhs->isConstant() ||
        (frame.shouldAvoidDataRemat(lhs) && !rhs->isConstant())) {
        FrameEntry *temp = rhs;
        rhs = lhs;
        lhs = temp;
    }

    bool lhsInt = lhs->isType(JSVAL_TYPE_INT32);
    bool rhsInt = rhs->isType(JSVAL_TYPE_INT32);

    
    bool flipCondition = (target && fused == JSOP_IFEQ);

    
    Assembler::Condition cond;
    switch (op) {
      case JSOP_EQ:
        cond = flipCondition ? Assembler::NotEqual : Assembler::Equal;
        break;
      case JSOP_NE:
        cond = flipCondition ? Assembler::Equal : Assembler::NotEqual;
        break;
      default:
        JS_NOT_REACHED("wat");
        return false;
    }

    if (target) {
        Value rval = UndefinedValue();  
        bool rhsConst = false;
        if (rhs->isConstant()) {
            rhsConst = true;
            rval = rhs->getValue();
        }

        ValueRemat lvr, rvr;
        frame.pinEntry(lhs, lvr);
        frame.pinEntry(rhs, rvr);

        



        fixDoubleTypes(Uses(2));
        frame.syncAndKill(Registers(Registers::AvailRegs), Uses(frame.frameSlots()), Uses(2));

        RegisterID tempReg = frame.allocReg();

        frame.pop();
        frame.pop();
        frame.discardFrame();

        JaegerSpew(JSpew_Insns, " ---- BEGIN STUB CALL CODE ---- \n");

        RESERVE_OOL_SPACE(stubcc.masm);

        
        Label stubEntry = stubcc.masm.label();

        
        frame.ensureValueSynced(stubcc.masm, lhs, lvr);
        frame.ensureValueSynced(stubcc.masm, rhs, rvr);

        bool needStub = true;
        
#ifdef JS_MONOIC
        EqualityGenInfo ic;

        ic.cond = cond;
        ic.tempReg = tempReg;
        ic.lvr = lvr;
        ic.rvr = rvr;
        ic.stubEntry = stubEntry;
        ic.stub = stub;

        bool useIC = !addTraceHints || target >= PC;

        
        if (useIC) {
            
            ic.addrLabel = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
            ic.stubCall = OOL_STUBCALL_LOCAL_SLOTS(ic::Equality,
                                                   frame.stackDepth() + script->nfixed + 2);
            needStub = false;
        }
#endif

        if (needStub)
            OOL_STUBCALL_LOCAL_SLOTS(stub, frame.stackDepth() + script->nfixed + 2);

        



        Assembler::Condition ncond = (fused == JSOP_IFEQ)
                                   ? Assembler::Zero
                                   : Assembler::NonZero;
        Jump stubBranch =
            stubcc.masm.branchTest32(ncond, Registers::ReturnReg, Registers::ReturnReg);
        Jump stubFallthrough = stubcc.masm.jump();

        JaegerSpew(JSpew_Insns, " ---- END STUB CALL CODE ---- \n");
        CHECK_OOL_SPACE();

        Jump fast;
        MaybeJump firstStubJump;

        if ((!lhs->isTypeKnown() || lhsInt) && (!rhs->isTypeKnown() || rhsInt)) {
            if (!lhsInt) {
                Jump lhsFail = masm.testInt32(Assembler::NotEqual, lvr.typeReg());
                stubcc.linkExitDirect(lhsFail, stubEntry);
                firstStubJump = lhsFail;
            }
            if (!rhsInt) {
                Jump rhsFail = masm.testInt32(Assembler::NotEqual, rvr.typeReg());
                stubcc.linkExitDirect(rhsFail, stubEntry);
                if (!firstStubJump.isSet())
                    firstStubJump = rhsFail;
            }

            if (rhsConst)
                fast = masm.branch32(cond, lvr.dataReg(), Imm32(rval.toInt32()));
            else
                fast = masm.branch32(cond, lvr.dataReg(), rvr.dataReg());
        } else {
            Jump j = masm.jump();
            stubcc.linkExitDirect(j, stubEntry);
            firstStubJump = j;

            
            fast = masm.jump();
        }

        
        stubcc.crossJump(stubFallthrough, masm.label());

        bool *ptrampoline = NULL;
#ifdef JS_MONOIC
        
        ic.trampoline = false;
        ic.trampolineStart = stubcc.masm.label();
        if (useIC)
            ptrampoline = &ic.trampoline;
#endif

        



        if (!jumpAndTrace(fast, target, &stubBranch, ptrampoline))
            return false;

#ifdef JS_MONOIC
        if (useIC) {
            ic.jumpToStub = firstStubJump;
            ic.fallThrough = masm.label();
            ic.jumpTarget = target;
            equalityICs.append(ic);
        }
#endif

    } else {
        

        
        JS_ASSERT(!lhs->isType(JSVAL_TYPE_STRING) && !rhs->isType(JSVAL_TYPE_STRING));

        
        if ((lhs->isTypeKnown() && !lhsInt) || (rhs->isTypeKnown() && !rhsInt)) {
            stubcc.linkExit(masm.jump(), Uses(2));
        } else {
            if (!lhsInt) {
                Jump lhsFail = frame.testInt32(Assembler::NotEqual, lhs);
                stubcc.linkExit(lhsFail, Uses(2));
            }
            if (!rhsInt) {
                Jump rhsFail = frame.testInt32(Assembler::NotEqual, rhs);
                stubcc.linkExit(rhsFail, Uses(2));
            }
        }

        stubcc.leave();
        OOL_STUBCALL(stub);

        RegisterID reg = frame.ownRegForData(lhs);

        
        RegisterID resultReg = reg;
        if (!(Registers::maskReg(reg) & Registers::SingleByteRegs))
            resultReg = frame.allocReg(Registers::SingleByteRegs).reg();

        
        if (rhs->isConstant()) {
            masm.set32(cond, reg, Imm32(rhs->getValue().toInt32()), resultReg);
        } else if (frame.shouldAvoidDataRemat(rhs)) {
            masm.set32(cond, reg,
                       masm.payloadOf(frame.addressOf(rhs)),
                       resultReg);
        } else {
            masm.set32(cond, reg, frame.tempRegForData(rhs), resultReg);
        }

        
        frame.pop();
        frame.pop();
        if (reg != resultReg)
            frame.freeReg(reg);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, resultReg);
        stubcc.rejoin(Changes(1));
    }
    return true;
}




void
mjit::Compiler::emitLeftDoublePath(FrameEntry *lhs, FrameEntry *rhs, FrameState::BinaryAlloc &regs,
                                   MaybeJump &lhsNotDouble, MaybeJump &rhsNotNumber,
                                   MaybeJump &lhsUnknownDone)
{
    
    Jump lhsNotInt32 = masm.testInt32(Assembler::NotEqual, regs.lhsType.reg());
    stubcc.linkExitDirect(lhsNotInt32, stubcc.masm.label());

    
    lhsNotDouble = stubcc.masm.testDouble(Assembler::NotEqual, regs.lhsType.reg());

    
    MaybeJump rhsIsDouble;
    if (!rhs->isTypeKnown()) {
        rhsIsDouble = stubcc.masm.testDouble(Assembler::Equal, regs.rhsType.reg());
        rhsNotNumber = stubcc.masm.testInt32(Assembler::NotEqual, regs.rhsType.reg());
    }

    
    if (rhs->isConstant())
        slowLoadConstantDouble(stubcc.masm, rhs, regs.rhsFP);
    else
        stubcc.masm.convertInt32ToDouble(regs.rhsData.reg(), regs.rhsFP);

    if (!rhs->isTypeKnown()) {
        
        Jump converted = stubcc.masm.jump();
        rhsIsDouble.get().linkTo(stubcc.masm.label(), &stubcc.masm);

        
        frame.loadDouble(regs.rhsType.reg(), regs.rhsData.reg(),
                         rhs, regs.rhsFP, stubcc.masm);

        converted.linkTo(stubcc.masm.label(), &stubcc.masm);
    }

    
    frame.loadDouble(regs.lhsType.reg(), regs.lhsData.reg(),
                     lhs, regs.lhsFP, stubcc.masm);
    lhsUnknownDone = stubcc.masm.jump();
}




void
mjit::Compiler::emitRightDoublePath(FrameEntry *lhs, FrameEntry *rhs, FrameState::BinaryAlloc &regs,
                                    MaybeJump &rhsNotNumber2)
{
    
    Jump notInt32 = masm.testInt32(Assembler::NotEqual, regs.rhsType.reg());
    stubcc.linkExitDirect(notInt32, stubcc.masm.label());

    
    rhsNotNumber2 = stubcc.masm.testDouble(Assembler::NotEqual, regs.rhsType.reg());

    
    if (lhs->isConstant())
        slowLoadConstantDouble(stubcc.masm, lhs, regs.lhsFP);
    else
        stubcc.masm.convertInt32ToDouble(regs.lhsData.reg(), regs.lhsFP);

    
    frame.loadDouble(regs.rhsType.reg(), regs.rhsData.reg(),
                     rhs, regs.rhsFP, stubcc.masm);
}

static inline Assembler::DoubleCondition
DoubleCondForOp(JSOp op, JSOp fused)
{
    bool ifeq = fused == JSOP_IFEQ;
    switch (op) {
      case JSOP_GT:
        return ifeq 
               ? Assembler::DoubleLessThanOrEqualOrUnordered
               : Assembler::DoubleGreaterThan;
      case JSOP_GE:
        return ifeq
               ? Assembler::DoubleLessThanOrUnordered
               : Assembler::DoubleGreaterThanOrEqual;
      case JSOP_LT:
        return ifeq
               ? Assembler::DoubleGreaterThanOrEqualOrUnordered
               : Assembler::DoubleLessThan;
      case JSOP_LE:
        return ifeq
               ? Assembler::DoubleGreaterThanOrUnordered
               : Assembler::DoubleLessThanOrEqual;
      default:
        JS_NOT_REACHED("unrecognized op");
        return Assembler::DoubleLessThan;
    }
}

bool
mjit::Compiler::jsop_relational_double(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    if (target)
        fixDoubleTypes(Uses(2));

    JS_ASSERT_IF(!target, fused != JSOP_IFEQ);

    FPRegisterID fpLeft, fpRight;
    bool allocateLeft, allocateRight;

    MaybeJump lhsNotNumber = loadDouble(lhs, &fpLeft, &allocateLeft);
    if (!allocateLeft)
        frame.pinReg(fpLeft);

    MaybeJump rhsNotNumber = loadDouble(rhs, &fpRight, &allocateRight);
    if (!allocateLeft)
        frame.unpinReg(fpLeft);

    Assembler::DoubleCondition dblCond = DoubleCondForOp(op, fused);

    if (target) {
        if (lhsNotNumber.isSet())
            stubcc.linkExitForBranch(lhsNotNumber.get());
        if (rhsNotNumber.isSet())
            stubcc.linkExitForBranch(rhsNotNumber.get());
        stubcc.leave();
        OOL_STUBCALL(stub);

        frame.syncAndForgetEverything();
        Jump j = masm.branchDouble(dblCond, fpLeft, fpRight);

        frame.popn(2);

        Assembler::Condition cond = (fused == JSOP_IFEQ)
                                    ? Assembler::Zero
                                    : Assembler::NonZero;
        Jump sj = stubcc.masm.branchTest32(cond, Registers::ReturnReg, Registers::ReturnReg);

        
        stubcc.rejoin(Changes(0));

        



        if (!jumpAndTrace(j, target, &sj))
            return false;
    } else {
        if (lhsNotNumber.isSet())
            stubcc.linkExit(lhsNotNumber.get(), Uses(2));
        if (rhsNotNumber.isSet())
            stubcc.linkExit(rhsNotNumber.get(), Uses(2));
        stubcc.leave();
        OOL_STUBCALL(stub);

        frame.popn(2);

        RegisterID reg = frame.allocReg();
        Jump j = masm.branchDouble(dblCond, fpLeft, fpRight);
        masm.move(Imm32(0), reg);
        Jump skip = masm.jump();
        j.linkTo(masm.label(), &masm);
        masm.move(Imm32(1), reg);
        skip.linkTo(masm.label(), &masm);

        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, reg);

        stubcc.rejoin(Changes(1));

        if (allocateLeft)
            frame.freeReg(fpLeft);
        if (allocateRight)
            frame.freeReg(fpRight);
    }

    return true;
}

static inline JSOp
ReverseCompareOp(JSOp op)
{
    switch (op) {
      case JSOP_GT:
        return JSOP_LT;
      case JSOP_GE:
        return JSOP_LE;
      case JSOP_LT:
        return JSOP_GT;
      case JSOP_LE:
        return JSOP_GE;
      default:
        JS_NOT_REACHED("unrecognized op");
        return op;
    }
}

bool
mjit::Compiler::jsop_relational_int(JSOp op, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    if (lhs->isConstant()) {
        JS_ASSERT(!rhs->isConstant());
        FrameEntry *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
        op = ReverseCompareOp(op);
    }

    JS_ASSERT_IF(!target, fused != JSOP_IFEQ);
    Assembler::Condition cond = GetCompareCondition(op, fused);

    if (target) {
        fixDoubleTypes(Uses(2));
        if (!frame.syncForBranch(target, Uses(2)))
            return false;

        Jump fast;
        if (rhs->isConstant())
            fast = masm.branch32(cond, frame.tempRegForData(lhs), Imm32(rhs->getValue().toInt32()));
        else
            fast = masm.branch32(cond, frame.tempRegForData(lhs), frame.tempRegForData(rhs));

        frame.popn(2);
        return jumpAndTrace(fast, target);
    } else {
        RegisterID lreg = frame.tempRegForData(lhs);
        RegisterID result = frame.allocReg();

        if (rhs->isConstant()) {
            masm.branchValue(cond, lreg, rhs->getValue().toInt32(), result);
        } else {
            RegisterID rreg = frame.tempRegForData(rhs);
            masm.branchValue(cond, lreg, rreg, result);
        }

        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
    }

    return true;
}

bool
mjit::Compiler::jsop_relational_self(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused)
{
#ifdef DEBUG
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    JS_ASSERT(frame.haveSameBacking(lhs, rhs));
#endif

    
    return emitStubCmpOp(stub, target, fused);
}


bool
mjit::Compiler::jsop_relational_full(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    if (target)
        fixDoubleTypes(Uses(2));

    
    FrameState::BinaryAlloc regs;
    frame.allocForBinary(lhs, rhs, op, regs, !target);

    MaybeJump lhsNotDouble, rhsNotNumber, lhsUnknownDone;
    if (!lhs->isTypeKnown())
        emitLeftDoublePath(lhs, rhs, regs, lhsNotDouble, rhsNotNumber, lhsUnknownDone);

    MaybeJump rhsNotNumber2;
    if (!rhs->isTypeKnown())
        emitRightDoublePath(lhs, rhs, regs, rhsNotNumber2);

    
    bool hasDoublePath = false;
    if (!rhs->isTypeKnown() || lhsUnknownDone.isSet())
        hasDoublePath = true;

    
    JSOp cmpOp = op;
    int32 value = 0;
    RegisterID cmpReg;
    MaybeRegisterID reg;
    if (regs.lhsData.isSet()) {
        cmpReg = regs.lhsData.reg();
        if (!regs.rhsData.isSet())
            value = rhs->getValue().toInt32();
        else
            reg = regs.rhsData.reg();
    } else {
        cmpReg = regs.rhsData.reg();
        value = lhs->getValue().toInt32();
        cmpOp = ReverseCompareOp(op);
    }

    





    if (target) {
        





        MaybeJump doubleTest, doubleFall;
        Assembler::DoubleCondition dblCond = DoubleCondForOp(op, fused);
        if (hasDoublePath) {
            if (lhsUnknownDone.isSet())
                lhsUnknownDone.get().linkTo(stubcc.masm.label(), &stubcc.masm);
            frame.sync(stubcc.masm, Uses(frame.frameSlots()));
            doubleTest = stubcc.masm.branchDouble(dblCond, regs.lhsFP, regs.rhsFP);
            doubleFall = stubcc.masm.jump();

            
            if (lhsNotDouble.isSet()) {
                lhsNotDouble.get().linkTo(stubcc.masm.label(), &stubcc.masm);
                if (rhsNotNumber.isSet())
                    rhsNotNumber.get().linkTo(stubcc.masm.label(), &stubcc.masm);
            }
            if (rhsNotNumber2.isSet())
                rhsNotNumber2.get().linkTo(stubcc.masm.label(), &stubcc.masm);

            




            frame.sync(stubcc.masm, Uses(frame.frameSlots()));
            stubcc.leave();
            OOL_STUBCALL(stub);
        }

        
        frame.pinReg(cmpReg);
        if (reg.isSet())
            frame.pinReg(reg.reg());
        
        frame.popn(2);

        frame.syncAndKillEverything();
        frame.unpinKilledReg(cmpReg);
        if (reg.isSet())
            frame.unpinKilledReg(reg.reg());
        frame.syncAndForgetEverything();
        
        
        Assembler::Condition i32Cond = GetCompareCondition(cmpOp, fused);

        
        Jump fast;
        if (reg.isSet())
            fast = masm.branch32(i32Cond, cmpReg, reg.reg());
        else
            fast = masm.branch32(i32Cond, cmpReg, Imm32(value));

        



        Assembler::Condition cond = (fused == JSOP_IFEQ)
                                    ? Assembler::Zero
                                    : Assembler::NonZero;
        Jump j = stubcc.masm.branchTest32(cond, Registers::ReturnReg, Registers::ReturnReg);

        
        Jump j2 = stubcc.masm.jump();
        stubcc.crossJump(j2, masm.label());

        
        if (hasDoublePath) {
            j.linkTo(stubcc.masm.label(), &stubcc.masm);
            doubleTest.get().linkTo(stubcc.masm.label(), &stubcc.masm);
            j = stubcc.masm.jump();
        }

        



        if (!jumpAndTrace(fast, target, &j))
            return false;

        
        if (hasDoublePath)
            stubcc.crossJump(doubleFall.get(), masm.label());
    } else {
        



        MaybeJump doubleDone;
        Assembler::DoubleCondition dblCond = DoubleCondForOp(op, JSOP_NOP);
        if (hasDoublePath) {
            if (lhsUnknownDone.isSet())
                lhsUnknownDone.get().linkTo(stubcc.masm.label(), &stubcc.masm);
            
            Jump test = stubcc.masm.branchDouble(dblCond, regs.lhsFP, regs.rhsFP);
            stubcc.masm.move(Imm32(0), regs.result);
            Jump skip = stubcc.masm.jump();
            test.linkTo(stubcc.masm.label(), &stubcc.masm);
            stubcc.masm.move(Imm32(1), regs.result);
            skip.linkTo(stubcc.masm.label(), &stubcc.masm);
            doubleDone = stubcc.masm.jump();

            
            if (lhsNotDouble.isSet()) {
                lhsNotDouble.get().linkTo(stubcc.masm.label(), &stubcc.masm);
                if (rhsNotNumber.isSet())
                    rhsNotNumber.get().linkTo(stubcc.masm.label(), &stubcc.masm);
            }
            if (rhsNotNumber2.isSet())
                rhsNotNumber2.get().linkTo(stubcc.masm.label(), &stubcc.masm);

            
            frame.sync(stubcc.masm, Uses(2));
            stubcc.leave();
            OOL_STUBCALL(stub);
        }

        
        Assembler::Condition i32Cond = GetCompareCondition(cmpOp, fused);

        
        if (reg.isSet())
            masm.branchValue(i32Cond, cmpReg, reg.reg(), regs.result);
        else
            masm.branchValue(i32Cond, cmpReg, value, regs.result);

        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, regs.result);

        if (hasDoublePath)
            stubcc.crossJump(doubleDone.get(), masm.label());
        stubcc.rejoin(Changes(1));

        frame.freeReg(regs.lhsFP);
        frame.freeReg(regs.rhsFP);
    }

    return true;
}

