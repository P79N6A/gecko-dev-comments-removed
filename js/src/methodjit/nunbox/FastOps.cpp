






































#include "jsbool.h"
#include "jslibmath.h"
#include "jsnum.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"
#include "methodjit/StubCalls.h"
#include "methodjit/FrameState-inl.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;

void
mjit::Compiler::jsop_bindname(uint32 index)
{
    RegisterID reg = frame.allocReg();
    Address scopeChain(JSFrameReg, offsetof(JSStackFrame, scopeChain));
    masm.loadData32(scopeChain, reg);

    Address address(reg, offsetof(JSObject, fslots) + JSSLOT_PARENT * sizeof(jsval));

    Jump j = masm.branch32(Assembler::NotEqual, masm.payloadOf(address), Imm32(0));

    stubcc.linkExit(j);
    stubcc.leave();
    stubcc.call(stubs::BindName);

    frame.pushTypedPayload(JSVAL_MASK32_NONFUNOBJ, reg);

    stubcc.rejoin(1);
}

void
mjit::Compiler::jsop_bitnot()
{
    FrameEntry *top = frame.peek(-1);

    
    if (top->isTypeKnown() && top->getTypeTag() != JSVAL_MASK32_INT32) {
        prepareStubCall();
        stubCall(stubs::BitNot, Uses(1), Defs(1));
        frame.pop();
        frame.pushSyncedType(JSVAL_MASK32_INT32);
        return;
    }
           
    
    bool stubNeeded = false;
    if (!top->isTypeKnown()) {
        Jump intFail = frame.testInt32(Assembler::NotEqual, top);
        stubcc.linkExit(intFail);
        frame.learnType(top, JSVAL_MASK32_INT32);
        stubNeeded = true;
    }

    if (stubNeeded) {
        stubcc.leave();
        stubcc.call(stubs::BitNot);
    }

    RegisterID reg = frame.ownRegForData(top);
    masm.not32(reg);
    frame.pop();
    frame.pushTypedPayload(JSVAL_MASK32_INT32, reg);

    if (stubNeeded)
        stubcc.rejoin(1);
}

void
mjit::Compiler::jsop_bitop(JSOp op)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    VoidStub stub;
    switch (op) {
      case JSOP_BITOR:
        stub = stubs::BitOr;
        break;
      case JSOP_BITAND:
        stub = stubs::BitAnd;
        break;
      case JSOP_BITXOR:
        stub = stubs::BitXor;
        break;
      case JSOP_LSH:
        stub = stubs::Lsh;
        break;
      case JSOP_RSH:
        stub = stubs::Rsh;
        break;
      default:
        JS_NOT_REACHED("wat");
        return;
    }

    
    if ((rhs->isTypeKnown() && rhs->getTypeTag() != JSVAL_MASK32_INT32) ||
        (lhs->isTypeKnown() && lhs->getTypeTag() != JSVAL_MASK32_INT32)) {
        prepareStubCall();
        stubCall(stub, Uses(2), Defs(1));
        frame.popn(2);
        frame.pushSyncedType(JSVAL_MASK32_INT32);
        return;
    }
           
    
    bool stubNeeded = false;
    if (!rhs->isTypeKnown()) {
        Jump rhsFail = frame.testInt32(Assembler::NotEqual, rhs);
        stubcc.linkExit(rhsFail);
        frame.learnType(rhs, JSVAL_MASK32_INT32);
        stubNeeded = true;
    }
    if (!lhs->isTypeKnown()) {
        Jump lhsFail = frame.testInt32(Assembler::NotEqual, lhs);
        stubcc.linkExit(lhsFail);
        stubNeeded = true;
    }

    if (stubNeeded) {
        stubcc.leave();
        stubcc.call(stub);
    }

    if (lhs->isConstant() && rhs->isConstant()) {
        int32 L = lhs->getValue().asInt32();
        int32 R = rhs->getValue().asInt32();

        frame.popn(2);
        switch (op) {
          case JSOP_BITOR:
            frame.push(Int32Tag(L | R));
            return;
          case JSOP_BITXOR:
            frame.push(Int32Tag(L ^ R));
            return;
          case JSOP_BITAND:
            frame.push(Int32Tag(L & R));
            return;
          case JSOP_LSH:
            frame.push(Int32Tag(L << R));
            return;
          case JSOP_RSH:
            frame.push(Int32Tag(L >> R));
            return;
          default:
            JS_NOT_REACHED("say wat");
        }
    }

    RegisterID reg;

    switch (op) {
      case JSOP_BITOR:
      case JSOP_BITXOR:
      case JSOP_BITAND:
      {
        
        if (lhs->isConstant()) {
            JS_ASSERT(!rhs->isConstant());
            FrameEntry *temp = rhs;
            rhs = lhs;
            lhs = temp;
        }

        reg = frame.ownRegForData(lhs);
        if (rhs->isConstant()) {
            if (op == JSOP_BITAND)
                masm.and32(Imm32(rhs->getValue().asInt32()), reg);
            else if (op == JSOP_BITXOR)
                masm.xor32(Imm32(rhs->getValue().asInt32()), reg);
            else
                masm.or32(Imm32(rhs->getValue().asInt32()), reg);
        } else if (frame.shouldAvoidDataRemat(rhs)) {
            if (op == JSOP_BITAND)
                masm.and32(masm.payloadOf(frame.addressOf(rhs)), reg);
            else if (op == JSOP_BITXOR)
                masm.xor32(masm.payloadOf(frame.addressOf(rhs)), reg);
            else
                masm.or32(masm.payloadOf(frame.addressOf(rhs)), reg);
        } else {
            RegisterID rhsReg = frame.tempRegForData(rhs);
            if (op == JSOP_BITAND)
                masm.and32(rhsReg, reg);
            else if (op == JSOP_BITXOR)
                masm.xor32(rhsReg, reg);
            else
                masm.or32(rhsReg, reg);
        }

        break;
      }

      case JSOP_LSH:
      case JSOP_RSH:
      {
        
        if (rhs->isConstant()) {
            int32 shift = rhs->getValue().asInt32() & 0x1F;

            reg = frame.ownRegForData(lhs);

            if (!shift) {
                



                frame.popn(2);
                frame.pushTypedPayload(JSVAL_MASK32_INT32, reg);
                if (stubNeeded)
                    stubcc.rejoin(1);
                return;
            }

            switch (op) {
              case JSOP_LSH:
                masm.lshift32(Imm32(shift), reg);
                break;
              case JSOP_RSH:
                masm.rshift32(Imm32(shift), reg);
                break;
              default:
                JS_NOT_REACHED("NYI");
            }
        } else {
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
            
            RegisterID rr = frame.tempRegForData(rhs, JSC::X86Registers::ecx);
#else
            RegisterID rr = frame.tempRegForData(rhs);
#endif

            frame.pinReg(rr);
            if (lhs->isConstant()) {
                reg = frame.allocReg();
                masm.move(Imm32(lhs->getValue().asInt32()), reg);
            } else {
                reg = frame.ownRegForData(lhs);
            }
            frame.unpinReg(rr);

            switch (op) {
              case JSOP_LSH:
                masm.lshift32(rr, reg);
                break;
              case JSOP_RSH:
                masm.rshift32(rr, reg);
                break;
              default:
                JS_NOT_REACHED("NYI");
            }
        }
        break;
      }

      default:
        JS_NOT_REACHED("NYI");
        return;
    }

    frame.pop();
    frame.pop();
    frame.pushTypedPayload(JSVAL_MASK32_INT32, reg);

    if (stubNeeded)
        stubcc.rejoin(2);
}

void
mjit::Compiler::jsop_globalinc(JSOp op, uint32 index)
{
    uint32 slot = script->getGlobalSlot(index);

    bool popped = false;
    PC += JSOP_GLOBALINC_LENGTH;
    if (JSOp(*PC) == JSOP_POP && !analysis[PC].nincoming) {
        popped = true;
        PC += JSOP_POP_LENGTH;
    }

    int amt = (js_CodeSpec[op].format & JOF_INC) ? 1 : -1;
    bool post = !!(js_CodeSpec[op].format & JOF_POST);

    RegisterID data;
    RegisterID reg = frame.allocReg();
    Address addr = masm.objSlotRef(globalObj, reg, slot);
    uint32 depth = frame.stackDepth();

    if (post && !popped) {
        frame.push(addr);
        FrameEntry *fe = frame.peek(-1);
        Jump notInt = frame.testInt32(Assembler::NotEqual, fe);
        stubcc.linkExit(notInt);
        data = frame.copyData(fe);
    } else {
        Jump notInt = masm.testInt32(Assembler::NotEqual, addr);
        stubcc.linkExit(notInt);
        data = frame.allocReg();
        masm.loadData32(addr, data);
    }

    Jump ovf;
    if (amt > 0)
        ovf = masm.branchAdd32(Assembler::Overflow, Imm32(1), data);
    else
        ovf = masm.branchSub32(Assembler::Overflow, Imm32(1), data);
    stubcc.linkExit(ovf);

    stubcc.leave();
    stubcc.masm.lea(addr, Registers::ArgReg1);
    stubcc.vpInc(op, depth);

    masm.storeData32(data, addr);

    if (!post && !popped)
        frame.pushUntypedPayload(JSVAL_MASK32_INT32, data);
    else
        frame.freeReg(data);

    frame.freeReg(reg);

    stubcc.rejoin(1);
}

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
mjit::Compiler::jsop_binary(JSOp op, VoidStub stub)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    if (JSOpBinaryTryConstantFold(cx, frame, op, lhs, rhs))
        return;

    



    if ((op == JSOP_DIV || op == JSOP_MOD)
#if defined(JS_CPU_ARM)
    
    || op == JSOP_MUL
#endif 
    ) {
        prepareStubCall();
        stubCall(stub, Uses(2), Defs(1));
        frame.popn(2);
        frame.pushSynced();
        return;
    }

    
    RegisterID reg = Registers::ReturnReg;
    if ((!lhs->isTypeKnown() || lhs->getTypeTag() == JSVAL_MASK32_INT32) &&
        (!rhs->isTypeKnown() || rhs->getTypeTag() == JSVAL_MASK32_INT32)) {
        if (!rhs->isTypeKnown()) {
            Jump rhsFail = frame.testInt32(Assembler::NotEqual, rhs);
            stubcc.linkExit(rhsFail);
        }
        if (!lhs->isTypeKnown()) {
            Jump lhsFail = frame.testInt32(Assembler::NotEqual, lhs);
            stubcc.linkExit(lhsFail);
        }

        



        bool swapped = false;
        if (lhs->isConstant()) {
            JS_ASSERT(!rhs->isConstant());
            swapped = true;
            FrameEntry *tmp = lhs;
            lhs = rhs;
            rhs = tmp;
        }

        reg = frame.copyData(lhs);
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
            JS_NOT_REACHED("kittens");
            break;
        }
        stubcc.linkExit(fail);
    } else {
        
        
        prepareStubCall();
        stubCall(stub, Uses(2), Defs(1));
        frame.popn(2);
        frame.pushSynced();
        return;
    }

    stubcc.leave();
    stubcc.call(stub);

    frame.popn(2);
    frame.pushUntypedPayload(JSVAL_MASK32_INT32, reg);

    stubcc.rejoin(1);
}

void
mjit::Compiler::jsop_relational(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    JS_ASSERT(!(rhs->isConstant() && lhs->isConstant()));

    
    if ((rhs->isTypeKnown() && rhs->getTypeTag() != JSVAL_MASK32_INT32) ||
        (lhs->isTypeKnown() && lhs->getTypeTag() != JSVAL_MASK32_INT32)) {
        emitStubCmpOp(stub, target, fused);
        return;
    }

    
    if (!rhs->isTypeKnown()) {
        Jump rhsFail = frame.testInt32(Assembler::NotEqual, rhs);
        stubcc.linkExit(rhsFail);
        frame.learnType(rhs, JSVAL_MASK32_INT32);
    }
    if (!lhs->isTypeKnown()) {
        Jump lhsFail = frame.testInt32(Assembler::NotEqual, lhs);
        stubcc.linkExit(lhsFail);
    }

    Assembler::Condition cond;
    switch (op) {
      case JSOP_LT:
        cond = Assembler::LessThan;
        break;
      case JSOP_LE:
        cond = Assembler::LessThanOrEqual;
        break;
      case JSOP_GT:
        cond = Assembler::GreaterThan;
        break;
      case JSOP_GE:
        cond = Assembler::GreaterThanOrEqual;
        break;
      case JSOP_EQ:
        cond = Assembler::Equal;
        break;
      case JSOP_NE:
        cond = Assembler::NotEqual;
        break;
      default:
        JS_NOT_REACHED("wat");
        return;
    }

    
    bool swapped = false;
    if (lhs->isConstant() ||
        (frame.shouldAvoidDataRemat(lhs) && !rhs->isConstant())) {
        FrameEntry *temp = rhs;
        rhs = lhs;
        lhs = temp;
        swapped = true;

        switch (cond) {
          case Assembler::LessThan:
            cond = Assembler::GreaterThan;
            break;
          case Assembler::LessThanOrEqual:
            cond = Assembler::GreaterThanOrEqual;
            break;
          case Assembler::GreaterThan:
            cond = Assembler::LessThan;
            break;
          case Assembler::GreaterThanOrEqual:
            cond = Assembler::LessThanOrEqual;
            break;
          case Assembler::Equal: 
          case Assembler::NotEqual:
            
            break;
          default:
            JS_NOT_REACHED("wat");
            break;
        }
    }

    stubcc.leave();
    stubcc.call(stub);

    if (target) {
        
        RegisterID lr = frame.ownRegForData(lhs);
        
        
        bool rhsConst;
        int32 rval = 0;
        RegisterID rr = Registers::ReturnReg;
        if (!(rhsConst = rhs->isConstant()))
            rr = frame.ownRegForData(rhs);
        else
            rval = rhs->getValue().asInt32();

        frame.pop();
        frame.pop();

        



        frame.forgetEverything();

        
        if (fused == JSOP_IFEQ) {
            switch (cond) {
              case Assembler::LessThan:
                cond = Assembler::GreaterThanOrEqual;
                break;
              case Assembler::LessThanOrEqual:
                cond = Assembler::GreaterThan;
                break;
              case Assembler::GreaterThan:
                cond = Assembler::LessThanOrEqual;
                break;
              case Assembler::GreaterThanOrEqual:
                cond = Assembler::LessThan;
                break;
              case Assembler::Equal:
                cond = Assembler::NotEqual;
                break;
              case Assembler::NotEqual:
                cond = Assembler::Equal;
                break;
              default:
                JS_NOT_REACHED("hello");
            }
        }

        Jump j;
        if (!rhsConst)
            j = masm.branch32(cond, lr, rr);
        else
            j = masm.branch32(cond, lr, Imm32(rval));

        jumpInScript(j, target);

        JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW RESTORE CODE ---- \n");
        



        Assembler::Condition cond = (fused == JSOP_IFEQ)
                                    ? Assembler::Zero
                                    : Assembler::NonZero;
        j = stubcc.masm.branchTest32(cond, Registers::ReturnReg, Registers::ReturnReg);
        stubcc.jumpInScript(j, target);

        
        j = stubcc.masm.jump();
        stubcc.crossJump(j, masm.label());
        JaegerSpew(JSpew_Insns, " ---- END SLOW RESTORE CODE ---- \n");
    } else {
        

        RegisterID reg = frame.ownRegForData(lhs);

        
        RegisterID resultReg = reg;
        if (!(Registers::maskReg(reg) & Registers::SingleByteRegs))
            resultReg = frame.allocReg(Registers::SingleByteRegs);

        
        if (rhs->isConstant()) {
            masm.set32(cond, reg, Imm32(rhs->getValue().asInt32()), resultReg);
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
        frame.pushTypedPayload(JSVAL_MASK32_BOOLEAN, resultReg);
        stubcc.rejoin(1);
    }
}

void
mjit::Compiler::jsop_neg()
{
    prepareStubCall();
    stubCall(stubs::Neg, Uses(1), Defs(1));
    frame.pop();
    frame.pushSynced();
}

void
mjit::Compiler::jsop_objtostr()
{
    prepareStubCall();
    stubCall(stubs::ObjToStr, Uses(1), Defs(1));
    frame.pop();
    frame.pushSynced();
}

void
mjit::Compiler::jsop_not()
{
    FrameEntry *top = frame.peek(-1);

    if (top->isConstant()) {
        const Value &v = top->getValue();
        frame.pop();
        frame.push(BooleanTag(!js_ValueToBoolean(v)));
        return;
    }

    if (top->isTypeKnown()) {
        uint32 mask = top->getTypeTag();
        switch (mask) {
          case JSVAL_MASK32_INT32:
          case JSVAL_MASK32_BOOLEAN:
          {
            
            
            RegisterID reg = frame.ownRegForData(top);
            Jump t = masm.branchTest32(Assembler::NotEqual, reg, reg);
            masm.move(Imm32(1), reg);
            Jump d = masm.jump();
            t.linkTo(masm.label(), &masm);
            masm.move(Imm32(0), reg);
            d.linkTo(masm.label(), &masm);
            frame.pop();
            frame.pushTypedPayload(JSVAL_MASK32_BOOLEAN, reg);
            break;
          }

          case JSVAL_MASK32_NONFUNOBJ:
          case JSVAL_MASK32_FUNOBJ:
          {
            frame.pop();
            frame.push(BooleanTag(false));
            break;
          }

          default:
          {
            
            RegisterID reg = Registers::ReturnReg;
            prepareStubCall();
            stubCall(stubs::ValueToBoolean, Uses(0), Defs(0));
            frame.takeReg(reg);
            Jump t = masm.branchTest32(Assembler::NotEqual, reg, reg);
            masm.move(Imm32(1), reg);
            Jump d = masm.jump();
            t.linkTo(masm.label(), &masm);
            masm.move(Imm32(0), reg);
            d.linkTo(masm.label(), &masm);
            frame.pop();
            frame.pushTypedPayload(JSVAL_MASK32_BOOLEAN, reg);
            break;
          }
        }

        return;
    }

    
    Jump boolFail = frame.testBoolean(Assembler::NotEqual, top);
    stubcc.linkExit(boolFail);
    frame.learnType(top, JSVAL_MASK32_BOOLEAN);

    stubcc.leave();
    stubcc.call(stubs::Not);

    RegisterID reg = frame.ownRegForData(top);
    masm.xor32(Imm32(1), reg);
    frame.pop();
    frame.pushTypedPayload(JSVAL_MASK32_BOOLEAN, reg);

    stubcc.rejoin(1);
}

void
mjit::Compiler::jsop_typeof()
{
    FrameEntry *fe = frame.peek(-1);

    if (fe->isTypeKnown()) {
        JSRuntime *rt = cx->runtime;

        JSAtom *atom = NULL;
        switch (fe->getTypeTag()) {
          case JSVAL_MASK32_STRING:
            atom = rt->atomState.typeAtoms[JSTYPE_STRING];
            break;
          case JSVAL_MASK32_UNDEFINED:
            atom = rt->atomState.typeAtoms[JSTYPE_VOID];
            break;
          case JSVAL_MASK32_NULL:
            atom = rt->atomState.typeAtoms[JSTYPE_OBJECT];
            break;
          case JSVAL_MASK32_FUNOBJ:
          case JSVAL_MASK32_NONFUNOBJ:
            atom = NULL;
            break;
          case JSVAL_MASK32_BOOLEAN:
            atom = rt->atomState.typeAtoms[JSTYPE_BOOLEAN];
            break;
          default:
            atom = rt->atomState.typeAtoms[JSTYPE_NUMBER];
            break;
        }

        if (atom) {
            frame.pop();
            frame.push(StringTag(ATOM_TO_STRING(atom)));
            return;
        }
    }

    prepareStubCall();
    stubCall(stubs::TypeOf, Uses(1), Defs(1));
    frame.pop();
    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_MASK32_STRING, Registers::ReturnReg);
}

void
mjit::Compiler::jsop_localinc(JSOp op, uint32 slot, bool popped)
{
    bool post = (op == JSOP_LOCALINC || op == JSOP_LOCALDEC);
    int32 amt = (op == JSOP_INCLOCAL || op == JSOP_LOCALINC) ? 1 : -1;
    uint32 depth = frame.stackDepth();

    frame.pushLocal(slot);

    FrameEntry *fe = frame.peek(-1);

    if (fe->isConstant() && fe->getValue().isPrimitive()) {
        Value v = fe->getValue();
        double d;
        ValueToNumber(cx, v, &d);
        d += amt;
        v.setNumber(d);
        frame.push(v);
        frame.storeLocal(slot);
        frame.pop();
        return;
    }

    if (post && !popped) {
        frame.dup();
        fe = frame.peek(-1);
    }

    if (!fe->isTypeKnown() || fe->getTypeTag() != JSVAL_MASK32_INT32) {
        
        if (fe->isTypeKnown()) {
            Jump j = masm.jump();
            stubcc.linkExit(j);
        } else {
            Jump intFail = frame.testInt32(Assembler::NotEqual, fe);
            stubcc.linkExit(intFail);
        }
    }

    RegisterID reg = frame.ownRegForData(fe);
    frame.pop();

    Jump ovf;
    if (amt > 0)
        ovf = masm.branchAdd32(Assembler::Overflow, Imm32(1), reg);
    else
        ovf = masm.branchSub32(Assembler::Overflow, Imm32(1), reg);
    stubcc.linkExit(ovf);

    
    stubcc.leave();
    stubcc.masm.addPtr(Imm32(sizeof(Value) * slot + sizeof(JSStackFrame)),
                       JSFrameReg,
                       Registers::ArgReg1);
    stubcc.vpInc(op, depth);

    
    frame.pushUntypedPayload(JSVAL_MASK32_INT32, reg);
    frame.storeLocal(slot);

    if (post || popped)
        frame.pop();

    stubcc.rejoin(1);
}

void
mjit::Compiler::jsop_arginc(JSOp op, uint32 slot, bool popped)
{
    int amt = (js_CodeSpec[op].format & JOF_INC) ? 1 : -1;
    bool post = !!(js_CodeSpec[op].format & JOF_POST);
    uint32 depth = frame.stackDepth();

    jsop_getarg(slot);
    if (post && !popped)
        frame.dup();

    FrameEntry *fe = frame.peek(-1);
    Jump notInt = frame.testInt32(Assembler::NotEqual, fe);
    stubcc.linkExit(notInt);

    RegisterID reg = frame.ownRegForData(fe);
    frame.pop();

    Jump ovf;
    if (amt > 0)
        ovf = masm.branchAdd32(Assembler::Overflow, Imm32(1), reg);
    else
        ovf = masm.branchSub32(Assembler::Overflow, Imm32(1), reg);
    stubcc.linkExit(ovf);

    Address argv(JSFrameReg, offsetof(JSStackFrame, argv));

    stubcc.leave();
    stubcc.masm.loadPtr(argv, Registers::ArgReg1);
    stubcc.masm.addPtr(Imm32(sizeof(Value) * slot), Registers::ArgReg1, Registers::ArgReg1);
    stubcc.vpInc(op, depth);

    frame.pushTypedPayload(JSVAL_MASK32_INT32, reg);
    fe = frame.peek(-1);

    reg = frame.allocReg();
    masm.loadPtr(argv, reg);
    Address address = Address(reg, slot * sizeof(Value));
    frame.storeTo(fe, address, popped);
    frame.freeReg(reg);

    if (post || popped)
        frame.pop();
    else
        frame.forgetType(fe);

    stubcc.rejoin(1);
}

void
mjit::Compiler::jsop_setelem()
{
    FrameEntry *obj = frame.peek(-3);
    FrameEntry *id = frame.peek(-2);
    FrameEntry *fe = frame.peek(-1);

    if ((obj->isTypeKnown() && obj->getTypeTag() != JSVAL_MASK32_NONFUNOBJ) ||
        (id->isTypeKnown() && id->getTypeTag() != JSVAL_MASK32_INT32) ||
        (id->isConstant() && id->getValue().asInt32() < 0)) {
        jsop_setelem_slow();
        return;
    }

    
    if (!id->isTypeKnown()) {
        Jump j = frame.testInt32(Assembler::NotEqual, id);
        stubcc.linkExit(j);
    }

    
    if (!obj->isTypeKnown()) {
        Jump j = frame.testNonFunObj(Assembler::NotEqual, obj);
        stubcc.linkExit(j);
    }

    
    RegisterID objReg = frame.copyData(obj);
    Jump guardDense = masm.branchPtr(Assembler::NotEqual,
                                      Address(objReg, offsetof(JSObject, clasp)),
                                      ImmPtr(&js_ArrayClass));
    stubcc.linkExit(guardDense);

    
    masm.loadPtr(Address(objReg, offsetof(JSObject, dslots)), objReg);
    Jump guardSlots = masm.branchTestPtr(Assembler::Zero, objReg, objReg);
    stubcc.linkExit(guardSlots);

    
    if (id->isConstant()) {
        Jump inRange = masm.branch32(Assembler::LessThanOrEqual,
                                     masm.payloadOf(Address(objReg, -int(sizeof(Value)))),
                                     Imm32(id->getValue().asInt32()));
        stubcc.linkExit(inRange);

        
        Address slot(objReg, id->getValue().asInt32() * sizeof(Value));
        Jump notHole = masm.branch32(Assembler::Equal, masm.tagOf(slot), Imm32(JSVAL_MASK32_MAGIC));
        stubcc.linkExit(notHole);

        stubcc.leave();
        stubcc.call(stubs::SetElem);

        
        frame.eviscerate(obj);
        frame.eviscerate(id);

        
        if (fe->isConstant()) {
            masm.storeValue(fe->getValue(), slot);
        } else {
            masm.storeData32(frame.tempRegForData(fe), slot);
            if (fe->isTypeKnown())
                masm.storeTypeTag(ImmTag(fe->getTypeTag()), slot);
            else
                masm.storeTypeTag(frame.tempRegForType(fe), slot);
        }
    } else {
        RegisterID idReg = frame.copyData(id);
        Jump inRange = masm.branch32(Assembler::AboveOrEqual,
                                     idReg,
                                     masm.payloadOf(Address(objReg, -int(sizeof(Value)))));
        stubcc.linkExit(inRange);

        
        BaseIndex slot(objReg, idReg, Assembler::JSVAL_SCALE);
        Jump notHole = masm.branch32(Assembler::Equal, masm.tagOf(slot), Imm32(JSVAL_MASK32_MAGIC));
        stubcc.linkExit(notHole);

        stubcc.leave();
        stubcc.call(stubs::SetElem);

        
        frame.eviscerate(obj);
        frame.eviscerate(id);

        
        if (fe->isConstant()) {
            masm.storeValue(fe->getValue(), slot);
        } else {
            masm.storeData32(frame.tempRegForData(fe), slot);
            if (fe->isTypeKnown())
                masm.storeTypeTag(ImmTag(fe->getTypeTag()), slot);
            else
                masm.storeTypeTag(frame.tempRegForType(fe), slot);
        }

        frame.freeReg(idReg);
    }
    frame.freeReg(objReg);

    frame.shimmy(2);
    stubcc.rejoin(0);
}

void
mjit::Compiler::jsop_getelem()
{
    FrameEntry *obj = frame.peek(-2);
    FrameEntry *id = frame.peek(-1);

    if ((obj->isTypeKnown() && obj->getTypeTag() != JSVAL_MASK32_NONFUNOBJ) ||
        (id->isTypeKnown() && id->getTypeTag() != JSVAL_MASK32_INT32) ||
        (id->isConstant() && id->getValue().asInt32() < 0)) {
        jsop_getelem_slow();
        return;
    }

    
    if (!id->isTypeKnown()) {
        Jump j = frame.testInt32(Assembler::NotEqual, id);
        stubcc.linkExit(j);
    }

    
    if (!obj->isTypeKnown()) {
        Jump j = frame.testNonFunObj(Assembler::NotEqual, obj);
        stubcc.linkExit(j);
    }

    
    RegisterID objReg = frame.copyData(obj);
    Jump guardDense = masm.branchPtr(Assembler::NotEqual,
                                      Address(objReg, offsetof(JSObject, clasp)),
                                      ImmPtr(&js_ArrayClass));
    stubcc.linkExit(guardDense);

    
    masm.loadPtr(Address(objReg, offsetof(JSObject, dslots)), objReg);
    Jump guardSlots = masm.branchTestPtr(Assembler::Zero, objReg, objReg);
    stubcc.linkExit(guardSlots);

    
    if (id->isConstant()) {
        Jump inRange = masm.branch32(Assembler::LessThanOrEqual,
                                     masm.payloadOf(Address(objReg, -int(sizeof(Value)))),
                                     Imm32(id->getValue().asInt32()));
        stubcc.linkExit(inRange);

        
        Address slot(objReg, id->getValue().asInt32() * sizeof(Value));
        Jump notHole = masm.branch32(Assembler::Equal, masm.tagOf(slot), Imm32(JSVAL_MASK32_MAGIC));
        stubcc.linkExit(notHole);

        stubcc.leave();
        stubcc.call(stubs::GetElem);

        frame.popn(2);
        frame.freeReg(objReg);
        frame.push(slot);
    } else {
        RegisterID idReg = frame.copyData(id);
        Jump inRange = masm.branch32(Assembler::AboveOrEqual,
                                     idReg,
                                     masm.payloadOf(Address(objReg, -int(sizeof(Value)))));
        stubcc.linkExit(inRange);

        
        BaseIndex slot(objReg, idReg, Assembler::JSVAL_SCALE);
        Jump notHole = masm.branch32(Assembler::Equal, masm.tagOf(slot), Imm32(JSVAL_MASK32_MAGIC));
        stubcc.linkExit(notHole);

        stubcc.leave();
        stubcc.call(stubs::GetElem);

        frame.popn(2);

        RegisterID reg = frame.allocReg();
        masm.loadTypeTag(slot, reg);
        masm.loadData32(slot, idReg);
        frame.freeReg(objReg);
        frame.pushRegs(reg, idReg);
    }

    stubcc.rejoin(0);
}

