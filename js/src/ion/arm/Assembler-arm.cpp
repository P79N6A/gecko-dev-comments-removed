








































#include "Assembler-arm.h"
#include "jsgcmark.h"

using namespace js;
using namespace js::ion;
int
js::ion::RT(Register r)
{
    return r.code() << 12;
}
int
js::ion::RN(Register r)
{
    return r.code() << 16;
}
int
js::ion::RD(Register r)
{
    return r.code() << 12;
}
void
Assembler::executableCopy(uint8 *buffer)
{
    executableCopy((void*)buffer);
    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        JS_NOT_REACHED("Feature NYI");
        
    }
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
Assembler::copyJumpRelocationTable(uint8 *dest)
{
    if (jumpRelocations_.length())
        memcpy(dest, jumpRelocations_.buffer(), jumpRelocations_.length());
}

void
Assembler::trace(JSTracer *trc)
{
    JS_NOT_REACHED("Feature NYI");
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
Assembler::inverseCondition(Condition cond)
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
    return op;
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
