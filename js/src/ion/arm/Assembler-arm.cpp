








































#include "Assembler-arm.h"
#include "jsgcmark.h"

using namespace js;
using namespace js::ion;



uint32
js::ion::RT(Register r)
{
    JS_ASSERT((r.code() & ~0xf) == 0);
    return r.code() << 12;
}

uint32
js::ion::RN(Register r)
{
    JS_ASSERT((r.code() & ~0xf) == 0);
    return r.code() << 16;
}

uint32
js::ion::RD(Register r)
{
    JS_ASSERT((r.code() & ~0xf) == 0);
    return r.code() << 12;
}

uint32
js::ion::RM(Register r)
{
    JS_ASSERT((r.code() & ~0xf) == 0);
    return r.code() << 8;
}




uint32
js::ion::maybeRT(Register r)
{
    if (r == InvalidReg)
        return 0;

    JS_ASSERT((r.code() & ~0xf) == 0);
    return r.code() << 12;
}

uint32
js::ion::maybeRN(Register r)
{

    if (r == InvalidReg)
        return 0;

    JS_ASSERT((r.code() & ~0xf) == 0);
    return r.code() << 16;
}

uint32
js::ion::maybeRD(Register r)
{
    if (r == InvalidReg)
        return 0;

    JS_ASSERT((r.code() & ~0xf) == 0);
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
        
        return VFPRegIndexSplit(_code >> 1, _code & 1);
    }
}

VFPRegister js::ion::NoVFPRegister(true);

void
Assembler::executableCopy(uint8 *buffer)
{
    ASSERT(m_buffer.sizeOfConstantPool() == 0);
    memcpy(buffer, m_buffer.data(), m_buffer.size());

    for (size_t i = 0; i < jumps_.length(); i++) {
        
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
        MarkIonCodeUnbarriered(trc, child, "rel32");
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
            MarkIonCodeUnbarriered(trc, IonCode::FromExecutable((uint8 *)rp.target), "masmrel32");
    }
#endif
}

void
Assembler::executableCopy(void *buffer)
{
    JS_NOT_REACHED("dead code");
}

void
Assembler::processDeferredData(IonCode *code, uint8 *data)
{

    JS_ASSERT(dataSize() == 0);
    for (size_t i = 0; i < data_.length(); i++) {
        DeferredData *deferred = data_[i];
        
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
    return Condition(0x80000000^cond);
#if 0
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
#endif
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
      default:
        return op;
    }
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
        return VFPRegister(_code << 1, Single);
    } else {
        return VFPRegister(_code, Single);
    }
}

VFPRegister
VFPRegister::sintOverlay()
{
    JS_ASSERT(!_isInvalid);
    if (kind == Double) {
        
        ASSERT(_code < 16);
        return VFPRegister(_code << 1, Int);
    } else {
        return VFPRegister(_code, Int);
    }
}
VFPRegister
VFPRegister::uintOverlay()
{
    JS_ASSERT(!_isInvalid);
    if (kind == Double) {
        
        ASSERT(_code < 16);
        return VFPRegister(_code << 1, UInt);
    } else {
        return VFPRegister(_code, UInt);
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
Assembler::writeInst(uint32 x, uint32 *dest)
{
    if (dest == NULL) {
        m_buffer.putInt(x);
    } else {
        writeInstStatic(x, dest);
    }
}
void
Assembler::writeInstStatic(uint32 x, uint32 *dest)
{
    JS_ASSERT(dest != NULL);
    *dest = x;
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
Assembler::as_genmul(Register dhi, Register dlo, Register rm, Register rn,
          MULOp op, SetCond_ sc, Condition c)
{

    writeInst(RN(dhi) | maybeRD(dlo) | RM(rm) | rn.code() | op | sc | c);
}
void
Assembler::as_mul(Register dest, Register src1, Register src2,
       SetCond_ sc, Condition c)
{
    as_genmul(dest, InvalidReg, src1, src2, opm_mul, sc, c);
}
void
Assembler::as_mla(Register dest, Register acc, Register src1, Register src2,
       SetCond_ sc, Condition c)
{
    as_genmul(dest, acc, src1, src2, opm_mla, sc, c);
}
void
Assembler::as_umaal(Register destHI, Register destLO, Register src1, Register src2, Condition c)
{
    as_genmul(destHI, destLO, src1, src2, opm_umaal, NoSetCond, c);
}
void
Assembler::as_mls(Register dest, Register acc, Register src1, Register src2, Condition c)
{
    as_genmul(dest, acc, src1, src2, opm_mls, NoSetCond, c);
}

void
Assembler::as_umull(Register destHI, Register destLO, Register src1, Register src2,
                SetCond_ sc, Condition c)
{
    as_genmul(destHI, destLO, src1, src2, opm_umull, sc, c);
}

void
Assembler::as_umlal(Register destHI, Register destLO, Register src1, Register src2,
                SetCond_ sc, Condition c)
{
    as_genmul(destHI, destLO, src1, src2, opm_umlal, sc, c);
}

void
Assembler::as_smull(Register destHI, Register destLO, Register src1, Register src2,
                SetCond_ sc, Condition c)
{
    as_genmul(destHI, destLO, src1, src2, opm_smull, sc, c);
}

void
Assembler::as_smlal(Register destHI, Register destLO, Register src1, Register src2,
                SetCond_ sc, Condition c)
{
    as_genmul(destHI, destLO, src1, src2, opm_smlal, sc, c);
}




void
Assembler::as_dtr(LoadStore ls, int size, Index mode,
                  Register rt, DTRAddr addr, Condition c, uint32 *dest)
{
    JS_ASSERT(size == 32 || size == 8);
    writeInst( 0x04000000 | ls | (size == 8 ? 0x00400000 : 0) | mode | c |
               RT(rt) | addr.encode(), dest);

    return;
}
struct PoolHintData {
    int32  index : 21;
    enum LoadType {
        
        
        poolBOGUS = 0,
        poolDTR   = 1,
        poolEDTR  = 2,
        poolVDTR  = 3
    };
    LoadType loadType : 2;
    uint32 destReg : 5;
    uint32 ONES : 4;
};

union PoolHintPun {
    PoolHintData phd;
    uint32 raw;
};



void
Assembler::as_extdtr(LoadStore ls, int size, bool IsSigned, Index mode,
                     Register rt, EDtrAddr addr, Condition c, uint32 *dest)
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
              addr.encode() | RT(rt) | c, dest);
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
Assembler::as_Imm32Pool(Register dest, uint32 value)
{
    PoolHintPun php;
    php.phd.loadType = PoolHintData::poolDTR;
    php.phd.destReg = dest.code();
    php.phd.index = 0;
    php.phd.ONES = 0xf;
    m_buffer.putIntWithConstantInt(php.raw, value);
}

void
Assembler::as_FImm64Pool(VFPRegister dest, double value)
{
    JS_ASSERT(dest.isDouble());
    PoolHintPun php;
    php.phd.loadType = PoolHintData::poolVDTR;
    php.phd.destReg = dest.code();
    php.phd.index = 0;
    php.phd.ONES = 0xf;
    m_buffer.putIntWithConstantDouble(php.raw, value);
}

uint32
Assembler::patchConstantPoolLoad(uint32 load, int32 index)
{
    PoolHintPun php;
    php.raw = load;
    JS_ASSERT(php.phd.ONES == 0xf && php.phd.loadType != PoolHintData::poolBOGUS);
    php.phd.index = index;
    JS_ASSERT(index == php.phd.index);
    return php.raw;
}
void
Assembler::patchConstantPoolLoad(void* loadAddr, void* constPoolAddr)
{
    PoolHintData data = *(PoolHintData*)loadAddr;
    uint32 *instAddr = (uint32*) loadAddr;
    int offset = (char *)constPoolAddr - (char *)loadAddr;
    switch(data.loadType) {
      case PoolHintData::poolBOGUS:
        JS_NOT_REACHED("bogus load type!");
      case PoolHintData::poolDTR:
        dummy->as_dtr(IsLoad, 32, Offset, Register::FromCode(data.destReg),
                      DTRAddr(pc, DtrOffImm(offset+4*data.index - 8)), Always, instAddr);
        break;
      case PoolHintData::poolEDTR:
        JS_NOT_REACHED("edtr is too small/NYI");
        break;
      case PoolHintData::poolVDTR:
        dummy->as_vdtr(IsLoad, VFPRegister(FloatRegister::FromCode(data.destReg)),
                       VFPAddr(pc, VFPOffImm(offset+4*data.index - 8)), Always, instAddr);
        break;
    }
}

uint32
Assembler::placeConstantPoolBarrier(int offset)
{
    
    
    
    JS_NOT_REACHED("ARMAssembler holdover");
#if 0
    offset = (offset - sizeof(ARMWord)) >> 2;
    ASSERT((offset <= BOFFSET_MAX && offset >= BOFFSET_MIN));
    return AL | B | (offset & BRANCH_MASK);
#endif
    return -1;
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


enum vfp_tags {
    vfp_tag   = 0x0C000A00,
    vfp_arith = 0x02000000
};
void
Assembler::writeVFPInst(vfp_size sz, uint32 blob, uint32 *dest)
{
    JS_ASSERT((sz & blob) == 0);
    JS_ASSERT((vfp_tag & blob) == 0);
    writeInst(vfp_tag | sz | blob, dest);
}



void
Assembler::as_vfp_float(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  VFPOp op, Condition c)
{
    
    JS_ASSERT(vd.equiv(vn) && vd.equiv(vm));
    vfp_size sz = vd.isDouble() ? isDouble : isSingle;
    writeVFPInst(sz, VD(vd) | VN(vn) | VM(vm) | op | vfp_arith | c);
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
    as_vfp_float(vd, NoVFPRegister, vm, opv_cmp, c);
}
void
Assembler::as_vcmpz(VFPRegister vd, Condition c)
{
    as_vfp_float(vd, NoVFPRegister, NoVFPRegister, opv_cmpz, c);
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
    uint32 (*encodeVFP)(VFPRegister) = VN;
    if (vt2 != InvalidReg) {
        
        xfersz = DoubleTransfer;
        encodeVFP = VM;
    }

    writeVFPInst(sz, xfersz | f2c | c |
                 RT(vt1) | maybeRN(vt2) | encodeVFP(vm));
}
enum vcvt_destFloatness {
    toInteger = 1 << 18,
    toFloat  = 0 << 18
};
enum vcvt_toZero {
    toZero = 1 << 7,
    toFPSCR = 0 << 7
};
enum vcvt_Signedness {
    toSigned   = 1 << 16,
    toUnsigned = 0 << 16,
    fromSigned   = 1 << 7,
    fromUnsigned = 0 << 7
};



void
Assembler::as_vcvt(VFPRegister vd, VFPRegister vm,
                 Condition c)
{
    
    JS_ASSERT(!vd.equiv(vm));
    vfp_size sz = isDouble;
    if (vd.isFloat() && vm.isFloat()) {
        
        if (vm.isSingle()) {
            sz = isSingle;
        }
        writeVFPInst(sz, c | 0x02B700C0 |
                  VM(vm) | VD(vd));
    } else {
        
        vcvt_destFloatness destFloat;
        vcvt_Signedness opSign;
        vcvt_toZero doToZero = toFPSCR;
        JS_ASSERT(vd.isFloat() || vm.isFloat());
        if (vd.isSingle() || vm.isSingle()) {
            sz = isSingle;
        }
        if (vd.isFloat()) {
            destFloat = toFloat;
            if (vm.isSInt()) {
                opSign = fromSigned;
            } else {
                opSign = fromUnsigned;
            }
        } else {
            destFloat = toInteger;
            if (vd.isSInt()) {
                opSign = toSigned;
            } else {
                opSign = toUnsigned;
            }
            doToZero = toZero;
        }
        writeVFPInst(sz, c | 0x02B80040 | VD(vd) | VM(vm) | destFloat | opSign | doToZero);
    }

}

void
Assembler::as_vdtr(LoadStore ls, VFPRegister vd, VFPAddr addr,
                   Condition c ,
                   uint32 *dest)
{
    vfp_size sz = vd.isDouble() ? isDouble : isSingle;
    writeVFPInst(sz, ls | 0x01000000 | addr.encode() | VD(vd) | c, dest);
}




void
Assembler::as_vdtm(LoadStore st, Register rn, VFPRegister vd, int length,
                 Condition c)
{
    JS_ASSERT(length <= 16 && length >= 0);
    vfp_size sz = vd.isDouble() ? isDouble : isSingle;

    if (vd.isDouble())
        length *= 2;

    writeVFPInst(sz, dtmLoadStore | RN(rn) | VD(vd) |
              length |
              dtmMode | dtmUpdate | dtmCond);
}

void
Assembler::as_vimm(VFPRegister vd, VFPImm imm, Condition c)
{
    vfp_size sz = vd.isDouble() ? isDouble : isSingle;

    if (!vd.isDouble()) {
        
        JS_NOT_REACHED("non-double immediate");
    }
    writeVFPInst(sz,  c | imm.encode() | VD(vd) | 0x02B00000);

}
void
Assembler::as_vmrs(Register r, Condition c)
{
    writeInst(c | 0x0ef10a10 | RT(r));
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
Assembler::bind(Label *label, BufferOffset boff)
{
    if (label->used()) {
        bool more;
        
        
        BufferOffset dest = boff.assigned() ? boff : nextOffset();
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
Assembler::retarget(Label *label, Label *target)
{
    if (label->used()) {
        if (target->bound()) {
            bind(label, BufferOffset(target));
        } else {
            
            
            DebugOnly<uint32> prev = target->use(label->offset());
            JS_ASSERT((int32)prev == Label::INVALID_OFFSET);
        }
    }
    label->reset();

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

#if 0


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
Assembler::call(void *addr)
{
    JS_NOT_REACHED("Feature NYI");
}
#endif
void
Assembler::as_bkpt()
{
    writeInst(0xe1200070);
}

void
Assembler::dumpPool()
{
    JS_ASSERT(lastWasUBranch);
    m_buffer.flushWithoutBarrier(true);
}

void
Assembler::as_jumpPool(uint32 numCases)
{
    for (uint32 i = 0; i < numCases; i++)
        writeInst(-1);
}

Assembler *Assembler::dummy = NULL;

