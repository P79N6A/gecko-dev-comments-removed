







































#include "jsbool.h"
#include "jscntxt.h"
#include "jslibmath.h"
#include "jsnum.h"
#include "jsscope.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"
#include "jstypedarrayinlines.h"

#include "frontend/BytecodeGenerator.h"
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
        FPRegisterID fptemp = frame.allocFPReg();
        RegisterID typeReg = frame.tempRegForType(fe);
        frame.pinReg(typeReg);
        RegisterID dataReg = frame.copyDataIntoReg(fe);
        frame.unpinReg(typeReg);

        Jump intGuard = masm.testInt32(Assembler::NotEqual, typeReg);

        Label syncPath = stubcc.syncExitAndJump(uses);
        stubcc.linkExitDirect(intGuard, stubcc.masm.label());

        
        Jump doubleGuard = stubcc.masm.testDouble(Assembler::NotEqual, typeReg);
        doubleGuard.linkTo(syncPath, &stubcc.masm);

        frame.loadDouble(fe, fptemp, stubcc.masm);
        Jump truncateGuard = stubcc.masm.branchTruncateDoubleToInt32(fptemp, dataReg);
        truncateGuard.linkTo(syncPath, &stubcc.masm);
        stubcc.crossJump(stubcc.masm.jump(), masm.label());

        frame.freeReg(fptemp);
        frame.learnType(fe, JSVAL_TYPE_INT32, dataReg);
    }
}

void
mjit::Compiler::jsop_bitnot()
{
    FrameEntry *top = frame.peek(-1);

    
    if (top->isNotType(JSVAL_TYPE_INT32) && top->isNotType(JSVAL_TYPE_DOUBLE)) {
        prepareStubCall(Uses(1));
        INLINE_STUBCALL(stubs::BitNot, REJOIN_FALLTHROUGH);
        frame.pop();
        frame.pushSynced(JSVAL_TYPE_INT32);
        return;
    }

    ensureInteger(top, Uses(1));

    stubcc.leave();
    OOL_STUBCALL(stubs::BitNot, REJOIN_FALLTHROUGH);

    RegisterID reg = frame.ownRegForData(top);
    masm.not32(reg);
    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_bitop(JSOp op)
{
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

    
    if (rhs->isConstant() && rhs->getValue().isDouble())
        rhs->convertConstantDoubleToInt32(cx);

    
    if ((lhs->isNotType(JSVAL_TYPE_INT32) && lhs->isNotType(JSVAL_TYPE_DOUBLE)) ||
        (rhs->isNotType(JSVAL_TYPE_INT32) && rhs->isNotType(JSVAL_TYPE_DOUBLE)) ||
        (op == JSOP_URSH && rhs->isConstant() && rhs->getValue().toInt32() % 32 == 0)) {
        prepareStubCall(Uses(2));
        INLINE_STUBCALL(stub, REJOIN_FALLTHROUGH);
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
            int32 rhsInt = rhs->getValue().toInt32();
            if (op == JSOP_BITAND)
                masm.and32(Imm32(rhsInt), reg);
            else if (op == JSOP_BITXOR)
                masm.xor32(Imm32(rhsInt), reg);
            else if (rhsInt != 0)
                masm.or32(Imm32(rhsInt), reg);
        } else if (frame.shouldAvoidDataRemat(rhs)) {
            Address rhsAddr = masm.payloadOf(frame.addressOf(rhs));
            if (op == JSOP_BITAND)
                masm.and32(rhsAddr, reg);
            else if (op == JSOP_BITXOR)
                masm.xor32(rhsAddr, reg);
            else
                masm.or32(rhsAddr, reg);
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
            OOL_STUBCALL(stub, REJOIN_FALLTHROUGH);

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
    OOL_STUBCALL(stub, REJOIN_FALLTHROUGH);

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

CompileStatus
mjit::Compiler::jsop_equality_obj_obj(JSOp op, jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    JS_ASSERT(cx->typeInferenceEnabled() &&
              lhs->isType(JSVAL_TYPE_OBJECT) && rhs->isType(JSVAL_TYPE_OBJECT));

    




    types::TypeSet *lhsTypes = analysis->poppedTypes(PC, 1);
    types::TypeSet *rhsTypes = analysis->poppedTypes(PC, 0);
    if (!lhsTypes->hasObjectFlags(cx, types::OBJECT_FLAG_SPECIAL_EQUALITY) &&
        !rhsTypes->hasObjectFlags(cx, types::OBJECT_FLAG_SPECIAL_EQUALITY)) {
        
        JS_ASSERT_IF(!target, fused != JSOP_IFEQ);
        frame.forgetMismatchedObject(lhs);
        frame.forgetMismatchedObject(rhs);
        Assembler::Condition cond = GetCompareCondition(op, fused);
        if (target) {
            Jump sj = stubcc.masm.branchTest32(GetStubCompareCondition(fused),
                                               Registers::ReturnReg, Registers::ReturnReg);
            if (!frame.syncForBranch(target, Uses(2)))
                return Compile_Error;
            RegisterID lreg = frame.tempRegForData(lhs);
            frame.pinReg(lreg);
            RegisterID rreg = frame.tempRegForData(rhs);
            frame.unpinReg(lreg);
            Jump fast = masm.branchPtr(cond, lreg, rreg);
            frame.popn(2);
            return jumpAndTrace(fast, target, &sj) ? Compile_Okay : Compile_Error;
        } else {
            RegisterID result = frame.allocReg();
            RegisterID lreg = frame.tempRegForData(lhs);
            frame.pinReg(lreg);
            RegisterID rreg = frame.tempRegForData(rhs);
            frame.unpinReg(lreg);
            masm.branchValue(cond, lreg, rreg, result);

            frame.popn(2);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, result);
            return Compile_Okay;
        }
    }

    return Compile_Skipped;
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

        if (test->isType(JSVAL_TYPE_NULL) || test->isType(JSVAL_TYPE_UNDEFINED)) {
            return emitStubCmpOp(stub, target, fused);
        } else if (test->isTypeKnown()) {
            
            bool result = GetCompareCondition(op, fused) == Assembler::NotEqual;
            frame.pop();
            frame.pop();
            if (target)
                return constantFoldBranch(target, result);
            frame.push(BooleanValue(result));
            return true;
        }

        
        RegisterID reg = frame.ownRegForType(test);
        frame.pop();
        frame.pop();

        




        if (target) {
            frame.syncAndKillEverything();
            frame.freeReg(reg);

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
        lhs->isType(JSVAL_TYPE_OBJECT) && rhs->isType(JSVAL_TYPE_OBJECT))
    {
        CompileStatus status = jsop_equality_obj_obj(op, target, fused);
        if (status == Compile_Okay) return true;
        else if (status == Compile_Error) return false;
    }

    return emitStubCmpOp(stub, target, fused);
}

bool
mjit::Compiler::jsop_relational(JSOp op, BoolStub stub,
                                jsbytecode *target, JSOp fused)
{
    FrameEntry *rhs = frame.peek(-1);
    FrameEntry *lhs = frame.peek(-2);

    
    JS_ASSERT(!(rhs->isConstant() && lhs->isConstant()));
    JS_ASSERT(fused == JSOP_NOP || fused == JSOP_IFEQ || fused == JSOP_IFNE);

    
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
        return emitStubCmpOp(stub, target, fused);
    } else if (lhs->isType(JSVAL_TYPE_STRING) || rhs->isType(JSVAL_TYPE_STRING)) {
        return emitStubCmpOp(stub, target, fused);
    } else if (lhs->isType(JSVAL_TYPE_DOUBLE) || rhs->isType(JSVAL_TYPE_DOUBLE)) {
        return jsop_relational_double(op, stub, target, fused);
    } else if (cx->typeInferenceEnabled() &&
               lhs->isType(JSVAL_TYPE_INT32) && rhs->isType(JSVAL_TYPE_INT32)) {
        return jsop_relational_int(op, target, fused);
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
            INLINE_STUBCALL_USES(stubs::ValueToBoolean, REJOIN_NONE, Uses(1));

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
    OOL_STUBCALL(stubs::Not, REJOIN_FALLTHROUGH);

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
    INLINE_STUBCALL(stubs::TypeOf, REJOIN_NONE);
    frame.pop();
    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_TYPE_STRING, Registers::ReturnReg);
}

bool
mjit::Compiler::booleanJumpScript(JSOp op, jsbytecode *target)
{
    
    
    if (op == JSOP_AND || op == JSOP_OR) {
        frame.syncForBranch(target, Uses(0));
    } else {
        JS_ASSERT(op == JSOP_IFEQ || op == JSOP_IFEQX ||
                  op == JSOP_IFNE || op == JSOP_IFNEX);
        frame.syncForBranch(target, Uses(1));
    }

    FrameEntry *fe = frame.peek(-1);
    Assembler::Condition cond = (op == JSOP_IFNE || op == JSOP_IFNEX || op == JSOP_OR)
                                ? Assembler::NonZero
                                : Assembler::Zero;

    
    
    MaybeRegisterID data;
    if (!fe->isType(JSVAL_TYPE_DOUBLE)) {
        data = frame.tempRegForData(fe);
        frame.pinReg(data.reg());
    }

    
    bool needStub = false;
    if (!fe->isType(JSVAL_TYPE_BOOLEAN) && !fe->isType(JSVAL_TYPE_INT32)) {
        Jump notBool;
        if (fe->mightBeType(JSVAL_TYPE_BOOLEAN))
            notBool = frame.testBoolean(Assembler::NotEqual, fe);
        else
            notBool = masm.jump();

        stubcc.linkExitForBranch(notBool);
        needStub = true;
    }
    if (data.isSet())
        frame.unpinReg(data.reg());

    
    Jump branch;
    if (!fe->isType(JSVAL_TYPE_DOUBLE))
        branch = masm.branchTest32(cond, data.reg());
    else
        branch = masm.jump(); 

    
    if (needStub) {
        stubcc.leave();

        
        stubcc.masm.infallibleVMCall(JS_FUNC_TO_DATA_PTR(void *, stubs::ValueToBoolean),
                                     frame.totalDepth());
    }

    Jump stubBranch = stubcc.masm.branchTest32(cond, Registers::ReturnReg);

    
    if (needStub)
        stubcc.rejoin(Changes(0));

    frame.pop();

    return jumpAndTrace(branch, target, &stubBranch);
}

bool
mjit::Compiler::jsop_ifneq(JSOp op, jsbytecode *target)
{
    FrameEntry *fe = frame.peek(-1);

    if (fe->isConstant()) {
        JSBool b = js_ValueToBoolean(fe->getValue());

        frame.pop();

        if (op == JSOP_IFEQ || op == JSOP_IFEQX)
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
mjit::Compiler::jsop_localinc(JSOp op, uint32 slot)
{
    restoreVarType();

    types::TypeSet *types = pushedTypeSet(0);
    JSValueType type = types ? types->getKnownTypeTag(cx) : JSVAL_TYPE_UNKNOWN;

    int amt = (op == JSOP_LOCALINC || op == JSOP_INCLOCAL) ? 1 : -1;

    if (!analysis->incrementInitialValueObserved(PC)) {
        
        
        frame.pushLocal(slot);

        
        
        frame.push(Int32Value(-amt));

        
        
        
        if (!jsop_binary(JSOP_SUB, stubs::Sub, type, types))
            return false;

        
        
        frame.storeLocal(slot, analysis->popGuaranteed(PC));
    } else {
        
        
        frame.pushLocal(slot);

        
        
        jsop_pos();

        
        
        frame.dup();

        
        
        frame.push(Int32Value(amt));

        
        
        if (!jsop_binary(JSOP_ADD, stubs::Add, type, types))
            return false;

        
        
        frame.storeLocal(slot, true);

        
        
        frame.pop();
    }

    updateVarType();
    return true;
}

bool
mjit::Compiler::jsop_arginc(JSOp op, uint32 slot)
{
    restoreVarType();

    types::TypeSet *types = pushedTypeSet(0);
    JSValueType type = types ? types->getKnownTypeTag(cx) : JSVAL_TYPE_UNKNOWN;

    int amt = (op == JSOP_ARGINC || op == JSOP_INCARG) ? 1 : -1;

    if (!analysis->incrementInitialValueObserved(PC)) {
        
        
        frame.pushArg(slot);

        
        
        frame.push(Int32Value(-amt));

        
        
        
        if (!jsop_binary(JSOP_SUB, stubs::Sub, type, types))
            return false;

        
        
        frame.storeArg(slot, analysis->popGuaranteed(PC));
    } else {
        
        
        frame.pushArg(slot);

        
        
        jsop_pos();

        
        
        frame.dup();

        
        
        frame.push(Int32Value(amt));

        
        
        if (!jsop_binary(JSOP_ADD, stubs::Add, type, types))
            return false;

        
        
        frame.storeArg(slot, true);

        
        
        frame.pop();
    }

    updateVarType();
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
    frame.pinEntry(value, vr,  false);

    Int32Key key = id->isConstant()
                 ? Int32Key::FromConstant(id->getValue().toInt32())
                 : Int32Key::FromRegister(frame.tempRegForData(id));
    bool pinKey = !key.isConstant() && !frame.haveSameBacking(id, value);
    if (pinKey)
        frame.pinReg(key.reg());

    
    
    
    RegisterID slotsReg;
    analyze::CrossSSAValue objv(a->inlineIndex, analysis->poppedValue(PC, 2));
    analyze::CrossSSAValue indexv(a->inlineIndex, analysis->poppedValue(PC, 1));
    bool hoisted = loop && id->isType(JSVAL_TYPE_INT32) &&
        loop->hoistArrayLengthCheck(DENSE_ARRAY, objv, indexv);

    if (hoisted) {
        FrameEntry *slotsFe = loop->invariantArraySlots(objv);
        slotsReg = frame.tempRegForData(slotsFe);

        frame.unpinEntry(vr);
        if (pinKey)
            frame.unpinReg(key.reg());
    } else {
        
        if (frame.haveSameBacking(obj, value)) {
            slotsReg = frame.allocReg();
            masm.move(vr.dataReg(), slotsReg);
        } else if (frame.haveSameBacking(obj, id)) {
            slotsReg = frame.allocReg();
            masm.move(key.reg(), slotsReg);
        } else {
            slotsReg = frame.copyDataIntoReg(obj);
        }
        masm.loadPtr(Address(slotsReg, JSObject::offsetOfElements()), slotsReg);

        frame.unpinEntry(vr);
        if (pinKey)
            frame.unpinReg(key.reg());

        
        Label syncTarget = stubcc.syncExitAndJump(Uses(3));

        Jump initlenGuard = masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                                  slotsReg, key, Assembler::BelowOrEqual);
        stubcc.linkExitDirect(initlenGuard, stubcc.masm.label());

        
        
        Jump exactlenGuard = stubcc.masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                                          slotsReg, key, Assembler::NotEqual);
        exactlenGuard.linkTo(syncTarget, &stubcc.masm);

        
        Jump capacityGuard = stubcc.masm.guardArrayExtent(ObjectElements::offsetOfCapacity(),
                                                          slotsReg, key, Assembler::BelowOrEqual);
        capacityGuard.linkTo(syncTarget, &stubcc.masm);

        
        
        stubcc.masm.bumpKey(key, 1);

        
        stubcc.masm.storeKey(key, Address(slotsReg, ObjectElements::offsetOfInitializedLength()));

        
        Jump lengthGuard = stubcc.masm.guardArrayExtent(ObjectElements::offsetOfLength(),
                                                        slotsReg, key, Assembler::AboveOrEqual);
        stubcc.masm.storeKey(key, Address(slotsReg, ObjectElements::offsetOfLength()));
        lengthGuard.linkTo(stubcc.masm.label(), &stubcc.masm);

        
        stubcc.masm.bumpKey(key, -1);

        
        Jump initlenExit = stubcc.masm.jump();
        stubcc.crossJump(initlenExit, masm.label());
    }

    
    
    if (key.isConstant())
        masm.storeValue(vr, Address(slotsReg, key.index() * sizeof(Value)));
    else
        masm.storeValue(vr, BaseIndex(slotsReg, key.reg(), masm.JSVAL_SCALE));

    stubcc.leave();
    OOL_STUBCALL(STRICT_VARIANT(stubs::SetElem), REJOIN_FALLTHROUGH);

    if (!hoisted)
        frame.freeReg(slotsReg);
    frame.shimmy(2);
    stubcc.rejoin(Changes(2));
}

#ifdef JS_METHODJIT_TYPED_ARRAY
void
mjit::Compiler::convertForTypedArray(int atype, ValueRemat *vr, bool *allocated)
{
    FrameEntry *value = frame.peek(-1);
    bool floatArray = (atype == TypedArray::TYPE_FLOAT32 ||
                       atype == TypedArray::TYPE_FLOAT64);
    *allocated = false;

    if (value->isConstant()) {
        Value v = value->getValue();
        if (floatArray) {
            double d = v.isDouble() ? v.toDouble() : v.toInt32();
            *vr = ValueRemat::FromConstant(DoubleValue(d));
        } else {
            int i32;
            if (v.isInt32()) {
                i32 = v.toInt32();
                if (atype == TypedArray::TYPE_UINT8_CLAMPED)
                    i32 = ClampIntForUint8Array(i32);
            } else {
                i32 = (atype == TypedArray::TYPE_UINT8_CLAMPED)
                    ? js_TypedArray_uint8_clamp_double(v.toDouble())
                    : js_DoubleToECMAInt32(v.toDouble());
            }
            *vr = ValueRemat::FromConstant(Int32Value(i32));
        }
    } else {
        if (floatArray) {
            FPRegisterID fpReg;
            MaybeJump notNumber = loadDouble(value, &fpReg, allocated);
            if (notNumber.isSet())
                stubcc.linkExit(notNumber.get(), Uses(3));

            if (atype == TypedArray::TYPE_FLOAT32) {
                if (!*allocated) {
                    frame.pinReg(fpReg);
                    FPRegisterID newFpReg = frame.allocFPReg();
                    masm.convertDoubleToFloat(fpReg, newFpReg);
                    frame.unpinReg(fpReg);
                    fpReg = newFpReg;
                    *allocated = true;
                } else {
                    masm.convertDoubleToFloat(fpReg, fpReg);
                }
            }
            *vr = ValueRemat::FromFPRegister(fpReg);
        } else {
            










            MaybeRegisterID reg, dataReg;
            bool needsByteReg = (atype == TypedArray::TYPE_INT8 ||
                                 atype == TypedArray::TYPE_UINT8 ||
                                 atype == TypedArray::TYPE_UINT8_CLAMPED);
            FrameEntry *id = frame.peek(-2);
            if (!value->isType(JSVAL_TYPE_INT32) || atype == TypedArray::TYPE_UINT8_CLAMPED ||
                (needsByteReg && frame.haveSameBacking(id, value))) {
                
                if (value->mightBeType(JSVAL_TYPE_INT32)) {
                    dataReg = frame.tempRegForData(value);

                    
                    if (!frame.haveSameBacking(id, value))
                        frame.pinReg(dataReg.reg());
                }

                
                
                
                if (needsByteReg)
                    reg = frame.allocReg(Registers::SingleByteRegs).reg();
                else
                    reg = frame.allocReg();
                *allocated = true;
            } else {
                if (needsByteReg)
                    reg = frame.tempRegInMaskForData(value, Registers::SingleByteRegs).reg();
                else
                    reg = frame.tempRegForData(value);
            }

            
            MaybeRegisterID typeReg;
            if (!value->isTypeKnown()) {
                
                
                JS_ASSERT(*allocated);
                typeReg = frame.tempRegForType(value);
            }

            MaybeJump intDone;
            if (value->mightBeType(JSVAL_TYPE_INT32)) {
                
                MaybeJump notInt;
                if (!value->isTypeKnown()) {
                    JS_ASSERT(*allocated);
                    notInt = masm.testInt32(Assembler::NotEqual, typeReg.reg());
                }

                if (*allocated) {
                    masm.move(dataReg.reg(), reg.reg());
                    if (!frame.haveSameBacking(id, value))
                        frame.unpinReg(dataReg.reg());
                }

                if (atype == TypedArray::TYPE_UINT8_CLAMPED)
                    masm.clampInt32ToUint8(reg.reg());

                if (notInt.isSet()) {
                    intDone = masm.jump();
                    notInt.get().linkTo(masm.label(), &masm);
                }
            }
            if (value->mightBeType(JSVAL_TYPE_DOUBLE)) {
                
                if (!value->isTypeKnown()) {
                    Jump notNumber = masm.testDouble(Assembler::NotEqual, typeReg.reg());
                    stubcc.linkExit(notNumber, Uses(3));
                }

                
                FPRegisterID fpReg;
                if (value->isTypeKnown()) {
                    fpReg = frame.tempFPRegForData(value);
                } else {
                    fpReg = frame.allocFPReg();
                    frame.loadDouble(value, fpReg, masm);
                }

                
                if (atype == TypedArray::TYPE_UINT8_CLAMPED) {
                    if (value->isTypeKnown())
                        frame.pinReg(fpReg);
                    FPRegisterID fpTemp = frame.allocFPReg();
                    if (value->isTypeKnown())
                        frame.unpinReg(fpReg);
                    masm.clampDoubleToUint8(fpReg, fpTemp, reg.reg());
                    frame.freeReg(fpTemp);
                } else {
                    Jump j = masm.branchTruncateDoubleToInt32(fpReg, reg.reg());
                    stubcc.linkExit(j, Uses(3));
                }
                if (!value->isTypeKnown())
                    frame.freeReg(fpReg);
            }
            if (intDone.isSet())
                intDone.get().linkTo(masm.label(), &masm);
            *vr = ValueRemat::FromKnownType(JSVAL_TYPE_INT32, reg.reg());
        }
    }
}

void
mjit::Compiler::jsop_setelem_typed(int atype)
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
    frame.pinEntry(value, vr,  false);

    
    Int32Key key = id->isConstant()
                 ? Int32Key::FromConstant(id->getValue().toInt32())
                 : Int32Key::FromRegister(frame.tempRegForData(id));

    bool pinKey = !key.isConstant() && !frame.haveSameBacking(id, value);
    if (pinKey)
        frame.pinReg(key.reg());

    analyze::CrossSSAValue objv(a->inlineIndex, analysis->poppedValue(PC, 1));
    analyze::CrossSSAValue indexv(a->inlineIndex, analysis->poppedValue(PC, 0));
    bool hoisted = loop && id->isType(JSVAL_TYPE_INT32) &&
        loop->hoistArrayLengthCheck(TYPED_ARRAY, objv, indexv);

    RegisterID objReg;
    if (hoisted) {
        FrameEntry *slotsFe = loop->invariantArraySlots(objv);
        objReg = frame.tempRegForData(slotsFe);
        frame.pinReg(objReg);
    } else {
        objReg = frame.copyDataIntoReg(obj);

        
        int lengthOffset = TypedArray::lengthOffset() + offsetof(jsval_layout, s.payload);
        Jump lengthGuard = masm.guardArrayExtent(lengthOffset,
                                                 objReg, key, Assembler::BelowOrEqual);
        stubcc.linkExit(lengthGuard, Uses(3));

        
        masm.loadPtr(Address(objReg, TypedArray::dataOffset()), objReg);
    }

    
    
    frame.unpinEntry(vr);

    
    if (frame.haveSameBacking(id, value)) {
        frame.pinReg(key.reg());
        pinKey = true;
    }
    JS_ASSERT(pinKey == !id->isConstant());

    bool allocated;
    convertForTypedArray(atype, &vr, &allocated);

    
    masm.storeToTypedArray(atype, objReg, key, vr);
    if (allocated) {
        if (vr.isFPRegister())
            frame.freeReg(vr.fpReg());
        else
            frame.freeReg(vr.dataReg());
    }
    if (pinKey)
        frame.unpinReg(key.reg());
    if (hoisted)
        frame.unpinReg(objReg);
    else
        frame.freeReg(objReg);

    stubcc.leave();
    OOL_STUBCALL(STRICT_VARIANT(stubs::SetElem), REJOIN_FALLTHROUGH);

    frame.shimmy(2);
    stubcc.rejoin(Changes(2));
}
#endif 

bool
mjit::Compiler::jsop_setelem(bool popGuaranteed)
{
    FrameEntry *obj = frame.peek(-3);
    FrameEntry *id = frame.peek(-2);
    FrameEntry *value = frame.peek(-1);

    if (!IsCacheableSetElem(obj, id, value) || monitored(PC)) {
        jsop_setelem_slow();
        return true;
    }

    frame.forgetMismatchedObject(obj);

    
    
    if (cx->typeInferenceEnabled() && id->mightBeType(JSVAL_TYPE_INT32)) {
        types::TypeSet *types = analysis->poppedTypes(PC, 2);

        if (!types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY) &&
            !arrayPrototypeHasIndexedProperty()) {
            
            jsop_setelem_dense();
            return true;
        }

#ifdef JS_METHODJIT_TYPED_ARRAY
        if ((value->mightBeType(JSVAL_TYPE_INT32) || value->mightBeType(JSVAL_TYPE_DOUBLE)) &&
            !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_TYPED_ARRAY)) {
            
            int atype = types->getTypedArrayType(cx);
            if (atype != TypedArray::TYPE_MAX) {
                jsop_setelem_typed(atype);
                return true;
            }
        }
#endif
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

    
    ic.shapeGuard = masm.testObjClass(Assembler::NotEqual, ic.objReg, ic.objReg, &ArrayClass);
    stubcc.linkExitDirect(ic.shapeGuard, ic.slowPathStart);

    masm.rematPayload(ic.objRemat, ic.objReg);

    
    masm.loadPtr(Address(ic.objReg, JSObject::offsetOfElements()), ic.objReg);

    
    Jump initlenGuard = masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                              ic.objReg, ic.key, Assembler::BelowOrEqual);
    stubcc.linkExitDirect(initlenGuard, ic.slowPathStart);

    
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
    ic.slowPathCall = OOL_STUBCALL(STRICT_VARIANT(ic::SetElement), REJOIN_FALLTHROUGH);
#else
    OOL_STUBCALL(STRICT_VARIANT(stubs::SetElem), REJOIN_FALLTHROUGH);
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

    analyze::CrossSSAValue objv(a->inlineIndex, analysis->poppedValue(PC, 1));
    analyze::CrossSSAValue indexv(a->inlineIndex, analysis->poppedValue(PC, 0));
    bool hoisted = loop && id->isType(JSVAL_TYPE_INT32) &&
        loop->hoistArrayLengthCheck(DENSE_ARRAY, objv, indexv);

    
    
    RegisterID baseReg;
    if (hoisted) {
        FrameEntry *slotsFe = loop->invariantArraySlots(objv);
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
    if (type == JSVAL_TYPE_UNKNOWN || type == JSVAL_TYPE_DOUBLE || hasTypeBarriers(PC))
        typeReg = frame.allocReg();

    frame.unpinReg(baseReg);
    if (pinKey)
        frame.unpinReg(key.reg());

    RegisterID slotsReg;
    if (hoisted) {
        slotsReg = baseReg;
    } else {
        masm.loadPtr(Address(baseReg, JSObject::offsetOfElements()), dataReg);
        slotsReg = dataReg;
    }

    
    MaybeJump initlenGuard;
    if (!hoisted) {
        initlenGuard = masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                             slotsReg, key, Assembler::BelowOrEqual);
        if (!allowUndefined)
            stubcc.linkExit(initlenGuard.get(), Uses(2));
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

    if (!isPacked && !allowUndefined)
        stubcc.linkExit(holeCheck, Uses(2));

    stubcc.leave();
    OOL_STUBCALL(stubs::GetElem, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, -2);

    frame.popn(2);

    BarrierState barrier;
    if (typeReg.isSet()) {
        frame.pushRegs(typeReg.reg(), dataReg, type);
        barrier = testBarrier(typeReg.reg(), dataReg, false);
    } else {
        frame.pushTypedPayload(type, dataReg);
    }

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

    finishBarrier(barrier, REJOIN_FALLTHROUGH, 0);
}

void
mjit::Compiler::jsop_getelem_args()
{
    FrameEntry *id = frame.peek(-1);

    
    if (!id->isTypeKnown()) {
        Jump guard = frame.testInt32(Assembler::NotEqual, id);
        stubcc.linkExit(guard, Uses(2));
    }

    

    analyze::CrossSSAValue indexv(a->inlineIndex, analysis->poppedValue(PC, 0));
    bool hoistedLength = loop && id->isType(JSVAL_TYPE_INT32) &&
        loop->hoistArgsLengthCheck(indexv);
    FrameEntry *actualsFe = loop ? loop->invariantArguments() : NULL;

    Int32Key key = id->isConstant()
                 ? Int32Key::FromConstant(id->getValue().toInt32())
                 : Int32Key::FromRegister(frame.tempRegForData(id));
    if (!key.isConstant())
        frame.pinReg(key.reg());

    RegisterID dataReg = frame.allocReg();
    RegisterID typeReg = frame.allocReg();

    
    if (!hoistedLength) {
        Address nactualAddr(JSFrameReg, StackFrame::offsetOfArgs());
        MaybeJump rangeGuard;
        if (key.isConstant()) {
            JS_ASSERT(key.index() >= 0);
            rangeGuard = masm.branch32(Assembler::BelowOrEqual, nactualAddr, Imm32(key.index()));
        } else {
            rangeGuard = masm.branch32(Assembler::BelowOrEqual, nactualAddr, key.reg());
        }
        stubcc.linkExit(rangeGuard.get(), Uses(2));
    }

    RegisterID actualsReg;
    if (actualsFe) {
        actualsReg = frame.tempRegForData(actualsFe);
    } else {
        actualsReg = dataReg;
        masm.loadFrameActuals(outerScript->function(), actualsReg);
    }

    if (!key.isConstant())
        frame.unpinReg(key.reg());

    if (key.isConstant()) {
        Address arg(actualsReg, key.index() * sizeof(Value));
        masm.loadValueAsComponents(arg, typeReg, dataReg);
    } else {
        JS_ASSERT(key.reg() != dataReg);
        BaseIndex arg(actualsReg, key.reg(), masm.JSVAL_SCALE);
        masm.loadValueAsComponents(arg, typeReg, dataReg);
    }

    stubcc.leave();
    OOL_STUBCALL(stubs::GetElem, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, -2);

    frame.popn(2);
    frame.pushRegs(typeReg, dataReg, knownPushedType(0));
    BarrierState barrier = testBarrier(typeReg, dataReg, false);

    stubcc.rejoin(Changes(2));

    finishBarrier(barrier, REJOIN_FALLTHROUGH, 0);
}

#ifdef JS_METHODJIT_TYPED_ARRAY
bool
mjit::Compiler::jsop_getelem_typed(int atype)
{
    
    
    
    

    
    
    
    
    
    
    
    
    

    
    types::TypeSet *pushedTypes = pushedTypeSet(0);
    if (atype == TypedArray::TYPE_FLOAT32 || atype == TypedArray::TYPE_FLOAT64) {
        if (!pushedTypes->hasType(types::Type::DoubleType()))
            return false;
    } else {
        if (!pushedTypes->hasType(types::Type::Int32Type()))
            return false;
    }

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

    
    Int32Key key = id->isConstant()
                 ? Int32Key::FromConstant(id->getValue().toInt32())
                 : Int32Key::FromRegister(frame.tempRegForData(id));
    if (!key.isConstant())
        frame.pinReg(key.reg());

    analyze::CrossSSAValue objv(a->inlineIndex, analysis->poppedValue(PC, 1));
    analyze::CrossSSAValue indexv(a->inlineIndex, analysis->poppedValue(PC, 0));
    bool hoisted = loop && id->isType(JSVAL_TYPE_INT32) &&
        loop->hoistArrayLengthCheck(TYPED_ARRAY, objv, indexv);

    RegisterID objReg;
    if (hoisted) {
        FrameEntry *slotsFe = loop->invariantArraySlots(objv);
        objReg = frame.tempRegForData(slotsFe);
        frame.pinReg(objReg);
    } else {
        objReg = frame.copyDataIntoReg(obj);

        
        int lengthOffset = TypedArray::lengthOffset() + offsetof(jsval_layout, s.payload);
        Jump lengthGuard = masm.guardArrayExtent(lengthOffset,
                                                 objReg, key, Assembler::BelowOrEqual);
        stubcc.linkExit(lengthGuard, Uses(2));

        
        masm.loadPtr(Address(objReg, TypedArray::dataOffset()), objReg);
    }

    
    
    
    
    
    
    AnyRegisterID dataReg;
    MaybeRegisterID typeReg, tempReg;
    JSValueType type = knownPushedType(0);
    bool maybeReadFloat = (atype == TypedArray::TYPE_FLOAT32 ||
                           atype == TypedArray::TYPE_FLOAT64 ||
                           atype == TypedArray::TYPE_UINT32);
    if (maybeReadFloat && type == JSVAL_TYPE_DOUBLE) {
        dataReg = frame.allocFPReg();
        
        if (atype == TypedArray::TYPE_UINT32)
            tempReg = frame.allocReg();
    } else {
        dataReg = frame.allocReg();
        
        
        
        
        if (maybeReadFloat || type != JSVAL_TYPE_INT32)
            typeReg = frame.allocReg();
    }

    
    masm.loadFromTypedArray(atype, objReg, key, typeReg, dataReg, tempReg);

    if (hoisted)
        frame.unpinReg(objReg);
    else
        frame.freeReg(objReg);
    if (!key.isConstant())
        frame.unpinReg(key.reg());
    if (tempReg.isSet())
        frame.freeReg(tempReg.reg());

    if (atype == TypedArray::TYPE_UINT32 &&
        !pushedTypes->hasType(types::Type::DoubleType())) {
        Jump isDouble = masm.testDouble(Assembler::Equal, typeReg.reg());
        stubcc.linkExit(isDouble, Uses(2));
    }

    stubcc.leave();
    OOL_STUBCALL(stubs::GetElem, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, -2);

    frame.popn(2);

    if (dataReg.isFPReg()) {
        frame.pushDouble(dataReg.fpreg());
    } else if (typeReg.isSet()) {
        frame.pushRegs(typeReg.reg(), dataReg.reg(), knownPushedType(0));
    } else {
        JS_ASSERT(type == JSVAL_TYPE_INT32);
        frame.pushTypedPayload(JSVAL_TYPE_INT32, dataReg.reg());
    }
    stubcc.rejoin(Changes(2));

    return true;
}
#endif 

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

    
    
    if (cx->typeInferenceEnabled() && id->mightBeType(JSVAL_TYPE_INT32) && !isCall) {
        types::TypeSet *types = analysis->poppedTypes(PC, 1);
        if (types->isLazyArguments(cx) && !outerScript->analysis()->modifiesArguments()) {
            
            
            
            
            
            
            jsop_getelem_args();
            return true;
        }

        if (obj->mightBeType(JSVAL_TYPE_OBJECT) &&
            !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY) &&
            !arrayPrototypeHasIndexedProperty()) {
            
            bool packed = !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED_ARRAY);
            jsop_getelem_dense(packed);
            return true;
        }

#ifdef JS_METHODJIT_TYPED_ARRAY
        if (obj->mightBeType(JSVAL_TYPE_OBJECT) &&
            !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_TYPED_ARRAY)) {
            
            int atype = types->getTypedArrayType(cx);
            if (atype != TypedArray::TYPE_MAX) {
                if (jsop_getelem_typed(atype))
                    return true;
                
            }
        }
#endif
    }

    frame.forgetMismatchedObject(obj);

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

        
        ic.shapeGuard = masm.testObjClass(Assembler::NotEqual, ic.objReg, ic.typeReg, &ArrayClass);
        stubcc.linkExitDirect(ic.shapeGuard, ic.slowPathStart);

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
        
        
        ic.shapeGuard = masm.jump();
        stubcc.linkExitDirect(ic.shapeGuard, ic.slowPathStart);
    }

    stubcc.leave();
    if (objTypeGuard.isSet())
        objTypeGuard.get().linkTo(stubcc.masm.label(), &stubcc.masm);
#ifdef JS_POLYIC
    passICAddress(&ic);
    if (isCall)
        ic.slowPathCall = OOL_STUBCALL(ic::CallElement, REJOIN_FALLTHROUGH);
    else
        ic.slowPathCall = OOL_STUBCALL(ic::GetElement, REJOIN_FALLTHROUGH);
#else
    if (isCall)
        ic.slowPathCall = OOL_STUBCALL(stubs::CallElem, REJOIN_FALLTHROUGH);
    else
        ic.slowPathCall = OOL_STUBCALL(stubs::GetElem, REJOIN_FALLTHROUGH);
#endif

    testPushedType(REJOIN_FALLTHROUGH, -2);

    ic.fastPathRejoin = masm.label();
    ic.forcedTypeBarrier = analysis->getCode(PC).getStringElement;

    CHECK_IC_SPACE();

    frame.popn(2);
    frame.pushRegs(ic.typeReg, ic.objReg, knownPushedType(0));
    BarrierState barrier = testBarrier(ic.typeReg, ic.objReg, false, false,
                                        ic.forcedTypeBarrier);
    if (isCall)
        frame.pushSynced(knownPushedType(1));

    stubcc.rejoin(Changes(isCall ? 2 : 1));

#ifdef JS_POLYIC
    if (!getElemICs.append(ic))
        return false;
#endif

    finishBarrier(barrier, REJOIN_FALLTHROUGH, isCall ? 1 : 0);
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
        
        frame.pop();

        if (lhs->isTypeKnown() && lhs->isNotType(JSVAL_TYPE_DOUBLE)) {
            frame.pop();
            frame.push(BooleanValue(op == JSOP_STRICTEQ));
            return;
        }

        if (lhs->isType(JSVAL_TYPE_DOUBLE))
            frame.forgetKnownDouble(lhs);

        
        RegisterID result = frame.allocReg(Registers::SingleByteRegs).reg();
        RegisterID treg = frame.copyTypeIntoReg(lhs);

        Assembler::Condition oppositeCond = (op == JSOP_STRICTEQ) ? Assembler::NotEqual : Assembler::Equal;

        
        masm.lshiftPtr(Imm32(1), treg);
#ifdef JS_CPU_SPARC
        
        static const int ShiftedCanonicalNaNType1 = 0x7FFFFFFF << 1;
        static const int ShiftedCanonicalNaNType2 = 0x7FF80000 << 1;
        RegisterID result1 = frame.allocReg();
        masm.setPtr(oppositeCond, treg, Imm32(ShiftedCanonicalNaNType1), result1);
        masm.setPtr(oppositeCond, treg, Imm32(ShiftedCanonicalNaNType2), result);
        if(op == JSOP_STRICTEQ) {
            masm.and32(result1, result);
        } else {
            masm.or32(result1, result);
        }
        frame.freeReg(result1);
#elif !defined(JS_CPU_X64)
        static const int ShiftedCanonicalNaNType = 0x7FF80000 << 1;
        masm.setPtr(oppositeCond, treg, Imm32(ShiftedCanonicalNaNType), result);
#else
        static const void *ShiftedCanonicalNaNType = (void *)(0x7FF8000000000000 << 1);
        masm.move(ImmPtr(ShiftedCanonicalNaNType), Registers::ScratchReg);
        masm.setPtr(oppositeCond, treg, Registers::ScratchReg, result);
#endif
        frame.freeReg(treg);

        frame.pop();
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

    if (lhs->isType(JSVAL_TYPE_STRING) || rhs->isType(JSVAL_TYPE_STRING)) {
        FrameEntry *maybeNotStr = lhs->isType(JSVAL_TYPE_STRING) ? rhs : lhs;

        if (maybeNotStr->isNotType(JSVAL_TYPE_STRING)) {
            frame.popn(2);
            frame.push(BooleanValue(op == JSOP_STRICTNE));
            return;
        }

        if (!maybeNotStr->isTypeKnown()) {
            JS_ASSERT(!maybeNotStr->isConstant());
            Jump j = frame.testString(Assembler::NotEqual, maybeNotStr);
            stubcc.linkExit(j, Uses(2));
        }

        FrameEntry *op1 = lhs->isConstant() ? rhs : lhs;
        FrameEntry *op2 = lhs->isConstant() ? lhs : rhs;
        JS_ASSERT(!op1->isConstant());

        
        RegisterID resultReg = Registers::ReturnReg;
        frame.takeReg(resultReg);
        RegisterID tmpReg = frame.allocReg();
        RegisterID reg1 = frame.tempRegForData(op1);
        frame.pinReg(reg1);

        RegisterID reg2;
        if (op2->isConstant()) {
            reg2 = frame.allocReg();
            JSString *str = op2->getValue().toString();
            JS_ASSERT(str->isAtom());
            masm.move(ImmPtr(str), reg2);
        } else {
            reg2 = frame.tempRegForData(op2);
            frame.pinReg(reg2);
        }

        JS_ASSERT(reg1 != resultReg);
        JS_ASSERT(reg1 != tmpReg);
        JS_ASSERT(reg2 != resultReg);
        JS_ASSERT(reg2 != tmpReg);

        
        JS_STATIC_ASSERT(JSString::ATOM_FLAGS == 0);
        Imm32 atomMask(JSString::ATOM_MASK);

        masm.load32(Address(reg1, JSString::offsetOfLengthAndFlags()), tmpReg);
        Jump op1NotAtomized = masm.branchTest32(Assembler::NonZero, tmpReg, atomMask);
        stubcc.linkExit(op1NotAtomized, Uses(2));

        if (!op2->isConstant()) {
            masm.load32(Address(reg2, JSString::offsetOfLengthAndFlags()), tmpReg);
            Jump op2NotAtomized = masm.branchTest32(Assembler::NonZero, tmpReg, atomMask);
            stubcc.linkExit(op2NotAtomized, Uses(2));
        }

        masm.set32(cond, reg1, reg2, resultReg);

        frame.unpinReg(reg1);
        if (op2->isConstant())
            frame.freeReg(reg2);
        else
            frame.unpinReg(reg2);
        frame.freeReg(tmpReg);

        stubcc.leave();
        if (op == JSOP_STRICTEQ)
            OOL_STUBCALL_USES(stubs::StrictEq, REJOIN_NONE, Uses(2));
        else
            OOL_STUBCALL_USES(stubs::StrictNe, REJOIN_NONE, Uses(2));

        frame.popn(2);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, resultReg);

        stubcc.rejoin(Changes(1));
        return;
    }

    if (cx->typeInferenceEnabled() &&
        lhs->isType(JSVAL_TYPE_OBJECT) && rhs->isType(JSVAL_TYPE_OBJECT))
    {
        CompileStatus status = jsop_equality_obj_obj(op, NULL, JSOP_NOP);
        if (status == Compile_Okay) return;
        JS_ASSERT(status == Compile_Skipped);
    }

    
    if ((lhs->isTypeKnown() && lhs->isNotType(JSVAL_TYPE_INT32)) ||
        (rhs->isTypeKnown() && rhs->isNotType(JSVAL_TYPE_INT32))) {
        prepareStubCall(Uses(2));

        if (op == JSOP_STRICTEQ)
            INLINE_STUBCALL_USES(stubs::StrictEq, REJOIN_NONE, Uses(2));
        else
            INLINE_STUBCALL_USES(stubs::StrictNe, REJOIN_NONE, Uses(2));

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
            OOL_STUBCALL_USES(stubs::StrictEq, REJOIN_NONE, Uses(2));
        else
            OOL_STUBCALL_USES(stubs::StrictNe, REJOIN_NONE, Uses(2));
    }

    frame.popn(2);
    frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, resultReg);

    if (needStub)
        stubcc.rejoin(Changes(1));
#else
    
    prepareStubCall(Uses(2));

    if (op == JSOP_STRICTEQ)
        INLINE_STUBCALL_USES(stubs::StrictEq, REJOIN_NONE, Uses(2));
    else
        INLINE_STUBCALL_USES(stubs::StrictNe, REJOIN_NONE, Uses(2));

    frame.popn(2);
    frame.pushSynced(JSVAL_TYPE_BOOLEAN);
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
        INLINE_STUBCALL(stubs::Pos, REJOIN_POS);
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
    OOL_STUBCALL(stubs::Pos, REJOIN_POS);

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_initmethod()
{
#ifdef DEBUG
    FrameEntry *obj = frame.peek(-2);
#endif
    JSAtom *atom = script->getAtom(fullAtomIndex(PC));

    
    JS_ASSERT(!frame.extra(obj).initObject);

    prepareStubCall(Uses(2));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::InitMethod, REJOIN_FALLTHROUGH);
}

void
mjit::Compiler::jsop_initprop()
{
    FrameEntry *obj = frame.peek(-2);
    FrameEntry *fe = frame.peek(-1);
    JSAtom *atom = script->getAtom(fullAtomIndex(PC));

    JSObject *baseobj = frame.extra(obj).initObject;

    if (!baseobj || monitored(PC)) {
        prepareStubCall(Uses(2));
        masm.move(ImmPtr(atom), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::InitProp, REJOIN_FALLTHROUGH);
        return;
    }

    JSObject *holder;
    JSProperty *prop = NULL;
#ifdef DEBUG
    bool res =
#endif
    LookupPropertyWithFlags(cx, baseobj, ATOM_TO_JSID(atom),
                            JSRESOLVE_QUALIFIED, &holder, &prop);
    JS_ASSERT(res && prop && holder == baseobj);

    RegisterID objReg = frame.copyDataIntoReg(obj);

    
    Shape *shape = (Shape *) prop;
    Address address = masm.objPropAddress(baseobj, objReg, shape->slot());
    frame.storeTo(fe, address);
    frame.freeReg(objReg);
}

void
mjit::Compiler::jsop_initelem()
{
    FrameEntry *obj = frame.peek(-3);
    FrameEntry *id = frame.peek(-2);
    FrameEntry *fe = frame.peek(-1);

    






    if (!id->isConstant() || !frame.extra(obj).initArray) {
        JSOp next = JSOp(PC[JSOP_INITELEM_LENGTH]);

        prepareStubCall(Uses(3));
        masm.move(Imm32(next == JSOP_ENDINIT ? 1 : 0), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::InitElem, REJOIN_FALLTHROUGH);
        return;
    }

    int32 idx = id->getValue().toInt32();

    RegisterID objReg = frame.copyDataIntoReg(obj);
    masm.loadPtr(Address(objReg, JSObject::offsetOfElements()), objReg);

    
    masm.store32(Imm32(idx + 1), Address(objReg, ObjectElements::offsetOfInitializedLength()));

    
    frame.storeTo(fe, Address(objReg, idx * sizeof(Value)));
    frame.freeReg(objReg);
}
