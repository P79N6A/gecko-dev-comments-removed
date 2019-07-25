






































#include "jsbool.h"
#include "jscntxt.h"
#include "jsemit.h"
#include "jslibmath.h"
#include "jsnum.h"
#include "jsscope.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"
#include "methodjit/StubCalls.h"
#include "methodjit/FrameState-inl.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;

typedef JSC::MacroAssembler::RegisterID RegisterID;

RegisterID
mjit::Compiler::rightRegForShift(FrameEntry *rhs)
{
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    




    RegisterID reg = JSC::X86Registers::ecx;
    if (!rhs->isConstant())
        frame.copyDataIntoReg(rhs, reg);
    return reg;
#else
    if (rhs->isConstant())
        return frame.allocReg();
    return frame.copyDataIntoReg(rhs);
#endif
}

void
mjit::Compiler::jsop_rsh_const_int(FrameEntry *lhs, FrameEntry *rhs)
{
    RegisterID rhsData = rightRegForShift(rhs);
    RegisterID result = frame.allocReg();
    masm.move(Imm32(lhs->getValue().toInt32()), result);
    masm.rshift32(rhsData, result);

    frame.freeReg(rhsData);
    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, result);
}

void
mjit::Compiler::jsop_rsh_int_int(FrameEntry *lhs, FrameEntry *rhs)
{
    RegisterID rhsData = rightRegForShift(rhs);
    RegisterID lhsData = frame.copyDataIntoReg(lhs);
    masm.rshift32(rhsData, lhsData);
    frame.freeReg(rhsData);
    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, lhsData);
}

void
mjit::Compiler::jsop_rsh_int_const(FrameEntry *lhs, FrameEntry *rhs)
{
    int32 shiftAmount = rhs->getValue().toInt32();

    if (!shiftAmount) {
        frame.pop();
        return;
    }

    RegisterID result = frame.copyDataIntoReg(lhs);
    masm.rshift32(Imm32(shiftAmount), result);
    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, result);
}

void
mjit::Compiler::jsop_rsh_unknown_const(FrameEntry *lhs, FrameEntry *rhs)
{
    int32 shiftAmount = rhs->getValue().toInt32();

    RegisterID lhsType = frame.tempRegForType(lhs);
    frame.pinReg(lhsType);
    RegisterID lhsData = frame.copyDataIntoReg(lhs);
    frame.unpinReg(lhsType);

    Jump lhsIntGuard = masm.testInt32(Assembler::NotEqual, lhsType);
    stubcc.linkExitDirect(lhsIntGuard, stubcc.masm.label());

    Jump lhsDoubleGuard = stubcc.masm.testDouble(Assembler::NotEqual, lhsType);
    frame.loadDouble(lhs, FPRegisters::First, stubcc.masm);
    Jump lhsTruncateGuard = stubcc.masm.branchTruncateDoubleToInt32(FPRegisters::First, lhsData);
    stubcc.crossJump(stubcc.masm.jump(), masm.label());

    lhsDoubleGuard.linkTo(stubcc.masm.label(), &stubcc.masm);
    lhsTruncateGuard.linkTo(stubcc.masm.label(), &stubcc.masm);

    frame.sync(stubcc.masm, Uses(2));
    OOL_STUBCALL(stubs::Rsh);

    if (shiftAmount)
        masm.rshift32(Imm32(shiftAmount), lhsData);

    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, lhsData);

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_rsh_const_unknown(FrameEntry *lhs, FrameEntry *rhs)
{
    RegisterID rhsData = rightRegForShift(rhs);
    RegisterID rhsType = frame.tempRegForType(rhs);
    frame.pinReg(rhsType);
    RegisterID result = frame.allocReg();
    frame.unpinReg(rhsType);

    Jump rhsIntGuard = masm.testInt32(Assembler::NotEqual, rhsType);
    stubcc.linkExit(rhsIntGuard, Uses(2));
    stubcc.leave();
    OOL_STUBCALL(stubs::Rsh);
    masm.move(Imm32(lhs->getValue().toInt32()), result);
    masm.rshift32(rhsData, result);
    frame.freeReg(rhsData);

    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, result);
    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_rsh_int_unknown(FrameEntry *lhs, FrameEntry *rhs)
{
    RegisterID rhsData = rightRegForShift(rhs);
    RegisterID rhsType = frame.tempRegForType(rhs);
    frame.pinReg(rhsType);
    RegisterID lhsData = frame.copyDataIntoReg(lhs);
    frame.unpinReg(rhsType);

    Jump rhsIntGuard = masm.testInt32(Assembler::NotEqual, rhsType);
    stubcc.linkExit(rhsIntGuard, Uses(2));
    stubcc.leave();
    OOL_STUBCALL(stubs::Rsh);

    masm.rshift32(rhsData, lhsData);
    frame.freeReg(rhsData);
    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, lhsData);

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_rsh_unknown_any(FrameEntry *lhs, FrameEntry *rhs)
{
    JS_ASSERT(!lhs->isTypeKnown());
    JS_ASSERT(!rhs->isNotType(JSVAL_TYPE_INT32));

    
    RegisterID rhsData = rightRegForShift(rhs);

    MaybeRegisterID rhsType;
    if (!rhs->isTypeKnown()) {
        rhsType.setReg(frame.tempRegForType(rhs));
        frame.pinReg(rhsType.reg());
    }

    RegisterID lhsData = frame.copyDataIntoReg(lhs);
    MaybeRegisterID lhsType;
    if (rhsType.isSet() && frame.haveSameBacking(lhs, rhs))
        lhsType = rhsType;
    else
        lhsType = frame.tempRegForType(lhs);

    
    MaybeJump rhsIntGuard;
    if (rhsType.isSet()) {
        rhsIntGuard.setJump(masm.testInt32(Assembler::NotEqual, rhsType.reg()));
        frame.unpinReg(rhsType.reg());
    }

    
    Jump lhsIntGuard = masm.testInt32(Assembler::NotEqual, lhsType.reg());
    stubcc.linkExitDirect(lhsIntGuard, stubcc.masm.label());

    
    Jump lhsDoubleGuard = stubcc.masm.testDouble(Assembler::NotEqual, lhsType.reg());
    frame.loadDouble(lhs, FPRegisters::First, stubcc.masm);
    Jump lhsTruncateGuard = stubcc.masm.branchTruncateDoubleToInt32(FPRegisters::First, lhsData);
    stubcc.crossJump(stubcc.masm.jump(), masm.label());

    lhsDoubleGuard.linkTo(stubcc.masm.label(), &stubcc.masm);
    lhsTruncateGuard.linkTo(stubcc.masm.label(), &stubcc.masm);

    if (rhsIntGuard.isSet())
        stubcc.linkExitDirect(rhsIntGuard.getJump(), stubcc.masm.label());
    frame.sync(stubcc.masm, Uses(2));
    OOL_STUBCALL(stubs::Rsh);

    masm.rshift32(rhsData, lhsData);

    frame.freeReg(rhsData);
    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_INT32, lhsData);

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_rsh()
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    if (tryBinaryConstantFold(cx, frame, JSOP_RSH, lhs, rhs))
        return;

    if (lhs->isNotType(JSVAL_TYPE_INT32) || rhs->isNotType(JSVAL_TYPE_INT32)) {
        prepareStubCall(Uses(2));
        INLINE_STUBCALL(stubs::Rsh);
        frame.popn(2);
        frame.pushSyncedType(JSVAL_TYPE_INT32);
        return;
    }

    JS_ASSERT(!(lhs->isConstant() && rhs->isConstant()));
    if (lhs->isConstant()) {
        if (rhs->isType(JSVAL_TYPE_INT32))
            jsop_rsh_const_int(lhs, rhs);
        else
            jsop_rsh_const_unknown(lhs, rhs);
    } else if (rhs->isConstant()) {
        if (lhs->isType(JSVAL_TYPE_INT32))
            jsop_rsh_int_const(lhs, rhs);
        else
            jsop_rsh_unknown_const(lhs, rhs);
    } else {
        if (lhs->isType(JSVAL_TYPE_INT32) && rhs->isType(JSVAL_TYPE_INT32))
            jsop_rsh_int_int(lhs, rhs);
        else if (lhs->isType(JSVAL_TYPE_INT32))
            jsop_rsh_int_unknown(lhs, rhs);
        else
            jsop_rsh_unknown_any(lhs, rhs);
    }
}

void
mjit::Compiler::jsop_bitnot()
{
    FrameEntry *top = frame.peek(-1);

    
    if (top->isTypeKnown() && top->getKnownType() != JSVAL_TYPE_INT32) {
        prepareStubCall(Uses(1));
        INLINE_STUBCALL(stubs::BitNot);
        frame.pop();
        frame.pushSyncedType(JSVAL_TYPE_INT32);
        return;
    }
           
    
    bool stubNeeded = false;
    if (!top->isTypeKnown()) {
        Jump intFail = frame.testInt32(Assembler::NotEqual, top);
        stubcc.linkExit(intFail, Uses(1));
        frame.learnType(top, JSVAL_TYPE_INT32);
        stubNeeded = true;
    }

    if (stubNeeded) {
        stubcc.leave();
        OOL_STUBCALL(stubs::BitNot);
    }

    RegisterID reg = frame.ownRegForData(top);
    masm.not32(reg);
    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

    if (stubNeeded)
        stubcc.rejoin(Changes(1));
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
      case JSOP_URSH:
        stub = stubs::Ursh;
        break;
      default:
        JS_NOT_REACHED("wat");
        return;
    }

    bool lhsIntOrDouble = !(lhs->isNotType(JSVAL_TYPE_DOUBLE) && 
                            lhs->isNotType(JSVAL_TYPE_INT32));
    
    
    if (!lhs->isConstant() && rhs->isConstant() && lhsIntOrDouble &&
        rhs->isType(JSVAL_TYPE_INT32) && rhs->getValue().toInt32() == 0 &&
        (op == JSOP_BITOR || op == JSOP_LSH)) {
        RegisterID reg = frame.copyDataIntoReg(lhs);
        if (lhs->isType(JSVAL_TYPE_INT32)) {
            frame.popn(2);
            frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);
            return;
        }
        MaybeJump isInt;
        if (!lhs->isType(JSVAL_TYPE_DOUBLE)) {
            RegisterID typeReg = frame.tempRegForType(lhs);
            isInt = masm.testInt32(Assembler::Equal, typeReg);
            Jump notDouble = masm.testDouble(Assembler::NotEqual, typeReg);
            stubcc.linkExit(notDouble, Uses(2));
        }
        frame.loadDouble(lhs, FPRegisters::First, masm);
        
        Jump truncateGuard = masm.branchTruncateDoubleToInt32(FPRegisters::First, reg);
        stubcc.linkExit(truncateGuard, Uses(2));
        stubcc.leave();
        OOL_STUBCALL(stub);
        
        if (isInt.isSet())
            isInt.get().linkTo(masm.label(), &masm);
        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);
        stubcc.rejoin(Changes(1));
        return;
    }

    
    if (rhs->isNotType(JSVAL_TYPE_INT32) || lhs->isNotType(JSVAL_TYPE_INT32) || 
        (op == JSOP_URSH && rhs->isConstant() && rhs->getValue().toInt32() % 32 == 0)) {
        prepareStubCall(Uses(2));
        INLINE_STUBCALL(stub);
        frame.popn(2);
        if (op == JSOP_URSH)
            frame.pushSynced();
        else
            frame.pushSyncedType(JSVAL_TYPE_INT32);
        return;
    }
           
    
    bool stubNeeded = false;
    if (!rhs->isTypeKnown()) {
        Jump rhsFail = frame.testInt32(Assembler::NotEqual, rhs);
        stubcc.linkExit(rhsFail, Uses(2));
        frame.learnType(rhs, JSVAL_TYPE_INT32);
        stubNeeded = true;
    }
    if (!lhs->isTypeKnown() && !frame.haveSameBacking(lhs, rhs)) {
        Jump lhsFail = frame.testInt32(Assembler::NotEqual, lhs);
        stubcc.linkExit(lhsFail, Uses(2));
        stubNeeded = true;
    }

    if (lhs->isConstant() && rhs->isConstant()) {
        int32 L = lhs->getValue().toInt32();
        int32 R = rhs->getValue().toInt32();

        frame.popn(2);
        switch (op) {
          case JSOP_BITOR:
            frame.push(Int32Value(L | R));
            return;
          case JSOP_BITXOR:
            frame.push(Int32Value(L ^ R));
            return;
          case JSOP_BITAND:
            frame.push(Int32Value(L & R));
            return;
          case JSOP_LSH:
            frame.push(Int32Value(L << R));
            return;
          case JSOP_URSH: 
          {
            uint32 unsignedL;
            if (ValueToECMAUint32(cx, lhs->getValue(), (uint32_t*)&unsignedL)) {
                frame.push(NumberValue(uint32(unsignedL >> (R & 31))));
                return;
            }
            break;
          }
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
                masm.and32(Imm32(rhs->getValue().toInt32()), reg);
            else if (op == JSOP_BITXOR)
                masm.xor32(Imm32(rhs->getValue().toInt32()), reg);
            else
                masm.or32(Imm32(rhs->getValue().toInt32()), reg);
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
      case JSOP_URSH:
      {
        
        if (rhs->isConstant()) {
            RegisterID reg = frame.ownRegForData(lhs);
            int shift = rhs->getValue().toInt32() & 0x1F;

            if (shift) {
                if (op == JSOP_LSH)
                    masm.lshift32(Imm32(shift), reg);
                else
                    masm.urshift32(Imm32(shift), reg);
            }
            if (stubNeeded) {
                stubcc.leave();
                OOL_STUBCALL(stub);
            }
            frame.popn(2);
            
            
            JS_ASSERT_IF(op == JSOP_URSH, shift >= 1);
            frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

            if (stubNeeded)
                stubcc.rejoin(Changes(1));

            return;
        }
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
        
        RegisterID rr = frame.tempRegInMaskForData(rhs,
                                                   Registers::maskReg(JSC::X86Registers::ecx));
#else
        RegisterID rr = frame.tempRegForData(rhs);
#endif

        if (frame.haveSameBacking(lhs, rhs)) {
            
            
            reg = frame.allocReg();
            if (rr != reg)
                masm.move(rr, reg);
        } else {
            frame.pinReg(rr);
            if (lhs->isConstant()) {
                reg = frame.allocReg();
                masm.move(Imm32(lhs->getValue().toInt32()), reg);
            } else {
                reg = frame.copyDataIntoReg(lhs);
            }
            frame.unpinReg(rr);
        }
        
        if (op == JSOP_LSH) {
            masm.lshift32(rr, reg);
        } else {
            masm.urshift32(rr, reg);
            
            Jump isNegative = masm.branch32(Assembler::LessThan, reg, Imm32(0));
            stubcc.linkExit(isNegative, Uses(2));
            stubNeeded = true;
        }
        break;
      }

      default:
        JS_NOT_REACHED("NYI");
        return;
    }

    if (stubNeeded) {
        stubcc.leave();
        OOL_STUBCALL(stub);
    }

    frame.pop();
    frame.pop();

    if (op == JSOP_URSH)
        frame.pushNumber(reg, true);
    else
        frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

    if (stubNeeded)
        stubcc.rejoin(Changes(1));
}

static inline bool
CheckNullOrUndefined(FrameEntry *fe)
{
    if (!fe->isTypeKnown())
        return false;
    JSValueType type = fe->getKnownType();
    return type == JSVAL_TYPE_NULL || type == JSVAL_TYPE_UNDEFINED;
}

bool
mjit::Compiler::jsop_equality(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    JS_ASSERT(!(rhs->isConstant() && lhs->isConstant()));

    bool lhsTest;
    if ((lhsTest = CheckNullOrUndefined(lhs)) || CheckNullOrUndefined(rhs)) {
        
        FrameEntry *test = lhsTest ? rhs : lhs;

        if (test->isTypeKnown())
            return emitStubCmpOp(stub, target, fused);

        
        RegisterID reg = frame.ownRegForType(test);
        frame.pop();
        frame.pop();

        




        if (target) {
            frame.syncAndForgetEverything();

            if ((op == JSOP_EQ && fused == JSOP_IFNE) ||
                (op == JSOP_NE && fused == JSOP_IFEQ)) {
                






                Jump b1 = masm.branchPtr(Assembler::Equal, reg, ImmType(JSVAL_TYPE_UNDEFINED));
                Jump b2 = masm.branchPtr(Assembler::Equal, reg, ImmType(JSVAL_TYPE_NULL));
                Jump j1 = masm.jump();
                b1.linkTo(masm.label(), &masm);
                b2.linkTo(masm.label(), &masm);
                Jump j2 = masm.jump();
                if (!jumpAndTrace(j2, target))
                    return false;
                j1.linkTo(masm.label(), &masm);
            } else {
                Jump j = masm.branchPtr(Assembler::Equal, reg, ImmType(JSVAL_TYPE_UNDEFINED));
                Jump j2 = masm.branchPtr(Assembler::NotEqual, reg, ImmType(JSVAL_TYPE_NULL));
                if (!jumpAndTrace(j2, target))
                    return false;
                j.linkTo(masm.label(), &masm);
            }
        } else {
            Jump j = masm.branchPtr(Assembler::Equal, reg, ImmType(JSVAL_TYPE_UNDEFINED));
            Jump j2 = masm.branchPtr(Assembler::Equal, reg, ImmType(JSVAL_TYPE_NULL));
            masm.move(Imm32(op == JSOP_NE), reg);
            Jump j3 = masm.jump();
            j2.linkTo(masm.label(), &masm);
            j.linkTo(masm.label(), &masm);
            masm.move(Imm32(op == JSOP_EQ), reg);
            j3.linkTo(masm.label(), &masm);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, reg);
        }
        return true;
    }

    return emitStubCmpOp(stub, target, fused);
}

bool
mjit::Compiler::jsop_relational(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    JS_ASSERT(!(rhs->isConstant() && lhs->isConstant()));

    
    if ((lhs->isNotType(JSVAL_TYPE_INT32) && lhs->isNotType(JSVAL_TYPE_DOUBLE) &&
         lhs->isNotType(JSVAL_TYPE_STRING)) ||
        (rhs->isNotType(JSVAL_TYPE_INT32) && rhs->isNotType(JSVAL_TYPE_DOUBLE) &&
         rhs->isNotType(JSVAL_TYPE_STRING))) {
        if (op == JSOP_EQ || op == JSOP_NE)
            return jsop_equality(op, stub, target, fused);
        return emitStubCmpOp(stub, target, fused);
    }

    if (op == JSOP_EQ || op == JSOP_NE) {
        if ((lhs->isNotType(JSVAL_TYPE_INT32) && lhs->isNotType(JSVAL_TYPE_STRING)) ||
            (rhs->isNotType(JSVAL_TYPE_INT32) && rhs->isNotType(JSVAL_TYPE_STRING))) {
            return emitStubCmpOp(stub, target, fused);
        } else if (!target && (lhs->isType(JSVAL_TYPE_STRING) || rhs->isType(JSVAL_TYPE_STRING))) {
            return emitStubCmpOp(stub, target, fused);
        } else if (frame.haveSameBacking(lhs, rhs)) {
            return emitStubCmpOp(stub, target, fused);
        } else {
            return jsop_equality_int_string(op, stub, target, fused);
        }
    }

    if (frame.haveSameBacking(lhs, rhs)) {
        return jsop_relational_self(op, stub, target, fused);
    } else if (lhs->isType(JSVAL_TYPE_STRING) || rhs->isType(JSVAL_TYPE_STRING)) {
        return emitStubCmpOp(stub, target, fused);
    } else if (lhs->isType(JSVAL_TYPE_DOUBLE) || rhs->isType(JSVAL_TYPE_DOUBLE)) {
        return jsop_relational_double(op, stub, target, fused);
    } else {
        return jsop_relational_full(op, stub, target, fused);
    }
}

void
mjit::Compiler::jsop_not()
{
    FrameEntry *top = frame.peek(-1);

    if (top->isConstant()) {
        const Value &v = top->getValue();
        frame.pop();
        frame.push(BooleanValue(!js_ValueToBoolean(v)));
        return;
    }

    if (top->isTypeKnown()) {
        JSValueType type = top->getKnownType();
        switch (type) {
          case JSVAL_TYPE_INT32:
          {
            RegisterID data = frame.allocReg(Registers::SingleByteRegs);
            if (frame.shouldAvoidDataRemat(top))
                masm.loadPayload(frame.addressOf(top), data);
            else
                masm.move(frame.tempRegForData(top), data);

            masm.set32(Assembler::Equal, data, Imm32(0), data);

            frame.pop();
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, data);
            break;
          }

          case JSVAL_TYPE_BOOLEAN:
          {
            RegisterID reg = frame.ownRegForData(top);

            masm.xor32(Imm32(1), reg);

            frame.pop();
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, reg);
            break;
          }

          case JSVAL_TYPE_OBJECT:
          {
            frame.pop();
            frame.push(BooleanValue(false));
            break;
          }

          default:
          {
            prepareStubCall(Uses(1));
            INLINE_STUBCALL(stubs::ValueToBoolean);

            RegisterID reg = Registers::ReturnReg;
            frame.takeReg(reg);
            masm.xor32(Imm32(1), reg);

            frame.pop();
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, reg);
            break;
          }
        }

        return;
    }

    RegisterID data = frame.allocReg(Registers::SingleByteRegs);
    if (frame.shouldAvoidDataRemat(top))
        masm.loadPayload(frame.addressOf(top), data);
    else
        masm.move(frame.tempRegForData(top), data);
    RegisterID type = frame.tempRegForType(top);
    Label syncTarget = stubcc.syncExitAndJump(Uses(1));


    
    Jump jmpNotBool = masm.testBoolean(Assembler::NotEqual, type);
    masm.xor32(Imm32(1), data);


    
    Label lblMaybeInt32 = stubcc.masm.label();

    Jump jmpNotInt32 = stubcc.masm.testInt32(Assembler::NotEqual, type);
    stubcc.masm.set32(Assembler::Equal, data, Imm32(0), data);
    Jump jmpInt32Exit = stubcc.masm.jump();

    Label lblMaybeObject = stubcc.masm.label();
    Jump jmpNotObject = stubcc.masm.testPrimitive(Assembler::Equal, type);
    stubcc.masm.move(Imm32(0), data);
    Jump jmpObjectExit = stubcc.masm.jump();


    
    Label lblRejoin = masm.label();

    
    stubcc.linkExitDirect(jmpNotBool, lblMaybeInt32);

    jmpNotInt32.linkTo(lblMaybeObject, &stubcc.masm);
    stubcc.crossJump(jmpInt32Exit, lblRejoin);

    jmpNotObject.linkTo(syncTarget, &stubcc.masm);
    stubcc.crossJump(jmpObjectExit, lblRejoin);
    

    
    stubcc.leave();
    OOL_STUBCALL(stubs::Not);

    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, data);

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_typeof()
{
    FrameEntry *fe = frame.peek(-1);

    if (fe->isTypeKnown()) {
        JSRuntime *rt = cx->runtime;

        JSAtom *atom = NULL;
        switch (fe->getKnownType()) {
          case JSVAL_TYPE_STRING:
            atom = rt->atomState.typeAtoms[JSTYPE_STRING];
            break;
          case JSVAL_TYPE_UNDEFINED:
            atom = rt->atomState.typeAtoms[JSTYPE_VOID];
            break;
          case JSVAL_TYPE_NULL:
            atom = rt->atomState.typeAtoms[JSTYPE_OBJECT];
            break;
          case JSVAL_TYPE_OBJECT:
            atom = NULL;
            break;
          case JSVAL_TYPE_BOOLEAN:
            atom = rt->atomState.typeAtoms[JSTYPE_BOOLEAN];
            break;
          default:
            atom = rt->atomState.typeAtoms[JSTYPE_NUMBER];
            break;
        }

        if (atom) {
            frame.pop();
            frame.push(StringValue(atom));
            return;
        }
    }

    JSOp fused = JSOp(PC[JSOP_TYPEOF_LENGTH]);
    if (fused == JSOP_STRING && !fe->isTypeKnown()) {
        JSOp op = JSOp(PC[JSOP_TYPEOF_LENGTH + JSOP_STRING_LENGTH]);

        if (op == JSOP_STRICTEQ || op == JSOP_EQ || op == JSOP_STRICTNE || op == JSOP_NE) {
            JSAtom *atom = script->getAtom(fullAtomIndex(PC + JSOP_TYPEOF_LENGTH));
            JSRuntime *rt = cx->runtime;
            JSValueType type = JSVAL_TYPE_BOXED;
            Assembler::Condition cond = (op == JSOP_STRICTEQ || op == JSOP_EQ)
                                        ? Assembler::Equal
                                        : Assembler::NotEqual;
            
            if (atom == rt->atomState.typeAtoms[JSTYPE_VOID]) {
                type = JSVAL_TYPE_UNDEFINED;
            } else if (atom == rt->atomState.typeAtoms[JSTYPE_STRING]) {
                type = JSVAL_TYPE_STRING;
            } else if (atom == rt->atomState.typeAtoms[JSTYPE_BOOLEAN]) {
                type = JSVAL_TYPE_BOOLEAN;
            } else if (atom == rt->atomState.typeAtoms[JSTYPE_NUMBER]) {
                type = JSVAL_TYPE_INT32;

                
                cond = (cond == Assembler::Equal) ? Assembler::BelowOrEqual : Assembler::Above;
            }

            if (type != JSVAL_TYPE_BOXED) {
                PC += JSOP_STRING_LENGTH;;
                PC += JSOP_EQ_LENGTH;

                RegisterID result = frame.allocReg(Registers::SingleByteRegs);

#if defined JS_NUNBOX32
                if (frame.shouldAvoidTypeRemat(fe))
                    masm.set32(cond, masm.tagOf(frame.addressOf(fe)), ImmType(type), result);
                else
                    masm.set32(cond, frame.tempRegForType(fe), ImmType(type), result);
#elif defined JS_PUNBOX64
                masm.setPtr(cond, frame.tempRegForType(fe), ImmType(type), result);
#endif

                frame.pop();
                frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
                return;
            }
        }
    }

    prepareStubCall(Uses(1));
    INLINE_STUBCALL(stubs::TypeOf);
    frame.pop();
    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_TYPE_STRING, Registers::ReturnReg);
}

bool
mjit::Compiler::booleanJumpScript(JSOp op, jsbytecode *target)
{
    FrameEntry *fe = frame.peek(-1);

    MaybeRegisterID type;
    MaybeRegisterID data;

    if (!fe->isTypeKnown() && !frame.shouldAvoidTypeRemat(fe))
        type.setReg(frame.copyTypeIntoReg(fe));
    data.setReg(frame.copyDataIntoReg(fe));

    frame.syncAndForgetEverything();

    Assembler::Condition cond = (op == JSOP_IFNE || op == JSOP_OR)
                                ? Assembler::NonZero
                                : Assembler::Zero;
    Assembler::Condition ncond = (op == JSOP_IFNE || op == JSOP_OR)
                                 ? Assembler::Zero
                                 : Assembler::NonZero;

    
    MaybeJump jmpNotBool;
    MaybeJump jmpNotExecScript;
    if (type.isSet()) {
        jmpNotBool.setJump(masm.testBoolean(Assembler::NotEqual, type.reg()));
    } else {
        if (!fe->isTypeKnown()) {
            jmpNotBool.setJump(masm.testBoolean(Assembler::NotEqual,
                                                frame.addressOf(fe)));
        } else if (fe->isNotType(JSVAL_TYPE_BOOLEAN) &&
                   fe->isNotType(JSVAL_TYPE_INT32)) {
            jmpNotBool.setJump(masm.jump());
        }
    }

    



    jmpNotExecScript.setJump(masm.branchTest32(ncond, data.reg(), data.reg()));
    Label lblExecScript = masm.label();
    Jump j = masm.jump();


    
    MaybeJump jmpCvtExecScript;
    MaybeJump jmpCvtRejoin;
    Label lblCvtPath = stubcc.masm.label();

    if (!fe->isTypeKnown() ||
        !(fe->isType(JSVAL_TYPE_BOOLEAN) || fe->isType(JSVAL_TYPE_INT32))) {
        stubcc.masm.infallibleVMCall(JS_FUNC_TO_DATA_PTR(void *, stubs::ValueToBoolean),
                                     frame.localSlots());

        jmpCvtExecScript.setJump(stubcc.masm.branchTest32(cond, Registers::ReturnReg,
                                                          Registers::ReturnReg));
        jmpCvtRejoin.setJump(stubcc.masm.jump());
    }

    
    Label lblAfterScript = masm.label();

    
    if (jmpNotBool.isSet())
        stubcc.linkExitDirect(jmpNotBool.getJump(), lblCvtPath);
    if (jmpNotExecScript.isSet())
        jmpNotExecScript.getJump().linkTo(lblAfterScript, &masm);

    if (jmpCvtExecScript.isSet())
        stubcc.crossJump(jmpCvtExecScript.getJump(), lblExecScript);
    if (jmpCvtRejoin.isSet())
        stubcc.crossJump(jmpCvtRejoin.getJump(), lblAfterScript);

    frame.pop();

    return jumpAndTrace(j, target);
}

bool
mjit::Compiler::jsop_ifneq(JSOp op, jsbytecode *target)
{
    FrameEntry *fe = frame.peek(-1);

    if (fe->isConstant()) {
        JSBool b = js_ValueToBoolean(fe->getValue());

        frame.pop();

        if (op == JSOP_IFEQ)
            b = !b;
        if (b) {
            frame.syncAndForgetEverything();
            if (!jumpAndTrace(masm.jump(), target))
                return false;
        }
        return true;
    }

    return booleanJumpScript(op, target);
}

bool
mjit::Compiler::jsop_andor(JSOp op, jsbytecode *target)
{
    FrameEntry *fe = frame.peek(-1);

    if (fe->isConstant()) {
        JSBool b = js_ValueToBoolean(fe->getValue());
        
        
        if ((op == JSOP_OR && b == JS_TRUE) ||
            (op == JSOP_AND && b == JS_FALSE)) {
            frame.syncAndForgetEverything();
            if (!jumpAndTrace(masm.jump(), target))
                return false;
        }

        frame.pop();
        return true;
    }

    return booleanJumpScript(op, target);
}

void
mjit::Compiler::jsop_localinc(JSOp op, uint32 slot, bool popped)
{
    if (popped || (op == JSOP_INCLOCAL || op == JSOP_DECLOCAL)) {
        int amt = (op == JSOP_LOCALINC || op == JSOP_INCLOCAL) ? -1 : 1;

        
        
        frame.pushLocal(slot);

        
        
        frame.push(Int32Value(amt));

        
        
        
        jsop_binary(JSOP_SUB, stubs::Sub);

        
        
        frame.storeLocal(slot, popped);

        if (popped)
            frame.pop();
    } else {
        int amt = (op == JSOP_LOCALINC || op == JSOP_INCLOCAL) ? 1 : -1;

        
        
        frame.pushLocal(slot);

        
        
        jsop_pos();

        
        
        frame.dup();

        
        
        frame.push(Int32Value(amt));

        
        
        jsop_binary(JSOP_ADD, stubs::Add);

        
        
        frame.storeLocal(slot, true);

        
        
        frame.pop();
    }
}

void
mjit::Compiler::jsop_arginc(JSOp op, uint32 slot, bool popped)
{
    if (popped || (op == JSOP_INCARG || op == JSOP_DECARG)) {
        int amt = (op == JSOP_ARGINC || op == JSOP_INCARG) ? -1 : 1;

        
        
        frame.pushArg(slot);

        
        
        frame.push(Int32Value(amt));

        
        
        
        jsop_binary(JSOP_SUB, stubs::Sub);

        
        
        frame.storeArg(slot, popped);

        if (popped)
            frame.pop();
    } else {
        int amt = (op == JSOP_ARGINC || op == JSOP_INCARG) ? 1 : -1;

        
        
        frame.pushArg(slot);

        
        
        jsop_pos();

        
        
        frame.dup();

        
        
        frame.push(Int32Value(amt));

        
        
        jsop_binary(JSOP_ADD, stubs::Add);

        
        
        frame.storeArg(slot, true);

        
        
        frame.pop();
    }
}

static inline bool
IsCacheableSetElem(FrameEntry *obj, FrameEntry *id, FrameEntry *value)
{
    if (obj->isNotType(JSVAL_TYPE_OBJECT))
        return false;
    if (id->isNotType(JSVAL_TYPE_INT32))
        return false;
    if (id->isConstant() && id->getValue().toInt32() < 0)
        return false;

    
    
    
    if (obj->hasSameBacking(id))
        return false;

    return true;
}

bool
mjit::Compiler::jsop_setelem(bool popGuaranteed)
{
    FrameEntry *obj = frame.peek(-3);
    FrameEntry *id = frame.peek(-2);
    FrameEntry *value = frame.peek(-1);

    if (!IsCacheableSetElem(obj, id, value)) {
        jsop_setelem_slow();
        return true;
    }

    SetElementICInfo ic = SetElementICInfo(JSOp(*PC));

    
    
    
    MaybeRegisterID pinnedValueType = frame.maybePinType(value);
    MaybeRegisterID pinnedValueData = frame.maybePinData(value);

    
    MaybeRegisterID pinnedObjData;
    if (!obj->hasSameBacking(value))
        pinnedObjData = frame.maybePinData(obj);

    
    MaybeRegisterID pinnedIdData;
    if (!id->hasSameBacking(value))
        pinnedIdData = frame.maybePinData(id);

    
    
    
    
    
    
 
    
    if (!obj->isTypeKnown()) {
        Jump j = frame.testObject(Assembler::NotEqual, obj);
        stubcc.linkExit(j, Uses(3));
    }

    
    if (!id->isTypeKnown()) {
        Jump j = frame.testInt32(Assembler::NotEqual, id);
        stubcc.linkExit(j, Uses(3));
    }

    
    
    
    
    frame.maybeUnpinReg(pinnedObjData);
    if (obj->hasSameBacking(value) && pinnedValueData.isSet()) {
        ic.objReg = frame.allocReg();
        masm.move(pinnedValueData.reg(), ic.objReg);
    } else {
        ic.objReg = frame.copyDataIntoReg(obj);
    }

    
    
    frame.maybeUnpinReg(pinnedValueType);
    frame.maybeUnpinReg(pinnedValueData);
    frame.pinEntry(value, ic.vr);

    
    
    
    
    
    frame.maybeUnpinReg(pinnedIdData);
    if (id->isConstant())
        ic.key = Int32Key::FromConstant(id->getValue().toInt32());
    else
        ic.key = Int32Key::FromRegister(frame.tempRegForData(id));

    
    frame.unpinEntry(ic.vr);

    
    
    ic.objRemat = frame.dataRematInfo(obj);

    
    RESERVE_IC_SPACE(masm);
    ic.fastPathStart = masm.label();

    
    
    RESERVE_OOL_SPACE(stubcc.masm);
    ic.slowPathStart = stubcc.syncExit(Uses(3));

    
    ic.claspGuard = masm.testObjClass(Assembler::NotEqual, ic.objReg, &js_ArrayClass);
    stubcc.linkExitDirect(ic.claspGuard, ic.slowPathStart);

    
    Jump capacityGuard = masm.guardArrayCapacity(ic.objReg, ic.key);
    stubcc.linkExitDirect(capacityGuard, ic.slowPathStart);

    
    masm.loadPtr(Address(ic.objReg, offsetof(JSObject, slots)), ic.objReg);

    
    if (ic.key.isConstant()) {
        Address slot(ic.objReg, ic.key.index() * sizeof(Value));
        ic.holeGuard = masm.guardNotHole(slot);
        masm.storeValue(ic.vr, slot);
    } else {
        BaseIndex slot(ic.objReg, ic.key.reg(), Assembler::JSVAL_SCALE);
        ic.holeGuard = masm.guardNotHole(slot);
        masm.storeValue(ic.vr, slot);
    }
    stubcc.linkExitDirect(ic.holeGuard, ic.slowPathStart);

    stubcc.leave();
#if defined JS_POLYIC
    passICAddress(&ic);
    ic.slowPathCall = OOL_STUBCALL(STRICT_VARIANT(ic::SetElement));
#else
    OOL_STUBCALL(STRICT_VARIANT(stubs::SetElem));
#endif

    ic.fastPathRejoin = masm.label();

    
    
    
    
    ic.volatileMask = frame.regsInUse();

    
    
    
    
    
    
    
    
    if (popGuaranteed &&
        !ic.vr.isConstant() &&
        !value->isCopy() &&
        !frame.haveSameBacking(value, obj) &&
        !frame.haveSameBacking(value, id))
    {
        ic.volatileMask &= ~Registers::maskReg(ic.vr.dataReg());
        if (!ic.vr.isTypeKnown())
            ic.volatileMask &= ~Registers::maskReg(ic.vr.typeReg());
    } else if (!ic.vr.isConstant()) {
        ic.volatileMask |= Registers::maskReg(ic.vr.dataReg());
    }

    frame.freeReg(ic.objReg);
    frame.shimmy(2);
    stubcc.rejoin(Changes(2));

#if defined JS_POLYIC
    if (!setElemICs.append(ic))
        return false;
#endif

    return true;
}

static inline bool
IsCacheableGetElem(FrameEntry *obj, FrameEntry *id)
{
    if (obj->isTypeKnown() && obj->getKnownType() != JSVAL_TYPE_OBJECT)
        return false;
    if (id->isTypeKnown() &&
        !(id->getKnownType() == JSVAL_TYPE_INT32
#if defined JS_POLYIC
          || id->getKnownType() == JSVAL_TYPE_STRING
#endif
         )) {
        return false;
    }

    if (id->isTypeKnown() && id->getKnownType() == JSVAL_TYPE_INT32 && id->isConstant() &&
        id->getValue().toInt32() < 0) {
        return false;
    }

    
    if (obj->hasSameBacking(id))
        return false;

    return true;
}

bool
mjit::Compiler::jsop_getelem(bool isCall)
{
    FrameEntry *obj = frame.peek(-2);
    FrameEntry *id = frame.peek(-1);

    if (!IsCacheableGetElem(obj, id)) {
        if (isCall)
            jsop_callelem_slow();
        else
            jsop_getelem_slow();
        return true;
    }

    GetElementICInfo ic = GetElementICInfo(JSOp(*PC));

    
    MaybeRegisterID pinnedIdData = frame.maybePinData(id);
    MaybeRegisterID pinnedIdType = frame.maybePinType(id);

    MaybeJump objTypeGuard;
    if (!obj->isTypeKnown()) {
        
        MaybeRegisterID pinnedObjData = frame.maybePinData(obj);
        Jump guard = frame.testObject(Assembler::NotEqual, obj);
        frame.maybeUnpinReg(pinnedObjData);

        
        
        
        
        stubcc.linkExit(guard, Uses(2));
        objTypeGuard = stubcc.masm.jump();
    }

    
    ic.objReg = frame.copyDataIntoReg(obj);

    
    
    MaybeRegisterID thisReg;
    if (isCall && id->mightBeType(JSVAL_TYPE_INT32)) {
        thisReg = frame.allocReg();
        masm.move(ic.objReg, thisReg.reg());
    }

    
    
    
    frame.maybeUnpinReg(pinnedIdType);
    if (id->isConstant() || id->isTypeKnown())
        ic.typeReg = frame.allocReg();
    else
        ic.typeReg = frame.copyTypeIntoReg(id);

    
    frame.maybeUnpinReg(pinnedIdData);
    if (id->isConstant()) {
        ic.id = ValueRemat::FromConstant(id->getValue());
    } else {
        RegisterID dataReg = frame.tempRegForData(id);
        if (id->isTypeKnown())
            ic.id = ValueRemat::FromKnownType(id->getKnownType(), dataReg);
        else
            ic.id = ValueRemat::FromRegisters(ic.typeReg, dataReg);
    }

    RESERVE_IC_SPACE(masm);
    ic.fastPathStart = masm.label();

    
    RESERVE_OOL_SPACE(stubcc.masm);
    ic.slowPathStart = stubcc.masm.label();
    frame.sync(stubcc.masm, Uses(2));

    if (id->mightBeType(JSVAL_TYPE_INT32)) {
        
        if (!id->isTypeKnown()) {
            ic.typeGuard = masm.testInt32(Assembler::NotEqual, ic.typeReg);
            stubcc.linkExitDirect(ic.typeGuard.get(), ic.slowPathStart);
        }

        
        ic.claspGuard = masm.testObjClass(Assembler::NotEqual, ic.objReg, &js_ArrayClass);
        stubcc.linkExitDirect(ic.claspGuard, ic.slowPathStart);

        Int32Key key = id->isConstant()
                       ? Int32Key::FromConstant(id->getValue().toInt32())
                       : Int32Key::FromRegister(ic.id.dataReg());

        Assembler::FastArrayLoadFails fails =
            masm.fastArrayLoad(ic.objReg, key, ic.typeReg, ic.objReg);

        
        
        if (isCall) {
            Address thisSlot = frame.addressOf(id);
            masm.storeValueFromComponents(ImmType(JSVAL_TYPE_OBJECT), thisReg.reg(), thisSlot);
            frame.freeReg(thisReg.reg());
        }

        stubcc.linkExitDirect(fails.rangeCheck, ic.slowPathStart);
        stubcc.linkExitDirect(fails.holeCheck, ic.slowPathStart);
    } else {
        
        
        ic.claspGuard = masm.jump();
        stubcc.linkExitDirect(ic.claspGuard, ic.slowPathStart);
    }

    stubcc.leave();
    if (objTypeGuard.isSet())
        objTypeGuard.get().linkTo(stubcc.masm.label(), &stubcc.masm);
#ifdef JS_POLYIC
    passICAddress(&ic);
    if (isCall)
        ic.slowPathCall = OOL_STUBCALL(ic::CallElement);
    else
        ic.slowPathCall = OOL_STUBCALL(ic::GetElement);
#else
    if (isCall)
        ic.slowPathCall = OOL_STUBCALL(stubs::CallElem);
    else
        ic.slowPathCall = OOL_STUBCALL(stubs::GetElem);
#endif

    ic.fastPathRejoin = masm.label();

    frame.popn(2);
    frame.pushRegs(ic.typeReg, ic.objReg);
    if (isCall)
        frame.pushSynced();

    stubcc.rejoin(Changes(2));

#ifdef JS_POLYIC
    if (!getElemICs.append(ic))
        return false;
#endif

    return true;
}

static inline bool
ReallySimpleStrictTest(FrameEntry *fe)
{
    if (!fe->isTypeKnown())
        return false;
    JSValueType type = fe->getKnownType();
    return type == JSVAL_TYPE_NULL || type == JSVAL_TYPE_UNDEFINED;
}

static inline bool
BooleanStrictTest(FrameEntry *fe)
{
    return fe->isConstant() && fe->getKnownType() == JSVAL_TYPE_BOOLEAN;
}

void
mjit::Compiler::jsop_stricteq(JSOp op)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    Assembler::Condition cond = (op == JSOP_STRICTEQ) ? Assembler::Equal : Assembler::NotEqual;

    




    
    if (lhs->isConstant() && rhs->isConstant()) {
        JSBool b;
        StrictlyEqual(cx, lhs->getValue(), rhs->getValue(), &b);
        frame.popn(2);
        frame.push(BooleanValue((op == JSOP_STRICTEQ) ? b : !b));
        return;
    }

    if (frame.haveSameBacking(lhs, rhs)) {
        
        if (lhs->isTypeKnown() && lhs->isNotType(JSVAL_TYPE_DOUBLE)) {
            frame.popn(2);
            frame.push(BooleanValue(op == JSOP_STRICTEQ));
            return;
        }
        
        
        RegisterID result = frame.allocReg(Registers::SingleByteRegs);
        RegisterID treg = frame.copyTypeIntoReg(lhs);

        Assembler::Condition oppositeCond = (op == JSOP_STRICTEQ) ? Assembler::NotEqual : Assembler::Equal;

        
        masm.lshiftPtr(Imm32(1), treg);
#ifndef JS_CPU_X64
        static const int ShiftedCanonicalNaNType = 0x7FF80000 << 1;
#ifdef JS_CPU_SPARC
        
        masm.and32(Imm32(ShiftedCanonicalNaNType), treg);
#endif
        masm.setPtr(oppositeCond, treg, Imm32(ShiftedCanonicalNaNType), result);
#else
        static const void *ShiftedCanonicalNaNType = (void *)(0x7FF8000000000000 << 1);
        masm.move(ImmPtr(ShiftedCanonicalNaNType), Registers::ScratchReg);
        masm.setPtr(oppositeCond, treg, Registers::ScratchReg, result);
#endif
        frame.freeReg(treg);

        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
        return;
    }

    
    bool lhsTest;
    if ((lhsTest = ReallySimpleStrictTest(lhs)) || ReallySimpleStrictTest(rhs)) {
        FrameEntry *test = lhsTest ? rhs : lhs;
        FrameEntry *known = lhsTest ? lhs : rhs;

        if (test->isTypeKnown()) {
            frame.popn(2);
            frame.push(BooleanValue((test->getKnownType() == known->getKnownType()) ==
                                  (op == JSOP_STRICTEQ)));
            return;
        }

        
        RegisterID result = frame.allocReg(Registers::SingleByteRegs);
#ifndef JS_CPU_X64
        JSValueTag mask = known->getKnownTag();
        if (frame.shouldAvoidTypeRemat(test))
            masm.set32(cond, masm.tagOf(frame.addressOf(test)), Imm32(mask), result);
        else
            masm.set32(cond, frame.tempRegForType(test), Imm32(mask), result);
#else
        RegisterID maskReg = frame.allocReg();
        masm.move(ImmTag(known->getKnownTag()), maskReg);

        RegisterID r = frame.tempRegForType(test);
        masm.setPtr(cond, r, maskReg, result);

        frame.freeReg(maskReg);
#endif
        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
        return;
    }

    
    if ((lhsTest = BooleanStrictTest(lhs)) || BooleanStrictTest(rhs)) {
        FrameEntry *test = lhsTest ? rhs : lhs;

        if (test->isTypeKnown() && test->isNotType(JSVAL_TYPE_BOOLEAN)) {
            frame.popn(2);
            frame.push(BooleanValue(op == JSOP_STRICTNE));
            return;
        }

        if (test->isConstant()) {
            frame.popn(2);
            const Value &L = lhs->getValue();
            const Value &R = rhs->getValue();
            frame.push(BooleanValue((L.toBoolean() == R.toBoolean()) == (op == JSOP_STRICTEQ)));
            return;
        }

        RegisterID data = frame.copyDataIntoReg(test);

        RegisterID result = data;
        if (!(Registers::maskReg(data) & Registers::SingleByteRegs))
            result = frame.allocReg(Registers::SingleByteRegs);
        
        Jump notBoolean;
        if (!test->isTypeKnown())
           notBoolean = frame.testBoolean(Assembler::NotEqual, test);

        
        bool val = lhsTest ? lhs->getValue().toBoolean() : rhs->getValue().toBoolean();
        masm.set32(cond, data, Imm32(val), result);

        if (!test->isTypeKnown()) {
            Jump done = masm.jump();
            notBoolean.linkTo(masm.label(), &masm);
            masm.move(Imm32((op == JSOP_STRICTNE)), result);
            done.linkTo(masm.label(), &masm);
        }

        if (data != result)
            frame.freeReg(data);

        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
        return;
    }

    
    if ((lhs->isTypeKnown() && lhs->isNotType(JSVAL_TYPE_INT32)) ||
        (rhs->isTypeKnown() && rhs->isNotType(JSVAL_TYPE_INT32))) {
        prepareStubCall(Uses(2));

        if (op == JSOP_STRICTEQ)
            INLINE_STUBCALL(stubs::StrictEq);
        else
            INLINE_STUBCALL(stubs::StrictNe);

        frame.popn(2);
        frame.pushSyncedType(JSVAL_TYPE_BOOLEAN);
        return;
    }

#if !defined JS_CPU_ARM && !defined JS_CPU_SPARC
    
    bool needStub = false;
    if (!lhs->isTypeKnown()) {
        Jump j = frame.testInt32(Assembler::NotEqual, lhs);
        stubcc.linkExit(j, Uses(2));
        needStub = true;
    }

    if (!rhs->isTypeKnown() && !frame.haveSameBacking(lhs, rhs)) {
        Jump j = frame.testInt32(Assembler::NotEqual, rhs);
        stubcc.linkExit(j, Uses(2));
        needStub = true;
    }

    FrameEntry *test  = lhs->isConstant() ? rhs : lhs;
    FrameEntry *other = lhs->isConstant() ? lhs : rhs;

    
    RegisterID resultReg = Registers::ReturnReg;
    frame.takeReg(resultReg);
    RegisterID testReg = frame.tempRegForData(test);
    frame.pinReg(testReg);

    JS_ASSERT(resultReg != testReg);

    
    if (other->isConstant()) {
        masm.set32(cond, testReg, Imm32(other->getValue().toInt32()), resultReg);
    } else if (frame.shouldAvoidDataRemat(other)) {
        masm.set32(cond, testReg, frame.addressOf(other), resultReg);
    } else {
        RegisterID otherReg = frame.tempRegForData(other);

        JS_ASSERT(otherReg != resultReg);
        JS_ASSERT(otherReg != testReg);

        masm.set32(cond, testReg, otherReg, resultReg);
    }

    frame.unpinReg(testReg);

    if (needStub) {
        stubcc.leave();
        if (op == JSOP_STRICTEQ)
            OOL_STUBCALL(stubs::StrictEq);
        else
            OOL_STUBCALL(stubs::StrictNe);
    }

    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, resultReg);

    if (needStub)
        stubcc.rejoin(Changes(1));
#else
    
    prepareStubCall(Uses(2));

    if (op == JSOP_STRICTEQ)
        INLINE_STUBCALL(stubs::StrictEq);
    else
        INLINE_STUBCALL(stubs::StrictNe);

    frame.popn(2);
    frame.pushSyncedType(JSVAL_TYPE_BOOLEAN);
    return;
#endif
}

void
mjit::Compiler::jsop_pos()
{
    FrameEntry *top = frame.peek(-1);

    if (top->isTypeKnown()) {
        if (top->getKnownType() <= JSVAL_TYPE_INT32)
            return;
        prepareStubCall(Uses(1));
        INLINE_STUBCALL(stubs::Pos);
        frame.pop();
        frame.pushSynced();
        return;
    }

    frame.giveOwnRegs(top);

    Jump j;
    if (frame.shouldAvoidTypeRemat(top))
        j = masm.testNumber(Assembler::NotEqual, frame.addressOf(top));
    else
        j = masm.testNumber(Assembler::NotEqual, frame.tempRegForType(top));
    stubcc.linkExit(j, Uses(1));

    stubcc.leave();
    OOL_STUBCALL(stubs::Pos);

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_initmethod()
{
#ifdef DEBUG
    FrameEntry *obj = frame.peek(-2);
#endif
    JSAtom *atom = script->getAtom(fullAtomIndex(PC));

    
    JS_ASSERT(!obj->initializerObject());

    prepareStubCall(Uses(2));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::InitMethod);
}

void
mjit::Compiler::jsop_initprop()
{
    FrameEntry *obj = frame.peek(-2);
    FrameEntry *fe = frame.peek(-1);
    JSAtom *atom = script->getAtom(fullAtomIndex(PC));

    JSObject *baseobj = obj->initializerObject();

    if (!baseobj) {
        prepareStubCall(Uses(2));
        masm.move(ImmPtr(atom), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::InitProp);
        return;
    }

    JSObject *holder;
    JSProperty *prop = NULL;
#ifdef DEBUG
    int res =
#endif
    js_LookupPropertyWithFlags(cx, baseobj, ATOM_TO_JSID(atom),
                               JSRESOLVE_QUALIFIED, &holder, &prop);
    JS_ASSERT(res >= 0 && prop && holder == baseobj);

    RegisterID objReg = frame.copyDataIntoReg(obj);
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);

    
    Shape *shape = (Shape *) prop;
    frame.storeTo(fe, Address(objReg, shape->slot * sizeof(Value)));
    frame.freeReg(objReg);
}

void
mjit::Compiler::jsop_initelem()
{
    FrameEntry *obj = frame.peek(-3);
    FrameEntry *id = frame.peek(-2);
    FrameEntry *fe = frame.peek(-1);

    






    if (!id->isConstant() || !obj->initializerArray()) {
        JSOp next = JSOp(PC[JSOP_INITELEM_LENGTH]);

        prepareStubCall(Uses(3));
        masm.move(Imm32(next == JSOP_ENDINIT ? 1 : 0), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::InitElem);
        return;
    }

    JS_ASSERT(id->getValue().isInt32());

    if (fe->isConstant() && fe->getValue().isMagic(JS_ARRAY_HOLE)) {
        
        return;
    }

    RegisterID objReg = frame.copyDataIntoReg(obj);
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);

    
    frame.storeTo(fe, Address(objReg, id->getValue().toInt32() * sizeof(Value)));
    frame.freeReg(objReg);
}
