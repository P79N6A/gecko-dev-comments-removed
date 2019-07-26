






#ifndef jsion_macro_assembler_arm_h__
#define jsion_macro_assembler_arm_h__

#include "mozilla/DebugOnly.h"

#include "ion/arm/Assembler-arm.h"
#include "ion/IonCaches.h"
#include "ion/IonFrames.h"
#include "ion/MoveResolver.h"
#include "jsopcode.h"

using mozilla::DebugOnly;

namespace js {
namespace ion {

static Register CallReg = ip;
static const int defaultShift = 3;
JS_STATIC_ASSERT(1 << defaultShift == sizeof(jsval));


class MacroAssemblerARM : public Assembler
{
  protected:
    
    
    
    
    Register secondScratchReg_;

  public:
    MacroAssemblerARM()
      : secondScratchReg_(lr)
    { }

    void setSecondScratchReg(Register reg) {
        JS_ASSERT(reg != ScratchRegister);
        secondScratchReg_ = reg;
    }

    void convertInt32ToDouble(const Register &src, const FloatRegister &dest);
    void convertUInt32ToDouble(const Register &src, const FloatRegister &dest);
    void convertDoubleToFloat(const FloatRegister &src, const FloatRegister &dest);
    void branchTruncateDouble(const FloatRegister &src, const Register &dest, Label *fail);
    void convertDoubleToInt32(const FloatRegister &src, const Register &dest, Label *fail,
                              bool negativeZeroCheck = true);

    void negateDouble(FloatRegister reg);

    void inc64(AbsoluteAddress dest);

    
    
    
    
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
    void ma_movPatchable(Imm32 imm, Register dest, Assembler::Condition c,
                         RelocStyle rs, Instruction *i = NULL);
    
    
    
    
    
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

    
    void ma_neg(Register src, Register dest,
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
    void ma_cmp(Register src1, ImmWord ptr, Condition c = Always);
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

    
    
    void ma_mod_mask(Register src, Register dest, Register hold, int32_t shift);

    
    
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
    void ma_strb(Register rt, DTRAddr addr, Index mode = Offset, Condition cc = Always);
    void ma_strh(Register rt, EDtrAddr addr, Index mode = Offset, Condition cc = Always);
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

    
    void ma_b(Label *dest, Condition c = Always, bool isPatchable = false);
    void ma_bx(Register dest, Condition c = Always);

    void ma_b(void *target, Relocation::Kind reloc, Condition c = Always);

    
    
    void ma_bl(Label *dest, Condition c = Always);

    void ma_blx(Register dest, Condition c = Always);

    
    void ma_vadd(FloatRegister src1, FloatRegister src2, FloatRegister dst);
    void ma_vsub(FloatRegister src1, FloatRegister src2, FloatRegister dst);

    void ma_vmul(FloatRegister src1, FloatRegister src2, FloatRegister dst);
    void ma_vdiv(FloatRegister src1, FloatRegister src2, FloatRegister dst);

    void ma_vneg(FloatRegister src, FloatRegister dest, Condition cc = Always);
    void ma_vmov(FloatRegister src, FloatRegister dest, Condition cc = Always);
    void ma_vabs(FloatRegister src, FloatRegister dest, Condition cc = Always);

    void ma_vsqrt(FloatRegister src, FloatRegister dest, Condition cc = Always);

    void ma_vimm(double value, FloatRegister dest, Condition cc = Always);

    void ma_vcmp(FloatRegister src1, FloatRegister src2, Condition cc = Always);
    void ma_vcmpz(FloatRegister src1, Condition cc = Always);

    
    void ma_vcvt_F64_I32(FloatRegister src, FloatRegister dest, Condition cc = Always);
    void ma_vcvt_F64_U32(FloatRegister src, FloatRegister dest, Condition cc = Always);

    
    void ma_vcvt_I32_F64(FloatRegister src, FloatRegister dest, Condition cc = Always);
    void ma_vcvt_U32_F64(FloatRegister src, FloatRegister dest, Condition cc = Always);

    void ma_vxfer(FloatRegister src, Register dest, Condition cc = Always);
    void ma_vxfer(FloatRegister src, Register dest1, Register dest2, Condition cc = Always);

    void ma_vxfer(VFPRegister src, Register dest, Condition cc = Always);
    void ma_vxfer(VFPRegister src, Register dest1, Register dest2, Condition cc = Always);

    void ma_vdtr(LoadStore ls, const Operand &addr, VFPRegister dest, Condition cc = Always);

    void ma_vldr(VFPAddr addr, VFPRegister dest, Condition cc = Always);
    void ma_vldr(const Operand &addr, VFPRegister dest, Condition cc = Always);

    void ma_vstr(VFPRegister src, VFPAddr addr, Condition cc = Always);
    void ma_vstr(VFPRegister src, const Operand &addr, Condition cc = Always);

    void ma_vstr(VFPRegister src, Register base, Register index, int32_t shift = defaultShift, Condition cc = Always);
    
    void ma_callIon(const Register reg);
    
    void ma_callIonNoPush(const Register reg);
    
    void ma_callIonHalfPush(const Register reg);

    void ma_call(void *dest);

    
    
    
    
    
    int32_t transferMultipleByRuns(FloatRegisterSet set, LoadStore ls,
                                   Register rm, DTMMode mode);
};

class MacroAssemblerARMCompat : public MacroAssemblerARM
{
    
    
    bool inCall_;
    uint32_t args_;
    
    
    uint32_t passedArgs_;

#ifdef JS_CPU_ARM_HARDFP
    uint32_t usedIntSlots_;
    uint32_t usedFloatSlots_;
    uint32_t padding_;
#else
    
    
    
    
    
    
    uint32_t usedSlots_;
#endif
    bool dynamicAlignment_;

    bool enoughMemory_;
    VFPRegister floatArgsInGPR[2];
    
    
    
    
    
    void setupABICall(uint32_t arg);

  protected:
    MoveResolver moveResolver_;

    
    
    
    
    
    uint32_t framePushed_;
    void adjustFrame(int value) {
        setFramePushed(framePushed_ + value);
    }
  public:
    typedef MoveResolver::MoveOperand MoveOperand;
    typedef MoveResolver::Move Move;

    enum Result {
        GENERAL,
        DOUBLE
    };

    MacroAssemblerARMCompat()
      : inCall_(false),
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
    void mov(ImmWord imm, Register dest) {
        ma_mov(Imm32(imm.value), dest);
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
        ma_mov(Imm32((uint32_t)c->raw()), ScratchRegister);
        ma_callIonHalfPush(ScratchRegister);
    }
    void branch(IonCode *c) {
        BufferOffset bo = m_buffer.nextOffset();
        addPendingJump(bo, c->raw(), Relocation::IONCODE);
        ma_mov(Imm32((uint32_t)c->raw()), ScratchRegister);
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
    void push(ImmWord imm) {
        push(Imm32(imm.value));
    }
    void push(ImmGCPtr imm) {
        ma_mov(imm, ScratchRegister);
        ma_push(ScratchRegister);
    }
    void push(const Register &reg) {
        ma_push(reg);
    }
    void pushWithPadding(const Register &reg, const Imm32 extraSpace) {
        Imm32 totSpace = Imm32(extraSpace.value + 4);
        ma_dtr(IsStore, sp, totSpace, reg, PreIndex);
    }
    void pushWithPadding(const Imm32 &imm, const Imm32 extraSpace) {
        Imm32 totSpace = Imm32(extraSpace.value + 4);
        
        
        ma_mov(imm, secondScratchReg_);
        ma_dtr(IsStore, sp, totSpace, secondScratchReg_, PreIndex);
    }

    void pop(const Register &reg) {
        ma_pop(reg);
    }

    void popN(const Register &reg, Imm32 extraSpace) {
        Imm32 totSpace = Imm32(extraSpace.value + 4);
        ma_dtr(IsLoad, sp, totSpace, reg, PostIndex);
    }

    CodeOffsetLabel toggledJump(Label *label);

    
    
    CodeOffsetLabel toggledCall(IonCode *target, bool enabled);

    CodeOffsetLabel pushWithPatch(ImmWord imm) {
        CodeOffsetLabel label = currentOffset();
        ma_movPatchable(Imm32(imm.value), ScratchRegister, Always, L_MOVWT);
        ma_push(ScratchRegister);
        return label;
    }

    void jump(Label *label) {
        as_b(label);
    }
    void jump(Register reg) {
        ma_bx(reg);
    }

    void neg32(Register reg) {
        ma_neg(reg, reg, SetCond);
    }
    void test32(Register lhs, Register rhs) {
        ma_tst(lhs, rhs);
    }
    void testPtr(Register lhs, Register rhs) {
        test32(lhs, rhs);
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

    Condition testPrimitive(Condition cond, const ValueOperand &value);

    
    Condition testInt32(Condition cond, const Register &tag);
    Condition testBoolean(Condition cond, const Register &tag);
    Condition testNull(Condition cond, const Register &tag);
    Condition testUndefined(Condition cond, const Register &tag);
    Condition testString(Condition cond, const Register &tag);
    Condition testObject(Condition cond, const Register &tag);
    Condition testDouble(Condition cond, const Register &tag);
    Condition testNumber(Condition cond, const Register &tag);
    Condition testMagic(Condition cond, const Register &tag);
    Condition testPrimitive(Condition cond, const Register &tag);

    Condition testGCThing(Condition cond, const Address &address);
    Condition testGCThing(Condition cond, const BaseIndex &address);
    Condition testMagic(Condition cond, const Address &address);
    Condition testMagic(Condition cond, const BaseIndex &address);

    template <typename T>
    void branchTestGCThing(Condition cond, const T &t, Label *label) {
        Condition c = testGCThing(cond, t);
        ma_b(label, c);
    }
    template <typename T>
    void branchTestPrimitive(Condition cond, const T &t, Label *label) {
        Condition c = testPrimitive(cond, t);
        ma_b(label, c);
    }

    void branchTestValue(Condition cond, const ValueOperand &value, const Value &v, Label *label);
    void branchTestValue(Condition cond, const Address &valaddr, const ValueOperand &value,
                         Label *label);

    
    void unboxInt32(const ValueOperand &operand, const Register &dest);
    void unboxInt32(const Address &src, const Register &dest);
    void unboxBoolean(const ValueOperand &operand, const Register &dest);
    void unboxBoolean(const Address &src, const Register &dest);
    void unboxDouble(const ValueOperand &operand, const FloatRegister &dest);
    void unboxValue(const ValueOperand &src, AnyRegister dest);
    void unboxPrivate(const ValueOperand &src, Register dest);

    
    void boxDouble(const FloatRegister &src, const ValueOperand &dest);
    void boxNonDouble(JSValueType type, const Register &src, const ValueOperand &dest);

    
    
    
    Register extractObject(const Address &address, Register scratch);
    Register extractObject(const ValueOperand &value, Register scratch) {
        return value.payloadReg();
    }
    Register extractTag(const Address &address, Register scratch);
    Register extractTag(const BaseIndex &address, Register scratch);
    Register extractTag(const ValueOperand &value, Register scratch) {
        return value.typeReg();
    }

    void boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest);
    void int32ValueToDouble(const ValueOperand &operand, const FloatRegister &dest);
    void loadInt32OrDouble(const Operand &src, const FloatRegister &dest);
    void loadInt32OrDouble(Register base, Register index,
                           const FloatRegister &dest, int32_t shift = defaultShift);
    void loadStaticDouble(const double *dp, const FloatRegister &dest);
    void loadConstantDouble(double dp, const FloatRegister &dest);
    
    Condition testInt32Truthy(bool truthy, const ValueOperand &operand);
    Condition testBooleanTruthy(bool truthy, const ValueOperand &operand);
    Condition testDoubleTruthy(bool truthy, const FloatRegister &reg);
    Condition testStringTruthy(bool truthy, const ValueOperand &value);

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
        load32(lhs, ScratchRegister);
        branch32(cond, ScratchRegister, rhs, label);
    }
    void branch32(Condition cond, const Address &lhs, Imm32 rhs, Label *label) {
        load32(lhs, ScratchRegister);
        branch32(cond, ScratchRegister, rhs, label);
    }
    void branchPtr(Condition cond, const Address &lhs, Register rhs, Label *label) {
        branch32(cond, lhs, rhs, label);
    }

    void branchPrivatePtr(Condition cond, const Address &lhs, ImmWord ptr, Label *label) {
        branchPtr(cond, lhs, ptr, label);
    }

    void branchPrivatePtr(Condition cond, Register lhs, ImmWord ptr, Label *label) {
        branchPtr(cond, lhs, ptr, label);
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
        
        
        if (lhs == rhs && (cond == Zero || cond == NonZero))
            ma_cmp(lhs, Imm32(0));
        else
            ma_tst(lhs, rhs);
        ma_b(label, cond);
    }
    void branchTest32(Condition cond, const Register &lhs, Imm32 imm, Label *label) {
        ma_tst(lhs, imm);
        ma_b(label, cond);
    }
    void branchTest32(Condition cond, const Address &address, Imm32 imm, Label *label) {
        ma_ldr(Operand(address.base, address.offset), ScratchRegister);
        branchTest32(cond, ScratchRegister, imm, label);
    }
    void branchTestPtr(Condition cond, const Register &lhs, const Register &rhs, Label *label) {
        branchTest32(cond, lhs, rhs, label);
    }
    void branchTestPtr(Condition cond, const Register &lhs, Imm32 imm, Label *label) {
        branchTest32(cond, lhs, imm, label);
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
    void decBranchPtr(Condition cond, const Register &lhs, Imm32 imm, Label *label) {
        subPtr(imm, lhs);
        branch32(cond, lhs, Imm32(0), label);
    }
    void moveValue(const Value &val, Register type, Register data);

    CodeOffsetJump jumpWithPatch(RepatchLabel *label, Condition cond = Always);
    template <typename T>
    CodeOffsetJump branchPtrWithPatch(Condition cond, Register reg, T ptr, RepatchLabel *label) {
        ma_cmp(reg, ptr);
        return jumpWithPatch(label, cond);
    }
    template <typename T>
    CodeOffsetJump branchPtrWithPatch(Condition cond, Address addr, T ptr, RepatchLabel *label) {
        ma_ldr(addr, secondScratchReg_);
        ma_cmp(secondScratchReg_, ptr);
        return jumpWithPatch(label, cond);
    }
    void branchPtr(Condition cond, Address addr, ImmGCPtr ptr, Label *label) {
        ma_ldr(addr, secondScratchReg_);
        ma_cmp(secondScratchReg_, ptr);
        ma_b(label, cond);
    }
    void branchPtr(Condition cond, Address addr, ImmWord ptr, Label *label) {
        ma_ldr(addr, secondScratchReg_);
        ma_cmp(secondScratchReg_, ptr);
        ma_b(label, cond);
    }
    void branchPtr(Condition cond, const AbsoluteAddress &addr, const Register &ptr, Label *label) {
        loadPtr(addr, secondScratchReg_); 
        ma_cmp(secondScratchReg_, ptr);
        ma_b(label, cond);
    }

    void loadUnboxedValue(Address address, MIRType type, AnyRegister dest) {
        if (dest.isFloat())
            loadInt32OrDouble(Operand(address), dest.fpu());
        else
            ma_ldr(address, dest.gpr());
    }

    void loadUnboxedValue(BaseIndex address, MIRType type, AnyRegister dest) {
        if (dest.isFloat())
            loadInt32OrDouble(address.base, address.index, dest.fpu(), address.scale);
        else
            load32(address, dest.gpr());
    }

    void moveValue(const Value &val, const ValueOperand &dest);

    void storeValue(ValueOperand val, Operand dst);
    void storeValue(ValueOperand val, const BaseIndex &dest);
    void storeValue(JSValueType type, Register reg, BaseIndex dest) {
        
        JS_ASSERT(dest.offset == 0);
        ma_alu(dest.base, lsl(dest.index, dest.scale), ScratchRegister, op_add);
        storeValue(type, reg, Address(ScratchRegister, 0));
    }
    void storeValue(ValueOperand val, const Address &dest) {
        storeValue(val, Operand(dest));
    }
    void storeValue(JSValueType type, Register reg, Address dest) {
        ma_mov(ImmTag(JSVAL_TYPE_TO_TAG(type)), secondScratchReg_);
        ma_str(secondScratchReg_, Address(dest.base, dest.offset + 4));
        ma_str(reg, dest);
    }
    void storeValue(const Value &val, Address dest) {
        jsval_layout jv = JSVAL_TO_IMPL(val);
        ma_mov(Imm32(jv.s.tag), secondScratchReg_);
        ma_str(secondScratchReg_, Address(dest.base, dest.offset + 4));
        if (val.isMarkable())
            ma_mov(ImmGCPtr(reinterpret_cast<gc::Cell *>(val.toGCThing())), secondScratchReg_);
        else
            ma_mov(Imm32(jv.s.payload.i32), secondScratchReg_);
        ma_str(secondScratchReg_, dest);
    }
    void storeValue(const Value &val, BaseIndex dest) {
        
        JS_ASSERT(dest.offset == 0);
        ma_alu(dest.base, lsl(dest.index, dest.scale), ScratchRegister, op_add);
        storeValue(val, Address(ScratchRegister, 0));
    }

    void loadValue(Address src, ValueOperand val);
    void loadValue(Operand dest, ValueOperand val) {
        loadValue(dest.toAddress(), val);
    }
    void loadValue(const BaseIndex &addr, ValueOperand val);
    void tagValue(JSValueType type, Register payload, ValueOperand dest);

    void pushValue(ValueOperand val);
    void popValue(ValueOperand val);
    void pushValue(const Value &val) {
        jsval_layout jv = JSVAL_TO_IMPL(val);
        push(Imm32(jv.s.tag));
        if (val.isMarkable())
            push(ImmGCPtr(reinterpret_cast<gc::Cell *>(val.toGCThing())));
        else
            push(Imm32(jv.s.payload.i32));
    }
    void pushValue(JSValueType type, Register reg) {
        push(ImmTag(JSVAL_TYPE_TO_TAG(type)));
        ma_push(reg);
    }
    void storePayload(const Value &val, Operand dest);
    void storePayload(Register src, Operand dest);
    void storePayload(const Value &val, Register base, Register index, int32_t shift = defaultShift);
    void storePayload(Register src, Register base, Register index, int32_t shift = defaultShift);
    void storeTypeTag(ImmTag tag, Operand dest);
    void storeTypeTag(ImmTag tag, Register base, Register index, int32_t shift = defaultShift);

    void makeFrameDescriptor(Register frameSizeReg, FrameType type) {
        ma_lsl(Imm32(FRAMESIZE_SHIFT), frameSizeReg, frameSizeReg);
        ma_orr(Imm32(type), frameSizeReg);
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
    void Push(const ImmWord imm) {
        push(imm);
        adjustFrame(STACK_SLOT_SIZE);
    }
    void Push(const ImmGCPtr ptr) {
        push(ptr);
        adjustFrame(STACK_SLOT_SIZE);
    }
    void Push(const FloatRegister &t) {
        VFPRegister r = VFPRegister(t);
        ma_vpush(VFPRegister(t));
        adjustFrame(r.size());
    }

    CodeOffsetLabel PushWithPatch(const ImmWord &word) {
        framePushed_ += sizeof(word.value);
        return pushWithPatch(word);
    }


    void PushWithPadding(const Register &reg, const Imm32 extraSpace) {
        pushWithPadding(reg, extraSpace);
        adjustFrame(STACK_SLOT_SIZE + extraSpace.value);
    }
    void PushWithPadding(const Imm32 imm, const Imm32 extraSpace) {
        pushWithPadding(imm, extraSpace);
        adjustFrame(STACK_SLOT_SIZE + extraSpace.value);
    }

    void Pop(const Register &reg) {
        ma_pop(reg);
        adjustFrame(-STACK_SLOT_SIZE);
    }
    void implicitPop(uint32_t args) {
        JS_ASSERT(args % STACK_SLOT_SIZE == 0);
        adjustFrame(-args);
    }
    uint32_t framePushed() const {
        return framePushed_;
    }
    void setFramePushed(uint32_t framePushed) {
        framePushed_ = framePushed;
    }

    
    
    bool buildFakeExitFrame(const Register &scratch, uint32_t *offset);
    bool buildOOLFakeExitFrame(void *fakeReturnAddr);

    void callWithExitFrame(IonCode *target);
    void callWithExitFrame(IonCode *target, Register dynStack);

    
    
    void callIon(const Register &callee);

    void reserveStack(uint32_t amount);
    void freeStack(uint32_t amount);
    void freeStack(Register amount);

    void add32(Imm32 imm, Register dest);
    void add32(Imm32 imm, const Address &dest);
    void sub32(Imm32 imm, Register dest);
    void xor32(Imm32 imm, Register dest);

    void and32(Imm32 imm, Register dest);
    void and32(Imm32 imm, const Address &dest);
    void or32(Imm32 imm, const Address &dest);
    void xorPtr(Imm32 imm, Register dest);
    void orPtr(Imm32 imm, Register dest);
    void orPtr(Register src, Register dest);
    void andPtr(Imm32 imm, Register dest);
    void addPtr(Register src, Register dest);
    void addPtr(const Address &src, Register dest);

    void move32(const Imm32 &imm, const Register &dest);

    void movePtr(const Register &src, const Register &dest);
    void movePtr(const ImmWord &imm, const Register &dest);
    void movePtr(const ImmGCPtr &imm, const Register &dest);

    void load8SignExtend(const Address &address, const Register &dest);
    void load8SignExtend(const BaseIndex &src, const Register &dest);

    void load8ZeroExtend(const Address &address, const Register &dest);
    void load8ZeroExtend(const BaseIndex &src, const Register &dest);

    void load16SignExtend(const Address &address, const Register &dest);
    void load16SignExtend(const BaseIndex &src, const Register &dest);

    void load16ZeroExtend(const Address &address, const Register &dest);
    void load16ZeroExtend(const BaseIndex &src, const Register &dest);

    void load32(const Address &address, const Register &dest);
    void load32(const BaseIndex &address, const Register &dest);
    void load32(const AbsoluteAddress &address, const Register &dest);

    void loadPtr(const Address &address, const Register &dest);
    void loadPtr(const BaseIndex &src, const Register &dest);
    void loadPtr(const AbsoluteAddress &address, const Register &dest);

    void loadPrivate(const Address &address, const Register &dest);

    void loadDouble(const Address &addr, const FloatRegister &dest);
    void loadDouble(const BaseIndex &src, const FloatRegister &dest);

    
    void loadFloatAsDouble(const Address &addr, const FloatRegister &dest);
    void loadFloatAsDouble(const BaseIndex &src, const FloatRegister &dest);

    void store8(const Register &src, const Address &address);
    void store8(const Imm32 &imm, const Address &address);
    void store8(const Register &src, const BaseIndex &address);
    void store8(const Imm32 &imm, const BaseIndex &address);

    void store16(const Register &src, const Address &address);
    void store16(const Imm32 &imm, const Address &address);
    void store16(const Register &src, const BaseIndex &address);
    void store16(const Imm32 &imm, const BaseIndex &address);

    void store32(const Register &src, const AbsoluteAddress &address);
    void store32(const Register &src, const Address &address);
    void store32(const Register &src, const BaseIndex &address);
    void store32(const Imm32 &src, const Address &address);
    void store32(const Imm32 &src, const BaseIndex &address);

    void storePtr(ImmWord imm, const Address &address);
    void storePtr(ImmGCPtr imm, const Address &address);
    void storePtr(Register src, const Address &address);
    void storePtr(const Register &src, const AbsoluteAddress &dest);
    void storeDouble(FloatRegister src, Address addr) {
        ma_vstr(src, Operand(addr));
    }
    void storeDouble(FloatRegister src, BaseIndex addr) {
        
        JS_ASSERT(addr.offset == 0);
        uint32_t scale = Imm32::ShiftOf(addr.scale).value;
        ma_vstr(src, addr.base, addr.index, scale);
    }

    void storeFloat(FloatRegister src, Address addr) {
        ma_vstr(VFPRegister(src).singleOverlay(), Operand(addr));
    }
    void storeFloat(FloatRegister src, BaseIndex addr) {
        
        JS_ASSERT(addr.offset == 0);
        uint32_t scale = Imm32::ShiftOf(addr.scale).value;
        ma_vstr(VFPRegister(src).singleOverlay(), addr.base, addr.index, scale);
    }

    void clampIntToUint8(Register src, Register dest) {
        
        
        as_mov(ScratchRegister, asr(src, 8), SetCond);
        ma_mov(src, dest);
        ma_mov(Imm32(0xff), dest, NoSetCond, NotEqual);
        ma_mov(Imm32(0), dest, NoSetCond, Signed);
    }

    void cmp32(const Register &lhs, const Imm32 &rhs);
    void cmp32(const Register &lhs, const Register &rhs);
    void cmp32(const Operand &lhs, const Imm32 &rhs);
    void cmp32(const Operand &lhs, const Register &rhs);
    void cmpPtr(const Register &lhs, const ImmWord &rhs);
    void cmpPtr(const Register &lhs, const Register &rhs);
    void cmpPtr(const Register &lhs, const ImmGCPtr &rhs);
    void cmpPtr(const Address &lhs, const Register &rhs);
    void cmpPtr(const Address &lhs, const ImmWord &rhs);

    void subPtr(Imm32 imm, const Register dest);
    void subPtr(const Address &addr, const Register dest);
    void subPtr(const Register &src, const Register &dest);
    void addPtr(Imm32 imm, const Register dest);
    void addPtr(Imm32 imm, const Address &dest);
    void addPtr(ImmWord imm, const Register dest) {
        addPtr(Imm32(imm.value), dest);
    }

    void setStackArg(const Register &reg, uint32_t arg);

    void breakpoint();
    
    void breakpoint(Condition cc);

    void compareDouble(FloatRegister lhs, FloatRegister rhs);
    void branchDouble(DoubleCondition cond, const FloatRegister &lhs, const FloatRegister &rhs,
                      Label *label);

    void checkStackAlignment();

    void rshiftPtr(Imm32 imm, Register dest) {
        ma_lsr(imm, dest, dest);
    }
    void lshiftPtr(Imm32 imm, Register dest) {
        ma_lsl(imm, dest, dest);
    }

    void
    emitSet(Assembler::Condition cond, const Register &dest)
    {
        ma_mov(Imm32(0), dest);
        ma_mov(Imm32(1), dest, NoSetCond, cond);
    }

    
    
    
    
    
    
    
    void setupAlignedABICall(uint32_t args);

    
    
    void setupUnalignedABICall(uint32_t args, const Register &scratch);

    
    
    
    
    
    void passABIArg(const MoveOperand &from);
    void passABIArg(const Register &reg);
    void passABIArg(const FloatRegister &reg);
    void passABIArg(const ValueOperand &regs);

  private:
    void callWithABIPre(uint32_t *stackAdjust);
    void callWithABIPost(uint32_t stackAdjust, Result result);

  public:
    
    void callWithABI(void *fun, Result result = GENERAL);
    void callWithABI(const Address &fun, Result result = GENERAL);

    CodeOffsetLabel labelForPatch() {
        return CodeOffsetLabel(nextOffset().getOffset());
    }

    void computeEffectiveAddress(const Address &address, Register dest) {
        ma_add(address.base, Imm32(address.offset), dest, NoSetCond);
    }
    void computeEffectiveAddress(const BaseIndex &address, Register dest) {
        ma_alu(address.base, lsl(address.index, address.scale), dest, op_add, NoSetCond);
        if (address.offset)
            ma_add(dest, Imm32(address.offset), dest, NoSetCond);
    }
    void floor(FloatRegister input, Register output, Label *handleNotAnInt);
    void round(FloatRegister input, Register output, Label *handleNotAnInt, FloatRegister tmp);

    void clampCheck(Register r, Label *handleNotAnInt) {
        
        
        
        ma_sub(r, Imm32(0x80000001), ScratchRegister);
        ma_cmn(ScratchRegister, Imm32(3));
        ma_b(handleNotAnInt, Above);
    }

    void enterOsr(Register calleeToken, Register code);
};

typedef MacroAssemblerARMCompat MacroAssemblerSpecific;

} 
} 

#endif 
