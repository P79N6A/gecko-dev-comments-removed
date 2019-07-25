








































#include "Assembler-arm.h"
#include "jsgcmark.h"

using namespace js;
using namespace js::ion;
uint32
js::ion::RT(Register r)
{
    return r.code() << 12;
}

uint32
js::ion::RN(Register r)
{
    return r.code() << 16;
}

uint32
js::ion::RD(Register r)
{
    return r.code() << 12;
}
uint32
js::ion::VD(VFPRegister vr)
{
    if (vr.isMissing())
        return 0;
    
    VFPRegister::VFPRegIndexSplit s = vr.encode();
    return s.bit << 22 | s.block << 12;
}
uint32
js::ion::VN(VFPRegister vr)
{
    if (vr.isMissing())
        return 0;
    
    VFPRegister::VFPRegIndexSplit s = vr.encode();
    return s.bit << 7 | s.block << 16;
}
uint32
js::ion::VM(VFPRegister vr)
{
    if (vr.isMissing())
        return 0;
    
    VFPRegister::VFPRegIndexSplit s = vr.encode();
    return s.bit << 5 | s.block;
}


VFPRegister::VFPRegIndexSplit
ion::VFPRegister::encode()
{
    JS_ASSERT(!_isInvalid);

    switch (kind) {
      case Double:
        return VFPRegIndexSplit(_code &0xf , _code >> 4);
      case Single:
        return VFPRegIndexSplit(_code >> 1, _code & 1);
      default:
        JS_ASSERT(_code &0xf == _code);
        return VFPRegIndexSplit(_code, 0);
    }
}

VFPRegister js::ion::NoVFPRegister(true);

void
Assembler::executableCopy(uint8 *buffer)
{
    executableCopy((void*)buffer);
    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        JS_NOT_REACHED("Feature NYI");
        
    }
    JSC::ExecutableAllocator::cacheFlush(buffer, m_buffer.size());
}

class RelocationIterator
{
    CompactBufferReader reader_;
    uint32 offset_;

  public:
    RelocationIterator(CompactBufferReader &reader)
      : reader_(reader)
    { }

    bool read() {
        if (!reader_.more())
            return false;
        offset_ = reader_.readUnsigned();
        return true;
    }

    uint32 offset() const {
        return offset_;
    }
};

static inline IonCode *
CodeFromJump(uint8 *jump)
{
    JS_NOT_REACHED("Feature NYI");
#if 0
    uint8 *target = (uint8 *)JSC::ARMAssembler::getRel32Target(jump);
    return IonCode::FromExecutable(target);
#endif
    return NULL;
}

void
Assembler::TraceJumpRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader)
{
    JS_NOT_REACHED("Feature NYI");
#if 0
    RelocationIterator iter(reader);
    while (iter.read()) {
        IonCode *child = CodeFromJump(code->raw() + iter.offset());
        MarkIonCode(trc, child, "rel32");
    };
#endif
}

void
Assembler::TraceDataRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader)
{
    JS_NOT_REACHED("Feature NYI");
}

void
Assembler::copyJumpRelocationTable(uint8 *dest)
{
    if (jumpRelocations_.length())
        memcpy(dest, jumpRelocations_.buffer(), jumpRelocations_.length());
}

void
Assembler::copyDataRelocationTable(uint8 *dest)
{
    if (dataRelocations_.length())
        memcpy(dest, dataRelocations_.buffer(), dataRelocations_.length());
}

void
Assembler::trace(JSTracer *trc)
{
    JS_NOT_REACHED("Feature NYI - must trace jump and data");
#if 0
    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        if (rp.kind == Relocation::CODE)
            MarkIonCode(trc, IonCode::FromExecutable((uint8 *)rp.target), "masmrel32");
    }
#endif
}

void
Assembler::executableCopy(void *buffer)
{
    ASSERT(m_buffer.sizeOfConstantPool() == 0);
    memcpy(buffer, m_buffer.data(), m_buffer.size());
    
    
    
    
    
    
#if 0
    fixUpOffsets(buffer);
#endif
}

void
Assembler::processDeferredData(IonCode *code, uint8 *data)
{
    for (size_t i = 0; i < data_.length(); i++) {
        DeferredData *deferred = data_[i];
        Bind(code, deferred->label(), data + deferred->offset());
        deferred->copy(code, data + deferred->offset());
    }
}

void
Assembler::processCodeLabels(IonCode *code)
{
    for (size_t i = 0; i < codeLabels_.length(); i++) {
        CodeLabel *label = codeLabels_[i];
        Bind(code, label->dest(), code->raw() + label->src()->offset());
    }
}

Assembler::Condition
Assembler::InvertCondition(Condition cond)
{
    switch (cond) {
      case Equal:
        
        return NotEqual;
      case Above:
        return BelowOrEqual;
      case AboveOrEqual:
        return Below;
      case Below:
        return AboveOrEqual;
      case BelowOrEqual:
        return Above;
      case LessThan:
        return GreaterThanOrEqual;
        
        
      case GreaterThan:
        return LessThanOrEqual;
      case GreaterThanOrEqual:
        return LessThan;
        
      default:
        JS_NOT_REACHED("Comparisons other than LT, LE, GT, GE not yet supported");
        return Equal;
    }

}

Imm8::TwoImm8mData
Imm8::encodeTwoImms(uint32 imm)
{
    
    
    
    
    
    
    
    
    
    
    
    uint32 imm1, imm2;
    int left = (js_bitscan_clz32(imm)) & 30;
    uint32 no_n1 = imm & ~(0xff << (24 - left));
    
    
    
    if (no_n1 == 0)
        return TwoImm8mData();
    int mid = ((js_bitscan_clz32(no_n1)) & 30);
    uint32 no_n2 =
        no_n1  & ~((0xff << ((24 - mid)&31)) | 0xff >> ((8 + mid)&31));
    if (no_n2 == 0) {
        
        
        int imm1shift = left + 8;
        int imm2shift = mid + 8;
        imm1 = (imm >> (32 - imm1shift)) & 0xff;
        if (imm2shift >= 32) {
            imm2shift = 0;
            
            
            
            imm2 = no_n1;
        } else {
            imm2 = ((imm >> (32 - imm2shift)) | (imm << imm2shift)) & 0xff;
            JS_ASSERT( ((no_n1 >> (32 - imm2shift)) | (no_n1 << imm2shift)) ==
                       imm2);
        }
        return TwoImm8mData(datastore::Imm8mData(imm1,imm1shift),
                            datastore::Imm8mData(imm2, imm2shift));
    } else {
        
        
        if (left >= 8)
            return TwoImm8mData();
        int right = 32 - (js_bitscan_clz32(no_n2) & 30);
        
        
        if (right > 8) {
            return TwoImm8mData();
        }
        
        
        if (((imm & (0xff << (24 - left))) << (8-right)) != 0) {
            
            
            
            
            
            no_n1 = imm & ~((0xff >> (8-right)) | (0xff << (24 + right)));
            mid = (js_bitscan_clz32(no_n1)) & 30;
            no_n2 =
                no_n1  & ~((0xff << ((24 - mid)&31)) | 0xff >> ((8 + mid)&31));
            if (no_n2 != 0) {
                return TwoImm8mData();
            }
        }
        
        
        int imm1shift = 8 - right;
        imm1 = 0xff & ((imm << imm1shift) | (imm >> (32 - imm1shift)));
        JS_ASSERT ((imm1shift&~0x1e) == 0);
        
        
        
        
        int imm2shift =  mid + 8;
        imm2 = ((imm >> (32 - imm2shift)) | (imm << imm2shift)) & 0xff;
        return TwoImm8mData(datastore::Imm8mData(imm1,imm1shift),
                            datastore::Imm8mData(imm2, imm2shift));
    }
}

ALUOp
ion::ALUNeg(ALUOp op, Imm32 *imm)
{
    
    switch (op) {
      case op_mov:
        *imm = Imm32(~imm->value);
        return op_mvn;
      case op_mvn:
        *imm = Imm32(~imm->value);
        return op_mov;
      case op_and:
        *imm = Imm32(~imm->value);
        return op_bic;
      case op_bic:
        *imm = Imm32(~imm->value);
        return op_and;
      case op_add:
        *imm = Imm32(-imm->value);
        return op_sub;
      case op_sub:
        *imm = Imm32(-imm->value);
        return op_add;
      case op_cmp:
        *imm = Imm32(-imm->value);
        return op_cmn;
      case op_cmn:
        *imm = Imm32(-imm->value);
        return op_cmp;
        
      default:
        break;
    }
    return op_invalid;
}

bool
ion::can_dbl(ALUOp op)
{
    
    
    return false;
}

bool
ion::condsAreSafe(ALUOp op) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return false;
}

ALUOp
ion::getDestVariant(ALUOp op)
{
    
    
    switch (op) {
      case op_cmp:
        return op_sub;
      case op_cmn:
        return op_add;
      case op_tst:
        return op_and;
      case op_teq:
        return op_eor;
    }
    return op;
}

O2RegImmShift
ion::O2Reg(Register r) {
    return O2RegImmShift(r, LSL, 0);
}

O2RegImmShift
ion::lsl(Register r, int amt)
{
    return O2RegImmShift(r, LSL, amt);
}

O2RegImmShift
ion::lsr(Register r, int amt)
{
    return O2RegImmShift(r, LSR, amt);
}

O2RegImmShift
ion::ror(Register r, int amt)
{
    return O2RegImmShift(r, ROR, amt);
}
O2RegImmShift
ion::rol(Register r, int amt)
{
    return O2RegImmShift(r, ROR, 32 - amt);
}

O2RegImmShift
ion::asr (Register r, int amt)
{
    return O2RegImmShift(r, ASR, amt);
}


O2RegRegShift
ion::lsl(Register r, Register amt)
{

    return O2RegRegShift(r, LSL, amt);
}

O2RegRegShift
ion::lsr(Register r, Register amt)
{
    return O2RegRegShift(r, LSR, amt);
}

O2RegRegShift
ion::ror(Register r, Register amt)
{
    return O2RegRegShift(r, ROR, amt);
}

O2RegRegShift
ion::asr (Register r, Register amt)
{
    return O2RegRegShift(r, ASR, amt);
}


js::ion::VFPImm::VFPImm(uint32 top)
{
    data = -1;
    datastore::Imm8VFPImmData tmp;
    if (DoubleEncoder::lookup(top, &tmp)) {
        data = tmp.encode();
    }
}

js::ion::DoubleEncoder js::ion::DoubleEncoder::_this;


VFPRegister
VFPRegister::doubleOverlay()
{
    JS_ASSERT(!_isInvalid);
    if (kind != Double) {
        return VFPRegister(_code >> 1, Double);
    } else {
        return *this;
    }
}
VFPRegister
VFPRegister::singleOverlay()
{
    JS_ASSERT(!_isInvalid);
    if (kind == Double) {
        
        ASSERT(_code < 16);
        return VFPRegister(_code << 1, Double);
    } else {
        return VFPRegister(_code, Single);
    }
}

VFPRegister
VFPRegister::intOverlay()
{
    JS_ASSERT(!_isInvalid);
    if (kind == Double) {
        
        ASSERT(_code < 16);
        return VFPRegister(_code << 1, Double);
    } else {
        return VFPRegister(_code, Int);
    }
}
bool
VFPRegister::isInvalid()
{
    return _isInvalid;
}

bool
VFPRegister::isMissing()
{
    JS_ASSERT(!_isInvalid);
    return _isMissing;
}


bool
Assembler::oom() const
{
    return m_buffer.oom() ||
        !enoughMemory_ ||
        jumpRelocations_.oom();
}

bool
Assembler::addDeferredData(DeferredData *data, size_t bytes)
{
    data->setOffset(dataBytesNeeded_);
    dataBytesNeeded_ += bytes;
    if (dataBytesNeeded_ >= MAX_BUFFER_SIZE)
        return false;
    return data_.append(data);
}

bool
Assembler::addCodeLabel(CodeLabel *label)
{
    return codeLabels_.append(label);
}


size_t
Assembler::size() const
{
    return m_buffer.uncheckedSize();
}

size_t
Assembler::jumpRelocationTableBytes() const
{
    return jumpRelocations_.length();
}
size_t
Assembler::dataRelocationTableBytes() const
{
    return dataRelocations_.length();
}


size_t
Assembler::dataSize() const
{
    return dataBytesNeeded_;
}
size_t
Assembler::bytesNeeded() const
{
    return size() +
        dataSize() +
        jumpRelocationTableBytes() +
        dataRelocationTableBytes();
}

void
Assembler::writeInst(uint32 x)
{
    m_buffer.putInt(x);
}

void
Assembler::align(int alignment)
{
    while (!m_buffer.isAligned(alignment))
        as_mov(r0, O2Reg(r0));

}
void
Assembler::as_alu(Register dest, Register src1, Operand2 op2,
                ALUOp op, SetCond_ sc, Condition c)
{
    writeInst((int)op | (int)sc | (int) c | op2.encode() |
              ((dest == InvalidReg) ? 0 : RD(dest)) |
              ((src1 == InvalidReg) ? 0 : RN(src1)));
}
void
Assembler::as_mov(Register dest,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, InvalidReg, op2, op_mov, sc, c);
}
void
Assembler::as_mvn(Register dest, Operand2 op2,
                SetCond_ sc, Condition c)
{
    as_alu(dest, InvalidReg, op2, op_mvn, sc, c);
}

void
Assembler::as_and(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_and, sc, c);
}
void
Assembler::as_bic(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_bic, sc, c);
}
void
Assembler::as_eor(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_eor, sc, c);
}
void
Assembler::as_orr(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_orr, sc, c);
}

void
Assembler::as_adc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_adc, sc, c);
}
void
Assembler::as_add(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_add, sc, c);
}
void
Assembler::as_sbc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_sbc, sc, c);
}
void
Assembler::as_sub(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_sub, sc, c);
}
void
Assembler::as_rsb(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_rsb, sc, c);
}
void
Assembler::as_rsc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op_rsc, sc, c);
}

void
Assembler::as_cmn(Register src1, Operand2 op2,
                Condition c)
{
    as_alu(InvalidReg, src1, op2, op_cmn, SetCond, c);
}
void
Assembler::as_cmp(Register src1, Operand2 op2,
                Condition c)
{
    as_alu(InvalidReg, src1, op2, op_cmp, SetCond, c);
}
void
Assembler::as_teq(Register src1, Operand2 op2,
                Condition c)
{
    as_alu(InvalidReg, src1, op2, op_teq, SetCond, c);
}
void
Assembler::as_tst(Register src1, Operand2 op2,
                Condition c)
{
    as_alu(InvalidReg, src1, op2, op_tst, SetCond, c);
}




void
Assembler::as_movw(Register dest, Imm16 imm, Condition c)
{
    JS_ASSERT(hasMOVWT());
    writeInst(0x03000000 | c | imm.encode() | RD(dest));
}
void
Assembler::as_movt(Register dest, Imm16 imm, Condition c)
{
    JS_ASSERT(hasMOVWT());
    writeInst(0x03400000 | c | imm.encode() | RD(dest));
}



void
Assembler::as_dtr(LoadStore ls, int size, Index mode,
                Register rt, DTRAddr addr, Condition c)
{
    JS_ASSERT(size == 32 || size == 8);
    writeInst( 0x04000000 | ls | (size == 8 ? 0x00400000 : 0) | mode | c |
               RT(rt) | addr.encode());
    return;
}



void
Assembler::as_extdtr(LoadStore ls, int size, bool IsSigned, Index mode,
                   Register rt, EDtrAddr addr, Condition c)
{
    int extra_bits2 = 0;
    int extra_bits1 = 0;
    switch(size) {
      case 8:
        JS_ASSERT(IsSigned);
        JS_ASSERT(ls!=IsStore);
        break;
      case 16:
        
        
        extra_bits2 = 0x01;
        extra_bits1 = (ls == IsStore) ? 0 : 1;
        if (IsSigned) {
            JS_ASSERT(ls != IsStore);
            extra_bits2 |= 0x2;
        }
        break;
      case 64:
        if (ls == IsStore) {
            extra_bits2 = 0x3;
        } else {
            extra_bits2 = 0x2;
        }
        extra_bits1 = 0;
        break;
      default:
        JS_NOT_REACHED("SAY WHAT?");
    }
    writeInst(extra_bits2 << 5 | extra_bits1 << 20 | 0x90 |
              addr.encode() | RT(rt) | c);
    return;
}

void
Assembler::as_dtm(LoadStore ls, Register rn, uint32 mask,
                DTMMode mode, DTMWriteBack wb, Condition c)
{
    writeInst(0x08000000 | RN(rn) | ls |
              mode | mask | c | wb);

    return;
}





void
Assembler::as_bx(Register r, Condition c)
{
    writeInst(((int) c) | op_bx | r.code());
}




void
Assembler::as_b(BOffImm off, Condition c)
{
    writeInst(((int)c) | op_b | off.encode());
}

void
Assembler::as_b(Label *l, Condition c)
{
    BufferOffset next = nextOffset();
    if (l->bound()) {
        as_b(BufferOffset(l).diffB(next), c);
    } else {
        
        int32 old = l->use(next.getOffset());
        if (old == LabelBase::INVALID_OFFSET) {
            old = -4;
        }
        
        
        as_b(BOffImm(old), c);
    }
}
void
Assembler::as_b(BOffImm off, Condition c, BufferOffset inst)
{
    *editSrc(inst) = ((int)c) | op_b | off.encode();
}





void
Assembler::as_blx(Label *l)
{
    JS_NOT_REACHED("Feature NYI");
}

void
Assembler::as_blx(Register r, Condition c)
{
    writeInst(((int) c) | op_blx | r.code());
}
void
Assembler::as_bl(BOffImm off, Condition c)
{
    writeInst(((int)c) | op_bl | off.encode());
}


void
Assembler::as_bl()
{
    JS_NOT_REACHED("Feature NYI");
}


void
Assembler::as_bl(Label *l, Condition c)
{
    BufferOffset next = nextOffset();
    if (l->bound()) {
        as_bl(BufferOffset(l).diffB(next), c);
    } else {
        int32 old = l->use(next.getOffset());
        
        if (old == -1) {
            old = -4;
        }
        
        
        as_bl(BOffImm(old), c);
    }
}
void
Assembler::as_bl(BOffImm off, Condition c, BufferOffset inst)
{
    *editSrc(inst) = ((int)c) | op_bl | off.encode();
}





void
Assembler::as_vfp_float(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  VFPOp op, Condition c)
{
    
    JS_ASSERT(vd.equiv(vn) && vd.equiv(vm));
    vfp_size sz = isDouble;
    if (!vd.isDouble()) {
        sz = isSingle;
    }
    writeInst(VD(vd) | VN(vn) | VM(vm) | op | c | sz | 0x0e000a00);
}

void
Assembler::as_vadd(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c)
{
    as_vfp_float(vd, vn, vm, opv_add, c);
}

void
Assembler::as_vdiv(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c)
{
    as_vfp_float(vd, vn, vm, opv_mul, c);
}

void
Assembler::as_vmul(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c)
{
    as_vfp_float(vd, vn, vm, opv_mul, c);
}

void
Assembler::as_vnmul(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c)
{
    as_vfp_float(vd, vn, vm, opv_mul, c);
    JS_NOT_REACHED("Feature NYI");
}

void
Assembler::as_vnmla(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c)
{
    JS_NOT_REACHED("Feature NYI");
}

void
Assembler::as_vnmls(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c)
{
    JS_NOT_REACHED("Feature NYI");
}

void
Assembler::as_vneg(VFPRegister vd, VFPRegister vm, Condition c)
{
    as_vfp_float(vd, NoVFPRegister, vm, opv_neg, c);
}

void
Assembler::as_vsqrt(VFPRegister vd, VFPRegister vm, Condition c)
{
    as_vfp_float(vd, NoVFPRegister, vm, opv_sqrt, c);
}

void
Assembler::as_vabs(VFPRegister vd, VFPRegister vm, Condition c)
{
    as_vfp_float(vd, NoVFPRegister, vm, opv_abs, c);
}

void
Assembler::as_vsub(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c)
{
    as_vfp_float(vd, vn, vm, opv_sub, c);
}

void
Assembler::as_vcmp(VFPRegister vd, VFPRegister vm,
                 Condition c)
{
    as_vfp_float(vd, NoVFPRegister, vm, opv_sub, c);
}


void
Assembler::as_vmov(VFPRegister vd, VFPRegister vsrc, Condition c)
{
    as_vfp_float(vd, NoVFPRegister, vsrc, opv_mov, c);
}








void
Assembler::as_vxfer(Register vt1, Register vt2, VFPRegister vm, FloatToCore_ f2c,
                  Condition c)
{
    vfp_size sz = isSingle;
    if (vm.isDouble()) {
        
        
        
        
        
        
        JS_ASSERT(vt2 != InvalidReg);
        sz = isDouble;
    }
    VFPXferSize xfersz = WordTransfer;
    if (vt2 != InvalidReg) {
        
        xfersz = DoubleTransfer;
    }
    writeInst(xfersz | f2c | c | sz |
              RT(vt1) | ((vt2 != InvalidReg) ? RN(vt2) : 0) | VM(vm));
}



void
Assembler::as_vcvt(VFPRegister vd, VFPRegister vm,
                 Condition c)
{
    JS_NOT_REACHED("Feature NYI");
}

void
Assembler::as_vdtr(LoadStore ls, VFPRegister vd, VFPAddr addr,
                 Condition c )
{
    vfp_size sz = isDouble;
    if (!vd.isDouble()) {
        sz = isSingle;
    }

    writeInst(0x0D000A00 | addr.encode() | VD(vd) | sz | c);
}




void
Assembler::as_vdtm(LoadStore st, Register rn, VFPRegister vd, int length,
                 Condition c)
{
    JS_ASSERT(length <= 16 && length >= 0);
    vfp_size sz = isDouble;
    if (!vd.isDouble()) {
        sz = isSingle;
    } else {
        length *= 2;
    }
    writeInst(dtmLoadStore | RN(rn) | VD(vd) |
              length |
              dtmMode | dtmUpdate | dtmCond |
              0x0C000B00 | sz);
}

void
Assembler::as_vimm(VFPRegister vd, VFPImm imm, Condition c)
{
    vfp_size sz = isDouble;
    if (!vd.isDouble()) {
        
        sz = isSingle;
        JS_NOT_REACHED("non-double immediate");
    }
    writeInst(c | sz | imm.encode() | VD(vd) | 0x0EB00A00);

}

bool
Assembler::nextLink(BufferOffset b, BufferOffset *next)
{
    uint32 branch = *editSrc(b);
    JS_ASSERT(((branch & op_b_mask) == op_b) ||
              ((branch & op_b_mask) == op_bl));
    uint32 dest = (branch & op_b_dest_mask);
    
    if (dest == op_b_dest_mask)
        return false;
    
    dest = dest << 2;
    
    new (next) BufferOffset(dest);
    return true;
}

void
Assembler::bind(Label *label)
{
    
    if (label->used()) {
        bool more;
        BufferOffset dest = nextOffset();
        BufferOffset b(label);
        do {
            BufferOffset next;
            more = nextLink(b, &next);
            uint32 branch = *editSrc(b);
            Condition c = getCondition(branch);
            switch (branch & op_b_mask) {
              case op_b:
                as_b(dest.diffB(b), c, b);
                break;
              case op_bl:
                as_bl(dest.diffB(b), c, b);
                break;
              default:
                JS_NOT_REACHED("crazy fixup!");
            }
            b = next;
        } while (more);
    }
    label->bind(nextOffset().getOffset());
}

void
Assembler::Bind(IonCode *code, AbsoluteLabel *label, const void *address)
{
#if 0
    uint8 *raw = code->raw();
    if (label->used()) {
        intptr_t src = label->offset();
        do {
            intptr_t next = reinterpret_cast<intptr_t>(JSC::ARMAssembler::getPointer(raw + src));
            JSC::ARMAssembler::setPointer(raw + src, address);
            src = next;
        } while (src != AbsoluteLabel::INVALID_OFFSET);
    }
    JS_ASSERT(((uint8 *)address - raw) >= 0 && ((uint8 *)address - raw) < INT_MAX);
    label->bind();
#endif
    JS_NOT_REACHED("Feature NYI");
}

void
Assembler::call(Label *label)
{
#if 0
    if (label->bound()) {
        masm.linkJump(masm.call(), JmpDst(label->offset()));
    } else {
        JmpSrc j = masm.call();
        JmpSrc prev = JmpSrc(label->use(j.offset()));
        masm.setNextJump(j, prev);
    }
#endif
    JS_NOT_REACHED("Feature NYI");
}

void
Assembler::as_bkpt()
{
    writeInst(0xe1200070);
}
