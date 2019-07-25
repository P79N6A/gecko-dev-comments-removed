







































#include "jsbool.h"
#include "jslibmath.h"
#include "jsnum.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"
#include "methodjit/StubCalls.h"
#include "methodjit/FrameState-inl.h"

using namespace js;
using namespace js::mjit;

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
                dL = cx->runtime->negativeInfinityValue.asDouble();
            else
                dL = cx->runtime->positiveInfinityValue.asDouble();
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
mjit::Compiler::jsop_binary_intmath(JSOp op, RegisterID *returnReg, MaybeJump &jmpOverflow)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    



    bool swapped = false;
    if (lhs->isConstant()) {
        JS_ASSERT(!rhs->isConstant());
        swapped = true;
        FrameEntry *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    RegisterID reg = Registers::ReturnReg;
    reg = frame.copyDataIntoReg(lhs);
    if (swapped && op == JSOP_SUB) {
        masm.neg32(reg);
        op = JSOP_ADD;
    }

    Jump fail;
    switch(op) {
      case JSOP_ADD:
        if (rhs->isConstant()) {
            fail = masm.branchAdd32(Assembler::Overflow,
                                    Imm32(rhs->getValue().asInt32()), reg);
        } else if (frame.shouldAvoidDataRemat(rhs)) {
            fail = masm.branchAdd32(Assembler::Overflow,
                                    frame.addressOf(rhs), reg);
        } else {
            RegisterID rhsReg = frame.tempRegForData(rhs);
            fail = masm.branchAdd32(Assembler::Overflow,
                                    rhsReg, reg);
        }
        break;

      case JSOP_SUB:
        if (rhs->isConstant()) {
            fail = masm.branchSub32(Assembler::Overflow,
                                    Imm32(rhs->getValue().asInt32()), reg);
        } else if (frame.shouldAvoidDataRemat(rhs)) {
            fail = masm.branchSub32(Assembler::Overflow,
                                    frame.addressOf(rhs), reg);
        } else {
            RegisterID rhsReg = frame.tempRegForData(rhs);
            fail = masm.branchSub32(Assembler::Overflow,
                                    rhsReg, reg);
        }
        break;

#if !defined(JS_CPU_ARM)
      case JSOP_MUL:
        if (rhs->isConstant()) {
            RegisterID rhsReg = frame.tempRegForConstant(rhs);
            fail = masm.branchMul32(Assembler::Overflow,
                                    rhsReg, reg);
        } else if (frame.shouldAvoidDataRemat(rhs)) {
            fail = masm.branchMul32(Assembler::Overflow,
                                    frame.addressOf(rhs), reg);
        } else {
            RegisterID rhsReg = frame.tempRegForData(rhs);
            fail = masm.branchMul32(Assembler::Overflow,
                                    rhsReg, reg);
        }
        break;
#endif 

      default:
        JS_NOT_REACHED("unhandled int32 op.");
        break;
    }
    
    *returnReg = reg;
    jmpOverflow.setJump(fail);
}

void
mjit::Compiler::jsop_binary_dblmath(JSOp op, FPRegisterID rfp, FPRegisterID lfp)
{
    switch (op) {
      case JSOP_ADD:
        stubcc.masm.addDouble(rfp, lfp);
        break;
      case JSOP_SUB:
        stubcc.masm.subDouble(rfp, lfp);
        break;
      case JSOP_MUL:
        stubcc.masm.mulDouble(rfp, lfp);
        break;
      default:
        JS_NOT_REACHED("unhandled double op.");
        break;
    }
}







void
mjit::Compiler::slowLoadConstantDouble(Assembler &masm,
                                       FrameEntry *fe, FPRegisterID fpreg)
{
    double d;
    if (fe->getTypeTag() == JSVAL_TAG_INT32)
        d = (double)fe->getValue().asInt32();
    else
        d = fe->getValue().asDouble();

    int32_t *ip = (int32_t *)&d;
    masm.storeData32(Imm32(ip[0]), frame.addressOf(fe));
    masm.storeTypeTag(ImmTag(JSValueTag(ip[1])), frame.addressOf(fe));
    masm.loadDouble(frame.addressOf(fe), fpreg);
}

void
mjit::Compiler::maybeJumpIfNotInt32(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                                    MaybeRegisterID &mreg)
{
    if (!fe->isTypeKnown()) {
        if (mreg.isSet())
            mj.setJump(masm.testInt32(Assembler::NotEqual, mreg.getReg()));
        else
            mj.setJump(masm.testInt32(Assembler::NotEqual, frame.addressOf(fe)));
    } else if (fe->getTypeTag() != JSVAL_TAG_INT32) {
        mj.setJump(masm.jump());
    }
}

void
mjit::Compiler::maybeJumpIfNotDouble(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                                    MaybeRegisterID &mreg)
{
    if (!fe->isTypeKnown()) {
        if (mreg.isSet())
            mj.setJump(masm.testDouble(Assembler::NotEqual, mreg.getReg()));
        else
            mj.setJump(masm.testDouble(Assembler::NotEqual, frame.addressOf(fe)));
    } else if (fe->getTypeTag() >= JSVAL_TAG_CLEAR) {
        mj.setJump(masm.jump());
    }
}

void
mjit::Compiler::jsop_binary(JSOp op, VoidStub stub)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);
    
    FrameEntry *returnFe = lhs;

    if (JSOpBinaryTryConstantFold(cx, frame, op, lhs, rhs))
        return;

    



    if ((op == JSOP_DIV || op == JSOP_MOD) ||
        (lhs->isTypeKnown() && (lhs->getTypeTag() > JSVAL_UPPER_INCL_TAG_OF_NUMBER_SET)) ||
        (rhs->isTypeKnown() && (rhs->getTypeTag() > JSVAL_UPPER_INCL_TAG_OF_NUMBER_SET)) 
#if defined(JS_CPU_ARM)
        
        || op == JSOP_MUL
#endif 
    ) {
        bool isStringResult = (op == JSOP_ADD) &&
                              ((lhs->isTypeKnown() && lhs->getTypeTag() == JSVAL_TAG_STRING) ||
                               (rhs->isTypeKnown() && rhs->getTypeTag() == JSVAL_TAG_STRING));
        prepareStubCall();
        stubCall(stub, Uses(2), Defs(1));
        frame.popn(2);
        if (isStringResult)
            frame.pushSyncedType(JSVAL_TAG_STRING);
        else
            frame.pushSynced();
        return;
    }

    
    bool canDoIntMath = !((rhs->isTypeKnown() && rhs->getTypeTag() < JSVAL_TAG_CLEAR) ||
                          (lhs->isTypeKnown() && lhs->getTypeTag() < JSVAL_TAG_CLEAR));

    frame.syncAllRegs(Registers::AvailRegs);

    
















    MaybeRegisterID rhsTypeReg;
    bool rhsTypeRegNeedsLoad = false;
    if (!rhs->isTypeKnown() && !frame.shouldAvoidTypeRemat(rhs)) {
        rhsTypeRegNeedsLoad = !frame.peekTypeInRegister(rhs);
        rhsTypeReg.setReg(frame.predictRegForType(rhs));
    }
    Label rhsSyncTarget = stubcc.syncExitAndJump();
    
    MaybeRegisterID lhsTypeReg;
    bool lhsTypeRegNeedsLoad = false;
    if (!lhs->isTypeKnown() && !frame.shouldAvoidTypeRemat(lhs)) {
        lhsTypeRegNeedsLoad = !frame.peekTypeInRegister(lhs);
        lhsTypeReg.setReg(frame.predictRegForType(lhs));
    }
    Label lhsSyncTarget = rhsSyncTarget;
    if (!rhsTypeReg.isSet() || lhsTypeReg.isSet())
        lhsSyncTarget = stubcc.syncExitAndJump();


    
    RegisterID returnReg = Registers::ReturnReg;

    






    FPRegisterID rfp = FPRegisters::First;
    FPRegisterID lfp = FPRegisters::Second;

    




    MaybeJump jmpCvtPath2;
    MaybeJump jmpCvtPath2NotInt;
    Label lblCvtPath2 = stubcc.masm.label();
    {
        
        if (!lhs->isTypeKnown() || lhs->getTypeTag() >= JSVAL_TAG_CLEAR) {
            maybeJumpIfNotInt32(stubcc.masm, jmpCvtPath2NotInt, lhs, lhsTypeReg);

            if (!lhs->isConstant())
                frame.convertInt32ToDouble(stubcc.masm, lhs, lfp);
            else
                slowLoadConstantDouble(stubcc.masm, lhs, lfp);

            jmpCvtPath2.setJump(stubcc.masm.jump());
        }
    }
    
    



    MaybeJump jmpCvtPath3;
    MaybeJump jmpCvtPath3NotDbl;
    Label lblCvtPath3 = stubcc.masm.label();
    {
        
        if (!lhs->isTypeKnown() || lhs->getTypeTag() != JSVAL_TAG_INT32) {
            maybeJumpIfNotDouble(stubcc.masm, jmpCvtPath3NotDbl, lhs, lhsTypeReg);

            if (!rhs->isConstant())
                frame.convertInt32ToDouble(stubcc.masm, rhs, rfp);
            else
                slowLoadConstantDouble(stubcc.masm, rhs, rfp);

            frame.copyEntryIntoFPReg(stubcc.masm, lhs, lfp);
            jmpCvtPath3.setJump(stubcc.masm.jump());
        }
    }

    
    MaybeJump jmpRhsNotDbl;
    MaybeJump jmpLhsNotDbl;
    Jump jmpDblRejoin;
    
    Label lblDblRhsTest = stubcc.masm.label();
    Label lblDblDoMath;
    {
        

        
        maybeJumpIfNotDouble(stubcc.masm, jmpRhsNotDbl, rhs, rhsTypeReg);
        frame.copyEntryIntoFPReg(stubcc.masm, rhs, rfp);

        if (lhsTypeRegNeedsLoad)
            frame.emitLoadTypeTag(stubcc.masm, lhs, lhsTypeReg.getReg());
        maybeJumpIfNotDouble(stubcc.masm, jmpLhsNotDbl, lhs, lhsTypeReg);
        frame.copyEntryIntoFPReg(stubcc.masm, lhs, lfp);

        lblDblDoMath = stubcc.masm.label();
        jsop_binary_dblmath(op, rfp, lfp);

        



    }


    




    
    MaybeJump jmpRhsNotInt;
    MaybeJump jmpLhsNotInt;
    MaybeJump jmpOverflow;
    {
        
        if (rhsTypeRegNeedsLoad)
            frame.emitLoadTypeTag(rhs, rhsTypeReg.getReg());
        maybeJumpIfNotInt32(masm, jmpRhsNotInt, rhs, rhsTypeReg);

        if (lhsTypeRegNeedsLoad)
            frame.emitLoadTypeTag(lhs, lhsTypeReg.getReg());
        maybeJumpIfNotInt32(masm, jmpLhsNotInt, lhs, lhsTypeReg);

        



        if (canDoIntMath)
            jsop_binary_intmath(op, &returnReg, jmpOverflow);
    }
    

    
    {
        



        stubcc.masm.storeDouble(lfp, frame.addressOf(returnFe));
        if (canDoIntMath)
            stubcc.masm.loadData32(frame.addressOf(returnFe), returnReg);

        jmpDblRejoin = stubcc.masm.jump();
    }

    







    MaybeJump jmpCvtPath1;
    Label lblCvtPath1 = stubcc.masm.label();
    {
        if (canDoIntMath) {
            







            if (!lhs->isConstant())
                frame.convertInt32ToDouble(stubcc.masm, lhs, lfp);
            else
                slowLoadConstantDouble(stubcc.masm, lhs, lfp);

            if (!rhs->isConstant())
                frame.convertInt32ToDouble(stubcc.masm, rhs, rfp);
            else
                slowLoadConstantDouble(stubcc.masm, rhs, rfp);

            jmpCvtPath1.setJump(stubcc.masm.jump());
        }
    }


    
    if (jmpRhsNotInt.isSet())
        stubcc.linkExitDirect(jmpRhsNotInt.getJump(), lblDblRhsTest);
    if (jmpLhsNotInt.isSet())
        stubcc.linkExitDirect(jmpLhsNotInt.getJump(), lblCvtPath3);
    if (jmpOverflow.isSet())
        stubcc.linkExitDirect(jmpOverflow.getJump(), lblCvtPath1);

    if (jmpRhsNotDbl.isSet())
        jmpRhsNotDbl.getJump().linkTo(rhsSyncTarget, &stubcc.masm);
    if (jmpLhsNotDbl.isSet())
        jmpLhsNotDbl.getJump().linkTo(lblCvtPath2, &stubcc.masm);

    if (jmpCvtPath1.isSet())
        jmpCvtPath1.getJump().linkTo(lblDblDoMath, &stubcc.masm);
    if (jmpCvtPath2.isSet())
        jmpCvtPath2.getJump().linkTo(lblDblDoMath, &stubcc.masm);
    if (jmpCvtPath2NotInt.isSet())
        jmpCvtPath2NotInt.getJump().linkTo(lhsSyncTarget, &stubcc.masm);
    if (jmpCvtPath3.isSet())
        jmpCvtPath3.getJump().linkTo(lblDblDoMath, &stubcc.masm);
    if (jmpCvtPath3NotDbl.isSet())
        jmpCvtPath3NotDbl.getJump().linkTo(lhsSyncTarget, &stubcc.masm);
    
        
    
    stubcc.leave();
    stubcc.call(stub);

    frame.popn(2);
    if (canDoIntMath)
        frame.pushUntypedPayload(JSVAL_TAG_INT32, returnReg);
    else
        frame.pushSynced();

    stubcc.crossJump(jmpDblRejoin, masm.label());
    stubcc.rejoin(1);
}

