

























#include "jit/arm64/vixl/Instructions-vixl.h"

namespace vixl {

bool Instruction::IsUncondB() const {
  return Mask(UnconditionalBranchMask) == (UnconditionalBranchFixed | B);
}


bool Instruction::IsCondB() const {
  return Mask(ConditionalBranchMask) == (ConditionalBranchFixed | B_cond);
}


bool Instruction::IsBL() const {
  return Mask(UnconditionalBranchMask) == (UnconditionalBranchFixed | BL);
}


bool Instruction::IsBR() const {
  return Mask(UnconditionalBranchToRegisterMask) == (UnconditionalBranchToRegisterFixed | BR);
}


bool Instruction::IsBLR() const {
  return Mask(UnconditionalBranchToRegisterMask) == (UnconditionalBranchToRegisterFixed | BLR);
}


bool Instruction::IsTBZ() const {
  return Mask(TestBranchMask) == TBZ;
}


bool Instruction::IsTBNZ() const {
  return Mask(TestBranchMask) == TBNZ;
}


bool Instruction::IsCBZ() const {
  return Mask(CompareBranchMask) == CBZ_w || Mask(CompareBranchMask) == CBZ_x;
}


bool Instruction::IsCBNZ() const {
  return Mask(CompareBranchMask) == CBNZ_w || Mask(CompareBranchMask) == CBNZ_x;
}


bool Instruction::IsLDR() const {
  return Mask(LoadLiteralMask) == LDR_x_lit;
}


bool Instruction::IsADR() const {
  return Mask(PCRelAddressingMask) == ADR;
}


bool Instruction::IsADRP() const {
  return Mask(PCRelAddressingMask) == ADRP;
}


bool Instruction::IsBranchLinkImm() const {
  return Mask(UnconditionalBranchFMask) == (UnconditionalBranchFixed | BL);
}


bool Instruction::IsTargetReachable(Instruction* target) const {
    VIXL_ASSERT(((target - this) & 3) == 0);
    int offset = (target - this) >> kInstructionSizeLog2;
    switch (BranchType()) {
      case CondBranchType:
        return is_int19(offset);
      case UncondBranchType:
        return is_int26(offset);
      case CompareBranchType:
        return is_int19(offset);
      case TestBranchType:
        return is_int14(offset);
      default:
        VIXL_UNREACHABLE();
    }
}


ptrdiff_t Instruction::ImmPCRawOffset() const {
  ptrdiff_t offset;
  if (IsPCRelAddressing()) {
    
    offset = ImmPCRel();
  } else if (BranchType() == UnknownBranchType) {
    offset = ImmLLiteral();
  } else {
    offset = ImmBranch();
  }
  return offset;
}


void Instruction::SetBits32(int msb, int lsb, unsigned value) {
  uint32_t me;
  memcpy(&me, this, sizeof(me));
  uint32_t new_mask = (1 << (msb+1)) - (1 << lsb);
  uint32_t keep_mask = ~new_mask;
  me = (me & keep_mask) | ((value << lsb) & new_mask);
  memcpy(this, &me, sizeof(me));
}


} 
