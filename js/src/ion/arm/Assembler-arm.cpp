








































#include "Assembler-arm.h"
#include "MacroAssembler-arm.h"
#include "jsgcmark.h"

#include "assembler/jit/ExecutableAllocator.h"

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

Register
js::ion::toRD(Instruction &i)
{
    return Register::FromCode((i.encode()>>12) & 0xf);
}
Register
js::ion::toR(Instruction &i)
{
    return Register::FromCode(i.encode() & 0xf);
}

Register
js::ion::toRM(Instruction &i)
{
    return Register::FromCode((i.encode()>>8) & 0xf);
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

bool
InstDTR::isTHIS(const Instruction &i)
{
    return (i.encode() & IsDTRMask) == (uint32)IsDTR;
}

InstDTR *
InstDTR::asTHIS(Instruction &i)
{
    if (isTHIS(i))
        return (InstDTR*)&i;
    return NULL;
}

bool
InstLDR::isTHIS(const Instruction &i)
{
    return (i.encode() & IsDTRMask) == (uint32)IsDTR;
}

InstLDR *
InstLDR::asTHIS(Instruction &i)
{
    if (isTHIS(i))
        return (InstLDR*)&i;
    return NULL;
}

bool
InstBranchReg::isTHIS(const Instruction &i)
{
    return InstBXReg::isTHIS(i) ||
        InstBLXReg::isTHIS(i);
}

InstBranchReg *
InstBranchReg::asTHIS(const Instruction &i)
{
    if (isTHIS(i))
        return (InstBranchReg*)&i;
    return NULL;
}
void
InstBranchReg::extractDest(Register *dest)
{
    *dest = toR(*this);
}
bool
InstBranchReg::checkDest(Register dest)
{
    return dest == toR(*this);
}

bool
InstBranchImm::isTHIS(const Instruction &i)
{
    return InstBImm::isTHIS(i) ||
        InstBLImm::isTHIS(i);
}

InstBranchImm *
InstBranchImm::asTHIS(const Instruction &i)
{
    if (isTHIS(i))
        return (InstBranchImm*)&i;
    return NULL;
}

void
InstBranchImm::extractImm(BOffImm *dest)
{
    *dest = BOffImm(*this);
}

bool
InstBXReg::isTHIS(const Instruction &i)
{
    return (i.encode() & IsBRegMask) == IsBX;
}

InstBXReg *
InstBXReg::asTHIS(const Instruction &i)
{
    if (isTHIS(i))
        return (InstBXReg*)&i;
    return NULL;
}

bool
InstBLXReg::isTHIS(const Instruction &i)
{
    return (i.encode() & IsBRegMask) == IsBLX;

}
InstBLXReg *
InstBLXReg::asTHIS(const Instruction &i)
{
    if (isTHIS(i))
        return (InstBLXReg*)&i;
    return NULL;
}

bool
InstBImm::isTHIS(const Instruction &i)
{
    return (i.encode () & IsBImmMask) == IsB;
}
InstBImm *
InstBImm::asTHIS(const Instruction &i)
{
    if (isTHIS(i))
        return (InstBImm*)&i;
    return NULL;
}

bool
InstBLImm::isTHIS(const Instruction &i)
{
    return (i.encode () & IsBImmMask) == IsBL;

}
InstBLImm *
InstBLImm::asTHIS(Instruction &i)
{
    if (isTHIS(i))
        return (InstBLImm*)&i;
    return NULL;
}

bool
InstMovWT::isTHIS(Instruction &i)
{
    return  InstMovW::isTHIS(i) ||
        InstMovT::isTHIS(i);
}
InstMovWT *
InstMovWT::asTHIS(Instruction &i)
{
    if (isTHIS(i))
        return (InstMovWT*)&i;
    return NULL;
}

void
InstMovWT::extractImm(Imm16 *imm)
{
    *imm = Imm16(*this);
}
bool
InstMovWT::checkImm(Imm16 imm)
{
    return (imm.decode() == Imm16(*this).decode());
}

void
InstMovWT::extractDest(Register *dest)
{
    *dest = toRD(*this);
}
bool
InstMovWT::checkDest(Register dest)
{
    return (dest == toRD(*this));
}

bool
InstMovW::isTHIS(const Instruction &i)
{
    return (i.encode() & IsWTMask) == IsW;
}

InstMovW *
InstMovW::asTHIS(const Instruction &i)
{
    if (isTHIS(i))
        return (InstMovW*) (&i);
    return NULL;
}
InstMovT *
InstMovT::asTHIS(const Instruction &i)
{
    if (isTHIS(i))
        return (InstMovT*) (&i);
    return NULL;
}

bool
InstMovT::isTHIS(const Instruction &i)
{
    return (i.encode() & IsWTMask) == IsT;
}

Imm16::Imm16(Instruction &inst)
  : lower(inst.encode() & 0xfff), upper(inst.encode() >> 16), invalid(0xfff) {}
Imm16::Imm16(uint32 imm)
   : lower(imm & 0xfff), pad(0), upper((imm>>12) & 0xf), invalid(0)
{
    JS_ASSERT(decode() == imm);
}
Imm16::Imm16() : invalid(0xfff) {}

void
ion::PatchJump(CodeLocationJump jump_, CodeLocationLabel label)
{
    
    
    Instruction *jump = (Instruction*)jump_.raw();
    Assembler::Condition c;
    jump->extractCond(&c);
    JS_ASSERT(jump->is<InstBranchImm>());
    Assembler::retargetBranch(jump, label.raw() - jump_.raw());
}

void
Assembler::finish()
{
    JS_ASSERT(!isFinished);
    isFinished = true;
    for (size_t i = 0; i < jumps_.length(); i++) {
        jumps_[i].fixOffset(m_buffer);
    }
    
    for (int i = 0; i < tmpDataRelocations_.length(); i++) {
        int offset = tmpDataRelocations_[i].getOffset();
        dataRelocations_.writeUnsigned(offset + m_buffer.poolSizeBefore(offset));
    }
}

void
Assembler::executableCopy(uint8 *buffer)
{
    JS_ASSERT(isFinished);
    m_buffer.executableCopy(buffer);

    JSC::ExecutableAllocator::cacheFlush(buffer, m_buffer.size());
}

void
Assembler::resetCounter()
{
    m_buffer.resetCounter();
}

uint32
Assembler::actualOffset(uint32 off_) const
{
    return off_ + m_buffer.poolSizeBefore(off_);
}

BufferOffset
Assembler::actualOffset(BufferOffset off_) const
{
    return BufferOffset(off_.getOffset() + m_buffer.poolSizeBefore(off_.getOffset()));
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
const uint32 *
Assembler::getCF32Target(Instruction *jump)
{
    if (jump->is<InstBranchImm>()) {
        
        BOffImm imm;
        InstBranchImm *jumpB = jump->as<InstBranchImm>();
        jumpB->extractImm(&imm);
        return imm.getDest(jump)->raw();
    } else if (jump->is<InstMovW>() &&
               jump->next()->is<InstMovT>() &&
               jump->next()->next()->is<InstBranchReg>()) {
        
        
        
        

        Imm16 targ_bot;
        Imm16 targ_top;
        Register temp;

        InstMovW *bottom = jump->as<InstMovW>();
        InstMovT *top = jump->next()->as<InstMovT>();
        InstBranchReg * branch = jump->next()->next()->as<InstBranchReg>();
        
        bottom->extractImm(&targ_bot);
        bottom->extractDest(&temp);
        
        top->extractImm(&targ_top);
        
        JS_ASSERT(top->checkDest(temp));
        
        JS_ASSERT(branch->checkDest(temp));
        uint32 *dest = (uint32*) (targ_bot.decode() | (targ_top.decode() << 16));
        return dest;
    } else if (jump->is<InstLDR>()) {
        JS_NOT_REACHED("ldr-based relocs NYI");
    }
    JS_NOT_REACHED("unsupported branch relocation");
    return NULL;
}

uintptr_t
Assembler::getPointer(uint8 *instPtr)
{
    Instruction *ptr = reinterpret_cast<Instruction*>(instPtr);
    uintptr_t ret = (uintptr_t)getPtr32Target(ptr, NULL, NULL);
    return ret;
}

const uint32 *
Assembler::getPtr32Target(Instruction *load, Register *dest, RelocStyle *style)
{
    if (load->is<InstMovW>() &&
        load->next()->is<InstMovT>()) {
        
        
        

        Imm16 targ_bot;
        Imm16 targ_top;
        Register temp;

        InstMovW *bottom = load->as<InstMovW>();
        InstMovT *top = load->next()->as<InstMovT>();
        
        bottom->extractImm(&targ_bot);
        bottom->extractDest(&temp);
        
        top->extractImm(&targ_top);
        
        JS_ASSERT(top->checkDest(temp));
        uint32 *value = (uint32*) (targ_bot.decode() | (targ_top.decode() << 16));
        if (dest)
            *dest = temp;
        if (style)
            *style = L_MOVWT;
        return value;
    }
    JS_NOT_REACHED("unsupported relocation");
    return NULL;
}

static inline IonCode *
CodeFromJump(Instruction *jump)
{
    uint8 *target = (uint8 *)Assembler::getCF32Target(jump);
    return IonCode::FromExecutable(target);
}

void
Assembler::TraceJumpRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader)
{
    RelocationIterator iter(reader);
    while (iter.read()) {
        IonCode *child = CodeFromJump((Instruction *) (code->raw() + iter.offset()));
        MarkIonCodeUnbarriered(trc, child, "rel32");
    };
}

static void
TraceDataRelocations(JSTracer *trc, uint8 *buffer, CompactBufferReader &reader)
{
    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        const void *ptr = js::ion::Assembler::getPtr32Target((Instruction*)(buffer + offset));
        gc::MarkThingOrValueRoot(trc, reinterpret_cast<uintptr_t *>(&ptr), "immgcptr");
    }

}
static void
TraceDataRelocations(JSTracer *trc, ARMBuffer *buffer, CompactBufferReader &reader)
{
    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        const void *ptr = ion::Assembler::getPtr32Target((Instruction*)(buffer->getInst(BufferOffset(offset))));
        gc::MarkThingOrValueRoot(trc, reinterpret_cast<uintptr_t *>(&ptr), "immgcptr");
    }

}
void
Assembler::TraceDataRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader)
{
    ::TraceDataRelocations(trc, code->raw(), reader);
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
    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        if (rp.kind == Relocation::IONCODE)
            MarkIonCodeUnbarriered(trc, IonCode::FromExecutable((uint8*)rp.target), "masmrel32");
    }
    if (tmpDataRelocations_.length()) {
        CompactBufferReader reader(dataRelocations_);
        ::TraceDataRelocations(trc, &m_buffer, reader);
    }
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
    const uint32 ConditionInversionBit = 0x10000000;
    return Condition(ConditionInversionBit ^ cond);
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
        JS_ASSERT((imm1shift & 0x1) == 0);
        JS_ASSERT((imm2shift & 0x1) == 0);
        return TwoImm8mData(datastore::Imm8mData(imm1, imm1shift >> 1),
                            datastore::Imm8mData(imm2, imm2shift >> 1));
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
        JS_ASSERT((imm1shift & 0x1) == 0);
        JS_ASSERT((imm2shift & 0x1) == 0);
        return TwoImm8mData(datastore::Imm8mData(imm1, imm1shift >> 1),
                            datastore::Imm8mData(imm2, imm2shift >> 1));
    }
}

ALUOp
ion::ALUNeg(ALUOp op, Register dest, Imm32 *imm, Register *negDest)
{
    
    *negDest = dest;
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
      case op_tst:
        JS_ASSERT(dest == InvalidReg);
        *imm = Imm32(~imm->value);
        *negDest = ScratchRegister;
        return op_bic;
        
      default:
        break;
    }
    return op_invalid;
}

bool
ion::can_dbl(ALUOp op)
{
    
    
    
    
    
    
    
    switch (op) {
      case op_bic:
      case op_add:
      case op_sub:
      case op_eor:
      case op_orr:
        return true;
      default:
        break;
    }
    return false;
}

bool
ion::condsAreSafe(ALUOp op) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    switch(op) {
      case op_bic:
      case op_orr:
      case op_eor:
        return true;
      default:
        break;
    }
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
BOffImm::BOffImm(Instruction &inst) : data(inst.encode() & 0x00ffffff) {}

Instruction *
BOffImm::getDest(Instruction *src)
{
    
    
    
    return &src[(((int32)data<<8)>>8) + 2];
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
    return m_buffer.size();
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
Assembler::as_nop()
{
    writeInst(0xe320f000);
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
Assembler::as_movw(Register dest, Imm16 imm, Condition c, Instruction *pos)
{
    JS_ASSERT(hasMOVWT());
    writeInst(0x03000000 | c | imm.encode() | RD(dest), (uint32*)pos);
}
void
Assembler::as_movt(Register dest, Imm16 imm, Condition c, Instruction *pos)
{
    JS_ASSERT(hasMOVWT());
    writeInst(0x03400000 | c | imm.encode() | RD(dest), (uint32*)pos);
}

const int mull_tag = 0x90;

void
Assembler::as_genmul(Register dhi, Register dlo, Register rm, Register rn,
          MULOp op, SetCond_ sc, Condition c)
{

    writeInst(RN(dhi) | maybeRD(dlo) | RM(rm) | rn.code() | op | sc | c | mull_tag);
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
class PoolHintData {
  public:
    enum LoadType {
        
        
        poolBOGUS = 0,
        poolDTR   = 1,
        poolEDTR  = 2,
        poolVDTR  = 3
    };
  private:
    uint32   index    : 17;
    uint32   cond     : 4;
    LoadType loadType : 2;
    uint32   destReg  : 5;
    uint32   ONES     : 4;
  public:
    void init(uint32 index_, Assembler::Condition cond_, LoadType lt, const Register &destReg_) {
        index = index_;
        JS_ASSERT(index == index_);
        cond = cond_ >> 28;
        JS_ASSERT(cond == cond_ >> 28);
        loadType = lt;
        ONES = 0xffffffff;
        destReg = destReg_.code();
    }
    void init(uint32 index_, Assembler::Condition cond_, LoadType lt, const VFPRegister &destReg_) {
        index = index_;
        JS_ASSERT(index == index_);
        cond = cond_ >> 28;
        JS_ASSERT(cond == cond_ >> 28);
        loadType = lt;
        ONES = 0xffffffff;
        destReg = destReg_.code();
    }
    Assembler::Condition getCond() {
        return Assembler::Condition(cond << 28);
    }

    Register getReg() {
        return Register::FromCode(destReg);
    }
    VFPRegister getVFPReg() {
        return VFPRegister(FloatRegister::FromCode(destReg));
    }

    int32 getIndex() {
        return index;
    }
    void setIndex(uint32 index_) {
        JS_ASSERT(ONES == 0xf && loadType != poolBOGUS);
        index = index_;
        JS_ASSERT(index == index_);
    }

    LoadType getLoadType() {
        return loadType;
    }
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
              addr.encode() | RT(rt) | mode | c, dest);
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
Assembler::as_Imm32Pool(Register dest, uint32 value, Condition c)
{
    PoolHintPun php;
    php.phd.init(0, c, PoolHintData::poolDTR, dest);
    m_buffer.insertEntry(4, (uint8*)&php.raw, int32Pool, (uint8*)&value);
}

void
Assembler::as_FImm64Pool(VFPRegister dest, double value, Condition c)
{
    JS_ASSERT(dest.isDouble());
    PoolHintPun php;
    php.phd.init(0, c, PoolHintData::poolVDTR, dest);
    m_buffer.insertEntry(4, (uint8*)&php.raw, doublePool, (uint8*)&value);
}

uint32
Assembler::patchConstantPoolLoad(uint32 load, int32 index)
{
    PoolHintPun php;
    php.raw = load;
    php.phd.setIndex(index);
    return php.raw;
}


void
Assembler::insertTokenIntoTag(uint32 instSize, uint8 *load_, int32 token)
{
    uint32 *load = (uint32*) load_;
    PoolHintPun php;
    php.raw = *load;
    php.phd.setIndex(token);
    *load = php.raw;
}


void
Assembler::patchConstantPoolLoad(void* loadAddr, void* constPoolAddr)
{
    PoolHintData data = *(PoolHintData*)loadAddr;
    uint32 *instAddr = (uint32*) loadAddr;
    int offset = (char *)constPoolAddr - (char *)loadAddr;
    switch(data.getLoadType()) {
      case PoolHintData::poolBOGUS:
        JS_NOT_REACHED("bogus load type!");
      case PoolHintData::poolDTR:
        dummy->as_dtr(IsLoad, 32, Offset, data.getReg(),
                      DTRAddr(pc, DtrOffImm(offset+4*data.getIndex() - 8)), data.getCond(), instAddr);
        break;
      case PoolHintData::poolEDTR:
        JS_NOT_REACHED("edtr is too small/NYI");
        break;
      case PoolHintData::poolVDTR:
        dummy->as_vdtr(IsLoad, data.getVFPReg(),
                       VFPAddr(pc, VFPOffImm(offset+8*data.getIndex() - 8)), data.getCond(), instAddr);
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
    if (c == Always)
        m_buffer.markGuard();
}
void
Assembler::writePoolGuard(BufferOffset branch, Instruction *dest, BufferOffset afterPool)
{
    BOffImm off = branch.diffB<BOffImm>(afterPool);
    *dest = InstBImm(off, Always);
}



void
Assembler::as_b(BOffImm off, Condition c)
{
    m_buffer.markNextAsBranch();
    writeInst(((int)c) | op_b | off.encode());
    if (c == Always)
        m_buffer.markGuard();
}

void
Assembler::as_b(Label *l, Condition c)
{
    BufferOffset next = nextOffset();
    m_buffer.markNextAsBranch();
    if (l->bound()) {
        as_b(BufferOffset(l).diffB<BOffImm>(next), c);
    } else {
        
        int32 old = l->use(next.getOffset());
        if (old != LabelBase::INVALID_OFFSET) {
            
            
            as_b(BOffImm(old), c);
        } else {
            BOffImm inv;
            as_b(inv, c);
        }
    }
}
void
Assembler::as_b(BOffImm off, Condition c, BufferOffset inst)
{
    *editSrc(inst) = InstBImm(off, c);
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
        as_bl(BufferOffset(l).diffB<BOffImm>(next), c);
    } else {
        int32 old = l->use(next.getOffset());
        
        if (old != LabelBase::INVALID_OFFSET) {
            
            
            as_bl(BOffImm(old), c);
        } else {
            BOffImm inv;
            as_bl(inv, c);
        }
    }
}
void
Assembler::as_bl(BOffImm off, Condition c, BufferOffset inst)
{
    *editSrc(inst) = InstBLImm(off, c);
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
    as_vfp_float(vd, vn, vm, opv_div, c);
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
    Instruction branch = *editSrc(b);
    JS_ASSERT(branch.is<InstBranchImm>());
    BOffImm destOff;
    branch.as<InstBranchImm>()->extractImm(&destOff);
    if (destOff.isInvalid())
        return false;

    
    
    
    new (next) BufferOffset(destOff.decode());
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
            Instruction branch = *editSrc(b);
            Condition c;
            branch.extractCond(&c);
            if (branch.is<InstBImm>())
                as_b(dest.diffB<BOffImm>(b), c, b);
            else if (branch.is<InstBLImm>())
                as_bl(dest.diffB<BOffImm>(b), c, b);
            else
                JS_NOT_REACHED("crazy fixup!");
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

void dbg_break() {}
static int stopBKPT = -1;
void
Assembler::as_bkpt()
{
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    static int hit = 0;
    if (stopBKPT == hit)
        dbg_break();
    writeInst(0xe1200070 | (hit & 0xf) | ((hit & 0xfff0)<<4));
    hit++;
}

void
Assembler::dumpPool()
{
    m_buffer.flushPool();
}

void
Assembler::flushBuffer()
{
    m_buffer.flushPool();
}

void
Assembler::as_jumpPool(uint32 numCases)
{
    for (uint32 i = 0; i < numCases; i++)
        writeInst(-1);
}

ptrdiff_t
Assembler::getBranchOffset(const Instruction *i_)
{
    InstBranchImm *i = i_->as<InstBranchImm>();
    BOffImm dest;
    i->extractImm(&dest);
    return dest.decode();
}

void
Assembler::retargetBranch(Instruction *i, int offset)
{
    Condition cond;
    i->extractCond(&cond);
    if (i->is<InstBImm>())
        new (i) InstBImm(BOffImm(offset), cond);
    else
        new (i) InstBLImm(BOffImm(offset), cond);
    JSC::ExecutableAllocator::cacheFlush(i, 4);
}
struct PoolHeader : Instruction {
    struct Header {
        
        
        uint32 size:15;
        bool isNatural:1;
        uint32 ONES:16;
        Header(int size_, bool isNatural_) : size(size_), isNatural(isNatural_), ONES(0xffff) {}
        Header(const Instruction *i) {
            JS_STATIC_ASSERT(sizeof(Header) == sizeof(uint32));
            memcpy(this, i, sizeof(Header));
            JS_ASSERT(ONES == 0xffff);
        }
        uint32 raw() const {
            JS_STATIC_ASSERT(sizeof(Header) == sizeof(uint32));
            uint32 dest;
            memcpy(&dest, this, sizeof(Header));
            return dest;
        }
    };
    PoolHeader(int size_, bool isNatural_) : Instruction (Header(size_, isNatural_).raw(), true) {}
    uint32 size() const {
        Header tmp(this);
        return tmp.size;
    }
    uint32 isNatural() const {
        Header tmp(this);
        return tmp.isNatural;
    }
    static bool isTHIS(const Instruction &i) {
        return (*i.raw() & 0xffff0000) == 0xffff0000;
    }
    static const PoolHeader *asTHIS(const Instruction &i) {
        if (!isTHIS(i))
            return NULL;
        return static_cast<const PoolHeader*>(&i);
    }
};


void
Assembler::writePoolHeader(uint8 *start, Pool *p, bool isNatural)
{
    STATIC_ASSERT(sizeof(PoolHeader) == 4);
    uint8 *pool = start+4;
    
    pool = p[0].addPoolSize(pool);
    pool = p[1].addPoolSize(pool);
    pool = p[1].other->addPoolSize(pool);
    pool = p[0].other->addPoolSize(pool);
    uint32 size = pool - start;
    JS_ASSERT((size & 3) == 0);
    size = size >> 2;
    JS_ASSERT(size < (1 << 15));
    PoolHeader header(size, isNatural);
    *(PoolHeader*)start = header;
}


void
Assembler::writePoolFooter(uint8 *start, Pool *p, bool isNatural)
{
    return;
}



uint32
Assembler::patchWrite_NearCallSize()
{
    return sizeof(uint32);
}
void
Assembler::patchWrite_NearCall(CodeLocationLabel start, CodeLocationLabel toCall)
{
    Instruction *inst = (Instruction *) start.raw();
    
    
    
    uint8 *dest = toCall.raw();
    new (inst) InstBLImm(BOffImm(dest - (uint8*)inst) , Always);
    
    JSC::ExecutableAllocator::cacheFlush(inst, sizeof(uint32));
}
void
Assembler::patchDataWithValueCheck(CodeLocationLabel label, ImmWord newValue, ImmWord expectedValue)
{
    Instruction *ptr = (Instruction *) label.raw();
    Register dest;
    Assembler::RelocStyle rs;
    const uint32 *val = getPtr32Target(ptr, &dest, &rs);
    JS_ASSERT((uint32)val == expectedValue.value);
    reinterpret_cast<MacroAssemblerARM*>(dummy)->ma_movPatchable(Imm32(newValue.value), dest, Always, rs, ptr);
    JSC::ExecutableAllocator::cacheFlush(ptr, sizeof(uintptr_t)*2);
}






void
Assembler::patchWrite_Imm32(CodeLocationLabel label, Imm32 imm) {
    
    uint32 *raw = (uint32*)label.raw();
    
    
    *(raw-1) = imm.value;
}


uint8 *
Assembler::nextInstruction(uint8 *inst_, uint32 *count)
{
    Instruction *inst = reinterpret_cast<Instruction*>(inst_);
    if (count != NULL)
        *count += sizeof(Instruction);
    return reinterpret_cast<uint8*>(inst->next());
}

bool instIsGuard(Instruction *inst, const PoolHeader **ph)
{
    Assembler::Condition c;
    inst->extractCond(&c);
    if (c != Assembler::Always)
        return false;
    if (!(inst->is<InstBXReg>() || inst->is<InstBImm>()))
        return false;
    *ph = inst->as<const PoolHeader>();
    return *ph != NULL;
}

bool instIsArtificialGuard(Instruction *inst, const PoolHeader **ph)
{
    if (!instIsGuard(inst, ph))
        return false;
    return !(*ph)->isNatural();
}






Instruction *
Instruction::next()
{
    Assembler::Condition c;
    Instruction *ret = this+1;
    const PoolHeader *ph;
    
    
    if (instIsGuard(this, &ph)) {
        return ret + ph->size();
    } else if (instIsArtificialGuard(ret, &ph)) {
            return ret + 1 + ph->size();
    }
    return ret;
}

Assembler *Assembler::dummy = NULL;
