






































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

void
mjit::Compiler::ensureInteger(FrameEntry *fe, Uses uses)
{
    if (fe->isConstant()) {
        if (!fe->isType(JSVAL_TYPE_INT32)) {
            JS_ASSERT(fe->isType(JSVAL_TYPE_DOUBLE));
            fe->convertConstantDoubleToInt32(cx);
        }
    } else if (fe->isType(JSVAL_TYPE_DOUBLE)) {
        FPRegisterID fpreg = frame.tempFPRegForData(fe);
        FPRegisterID fptemp = frame.allocFPReg();
        RegisterID data = frame.allocReg();
        Jump truncateGuard = masm.branchTruncateDoubleToInt32(fpreg, data);

        Label syncPath = stubcc.syncExitAndJump(uses);
        stubcc.linkExitDirect(truncateGuard, stubcc.masm.label());

        






        stubcc.masm.zeroDouble(fptemp);
        Jump positive = stubcc.masm.branchDouble(Assembler::DoubleGreaterThan, fpreg, fptemp);
        stubcc.masm.slowLoadConstantDouble(double(4294967296.0), fptemp);
        Jump skip = stubcc.masm.jump();
        positive.linkTo(stubcc.masm.label(), &stubcc.masm);
        stubcc.masm.slowLoadConstantDouble(double(-4294967296.0), fptemp);
        skip.linkTo(stubcc.masm.label(), &stubcc.masm);

        JumpList isDouble;
        stubcc.masm.addDouble(fpreg, fptemp);
        stubcc.masm.branchConvertDoubleToInt32(fptemp, data, isDouble, Registers::FPConversionTemp);
        stubcc.crossJump(stubcc.masm.jump(), masm.label());
        isDouble.linkTo(syncPath, &stubcc.masm);

        frame.freeReg(fptemp);
        frame.learnType(fe, JSVAL_TYPE_INT32, data);
    } else if (!fe->isType(JSVAL_TYPE_INT32)) {
        RegisterID typeReg = frame.tempRegForType(fe);
        frame.pinReg(typeReg);
        RegisterID dataReg = frame.copyDataIntoReg(fe);
        frame.unpinReg(typeReg);

        Jump intGuard = masm.testInt32(Assembler::NotEqual, typeReg);

        Label syncPath = stubcc.syncExitAndJump(uses);
        stubcc.linkExitDirect(intGuard, stubcc.masm.label());

        
        Jump doubleGuard = stubcc.masm.testDouble(Assembler::NotEqual, typeReg);
        doubleGuard.linkTo(syncPath, &stubcc.masm);

        frame.loadDouble(fe, Registers::FPConversionTemp, stubcc.masm);
        Jump truncateGuard = stubcc.masm.branchTruncateDoubleToInt32(Registers::FPConversionTemp, dataReg);
        truncateGuard.linkTo(syncPath, &stubcc.masm);
        stubcc.crossJump(stubcc.masm.jump(), masm.label());

        frame.learnType(fe, JSVAL_TYPE_INT32, dataReg);
    }
}

void
mjit::Compiler::jsop_bitnot()
{
    REJOIN_SITE_ANY();

    FrameEntry *top = frame.peek(-1);

    
    if (top->isNotType(JSVAL_TYPE_INT32) && top->isNotType(JSVAL_TYPE_DOUBLE)) {
        prepareStubCall(Uses(1));
        INLINE_STUBCALL(stubs::BitNot);
        frame.pop();
        frame.pushSynced(JSVAL_TYPE_INT32);
        return;
    }

    ensureInteger(top, Uses(1));

    stubcc.leave();
    OOL_STUBCALL(stubs::BitNot);

    RegisterID reg = frame.ownRegForData(top);
    masm.not32(reg);
    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_bitop(JSOp op)
{
    REJOIN_SITE_ANY();

    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    frame.separateBinaryEntries(lhs, rhs);

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
        ensureInteger(lhs, Uses(2));
        RegisterID reg = frame.copyDataIntoReg(lhs);

        stubcc.leave();
        OOL_STUBCALL(stub);

        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

        stubcc.rejoin(Changes(1));
        return;
    }

    
    if (rhs->isConstant() && rhs->getValue().isDouble())
        rhs->convertConstantDoubleToInt32(cx);

    
    if ((lhs->isNotType(JSVAL_TYPE_INT32) && lhs->isNotType(JSVAL_TYPE_DOUBLE)) ||
        (rhs->isNotType(JSVAL_TYPE_INT32) && rhs->isNotType(JSVAL_TYPE_DOUBLE)) ||
        (op == JSOP_URSH && rhs->isConstant() && rhs->getValue().toInt32() % 32 == 0)) {
        prepareStubCall(Uses(2));
        INLINE_STUBCALL(stub);
        frame.popn(2);
        frame.pushSynced(op != JSOP_URSH ? JSVAL_TYPE_INT32 : knownPushedType(0));
        return;
    }

    ensureInteger(lhs, Uses(2));
    ensureInteger(rhs, Uses(2));

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
            frame.push(Int32Value(L << (R & 31)));
            return;
          case JSOP_RSH:
            frame.push(Int32Value(L >> (R & 31)));
            return;
          case JSOP_URSH:
          {
            uint32 unsignedL;
            ValueToECMAUint32(cx, Int32Value(L), (uint32_t*)&unsignedL);  
            Value v = NumberValue(uint32(unsignedL >> (R & 31)));
            JS_ASSERT(v.isInt32());
            frame.push(v);
            return;
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
      case JSOP_RSH:
      case JSOP_URSH:
      {
        
        if (rhs->isConstant()) {
            RegisterID reg = frame.ownRegForData(lhs);
            int shift = rhs->getValue().toInt32() & 0x1F;

            stubcc.leave();
            OOL_STUBCALL(stub);

            if (shift) {
                if (op == JSOP_LSH)
                    masm.lshift32(Imm32(shift), reg);
                else if (op == JSOP_RSH)
                    masm.rshift32(Imm32(shift), reg);
                else
                    masm.urshift32(Imm32(shift), reg);
            }
            frame.popn(2);
            
            
            JS_ASSERT_IF(op == JSOP_URSH, shift >= 1);
            frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

            stubcc.rejoin(Changes(1));
            return;
        }
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
        
        RegisterID rr = frame.tempRegInMaskForData(rhs,
                                                   Registers::maskReg(JSC::X86Registers::ecx)).reg();
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
        } else if (op == JSOP_RSH) {
            masm.rshift32(rr, reg);
        } else {
            masm.urshift32(rr, reg);
            
            Jump isNegative = masm.branch32(Assembler::LessThan, reg, Imm32(0));
            stubcc.linkExit(isNegative, Uses(2));
        }
        break;
      }

      default:
        JS_NOT_REACHED("NYI");
        return;
    }

    stubcc.leave();
    OOL_STUBCALL(stub);

    frame.pop();
    frame.pop();

    JSValueType type = knownPushedType(0);

    if (type != JSVAL_TYPE_UNKNOWN && type != JSVAL_TYPE_DOUBLE)
        frame.pushTypedPayload(type, reg);
    else if (op == JSOP_URSH)
        frame.pushNumber(reg, true);
    else
        frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

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
mjit::Compiler::jsop_equality(JSOp op, BoolStub stub, AutoRejoinSite &autoRejoin, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    JS_ASSERT(!(rhs->isConstant() && lhs->isConstant()));

    bool lhsTest;
    if ((lhsTest = CheckNullOrUndefined(lhs)) || CheckNullOrUndefined(rhs)) {
        
        FrameEntry *test = lhsTest ? rhs : lhs;

        if (test->isTypeKnown())
            return emitStubCmpOp(stub, autoRejoin, target, fused);

        
        RegisterID reg = frame.ownRegForType(test);
        frame.pop();
        frame.pop();

        




        if (target) {
            fixDoubleTypes();
            frame.syncAndKillEverything();
            frame.freeReg(reg);

            autoRejoin.oolRejoin(stubcc.masm.label());
            Jump sj = stubcc.masm.branchTest32(GetStubCompareCondition(fused),
                                               Registers::ReturnReg, Registers::ReturnReg);

            if ((op == JSOP_EQ && fused == JSOP_IFNE) ||
                (op == JSOP_NE && fused == JSOP_IFEQ)) {
                






                Jump b1 = masm.branchPtr(Assembler::Equal, reg, ImmType(JSVAL_TYPE_UNDEFINED));
                Jump b2 = masm.branchPtr(Assembler::Equal, reg, ImmType(JSVAL_TYPE_NULL));
                Jump j1 = masm.jump();
                b1.linkTo(masm.label(), &masm);
                b2.linkTo(masm.label(), &masm);
                Jump j2 = masm.jump();
                if (!jumpAndTrace(j2, target, &sj))
                    return false;
                j1.linkTo(masm.label(), &masm);
            } else {
                Jump j = masm.branchPtr(Assembler::Equal, reg, ImmType(JSVAL_TYPE_UNDEFINED));
                Jump j2 = masm.branchPtr(Assembler::NotEqual, reg, ImmType(JSVAL_TYPE_NULL));
                if (!jumpAndTrace(j2, target, &sj))
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

    if (cx->typeInferenceEnabled() &&
        lhs->isType(JSVAL_TYPE_OBJECT) && rhs->isType(JSVAL_TYPE_OBJECT)) {
        




        types::TypeSet *lhsTypes = frame.extra(lhs).types;
        types::TypeSet *rhsTypes = frame.extra(rhs).types;
        types::ObjectKind lhsKind =
            lhsTypes ? lhsTypes->getKnownObjectKind(cx) : types::OBJECT_UNKNOWN;
        types::ObjectKind rhsKind =
            rhsTypes ? rhsTypes->getKnownObjectKind(cx) : types::OBJECT_UNKNOWN;

        if (lhsKind != types::OBJECT_UNKNOWN && rhsKind != types::OBJECT_UNKNOWN) {
            
            JS_ASSERT_IF(!target, fused != JSOP_IFEQ);
            frame.forgetMismatchedObject(lhs);
            frame.forgetMismatchedObject(rhs);
            Assembler::Condition cond = GetCompareCondition(op, fused);
            if (target) {
                fixDoubleTypes();
                autoRejoin.oolRejoin(stubcc.masm.label());
                Jump sj = stubcc.masm.branchTest32(GetStubCompareCondition(fused),
                                                   Registers::ReturnReg, Registers::ReturnReg);
                if (!frame.syncForBranch(target, Uses(2)))
                    return false;
                RegisterID lreg = frame.tempRegForData(lhs);
                frame.pinReg(lreg);
                RegisterID rreg = frame.tempRegForData(rhs);
                frame.unpinReg(lreg);
                Jump fast = masm.branchPtr(cond, lreg, rreg);
                frame.popn(2);
                return jumpAndTrace(fast, target, &sj);
            } else {
                RegisterID result = frame.allocReg();
                RegisterID lreg = frame.tempRegForData(lhs);
                frame.pinReg(lreg);
                RegisterID rreg = frame.tempRegForData(rhs);
                frame.unpinReg(lreg);
                masm.branchValue(cond, lreg, rreg, result);

                frame.popn(2);
                frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
                return true;
            }
        }
    }

    return emitStubCmpOp(stub, autoRejoin, target, fused);
}

bool
mjit::Compiler::jsop_relational(JSOp op, BoolStub stub, AutoRejoinSite &autoRejoin,
                                jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    JS_ASSERT(!(rhs->isConstant() && lhs->isConstant()));

    
    if ((lhs->isNotType(JSVAL_TYPE_INT32) && lhs->isNotType(JSVAL_TYPE_DOUBLE) &&
         lhs->isNotType(JSVAL_TYPE_STRING)) ||
        (rhs->isNotType(JSVAL_TYPE_INT32) && rhs->isNotType(JSVAL_TYPE_DOUBLE) &&
         rhs->isNotType(JSVAL_TYPE_STRING))) {
        if (op == JSOP_EQ || op == JSOP_NE)
            return jsop_equality(op, stub, autoRejoin, target, fused);
        return emitStubCmpOp(stub, autoRejoin, target, fused);
    }

    if (op == JSOP_EQ || op == JSOP_NE) {
        if ((lhs->isNotType(JSVAL_TYPE_INT32) && lhs->isNotType(JSVAL_TYPE_STRING)) ||
            (rhs->isNotType(JSVAL_TYPE_INT32) && rhs->isNotType(JSVAL_TYPE_STRING))) {
            return emitStubCmpOp(stub, autoRejoin, target, fused);
        } else if (!target && (lhs->isType(JSVAL_TYPE_STRING) || rhs->isType(JSVAL_TYPE_STRING))) {
            return emitStubCmpOp(stub, autoRejoin, target, fused);
        } else if (frame.haveSameBacking(lhs, rhs)) {
            return emitStubCmpOp(stub, autoRejoin, target, fused);
        } else {
            return jsop_equality_int_string(op, stub, autoRejoin, target, fused);
        }
    }

    if (frame.haveSameBacking(lhs, rhs)) {
        return emitStubCmpOp(stub, autoRejoin, target, fused);
    } else if (lhs->isType(JSVAL_TYPE_STRING) || rhs->isType(JSVAL_TYPE_STRING)) {
        return emitStubCmpOp(stub, autoRejoin, target, fused);
    } else if (lhs->isType(JSVAL_TYPE_DOUBLE) || rhs->isType(JSVAL_TYPE_DOUBLE)) {
        return jsop_relational_double(op, stub, autoRejoin, target, fused);
    } else if (cx->typeInferenceEnabled() &&
               lhs->isType(JSVAL_TYPE_INT32) && rhs->isType(JSVAL_TYPE_INT32)) {
        return jsop_relational_int(op, autoRejoin, target, fused);
    } else {
        return jsop_relational_full(op, stub, autoRejoin, target, fused);
    }
}

void
mjit::Compiler::jsop_not()
{
    REJOIN_SITE_ANY();

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
            RegisterID data = frame.allocReg(Registers::SingleByteRegs).reg();
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
            RegisterID reg = frame.allocReg();
            masm.move(Imm32(0), reg);

            frame.pop();
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, reg);
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

    RegisterID data = frame.allocReg(Registers::SingleByteRegs).reg();
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
            RegisterID reg = frame.allocReg();
            masm.move(ImmPtr(atom), reg);
            frame.pop();
            frame.pushTypedPayload(JSVAL_TYPE_STRING, reg);
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

                RegisterID result = frame.allocReg(Registers::SingleByteRegs).reg();

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
    INLINE_STUBCALL_NO_REJOIN(stubs::TypeOf);
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
    if (!fe->isType(JSVAL_TYPE_DOUBLE))
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

    



    if (!fe->isType(JSVAL_TYPE_DOUBLE))
        jmpNotExecScript.setJump(masm.branchTest32(ncond, data.reg(), data.reg()));
    Label lblExecScript = masm.label();
    Jump j = masm.jump();


    
    MaybeJump jmpCvtExecScript;
    MaybeJump jmpCvtRejoin;
    Label lblCvtPath = stubcc.masm.label();

    if (!fe->isTypeKnown() ||
        !(fe->isType(JSVAL_TYPE_BOOLEAN) || fe->isType(JSVAL_TYPE_INT32))) {
        
        stubcc.masm.infallibleVMCall(JS_FUNC_TO_DATA_PTR(void *, stubs::ValueToBoolean),
                                     frame.totalDepth());

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
            if (!frame.syncForBranch(target, Uses(0)))
                return false;
            if (!jumpAndTrace(masm.jump(), target))
                return false;
        } else {
            if (target < PC && !finishLoop(target))
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
            fixDoubleTypes();
            if (!frame.syncForBranch(target, Uses(0)))
                return false;
            if (!jumpAndTrace(masm.jump(), target))
                return false;
        }

        frame.pop();
        return true;
    }

    return booleanJumpScript(op, target);
}

bool
mjit::Compiler::jsop_localinc(JSOp op, uint32 slot, bool popped)
{
    JSValueType type = knownLocalType(slot);

    if (popped || (op == JSOP_INCLOCAL || op == JSOP_DECLOCAL)) {
        int amt = (op == JSOP_LOCALINC || op == JSOP_INCLOCAL) ? -1 : 1;

        
        
        frame.pushLocal(slot, type);

        
        
        frame.push(Int32Value(amt));

        
        
        
        if (!jsop_binary(JSOP_SUB, stubs::Sub, type, localTypeSet(slot)))
            return false;

        
        
        frame.storeLocal(slot, type, popped, true);

        if (popped)
            frame.pop();
    } else {
        int amt = (op == JSOP_LOCALINC || op == JSOP_INCLOCAL) ? 1 : -1;

        
        
        frame.pushLocal(slot, type);

        
        
        jsop_pos();

        
        
        frame.dup();

        
        
        frame.push(Int32Value(amt));

        
        
        if (!jsop_binary(JSOP_ADD, stubs::Add, type, localTypeSet(slot)))
            return false;

        
        
        frame.storeLocal(slot, type, true, true);

        
        
        frame.pop();
    }

    return true;
}

bool
mjit::Compiler::jsop_arginc(JSOp op, uint32 slot, bool popped)
{
    JSValueType type = knownArgumentType(slot);

    if (popped || (op == JSOP_INCARG || op == JSOP_DECARG)) {
        int amt = (op == JSOP_ARGINC || op == JSOP_INCARG) ? -1 : 1;

        
        
        frame.pushArg(slot, type);

        
        
        frame.push(Int32Value(amt));

        
        
        
        if (!jsop_binary(JSOP_SUB, stubs::Sub, type, argTypeSet(slot)))
            return false;

        
        
        frame.storeArg(slot, type, popped);

        if (popped)
            frame.pop();
    } else {
        int amt = (op == JSOP_ARGINC || op == JSOP_INCARG) ? 1 : -1;

        
        
        frame.pushArg(slot, type);

        
        
        jsop_pos();

        
        
        frame.dup();

        
        
        frame.push(Int32Value(amt));

        
        
        if (!jsop_binary(JSOP_ADD, stubs::Add, type, argTypeSet(slot)))
            return false;

        
        
        frame.storeArg(slot, type, true);

        
        
        frame.pop();
    }

    return true;
}

static inline bool
IsCacheableSetElem(FrameEntry *obj, FrameEntry *id, FrameEntry *value)
{
    if (obj->isNotType(JSVAL_TYPE_OBJECT))
        return false;
    if (id->isNotType(JSVAL_TYPE_INT32))
        return false;
    if (id->isConstant()) {
        if (id->getValue().toInt32() < 0)
            return false;
        if (id->getValue().toInt32() + 1 < 0)  
            return false;
    }

    
    
    
    if (obj->hasSameBacking(id))
        return false;

    return true;
}

void
mjit::Compiler::jsop_setelem_dense()
{
    FrameEntry *obj = frame.peek(-3);
    FrameEntry *id = frame.peek(-2);
    FrameEntry *value = frame.peek(-1);

    
    
    if (!obj->isTypeKnown()) {
        Jump guard = frame.testObject(Assembler::NotEqual, obj);
        stubcc.linkExit(guard, Uses(3));
    }

    
    if (!id->isTypeKnown()) {
        Jump guard = frame.testInt32(Assembler::NotEqual, id);
        stubcc.linkExit(guard, Uses(3));
    }

    

    ValueRemat vr;
    frame.pinEntry(value, vr);

    Int32Key key = id->isConstant()
                 ? Int32Key::FromConstant(id->getValue().toInt32())
                 : Int32Key::FromRegister(frame.tempRegForData(id));
    bool pinKey = !key.isConstant() && !frame.haveSameBacking(id, value);
    if (pinKey)
        frame.pinReg(key.reg());

    
    
    
    RegisterID slotsReg;
    bool hoisted = loop && !a->parent && loop->hoistArrayLengthCheck(obj, 1);

    if (hoisted) {
        FrameEntry *slotsFe = loop->invariantSlots(obj);
        slotsReg = frame.tempRegForData(slotsFe);

        frame.unpinEntry(vr);
        if (pinKey)
            frame.unpinReg(key.reg());
    } else {
        
        RegisterID objReg;
        if (frame.haveSameBacking(obj, value)) {
            objReg = frame.allocReg();
            masm.move(vr.dataReg(), objReg);
        } else if (frame.haveSameBacking(obj, id)) {
            objReg = frame.allocReg();
            masm.move(key.reg(), objReg);
        } else {
            objReg = frame.copyDataIntoReg(obj);
        }

        frame.unpinEntry(vr);
        if (pinKey)
            frame.unpinReg(key.reg());

        
        Label syncTarget = stubcc.syncExitAndJump(Uses(3));

        Jump initlenGuard = masm.guardArrayExtent(offsetof(JSObject, initializedLength),
                                                  objReg, key, Assembler::BelowOrEqual);
        stubcc.linkExitDirect(initlenGuard, stubcc.masm.label());

        
        
        Jump exactlenGuard = stubcc.masm.guardArrayExtent(offsetof(JSObject, initializedLength),
                                                          objReg, key, Assembler::NotEqual);
        exactlenGuard.linkTo(syncTarget, &stubcc.masm);

        
        Jump capacityGuard = stubcc.masm.guardArrayExtent(offsetof(JSObject, capacity),
                                                          objReg, key, Assembler::BelowOrEqual);
        capacityGuard.linkTo(syncTarget, &stubcc.masm);

        
        
        stubcc.masm.bumpKey(key, 1);

        
        stubcc.masm.storeKey(key, Address(objReg, offsetof(JSObject, initializedLength)));

        
        Jump lengthGuard = stubcc.masm.guardArrayExtent(offsetof(JSObject, privateData),
                                                        objReg, key, Assembler::AboveOrEqual);
        stubcc.masm.storeKey(key, Address(objReg, offsetof(JSObject, privateData)));
        lengthGuard.linkTo(stubcc.masm.label(), &stubcc.masm);

        
        stubcc.masm.bumpKey(key, -1);

        
        Jump initlenExit = stubcc.masm.jump();
        stubcc.crossJump(initlenExit, masm.label());

        masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);
        slotsReg = objReg;
    }

    
    
    if (key.isConstant())
        masm.storeValue(vr, Address(slotsReg, key.index() * sizeof(Value)));
    else
        masm.storeValue(vr, BaseIndex(slotsReg, key.reg(), masm.JSVAL_SCALE));

    stubcc.leave();
    OOL_STUBCALL(STRICT_VARIANT(stubs::SetElem));

    if (!hoisted)
        frame.freeReg(slotsReg);
    frame.shimmy(2);
    stubcc.rejoin(Changes(2));
}

bool
mjit::Compiler::jsop_setelem(bool popGuaranteed)
{
    REJOIN_SITE_2(STRICT_VARIANT(ic::SetElement), STRICT_VARIANT(stubs::SetElem));

    FrameEntry *obj = frame.peek(-3);
    FrameEntry *id = frame.peek(-2);
    FrameEntry *value = frame.peek(-1);

    if (!IsCacheableSetElem(obj, id, value) || monitored(PC)) {
        jsop_setelem_slow();
        return true;
    }

    frame.forgetMismatchedObject(obj);

    if (cx->typeInferenceEnabled()) {
        types::TypeSet *types = frame.extra(obj).types;
        types::ObjectKind kind = types
            ? types->getKnownObjectKind(cx)
            : types::OBJECT_UNKNOWN;
        if (id->mightBeType(JSVAL_TYPE_INT32) &&
            (kind == types::OBJECT_DENSE_ARRAY || kind == types::OBJECT_PACKED_ARRAY) &&
            !arrayPrototypeHasIndexedProperty()) {
            
            
            jsop_setelem_dense();
            return true;
        }
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

    
    Jump initlenGuard = masm.guardArrayExtent(offsetof(JSObject, initializedLength),
                                              ic.objReg, ic.key, Assembler::BelowOrEqual);
    stubcc.linkExitDirect(initlenGuard, ic.slowPathStart);

    
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

void
mjit::Compiler::jsop_getelem_dense(bool isPacked)
{
    FrameEntry *obj = frame.peek(-2);
    FrameEntry *id = frame.peek(-1);

    
    
    if (!obj->isTypeKnown()) {
        Jump guard = frame.testObject(Assembler::NotEqual, obj);
        stubcc.linkExit(guard, Uses(2));
    }

    
    if (!id->isTypeKnown()) {
        Jump guard = frame.testInt32(Assembler::NotEqual, id);
        stubcc.linkExit(guard, Uses(2));
    }

    JSValueType type = knownPushedType(0);

    

    
    
    
    bool allowUndefined = mayPushUndefined(0);

    bool hoisted = loop && !a->parent && loop->hoistArrayLengthCheck(obj, 0);

    
    
    RegisterID baseReg;
    if (hoisted) {
        FrameEntry *slotsFe = loop->invariantSlots(obj);
        baseReg = frame.tempRegForData(slotsFe);
    } else {
        baseReg = frame.tempRegForData(obj);
    }
    frame.pinReg(baseReg);

    Int32Key key = id->isConstant()
                 ? Int32Key::FromConstant(id->getValue().toInt32())
                 : Int32Key::FromRegister(frame.tempRegForData(id));
    bool pinKey = !key.isConstant() && key.reg() != baseReg;
    if (pinKey)
        frame.pinReg(key.reg());

    RegisterID dataReg = frame.allocReg();

    MaybeRegisterID typeReg;
    if (!isPacked || type == JSVAL_TYPE_UNKNOWN || type == JSVAL_TYPE_DOUBLE)
        typeReg = frame.allocReg();

    
    MaybeJump initlenGuard;
    if (!hoisted) {
        initlenGuard = masm.guardArrayExtent(offsetof(JSObject, initializedLength),
                                             baseReg, key, Assembler::BelowOrEqual);
    }

    frame.unpinReg(baseReg);
    if (pinKey)
        frame.unpinReg(key.reg());

    RegisterID slotsReg;
    if (hoisted) {
        slotsReg = baseReg;
    } else {
        if (!allowUndefined)
            stubcc.linkExit(initlenGuard.get(), Uses(2));
        masm.loadPtr(Address(baseReg, offsetof(JSObject, slots)), dataReg);
        slotsReg = dataReg;
    }

    
    Jump holeCheck;
    if (key.isConstant()) {
        Address slot(slotsReg, key.index() * sizeof(Value));
        holeCheck = masm.fastArrayLoadSlot(slot, !isPacked, typeReg, dataReg);
    } else {
        JS_ASSERT(key.reg() != dataReg);
        BaseIndex slot(slotsReg, key.reg(), masm.JSVAL_SCALE);
        holeCheck = masm.fastArrayLoadSlot(slot, !isPacked, typeReg, dataReg);
    }

    if (!isPacked) {
        if (!allowUndefined)
            stubcc.linkExit(holeCheck, Uses(2));
        if (type != JSVAL_TYPE_UNKNOWN && type != JSVAL_TYPE_DOUBLE)
            frame.freeReg(typeReg.reg());
    }

    stubcc.leave();
    OOL_STUBCALL(stubs::GetElem);

    frame.popn(2);

    if (type == JSVAL_TYPE_UNKNOWN || type == JSVAL_TYPE_DOUBLE)
        frame.pushRegs(typeReg.reg(), dataReg, type);
    else
        frame.pushTypedPayload(type, dataReg);

    stubcc.rejoin(Changes(2));

    if (allowUndefined) {
        if (!hoisted)
            stubcc.linkExitDirect(initlenGuard.get(), stubcc.masm.label());
        if (!isPacked)
            stubcc.linkExitDirect(holeCheck, stubcc.masm.label());
        JS_ASSERT(type == JSVAL_TYPE_UNKNOWN || type == JSVAL_TYPE_UNDEFINED);
        if (type == JSVAL_TYPE_UNDEFINED)
            stubcc.masm.loadValuePayload(UndefinedValue(), dataReg);
        else
            stubcc.masm.loadValueAsComponents(UndefinedValue(), typeReg.reg(), dataReg);
        stubcc.linkRejoin(stubcc.masm.jump());
    }
}

bool
mjit::Compiler::jsop_getelem(bool isCall)
{
    REJOIN_SITE_2(isCall ? ic::CallElement : ic::GetElement,
                  isCall ? stubs::CallElem : stubs::GetElem);

    FrameEntry *obj = frame.peek(-2);
    FrameEntry *id = frame.peek(-1);

    if (!IsCacheableGetElem(obj, id)) {
        if (isCall)
            jsop_callelem_slow();
        else
            jsop_getelem_slow();
        return true;
    }

    frame.forgetMismatchedObject(obj);

    if (cx->typeInferenceEnabled()) {
        types::TypeSet *types = frame.extra(obj).types;
        types::ObjectKind kind = types
            ? types->getKnownObjectKind(cx)
            : types::OBJECT_UNKNOWN;
        if (!isCall && id->mightBeType(JSVAL_TYPE_INT32) &&
            (kind == types::OBJECT_DENSE_ARRAY || kind == types::OBJECT_PACKED_ARRAY) &&
            !arrayPrototypeHasIndexedProperty()) {
            
            
            jsop_getelem_dense(kind == types::OBJECT_PACKED_ARRAY);
            return true;
        }
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
    frame.pushRegs(ic.typeReg, ic.objReg, knownPushedType(0));
    if (isCall)
        frame.pushSynced(knownPushedType(1));

    stubcc.rejoin(Changes(isCall ? 2 : 1));

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
        RegisterID result = frame.allocReg(Registers::SingleByteRegs).reg();

        
        if (lhs->isTypeKnown() && lhs->isNotType(JSVAL_TYPE_DOUBLE)) {
            frame.popn(2);

            masm.move(Imm32(op == JSOP_STRICTEQ), result);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
            return;
        }

        if (lhs->isType(JSVAL_TYPE_DOUBLE)) {
            FPRegisterID reg = frame.tempFPRegForData(lhs);

            bool equalValue = (op == JSOP_STRICTEQ);
            masm.move(Imm32(equalValue), result);
            Jump j = masm.branchDouble(Assembler::DoubleEqual, reg, reg);
            masm.move(Imm32(!equalValue), result);
            j.linkTo(masm.label(), &masm);

            frame.popn(2);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
            return;
        }

        
        RegisterID treg = frame.tempRegForType(lhs);

        Assembler::Condition oppositeCond = (op == JSOP_STRICTEQ) ? Assembler::NotEqual : Assembler::Equal;

#ifndef JS_CPU_X64
        static const int CanonicalNaNType = 0x7FF80000;
        masm.setPtr(oppositeCond, treg, Imm32(CanonicalNaNType), result);
#else
        static const void *CanonicalNaNType = (void *)0x7FF8000000000000; 
        masm.move(ImmPtr(CanonicalNaNType), Registers::ScratchReg);
        masm.setPtr(oppositeCond, treg, Registers::ScratchReg, result);
#endif

        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
        return;
    }

    
    bool lhsTest;
    if ((lhsTest = ReallySimpleStrictTest(lhs)) || ReallySimpleStrictTest(rhs)) {
        FrameEntry *test = lhsTest ? rhs : lhs;
        FrameEntry *known = lhsTest ? lhs : rhs;
        RegisterID result = frame.allocReg(Registers::SingleByteRegs).reg();

        if (test->isTypeKnown()) {
            masm.move(Imm32((test->getKnownType() == known->getKnownType()) ==
                            (op == JSOP_STRICTEQ)), result);
            frame.popn(2);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
            return;
        }

        
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
            RegisterID result = frame.allocReg(Registers::SingleByteRegs).reg();
            frame.popn(2);

            masm.move(Imm32(op == JSOP_STRICTNE), result);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
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
            result = frame.allocReg(Registers::SingleByteRegs).reg();
        
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
            INLINE_STUBCALL_NO_REJOIN(stubs::StrictEq);
        else
            INLINE_STUBCALL_NO_REJOIN(stubs::StrictNe);

        frame.popn(2);
        frame.pushSynced(JSVAL_TYPE_BOOLEAN);
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
            OOL_STUBCALL_NO_REJOIN(stubs::StrictEq);
        else
            OOL_STUBCALL_NO_REJOIN(stubs::StrictNe);
    }

    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, resultReg);

    if (needStub)
        stubcc.rejoin(Changes(1));
#else
    
    prepareStubCall(Uses(2));

    if (op == JSOP_STRICTEQ)
        INLINE_STUBCALL_NO_REJOIN(stubs::StrictEq);
    else
        INLINE_STUBCALL_NO_REJOIN(stubs::StrictNe);

    frame.popn(2);
    frame.pushSyncedType(JSVAL_TYPE_BOOLEAN);
    return;
#endif
}

void
mjit::Compiler::jsop_pos()
{
    REJOIN_SITE(stubs::Pos);

    FrameEntry *top = frame.peek(-1);

    if (top->isTypeKnown()) {
        if (top->getKnownType() <= JSVAL_TYPE_INT32)
            return;
        prepareStubCall(Uses(1));
        INLINE_STUBCALL(stubs::Pos);
        frame.pop();
        frame.pushSynced(knownPushedType(0));
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
    REJOIN_SITE_ANY();

#ifdef DEBUG
    FrameEntry *obj = frame.peek(-2);
#endif
    JSAtom *atom = script->getAtom(fullAtomIndex(PC));

    
    JS_ASSERT(!frame.extra(obj).initObject);

    prepareStubCall(Uses(2));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::InitMethod);
}

void
mjit::Compiler::jsop_initprop()
{
    REJOIN_SITE_ANY();

    FrameEntry *obj = frame.peek(-2);
    FrameEntry *fe = frame.peek(-1);
    JSAtom *atom = script->getAtom(fullAtomIndex(PC));

    JSObject *baseobj = frame.extra(obj).initObject;

    if (!baseobj || monitored(PC)) {
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

    
    Shape *shape = (Shape *) prop;
    Address address = masm.objPropAddress(baseobj, objReg, shape->slot);
    frame.storeTo(fe, address);
    frame.freeReg(objReg);
}

void
mjit::Compiler::jsop_initelem()
{
    REJOIN_SITE_ANY();

    FrameEntry *obj = frame.peek(-3);
    FrameEntry *id = frame.peek(-2);
    FrameEntry *fe = frame.peek(-1);

    






    if (!id->isConstant() || !frame.extra(obj).initArray) {
        JSOp next = JSOp(PC[JSOP_INITELEM_LENGTH]);

        prepareStubCall(Uses(3));
        masm.move(Imm32(next == JSOP_ENDINIT ? 1 : 0), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::InitElem);
        return;
    }

    int32 idx = id->getValue().toInt32();

    RegisterID objReg = frame.copyDataIntoReg(obj);

    if (cx->typeInferenceEnabled()) {
        
        masm.store32(Imm32(idx + 1), Address(objReg, offsetof(JSObject, initializedLength)));
    }

    
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);
    frame.storeTo(fe, Address(objReg, idx * sizeof(Value)));
    frame.freeReg(objReg);
}
