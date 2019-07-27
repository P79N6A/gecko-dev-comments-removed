

























#include "jsutil.h"

#include "jit/arm64/vixl/Assembler-vixl.h"
#include "jit/Label.h"

namespace vixl {



void Assembler::Reset() {
#ifdef DEBUG
    finalized_ = false;
#endif
    pc_ = nullptr;
}

void Assembler::FinalizeCode() {
#ifdef DEBUG
  finalized_ = true;
#endif
}











template <int element_size>
ptrdiff_t Assembler::LinkAndGetOffsetTo(BufferOffset branch, Label* label) {
  if (armbuffer_.oom())
    return js::jit::LabelBase::INVALID_OFFSET;

  if (label->bound()) {
    
    ptrdiff_t branch_offset = ptrdiff_t(branch.getOffset() / element_size);
    ptrdiff_t label_offset = ptrdiff_t(label->offset() / element_size);
    return label_offset - branch_offset;
  }

  if (!label->used()) {
    
    
    label->use(branch.getOffset());
    return js::jit::LabelBase::INVALID_OFFSET;
  }

  
  
  ptrdiff_t prevHeadOffset = static_cast<ptrdiff_t>(label->offset());
  label->use(branch.getOffset());
  VIXL_ASSERT(prevHeadOffset - branch.getOffset() != js::jit::LabelBase::INVALID_OFFSET);
  return prevHeadOffset - branch.getOffset();
}


ptrdiff_t Assembler::LinkAndGetByteOffsetTo(BufferOffset branch, Label* label) {
  return LinkAndGetOffsetTo<1>(branch, label);
}


ptrdiff_t Assembler::LinkAndGetInstructionOffsetTo(BufferOffset branch, Label* label) {
  return LinkAndGetOffsetTo<kInstructionSize>(branch, label);
}


ptrdiff_t Assembler::LinkAndGetPageOffsetTo(BufferOffset branch, Label* label) {
  return LinkAndGetOffsetTo<kPageSize>(branch, label);
}


BufferOffset Assembler::b(int imm26) {
  return EmitBranch(B | ImmUncondBranch(imm26));
}


void Assembler::b(Instruction* at, int imm26) {
  return EmitBranch(at, B | ImmUncondBranch(imm26));
}


BufferOffset Assembler::b(int imm19, Condition cond) {
  return EmitBranch(B_cond | ImmCondBranch(imm19) | cond);
}


void Assembler::b(Instruction* at, int imm19, Condition cond) {
  EmitBranch(at, B_cond | ImmCondBranch(imm19) | cond);
}


BufferOffset Assembler::b(Label* label) {
  
  BufferOffset branch = b(0);
  Instruction* ins = getInstructionAt(branch);
  VIXL_ASSERT(ins->IsUncondBranchImm());

  
  b(ins, LinkAndGetInstructionOffsetTo(branch, label));
  return branch;
}


BufferOffset Assembler::b(Label* label, Condition cond) {
  
  BufferOffset branch = b(0, Always);
  Instruction* ins = getInstructionAt(branch);
  VIXL_ASSERT(ins->IsCondBranchImm());

  
  b(ins, LinkAndGetInstructionOffsetTo(branch, label), cond);
  return branch;
}


void Assembler::bl(int imm26) {
  EmitBranch(BL | ImmUncondBranch(imm26));
}


void Assembler::bl(Instruction* at, int imm26) {
  EmitBranch(at, BL | ImmUncondBranch(imm26));
}


void Assembler::bl(Label* label) {
  
  BufferOffset branch = b(0);
  Instruction* ins = getInstructionAt(branch);

  
  bl(ins, LinkAndGetInstructionOffsetTo(branch, label));
}


void Assembler::cbz(const Register& rt, int imm19) {
  EmitBranch(SF(rt) | CBZ | ImmCmpBranch(imm19) | Rt(rt));
}


void Assembler::cbz(Instruction* at, const Register& rt, int imm19) {
  EmitBranch(at, SF(rt) | CBZ | ImmCmpBranch(imm19) | Rt(rt));
}


void Assembler::cbz(const Register& rt, Label* label) {
  
  BufferOffset branch = b(0);
  Instruction* ins = getInstructionAt(branch);

  
  cbz(ins, rt, LinkAndGetInstructionOffsetTo(branch, label));
}


void Assembler::cbnz(const Register& rt, int imm19) {
  EmitBranch(SF(rt) | CBNZ | ImmCmpBranch(imm19) | Rt(rt));
}


void Assembler::cbnz(Instruction* at, const Register& rt, int imm19) {
  EmitBranch(at, SF(rt) | CBNZ | ImmCmpBranch(imm19) | Rt(rt));
}


void Assembler::cbnz(const Register& rt, Label* label) {
  
  BufferOffset branch = b(0);
  Instruction* ins = getInstructionAt(branch);

  
  cbnz(ins, rt, LinkAndGetInstructionOffsetTo(branch, label));
}


void Assembler::tbz(const Register& rt, unsigned bit_pos, int imm14) {
  VIXL_ASSERT(rt.Is64Bits() || (rt.Is32Bits() && (bit_pos < kWRegSize)));
  EmitBranch(TBZ | ImmTestBranchBit(bit_pos) | ImmTestBranch(imm14) | Rt(rt));
}


void Assembler::tbz(Instruction* at, const Register& rt, unsigned bit_pos, int imm14) {
  VIXL_ASSERT(rt.Is64Bits() || (rt.Is32Bits() && (bit_pos < kWRegSize)));
  EmitBranch(at, TBZ | ImmTestBranchBit(bit_pos) | ImmTestBranch(imm14) | Rt(rt));
}


void Assembler::tbz(const Register& rt, unsigned bit_pos, Label* label) {
  
  BufferOffset branch = b(0);
  Instruction* ins = getInstructionAt(branch);

  
  tbz(ins, rt, bit_pos, LinkAndGetInstructionOffsetTo(branch, label));
}


void Assembler::tbnz(const Register& rt, unsigned bit_pos, int imm14) {
  VIXL_ASSERT(rt.Is64Bits() || (rt.Is32Bits() && (bit_pos < kWRegSize)));
  EmitBranch(TBNZ | ImmTestBranchBit(bit_pos) | ImmTestBranch(imm14) | Rt(rt));
}


void Assembler::tbnz(Instruction* at, const Register& rt, unsigned bit_pos, int imm14) {
  VIXL_ASSERT(rt.Is64Bits() || (rt.Is32Bits() && (bit_pos < kWRegSize)));
  EmitBranch(at, TBNZ | ImmTestBranchBit(bit_pos) | ImmTestBranch(imm14) | Rt(rt));
}


void Assembler::tbnz(const Register& rt, unsigned bit_pos, Label* label) {
  
  BufferOffset branch = b(0);
  Instruction* ins = getInstructionAt(branch);

  
  tbnz(ins, rt, bit_pos, LinkAndGetInstructionOffsetTo(branch, label));
}


void Assembler::adr(const Register& rd, int imm21) {
  VIXL_ASSERT(rd.Is64Bits());
  EmitBranch(ADR | ImmPCRelAddress(imm21) | Rd(rd));
}


void Assembler::adr(Instruction* at, const Register& rd, int imm21) {
  VIXL_ASSERT(rd.Is64Bits());
  EmitBranch(at, ADR | ImmPCRelAddress(imm21) | Rd(rd));
}


void Assembler::adr(const Register& rd, Label* label) {
  
  
  BufferOffset offset = Emit(0);
  Instruction* ins = getInstructionAt(offset);

  
  adr(ins, rd, LinkAndGetByteOffsetTo(offset, label));
}


void Assembler::adrp(const Register& rd, int imm21) {
  VIXL_ASSERT(rd.Is64Bits());
  EmitBranch(ADRP | ImmPCRelAddress(imm21) | Rd(rd));
}


void Assembler::adrp(Instruction* at, const Register& rd, int imm21) {
  VIXL_ASSERT(rd.Is64Bits());
  EmitBranch(at, ADRP | ImmPCRelAddress(imm21) | Rd(rd));
}


void Assembler::adrp(const Register& rd, Label* label) {
  VIXL_ASSERT(AllowPageOffsetDependentCode());

  
  BufferOffset offset = Emit(0);
  Instruction* ins = getInstructionAt(offset);

  
  adrp(ins, rd, LinkAndGetPageOffsetTo(offset, label));
}


BufferOffset Assembler::ands(const Register& rd, const Register& rn, const Operand& operand) {
  return Logical(rd, rn, operand, ANDS);
}


BufferOffset Assembler::tst(const Register& rn, const Operand& operand) {
  return ands(AppropriateZeroRegFor(rn), rn, operand);
}


void Assembler::ldr(Instruction* at, const CPURegister& rt, int imm19) {
  LoadLiteralOp op = LoadLiteralOpFor(rt);
  Emit(at, op | ImmLLiteral(imm19) | Rt(rt));
}


BufferOffset Assembler::hint(SystemHint code) {
  return Emit(HINT | ImmHint(code) | Rt(xzr));
}


void Assembler::hint(Instruction* at, SystemHint code) {
  Emit(at, HINT | ImmHint(code) | Rt(xzr));
}


void Assembler::svc(Instruction* at, int code) {
  VIXL_ASSERT(is_uint16(code));
  Emit(at, SVC | ImmException(code));
}


void Assembler::nop(Instruction* at) {
  hint(at, NOP);
}


BufferOffset Assembler::Logical(const Register& rd, const Register& rn,
                                const Operand& operand, LogicalOp op)
{
  VIXL_ASSERT(rd.size() == rn.size());
  if (operand.IsImmediate()) {
    int64_t immediate = operand.immediate();
    unsigned reg_size = rd.size();

    VIXL_ASSERT(immediate != 0);
    VIXL_ASSERT(immediate != -1);
    VIXL_ASSERT(rd.Is64Bits() || is_uint32(immediate));

    
    if ((op & NOT) == NOT) {
      op = static_cast<LogicalOp>(op & ~NOT);
      immediate = rd.Is64Bits() ? ~immediate : (~immediate & kWRegMask);
    }

    unsigned n, imm_s, imm_r;
    if (IsImmLogical(immediate, reg_size, &n, &imm_s, &imm_r)) {
      
      return LogicalImmediate(rd, rn, n, imm_s, imm_r, op);
    } else {
      
      VIXL_UNREACHABLE();
    }
  } else {
    VIXL_ASSERT(operand.IsShiftedRegister());
    VIXL_ASSERT(operand.reg().size() == rd.size());
    Instr dp_op = static_cast<Instr>(op | LogicalShiftedFixed);
    return DataProcShiftedRegister(rd, rn, operand, LeaveFlags, dp_op);
  }
}


BufferOffset Assembler::LogicalImmediate(const Register& rd, const Register& rn,
                                         unsigned n, unsigned imm_s, unsigned imm_r, LogicalOp op)
{
    unsigned reg_size = rd.size();
    Instr dest_reg = (op == ANDS) ? Rd(rd) : RdSP(rd);
    return Emit(SF(rd) | LogicalImmediateFixed | op | BitN(n, reg_size) |
                ImmSetBits(imm_s, reg_size) | ImmRotate(imm_r, reg_size) | dest_reg | Rn(rn));
}


BufferOffset Assembler::DataProcShiftedRegister(const Register& rd, const Register& rn,
                                                const Operand& operand, FlagsUpdate S, Instr op)
{
  VIXL_ASSERT(operand.IsShiftedRegister());
  VIXL_ASSERT(rn.Is64Bits() || (rn.Is32Bits() && is_uint5(operand.shift_amount())));
  return Emit(SF(rd) | op | Flags(S) |
              ShiftDP(operand.shift()) | ImmDPShift(operand.shift_amount()) |
              Rm(operand.reg()) | Rn(rn) | Rd(rd));
}


void MozBaseAssembler::InsertIndexIntoTag(uint8_t* load, uint32_t index) {
  
  
  
  *((uint32_t*)load) |= Assembler::ImmLLiteral(index);
}


bool MozBaseAssembler::PatchConstantPoolLoad(void* loadAddr, void* constPoolAddr) {
  Instruction* load = reinterpret_cast<Instruction*>(loadAddr);

  
  
  uint32_t index = load->ImmLLiteral();

  
  
  uint32_t* constPool = reinterpret_cast<uint32_t*>(constPoolAddr);
  Instruction* source = reinterpret_cast<Instruction*>(&constPool[index]);

  load->SetImmLLiteral(source);
  return false; 
}


uint32_t MozBaseAssembler::PlaceConstantPoolBarrier(int offset) {
  MOZ_CRASH("PlaceConstantPoolBarrier");
}


struct PoolHeader {
  uint32_t data;

  struct Header {
    
    
    union {
      struct {
        uint32_t size : 15;

	
	
	
        bool isNatural : 1;
        uint32_t ONES : 16;
      };
      uint32_t data;
    };

    Header(int size_, bool isNatural_)
      : size(size_),
        isNatural(isNatural_),
        ONES(0xffff)
    { }

    Header(uint32_t data)
      : data(data)
    {
      JS_STATIC_ASSERT(sizeof(Header) == sizeof(uint32_t));
      VIXL_ASSERT(ONES == 0xffff);
    }

    uint32_t raw() const {
      JS_STATIC_ASSERT(sizeof(Header) == sizeof(uint32_t));
      return data;
    }
  };

  PoolHeader(int size_, bool isNatural_)
    : data(Header(size_, isNatural_).raw())
  { }

  uint32_t size() const {
    Header tmp(data);
    return tmp.size;
  }

  uint32_t isNatural() const {
    Header tmp(data);
    return tmp.isNatural;
  }
};


void MozBaseAssembler::WritePoolHeader(uint8_t* start, js::jit::Pool* p, bool isNatural) {
  JS_STATIC_ASSERT(sizeof(PoolHeader) == 4);

  
  const uintptr_t totalPoolSize = sizeof(PoolHeader) + p->getPoolSize();
  const uintptr_t totalPoolInstructions = totalPoolSize / sizeof(Instruction);

  VIXL_ASSERT((totalPoolSize & 0x3) == 0);
  VIXL_ASSERT(totalPoolInstructions < (1 << 15));

  PoolHeader header(totalPoolInstructions, isNatural);
  *(PoolHeader*)start = header;
}


void MozBaseAssembler::WritePoolFooter(uint8_t* start, js::jit::Pool* p, bool isNatural) {
  return;
}


void MozBaseAssembler::WritePoolGuard(BufferOffset branch, Instruction* inst, BufferOffset dest) {
  int byteOffset = dest.getOffset() - branch.getOffset();
  VIXL_ASSERT(byteOffset % kInstructionSize == 0);

  int instOffset = byteOffset >> kInstructionSizeLog2;
  Assembler::b(inst, instOffset);
}


ptrdiff_t MozBaseAssembler::GetBranchOffset(const Instruction* ins) {
  
  if (ins->BranchType() != UnknownBranchType)
    return ins->ImmPCRawOffset() * kInstructionSize;

  
  
  if (ins->IsADR())
    return ins->ImmPCRawOffset();

  
  if (ins->IsADRP())
    return ins->ImmPCRawOffset() * kPageSize;

  MOZ_CRASH("Unsupported branch type");
}


void MozBaseAssembler::RetargetNearBranch(Instruction* i, int offset, Condition cond, bool final) {
  if (i->IsCondBranchImm()) {
    VIXL_ASSERT(i->IsCondB());
    Assembler::b(i, offset, cond);
    return;
  }
  MOZ_CRASH("Unsupported branch type");
}


void MozBaseAssembler::RetargetNearBranch(Instruction* i, int byteOffset, bool final) {
  const int instOffset = byteOffset >> kInstructionSizeLog2;

  
  if (i->IsCondBranchImm()) {
    VIXL_ASSERT(byteOffset % kInstructionSize == 0);
    VIXL_ASSERT(i->IsCondB());
    Condition cond = static_cast<Condition>(i->ConditionBranch());
    Assembler::b(i, instOffset, cond);
    return;
  }

  
  if (i->IsUncondBranchImm()) {
    VIXL_ASSERT(byteOffset % kInstructionSize == 0);
    if (i->IsUncondB()) {
      Assembler::b(i, instOffset);
    } else {
      VIXL_ASSERT(i->IsBL());
      Assembler::bl(i, instOffset);
    }

    VIXL_ASSERT(i->ImmUncondBranch() == instOffset);
    return;
  }

  
  if (i->IsCompareBranch()) {
    VIXL_ASSERT(byteOffset % kInstructionSize == 0);
    Register rt = i->SixtyFourBits() ? Register::XRegFromCode(i->Rt())
                                     : Register::WRegFromCode(i->Rt());

    if (i->IsCBZ()) {
      Assembler::cbz(i, rt, instOffset);
    } else {
      VIXL_ASSERT(i->IsCBNZ());
      Assembler::cbnz(i, rt, instOffset);
    }

    VIXL_ASSERT(i->ImmCmpBranch() == instOffset);
    return;
  }

  
  if (i->IsTestBranch()) {
    VIXL_ASSERT(byteOffset % kInstructionSize == 0);
    
    unsigned bit_pos = (i->ImmTestBranchBit5() << 5) | (i->ImmTestBranchBit40());
    VIXL_ASSERT(is_uint6(bit_pos));

    
    Register rt = Register::XRegFromCode(i->Rt());

    if (i->IsTBZ()) {
      Assembler::tbz(i, rt, bit_pos, instOffset);
    } else {
      VIXL_ASSERT(i->IsTBNZ());
      Assembler::tbnz(i, rt, bit_pos, instOffset);
    }

    VIXL_ASSERT(i->ImmTestBranch() == instOffset);
    return;
  }

  if (i->IsADR()) {
    Register rd = Register::XRegFromCode(i->Rd());
    Assembler::adr(i, rd, byteOffset);
    return;
  }

  if (i->IsADRP()) {
    const int pageOffset = byteOffset >> kPageSizeLog2;
    Register rd = Register::XRegFromCode(i->Rd());
    Assembler::adrp(i, rd, pageOffset);
    return;
  }

  MOZ_CRASH("Unsupported branch type");
}


void MozBaseAssembler::RetargetFarBranch(Instruction* i, uint8_t** slot, uint8_t* dest, Condition cond) {
  MOZ_CRASH("RetargetFarBranch()");
}


}  

