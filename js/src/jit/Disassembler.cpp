





#include "jit/Disassembler.h"

using namespace js;
using namespace js::jit;
using namespace js::jit::Disassembler;

#ifdef DEBUG
bool
Disassembler::ComplexAddress::operator==(const ComplexAddress& other) const
{
    return base_ == other.base_ &&
           index_ == other.index_ &&
           scale_ == other.scale_ &&
           disp_ == other.disp_ &&
           isPCRelative_ == other.isPCRelative_;
}

bool
Disassembler::ComplexAddress::operator!=(const ComplexAddress& other) const
{
    return !operator==(other);
}

bool
Disassembler::OtherOperand::operator==(const OtherOperand& other) const
{
    if (kind_ != other.kind_)
        return false;
    switch (kind_) {
      case Imm: return u_.imm == other.u_.imm;
      case GPR: return u_.gpr == other.u_.gpr;
      case FPR: return u_.fpr == other.u_.fpr;
    }
    MOZ_CRASH("Unexpected OtherOperand kind");
}

bool
Disassembler::OtherOperand::operator!=(const OtherOperand& other) const
{
    return !operator==(other);
}

bool
Disassembler::HeapAccess::operator==(const HeapAccess& other) const
{
    return kind_ == other.kind_ &&
           size_ == other.size_ &&
           address_ == other.address_ &&
           otherOperand_ == other.otherOperand_;
}

bool
Disassembler::HeapAccess::operator!=(const HeapAccess& other) const
{
    return !operator==(other);
}

#endif
