








































#ifndef jsion_macro_assembler_arm_h__
#define jsion_macro_assembler_arm_h__

#include "ion/arm/Assembler-arm.h"
#include "ion/IonCaches.h"
#include "ion/IonFrames.h"
#include "ion/MoveResolver.h"
#include "jsopcode.h"

namespace js {
namespace ion {

static Register CallReg = ip;
static const int defaultShift = 3;
JS_STATIC_ASSERT(1 << defaultShift == sizeof(jsval));

class MacroAssemblerARM : public Assembler
{
  public:
    void convertInt32ToDouble(const Register &src, const FloatRegister &dest);

    
    
    
    
  private:
    bool alu_dbl(Register src1, Imm32 imm, Register dest, ALUOp op,
                 SetCond_ sc, Condition c);
  public:
    void ma_alu(Register src1, Operand2 op2, Register dest, ALUOp op,
                SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_alu(Register src1, Imm32 imm, Register dest,
                ALUOp op,
                SetCond_ sc =  NoSetCond, Condition c = Always);

    void ma_alu(Register src1, Operand op2, Register dest, ALUOp op,
                SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_nop();
    void ma_movPatchable(Imm32 imm, Register dest, Assembler::Condition c, RelocStyle rs, Instruction *i = NULL);
    
    
    
    
    
    void ma_mov(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_mov(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_mov(const ImmGCPtr &ptr, Register dest);

    
    void ma_lsl(Imm32 shift, Register src, Register dst);
    void ma_lsr(Imm32 shift, Register src, Register dst);
    void ma_asr(Imm32 shift, Register src, Register dst);
    void ma_ror(Imm32 shift, Register src, Register dst);
    void ma_rol(Imm32 shift, Register src, Register dst);
    
    void ma_lsl(Register shift, Register src, Register dst);
    void ma_lsr(Register shift, Register src, Register dst);
    void ma_asr(Register shift, Register src, Register dst);
    void ma_ror(Register shift, Register src, Register dst);
    void ma_rol(Register shift, Register src, Register dst);

    
    void ma_mvn(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    void ma_mvn(Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    
    void ma_and(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_and(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_and(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_and(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);



    
    void ma_bic(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    
    void ma_eor(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_eor(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_eor(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_eor(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    
    void ma_orr(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_orr(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_orr(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_orr(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    
    
    void ma_adc(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_adc(Register src, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_adc(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_add(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_add(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_add(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_add(Register src1, Operand op, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_add(Register src1, Imm32 op, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_sbc(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sbc(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sbc(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_sub(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sub(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sub(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sub(Register src1, Operand op, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sub(Register src1, Imm32 op, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_rsb(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsb(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsb(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsb(Register src1, Imm32 op2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_rsc(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsc(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsc(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    
    void ma_cmn(Register src1, Imm32 imm, Condition c = Always);
    void ma_cmn(Register src1, Register src2, Condition c = Always);
    void ma_cmn(Register src1, Operand op, Condition c = Always);

    
    void ma_cmp(Register src1, Imm32 imm, Condition c = Always);
    void ma_cmp(Register src1, ImmGCPtr ptr, Condition c = Always);
    void ma_cmp(Register src1, Operand op, Condition c = Always);
    void ma_cmp(Register src1, Register src2, Condition c = Always);


    
    void ma_teq(Register src1, Imm32 imm, Condition c = Always);
    void ma_teq(Register src1, Register src2, Condition c = Always);
    void ma_teq(Register src1, Operand op, Condition c = Always);


    
    void ma_tst(Register src1, Imm32 imm, Condition c = Always);
    void ma_tst(Register src1, Register src2, Condition c = Always);
    void ma_tst(Register src1, Operand op, Condition c = Always);

    
    void ma_mul(Register src1, Register src2, Register dest);
    void ma_mul(Register src1, Imm32 imm, Register dest);
    Condition ma_check_mul(Register src1, Register src2, Register dest, Condition cond);
    Condition ma_check_mul(Register src1, Imm32 imm, Register dest, Condition cond);


    
    
    void ma_dtr(LoadStore ls, Register rn, Imm32 offset, Register rt,
                Index mode = Offset, Condition cc = Always);

    void ma_dtr(LoadStore ls, Register rn, Register rm, Register rt,
                Index mode = Offset, Condition cc = Always);


    void ma_str(Register rt, DTRAddr addr, Index mode = Offset, Condition cc = Always);
    void ma_str(Register rt, const Operand &addr, Index mode = Offset, Condition cc = Always);
    void ma_dtr(LoadStore ls, Register rt, const Operand &addr, Index mode, Condition cc);

    void ma_ldr(DTRAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldr(const Operand &addr, Register rt, Index mode = Offset, Condition cc = Always);

    void ma_ldrb(DTRAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldrh(EDtrAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldrsh(EDtrAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldrsb(EDtrAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldrd(EDtrAddr addr, Register rt, DebugOnly<Register> rt2, Index mode = Offset, Condition cc = Always);
    void ma_strd(Register rt, DebugOnly<Register> rt2, EDtrAddr addr, Index mode = Offset, Condition cc = Always);
    
    void ma_dataTransferN(LoadStore ls, int size, bool IsSigned,
                          Register rn, Register rm, Register rt,
                          Index mode = Offset, Condition cc = Always);

    void ma_dataTransferN(LoadStore ls, int size, bool IsSigned,
                          Register rn, Imm32 offset, Register rt,
                          Index mode = Offset, Condition cc = Always);
    void ma_pop(Register r);
    void ma_push(Register r);

    void ma_vpop(VFPRegister r);
    void ma_vpush(VFPRegister r);

    
    void ma_b(Label *dest, Condition c = Always);
    void ma_bx(Register dest, Condition c = Always);

    void ma_b(void *target, Relocation::Kind reloc, Condition c = Always);

    
    
    void ma_bl(Label *dest, Condition c = Always);


    
    void ma_vadd(FloatRegister src1, FloatRegister src2, FloatRegister dst);
    void ma_vsub(FloatRegister src1, FloatRegister src2, FloatRegister dst);

    void ma_vmul(FloatRegister src1, FloatRegister src2, FloatRegister dst);
    void ma_vdiv(FloatRegister src1, FloatRegister src2, FloatRegister dst);

    void ma_vmov(FloatRegister src, FloatRegister dest);

    void ma_vimm(double value, FloatRegister dest);

    void ma_vcmp(FloatRegister src1, FloatRegister src2);

    
    void ma_vcvt_F64_I32(FloatRegister src, FloatRegister dest);

    
    void ma_vcvt_I32_F64(FloatRegister src, FloatRegister dest);

    void ma_vxfer(FloatRegister src, Register dest);

    void ma_vdtr(LoadStore ls, const Operand &addr, FloatRegister dest, Condition cc = Always);

    void ma_vldr(VFPAddr addr, FloatRegister dest);
    void ma_vldr(const Operand &addr, FloatRegister dest);

    void ma_vstr(FloatRegister src, VFPAddr addr);
    void ma_vstr(FloatRegister src, const Operand &addr);

    void ma_vstr(FloatRegister src, Register base, Register index, int32 shift = defaultShift);
    
    void ma_callIon(const Register reg);
    
    void ma_callIonNoPush(const Register reg);
    
    void ma_callIonHalfPush(const Register reg);

    void ma_call(void *dest);
};

class MacroAssemblerARMCompat : public MacroAssemblerARM
{
    
    
    uint32 stackAdjust_;
    bool dynamicAlignment_;
    bool inCall_;
    bool enoughMemory_;

    
    
    
    
    
    uint32 setupABICall(uint32 arg);

  protected:
    MoveResolver moveResolver_;

    
    
    
    
    
    uint32 framePushed_;
    void adjustFrame(int value) {
        setFramePushed(framePushed_ + value);
    }
  public:
    typedef MoveResolver::MoveOperand MoveOperand;
    typedef MoveResolver::Move Move;

    MacroAssemblerARMCompat()
      : stackAdjust_(0),
        inCall_(false),
        enoughMemory_(true),
        framePushed_(0)
    { }

    bool oom() const {
        return Assembler::oom() || !enoughMemory_;
    }

  public:
    using MacroAssemblerARM::call;

    
    
    
    void j(Condition code , Label *dest)
    {
        as_b(dest, code);
    }
    void j(Label *dest)
    {
        as_b(dest, Always);
    }

    void mov(Register src, Register dest) {
        ma_mov(src, dest);
    }
    void mov(Imm32 imm, Register dest) {
        ma_mov(imm, dest);
    }
    void mov(Register src, Address dest) {
        JS_NOT_REACHED("NYI-IC");
    }
    void mov(Address src, Register dest) {
        JS_NOT_REACHED("NYI-IC");
    }

    void call(const Register reg) {
        as_blx(reg);
    }

    void call(Label *label) {
        JS_NOT_REACHED("Feature NYI");
        


    }
    void call(ImmWord word) {
        BufferOffset bo = m_buffer.nextOffset();
        addPendingJump(bo, (void*)word.value, Relocation::HARDCODED);
        ma_call((void *) word.value);
    }
    void call(IonCode *c) {
        BufferOffset bo = m_buffer.nextOffset();
        addPendingJump(bo, c->raw(), Relocation::IONCODE);
        ma_mov(Imm32((uint32)c->raw()), ScratchRegister);
        ma_callIonHalfPush(ScratchRegister);
    }
    void branch(IonCode *c) {
        BufferOffset bo = m_buffer.nextOffset();
        addPendingJump(bo, c->raw(), Relocation::IONCODE);
        ma_mov(Imm32((uint32)c->raw()), ScratchRegister);
        ma_bx(ScratchRegister);
    }
    void nop() {
        ma_nop();
    }
    void ret() {
        ma_pop(pc);
        m_buffer.markGuard();
    }
    void retn(Imm32 n) {
        
        ma_dtr(IsLoad, sp, n, pc, PostIndex);
        m_buffer.markGuard();
    }
    void push(Imm32 imm) {
        ma_mov(imm, ScratchRegister);
        ma_push(ScratchRegister);
    }
    void push(ImmGCPtr imm) {
        ma_mov(imm, ScratchRegister);
        ma_push(ScratchRegister);
    }

    CodeOffsetLabel pushWithPatch(ImmWord imm) {
        CodeOffsetLabel label = currentOffset();
        ma_movPatchable(Imm32(imm.value), ScratchRegister, Always, L_MOVWT);
        ma_push(ScratchRegister);
        return label;
    }

    void jump(Label *label) {
        as_b(label);
    }

    void neg32(const Register &reg) {
        ma_rsb(reg, Imm32(0), reg);
    }
    void test32(const Register &lhs, const Register &rhs) {
        ma_tst(lhs, rhs);
    }


    
    Register splitTagForTest(const ValueOperand &value) {
        return value.typeReg();
    }

    
    Condition testInt32(Condition cond, const ValueOperand &value);

    Condition testBoolean(Condition cond, const ValueOperand &value);
    Condition testDouble(Condition cond, const ValueOperand &value);
    Condition testNull(Condition cond, const ValueOperand &value);
    Condition testUndefined(Condition cond, const ValueOperand &value);
    Condition testString(Condition cond, const ValueOperand &value);
    Condition testObject(Condition cond, const ValueOperand &value);
    Condition testMagic(Condition cond, const ValueOperand &value);

    
    Condition testInt32(Condition cond, const Register &tag);
    Condition testBoolean(Condition cond, const Register &tag);
    Condition testNull(Condition cond, const Register &tag);
    Condition testUndefined(Condition cond, const Register &tag);
    Condition testString(Condition cond, const Register &tag);
    Condition testObject(Condition cond, const Register &tag);
    Condition testNumber(Condition cond, const Register &tag);
    Condition testMagic(Condition cond, const Register &tag);

    
    void unboxInt32(const ValueOperand &operand, const Register &dest);
    void unboxBoolean(const ValueOperand &operand, const Register &dest);
    void unboxDouble(const ValueOperand &operand, const FloatRegister &dest);
    void unboxValue(const ValueOperand &src, AnyRegister dest);

    
    
    
    Register extractObject(const Address &address, Register scratch);
    Register extractObject(const ValueOperand &value, Register scratch) {
        return value.payloadReg();
    }
    Register extractTag(const Address &address, Register scratch);
    Register extractTag(const ValueOperand &value, Register scratch) {
        return value.typeReg();
    }

    void boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest);
    void int32ValueToDouble(const ValueOperand &operand, const FloatRegister &dest);
    void loadInt32OrDouble(const Operand &src, const FloatRegister &dest);
    void loadInt32OrDouble(Register base, Register index,
                           const FloatRegister &dest, int32 shift = defaultShift);
    void loadStaticDouble(const double *dp, const FloatRegister &dest);
    
    Condition testInt32Truthy(bool truthy, const ValueOperand &operand);
    Condition testBooleanTruthy(bool truthy, const ValueOperand &operand);
    Condition testDoubleTruthy(bool truthy, const FloatRegister &reg);

    template<typename T>
    void branchTestInt32(Condition cond, const T & t, Label *label) {
        Condition c = testInt32(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestBoolean(Condition cond, const T & t, Label *label) {
        Condition c = testBoolean(cond, t);
        ma_b(label, c);
    }
    void branch32(Condition cond, Register lhs, Register rhs, Label *label) {
        ma_cmp(lhs, rhs);
        ma_b(label, cond);
    }
    void branch32(Condition cond, Register lhs, Imm32 imm, Label *label) {
        ma_cmp(lhs, imm);
        ma_b(label, cond);
    }
    void branch32(Condition cond, const Address &lhs, Register rhs, Label *label) {
        move32(lhs, ScratchRegister);
        branch32(cond, ScratchRegister, rhs, label);
    }
    void branch32(Condition cond, const Address &lhs, Imm32 rhs, Label *label) {
        move32(lhs, ScratchRegister);
        branch32(cond, ScratchRegister, rhs, label);
    }
    template<typename T>
    void branchTestDouble(Condition cond, const T & t, Label *label) {
        Condition c = testDouble(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestNull(Condition cond, const T & t, Label *label) {
        Condition c = testNull(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestObject(Condition cond, const T & t, Label *label) {
        Condition c = testObject(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestString(Condition cond, const T & t, Label *label) {
        Condition c = testString(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestUndefined(Condition cond, const T & t, Label *label) {
        Condition c = testUndefined(cond, t);
        ma_b(label, c);
    }
    template <typename T>
    void branchTestNumber(Condition cond, const T &t, Label *label) {
        cond = testNumber(cond, t);
        ma_b(label, cond);
    }
    template <typename T>
    void branchTestMagic(Condition cond, const T &t, Label *label) {
        cond = testMagic(cond, t);
        ma_b(label, cond);
    }

    template<typename T>
    void branchTestBooleanTruthy(bool b, const T & t, Label *label) {
        Condition c = testBooleanTruthy(b, t);
        ma_b(label, c);
    }
    void branchTest32(Condition cond, const Register &lhs, const Register &rhs, Label *label) {
        ma_tst(lhs, rhs);
        ma_b(label, cond);
    }
    void branchTest32(Condition cond, const Address &address, Imm32 imm, Label *label) {
        ma_ldr(Operand(address.base, address.offset), ScratchRegister);
        ma_tst(ScratchRegister, imm);
        ma_b(label, cond);
    }
    void branchPtr(Condition cond, Register lhs, Register rhs, Label *label) {
        branch32(cond, lhs, rhs, label);
    }
    void branchPtr(Condition cond, Register lhs, ImmGCPtr ptr, Label *label) {
        movePtr(ptr, ScratchRegister);
        branchPtr(cond, lhs, ScratchRegister, label);
    }
    void branchPtr(Condition cond, Register lhs, ImmWord imm, Label *label) {
        branch32(cond, lhs, Imm32(imm.value), label);
    }
    void moveValue(const Value &val, Register type, Register data);

    CodeOffsetJump jumpWithPatch(Label *label) {
        CodeOffsetJump ret(nextOffset().getOffset());
        jump(label);
        return ret;
    }
    CodeOffsetJump branchPtrWithPatch(Condition cond, Address addr, ImmGCPtr ptr, Label *label) {
        
        
        
        
        
        
        
        
        ma_ldr(addr, lr);
        ma_cmp(lr, ptr);
        CodeOffsetJump ret(nextOffset().getOffset());
        ma_b(label, cond);
        return ret;
    }

    void loadUnboxedValue(Address address, AnyRegister dest) {
        if (dest.isFloat())
            loadInt32OrDouble(Operand(address), dest.fpu());
        else
            ma_ldr(address, dest.gpr());
    }

    void moveValue(const Value &val, const ValueOperand &dest);

    void storeValue(ValueOperand val, Operand dst);
    void storeValue(ValueOperand val, Register base, Register index, int32 shift = defaultShift);
    void storeValue(ValueOperand val, const Address &dest) {
        storeValue(val, Operand(dest));
    }
    void storeValue(JSValueType type, Register reg, Address dest) {
        ma_mov(ImmTag(JSVAL_TYPE_TO_TAG(type)), ScratchRegister);
        ma_str(ScratchRegister, Address(dest.base, dest.offset + 4));
        ma_str(reg, dest);
    }
    void storeValue(ValueOperand val, const BaseIndex &dest) {
        
        JS_ASSERT(dest.offset == 0);
        storeValue(val, dest.base, dest.index);
    }
    void storeValue(const Value &val, Address dest) {
        jsval_layout jv = JSVAL_TO_IMPL(val);
        ma_mov(Imm32(jv.s.tag), ScratchRegister);
        ma_str(ScratchRegister, Address(dest.base, dest.offset + 4));
        if (val.isGCThing())
            ma_mov(ImmGCPtr(reinterpret_cast<gc::Cell *>(val.toGCThing())), ScratchRegister);
        else
            ma_mov(Imm32(jv.s.payload.i32), ScratchRegister);
        ma_str(ScratchRegister, dest);
    }

    void loadValue(Address src, ValueOperand val);
    void loadValue(Operand dest, ValueOperand val) {
        loadValue(dest.toAddress(), val);
    }
    void loadValue(Register base, Register index, ValueOperand val);
    void loadValue(const BaseIndex &addr, ValueOperand val) {
        
        JS_ASSERT(addr.offset == 0);
        loadValue(addr.base, addr.index, val);
    }
    void pushValue(ValueOperand val);
    void popValue(ValueOperand val);
    void pushValue(const Value &val) {
        JS_NOT_REACHED("NYI");
    }
    void pushValue(JSValueType type, Register reg) {
        JS_NOT_REACHED("NYI");
    }
    void storePayload(const Value &val, Operand dest);
    void storePayload(Register src, Operand dest);
    void storePayload(const Value &val, Register base, Register index, int32 shift = defaultShift);
    void storePayload(Register src, Register base, Register index, int32 shift = defaultShift);
    void storeTypeTag(ImmTag tag, Operand dest);
    void storeTypeTag(ImmTag tag, Register base, Register index, int32 shift = defaultShift);
    void makeFrameDescriptor(Register frameSizeReg, FrameType type) {
        ma_lsl(Imm32(FRAMETYPE_BITS), frameSizeReg, frameSizeReg);
        ma_orr(Imm32(type), frameSizeReg);
    }

    void loadDouble(Address addr, FloatRegister dest) {
        ma_vldr(Operand(addr), dest);
    }
    void storeDouble(FloatRegister src, Address addr) {
        ma_vstr(src, Operand(addr));
    }

    void linkExitFrame();
    void handleException();

    
    
    
  public:
    
    void Push(const Register &reg) {
        ma_push(reg);
        adjustFrame(STACK_SLOT_SIZE);
    }
    void Push(const Imm32 imm) {
        push(imm);
        adjustFrame(STACK_SLOT_SIZE);
    }
    void Push(const ImmGCPtr ptr) {
        push(ptr);
        adjustFrame(STACK_SLOT_SIZE);
    }
    void Push(const FloatRegister &t) {
        JS_NOT_REACHED("NYI");
    }
    void Pop(const Register &reg) {
        ma_pop(reg);
        adjustFrame(-STACK_SLOT_SIZE);
    }
    void implicitPop(uint32 args) {
        JS_ASSERT(args % STACK_SLOT_SIZE == 0);
        adjustFrame(-args);
    }
    uint32 framePushed() const {
        return framePushed_;
    }
    void setFramePushed(uint32 framePushed) {
        framePushed_ = framePushed;
    }

    
    
    uint32 buildFakeExitFrame(const Register &scratch);

    void callWithExitFrame(IonCode *target);

    
    
    void callIon(const Register &callee);

    void reserveStack(uint32 amount);
    void freeStack(uint32 amount);

    void add32(const Imm32 &imm, const Register &dest);
    void sub32(const Imm32 &imm, const Register &dest);

    void move32(const Imm32 &imm, const Register &dest);

    void move32(const Address &src, const Register &dest);
    void movePtr(const Register &src, const Register &dest);
    void movePtr(const ImmWord &imm, const Register &dest);
    void movePtr(const ImmGCPtr &imm, const Register &dest);
    void movePtr(const Address &src, const Register &dest);

    void load16(const Address &address, const Register &dest);
    void load32(const Address &address, const Register &dest);
    void load32(const ImmWord &imm, const Register &dest);
    void loadPtr(const Address &address, const Register &dest);
    void loadPtr(const ImmWord &imm, const Register &dest);
    void loadPrivate(const Address &address, const Register &dest);

    void store32(Register src, const ImmWord &imm);
    void store32(Register src, const Address &address);
    void store32(Imm32 src, const Address &address);
    void storePtr(Register src, const Address &address);
    void storePtr(Register src, const ImmWord &imm);

    void cmp32(const Register &lhs, const Imm32 &rhs);
    void cmp32(const Register &lhs, const Register &rhs);
    void cmpPtr(const Register &lhs, const ImmWord &rhs);

    void subPtr(Imm32 imm, const Register dest);
    void addPtr(Imm32 imm, const Register dest);

    void setStackArg(const Register &reg, uint32 arg);

    void breakpoint();

    Condition compareDoubles(JSOp compare, FloatRegister lhs, FloatRegister rhs);
    void checkStackAlignment();

    void rshiftPtr(Imm32 imm, const Register &dest) {
        ma_lsr(imm, dest, dest);
    }

    
    
    
    
    
    
    
    void setupAlignedABICall(uint32 args);

    
    
    void setupUnalignedABICall(uint32 args, const Register &scratch);

    
    
    
    
    
    
    void setABIArg(uint32 arg, const MoveOperand &from);
    void setABIArg(uint32 arg, const Register &reg);

    
    void callWithABI(void *fun);

    CodeOffsetLabel labelForPatch() {
        return CodeOffsetLabel(nextOffset().getOffset());
    }

};

typedef MacroAssemblerARMCompat MacroAssemblerSpecific;

} 
} 

#endif 
