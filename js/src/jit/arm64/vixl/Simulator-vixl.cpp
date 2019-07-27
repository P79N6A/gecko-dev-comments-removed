

























#include "js-config.h"

#ifdef JS_SIMULATOR_ARM64

#include "jit/arm64/vixl/Simulator-vixl.h"

#include <math.h>
#include <string.h>

namespace vixl {

const Instruction* Simulator::kEndOfSimAddress = NULL;

void SimSystemRegister::SetBits(int msb, int lsb, uint32_t bits) {
  int width = msb - lsb + 1;
  VIXL_ASSERT(is_uintn(width, bits) || is_intn(width, bits));

  bits <<= lsb;
  uint32_t mask = ((1 << width) - 1) << lsb;
  VIXL_ASSERT((mask & write_ignore_mask_) == 0);

  value_ = (value_ & ~mask) | (bits & mask);
}


SimSystemRegister SimSystemRegister::DefaultValueFor(SystemRegister id) {
  switch (id) {
    case NZCV:
      return SimSystemRegister(0x00000000, NZCVWriteIgnoreMask);
    case FPCR:
      return SimSystemRegister(0x00000000, FPCRWriteIgnoreMask);
    default:
      VIXL_UNREACHABLE();
      return SimSystemRegister();
  }
}


Simulator::~Simulator() {
  delete [] stack_;
  
  decoder_->RemoveVisitor(print_disasm_);
  delete print_disasm_;

  decoder_->RemoveVisitor(instrumentation_);
  delete instrumentation_;
}


void Simulator::Run() {
  pc_modified_ = false;
  while (pc_ != kEndOfSimAddress) {
    ExecuteInstruction();
  }
}


void Simulator::RunFrom(const Instruction* first) {
  set_pc(first);
  Run();
}


const char* Simulator::xreg_names[] = {
"x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
"x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
"x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
"x24", "x25", "x26", "x27", "x28", "x29", "lr",  "xzr", "sp"};


const char* Simulator::wreg_names[] = {
"w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
"w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
"w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
"w24", "w25", "w26", "w27", "w28", "w29", "w30", "wzr", "wsp"};

const char* Simulator::sreg_names[] = {
"s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
"s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
"s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
"s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};

const char* Simulator::dreg_names[] = {
"d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
"d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};

const char* Simulator::vreg_names[] = {
"v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
"v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
"v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
"v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"};



const char* Simulator::WRegNameForCode(unsigned code, Reg31Mode mode) {
  VIXL_ASSERT(code < kNumberOfRegisters);
  
  if ((code == kZeroRegCode) && (mode == Reg31IsStackPointer)) {
    code = kZeroRegCode + 1;
  }
  return wreg_names[code];
}


const char* Simulator::XRegNameForCode(unsigned code, Reg31Mode mode) {
  VIXL_ASSERT(code < kNumberOfRegisters);
  
  if ((code == kZeroRegCode) && (mode == Reg31IsStackPointer)) {
    code = kZeroRegCode + 1;
  }
  return xreg_names[code];
}


const char* Simulator::SRegNameForCode(unsigned code) {
  VIXL_ASSERT(code < kNumberOfFPRegisters);
  return sreg_names[code];
}


const char* Simulator::DRegNameForCode(unsigned code) {
  VIXL_ASSERT(code < kNumberOfFPRegisters);
  return dreg_names[code];
}


const char* Simulator::VRegNameForCode(unsigned code) {
  VIXL_ASSERT(code < kNumberOfFPRegisters);
  return vreg_names[code];
}


#define COLOUR(colour_code)       "\033[0;" colour_code "m"
#define COLOUR_BOLD(colour_code)  "\033[1;" colour_code "m"
#define NORMAL  ""
#define GREY    "30"
#define RED     "31"
#define GREEN   "32"
#define YELLOW  "33"
#define BLUE    "34"
#define MAGENTA "35"
#define CYAN    "36"
#define WHITE   "37"
void Simulator::set_coloured_trace(bool value) {
  coloured_trace_ = value;

  clr_normal          = value ? COLOUR(NORMAL)        : "";
  clr_flag_name       = value ? COLOUR_BOLD(WHITE)    : "";
  clr_flag_value      = value ? COLOUR(NORMAL)        : "";
  clr_reg_name        = value ? COLOUR_BOLD(CYAN)     : "";
  clr_reg_value       = value ? COLOUR(CYAN)          : "";
  clr_fpreg_name      = value ? COLOUR_BOLD(MAGENTA)  : "";
  clr_fpreg_value     = value ? COLOUR(MAGENTA)       : "";
  clr_memory_address  = value ? COLOUR_BOLD(BLUE)     : "";
  clr_warning         = value ? COLOUR_BOLD(YELLOW)   : "";
  clr_warning_message = value ? COLOUR(YELLOW)        : "";
  clr_printf          = value ? COLOUR(GREEN)         : "";
}
#undef COLOUR
#undef COLOUR_BOLD
#undef NORMAL
#undef GREY
#undef RED
#undef GREEN
#undef YELLOW
#undef BLUE
#undef MAGENTA
#undef CYAN
#undef WHITE


void Simulator::set_trace_parameters(int parameters) {
  bool disasm_before = trace_parameters_ & LOG_DISASM;
  trace_parameters_ = parameters;
  bool disasm_after = trace_parameters_ & LOG_DISASM;

  if (disasm_before != disasm_after) {
    if (disasm_after) {
      decoder_->InsertVisitorBefore(print_disasm_, this);
    } else {
      decoder_->RemoveVisitor(print_disasm_);
    }
  }
}


void Simulator::set_instruction_stats(bool value) {
  if (value != instruction_stats_) {
    if (value) {
      decoder_->AppendVisitor(instrumentation_);
    } else {
      decoder_->RemoveVisitor(instrumentation_);
    }
    instruction_stats_ = value;
  }
}


int64_t Simulator::AddWithCarry(unsigned reg_size,
                                bool set_flags,
                                int64_t src1,
                                int64_t src2,
                                int64_t carry_in) {
  VIXL_ASSERT((carry_in == 0) || (carry_in == 1));
  VIXL_ASSERT((reg_size == kXRegSize) || (reg_size == kWRegSize));

  uint64_t u1, u2;
  int64_t result;
  int64_t signed_sum = src1 + src2 + carry_in;

  uint32_t N, Z, C, V;

  if (reg_size == kWRegSize) {
    u1 = static_cast<uint64_t>(src1) & kWRegMask;
    u2 = static_cast<uint64_t>(src2) & kWRegMask;

    result = signed_sum & kWRegMask;
    
    C = ((kWMaxUInt - u1) < (u2 + carry_in)) ||
        ((kWMaxUInt - u1 - carry_in) < u2);
    
    
    int64_t s_src1 = src1 << (kXRegSize - kWRegSize);
    int64_t s_src2 = src2 << (kXRegSize - kWRegSize);
    int64_t s_result = result << (kXRegSize - kWRegSize);
    V = ((s_src1 ^ s_src2) >= 0) && ((s_src1 ^ s_result) < 0);

  } else {
    u1 = static_cast<uint64_t>(src1);
    u2 = static_cast<uint64_t>(src2);

    result = signed_sum;
    
    C = ((kXMaxUInt - u1) < (u2 + carry_in)) ||
        ((kXMaxUInt - u1 - carry_in) < u2);
    
    
    V = ((src1 ^ src2) >= 0) && ((src1 ^ result) < 0);
  }

  N = CalcNFlag(result, reg_size);
  Z = CalcZFlag(result);

  if (set_flags) {
    nzcv().SetN(N);
    nzcv().SetZ(Z);
    nzcv().SetC(C);
    nzcv().SetV(V);
    LogSystemRegister(NZCV);
  }
  return result;
}


int64_t Simulator::ShiftOperand(unsigned reg_size,
                                int64_t value,
                                Shift shift_type,
                                unsigned amount) {
  if (amount == 0) {
    return value;
  }
  int64_t mask = reg_size == kXRegSize ? kXRegMask : kWRegMask;
  switch (shift_type) {
    case LSL:
      return (value << amount) & mask;
    case LSR:
      return static_cast<uint64_t>(value) >> amount;
    case ASR: {
      
      unsigned s_shift = kXRegSize - reg_size;
      
      int64_t s_value = (value << s_shift) >> s_shift;
      return (s_value >> amount) & mask;
    }
    case ROR: {
      if (reg_size == kWRegSize) {
        value &= kWRegMask;
      }
      return (static_cast<uint64_t>(value) >> amount) |
             ((value & ((INT64_C(1) << amount) - 1)) <<
              (reg_size - amount));
    }
    default:
      VIXL_UNIMPLEMENTED();
      return 0;
  }
}


int64_t Simulator::ExtendValue(unsigned reg_size,
                               int64_t value,
                               Extend extend_type,
                               unsigned left_shift) {
  switch (extend_type) {
    case UXTB:
      value &= kByteMask;
      break;
    case UXTH:
      value &= kHalfWordMask;
      break;
    case UXTW:
      value &= kWordMask;
      break;
    case SXTB:
      value = (value << 56) >> 56;
      break;
    case SXTH:
      value = (value << 48) >> 48;
      break;
    case SXTW:
      value = (value << 32) >> 32;
      break;
    case UXTX:
    case SXTX:
      break;
    default:
      VIXL_UNREACHABLE();
  }
  int64_t mask = (reg_size == kXRegSize) ? kXRegMask : kWRegMask;
  return (value << left_shift) & mask;
}


template<> double Simulator::FPDefaultNaN<double>() const {
  return kFP64DefaultNaN;
}


template<> float Simulator::FPDefaultNaN<float>() const {
  return kFP32DefaultNaN;
}


void Simulator::FPCompare(double val0, double val1) {
  AssertSupportedFPCR();

  
  
  if ((isnan(val0) != 0) || (isnan(val1) != 0)) {
    nzcv().SetRawValue(FPUnorderedFlag);
  } else if (val0 < val1) {
    nzcv().SetRawValue(FPLessThanFlag);
  } else if (val0 > val1) {
    nzcv().SetRawValue(FPGreaterThanFlag);
  } else if (val0 == val1) {
    nzcv().SetRawValue(FPEqualFlag);
  } else {
    VIXL_UNREACHABLE();
  }
  LogSystemRegister(NZCV);
}


void Simulator::PrintSystemRegisters() {
  PrintSystemRegister(NZCV);
  PrintSystemRegister(FPCR);
}


void Simulator::PrintRegisters() {
  for (unsigned i = 0; i < kNumberOfRegisters; i++) {
    PrintRegister(i);
  }
}


void Simulator::PrintFPRegisters() {
  for (unsigned i = 0; i < kNumberOfFPRegisters; i++) {
    PrintFPRegister(i);
  }
}


void Simulator::PrintRegister(unsigned code, Reg31Mode r31mode) {
  
  if ((code == kZeroRegCode) && (r31mode == Reg31IsZeroRegister)) {
    return;
  }

  
  fprintf(stream_, "# %s%5s: %s0x%016" PRIx64 "%s\n",
          clr_reg_name, XRegNameForCode(code, r31mode),
          clr_reg_value, reg<uint64_t>(code, r31mode), clr_normal);
}


void Simulator::PrintFPRegister(unsigned code, PrintFPRegisterSizes sizes) {
  

  VIXL_ASSERT(sizes != 0);
  VIXL_ASSERT((sizes & kPrintAllFPRegValues) == sizes);

  
  fprintf(stream_, "# %s%5s: %s0x%016" PRIx64 "%s (",
          clr_fpreg_name, VRegNameForCode(code),
          clr_fpreg_value, fpreg<uint64_t>(code), clr_normal);

  
  bool need_separator = false;
  if (sizes & kPrintDRegValue) {
    fprintf(stream_, "%s%s%s: %s%g%s",
            need_separator ? ", " : "",
            clr_fpreg_name, DRegNameForCode(code),
            clr_fpreg_value, fpreg<double>(code), clr_normal);
    need_separator = true;
  }

  if (sizes & kPrintSRegValue) {
    fprintf(stream_, "%s%s%s: %s%g%s",
            need_separator ? ", " : "",
            clr_fpreg_name, SRegNameForCode(code),
            clr_fpreg_value, fpreg<float>(code), clr_normal);
    need_separator = true;
  }

  
  fprintf(stream_, ")\n");
}


void Simulator::PrintSystemRegister(SystemRegister id) {
  switch (id) {
    case NZCV:
      fprintf(stream_, "# %sNZCV: %sN:%d Z:%d C:%d V:%d%s\n",
              clr_flag_name, clr_flag_value,
              nzcv().N(), nzcv().Z(), nzcv().C(), nzcv().V(),
              clr_normal);
      break;
    case FPCR: {
      static const char * rmode[] = {
        "0b00 (Round to Nearest)",
        "0b01 (Round towards Plus Infinity)",
        "0b10 (Round towards Minus Infinity)",
        "0b11 (Round towards Zero)"
      };
      VIXL_ASSERT(fpcr().RMode() < (sizeof(rmode) / sizeof(rmode[0])));
      fprintf(stream_,
              "# %sFPCR: %sAHP:%d DN:%d FZ:%d RMode:%s%s\n",
              clr_flag_name, clr_flag_value,
              fpcr().AHP(), fpcr().DN(), fpcr().FZ(), rmode[fpcr().RMode()],
              clr_normal);
      break;
    }
    default:
      VIXL_UNREACHABLE();
  }
}


void Simulator::PrintRead(uintptr_t address,
                          size_t size,
                          unsigned reg_code) {
  USE(size);  

  
  fprintf(stream_, "# %s%5s: %s0x%016" PRIx64 "%s",
          clr_reg_name, XRegNameForCode(reg_code),
          clr_reg_value, reg<uint64_t>(reg_code), clr_normal);

  fprintf(stream_, " <- %s0x%016" PRIxPTR "%s\n",
          clr_memory_address, address, clr_normal);
}


void Simulator::PrintReadFP(uintptr_t address,
                            size_t size,
                            unsigned reg_code) {
  
  switch (size) {
    case kSRegSizeInBytes:
      VIXL_STATIC_ASSERT(kSRegSizeInBytes == 4);
      fprintf(stream_, "# %s%5s: %s0x%016" PRIx64 "%s (%s%s: %s%gf%s)",
              clr_fpreg_name, VRegNameForCode(reg_code),
              clr_fpreg_value, fpreg<uint64_t>(reg_code), clr_normal,
              clr_fpreg_name, SRegNameForCode(reg_code),
              clr_fpreg_value, fpreg<float>(reg_code), clr_normal);
      break;
    case kDRegSizeInBytes:
      VIXL_STATIC_ASSERT(kDRegSizeInBytes == 8);
      fprintf(stream_, "# %s%5s: %s0x%016" PRIx64 "%s (%s%s: %s%g%s)",
              clr_fpreg_name, VRegNameForCode(reg_code),
              clr_fpreg_value, fpreg<uint64_t>(reg_code), clr_normal,
              clr_fpreg_name, DRegNameForCode(reg_code),
              clr_fpreg_value, fpreg<double>(reg_code), clr_normal);
      break;
    default:
      VIXL_UNREACHABLE();
  }

  fprintf(stream_, " <- %s0x%016" PRIxPTR "%s\n",
          clr_memory_address, address, clr_normal);
}


void Simulator::PrintWrite(uintptr_t address,
                           size_t size,
                           unsigned reg_code) {
  
  
  switch (size) {
    case 1:
      fprintf(stream_, "# %s%5s<7:0>:          %s0x%02" PRIx8 "%s",
              clr_reg_name, WRegNameForCode(reg_code),
              clr_reg_value, reg<uint8_t>(reg_code), clr_normal);
      break;
    case 2:
      fprintf(stream_, "# %s%5s<15:0>:       %s0x%04" PRIx16 "%s",
              clr_reg_name, WRegNameForCode(reg_code),
              clr_reg_value, reg<uint16_t>(reg_code), clr_normal);
      break;
    case kWRegSizeInBytes:
      VIXL_STATIC_ASSERT(kWRegSizeInBytes == 4);
      fprintf(stream_, "# %s%5s:         %s0x%08" PRIx32 "%s",
              clr_reg_name, WRegNameForCode(reg_code),
              clr_reg_value, reg<uint32_t>(reg_code), clr_normal);
      break;
    case kXRegSizeInBytes:
      VIXL_STATIC_ASSERT(kXRegSizeInBytes == 8);
      fprintf(stream_, "# %s%5s: %s0x%016" PRIx64 "%s",
              clr_reg_name, XRegNameForCode(reg_code),
              clr_reg_value, reg<uint64_t>(reg_code), clr_normal);
      break;
    default:
      VIXL_UNREACHABLE();
  }

  fprintf(stream_, " -> %s0x%016" PRIxPTR "%s\n",
          clr_memory_address, address, clr_normal);
}


void Simulator::PrintWriteFP(uintptr_t address,
                             size_t size,
                             unsigned reg_code) {
  
  
  switch (size) {
    case kSRegSizeInBytes:
      VIXL_STATIC_ASSERT(kSRegSize == 32);
      fprintf(stream_, "# %s%5s<31:0>:   %s0x%08" PRIx32 "%s (%s%s: %s%gf%s)",
              clr_fpreg_name, VRegNameForCode(reg_code),
              clr_fpreg_value, fpreg<uint32_t>(reg_code), clr_normal,
              clr_fpreg_name, SRegNameForCode(reg_code),
              clr_fpreg_value, fpreg<float>(reg_code), clr_normal);
      break;
    case kDRegSizeInBytes:
      VIXL_STATIC_ASSERT(kDRegSize == 64);
      fprintf(stream_, "# %s%5s: %s0x%016" PRIx64 "%s (%s%s: %s%g%s)",
              clr_fpreg_name, VRegNameForCode(reg_code),
              clr_fpreg_value, fpreg<uint64_t>(reg_code), clr_normal,
              clr_fpreg_name, DRegNameForCode(reg_code),
              clr_fpreg_value, fpreg<double>(reg_code), clr_normal);
      break;
    default:
      VIXL_UNREACHABLE();
  }

  fprintf(stream_, " -> %s0x%016" PRIxPTR "%s\n",
          clr_memory_address, address, clr_normal);
}




void Simulator::VisitUnimplemented(const Instruction* instr) {
  printf("Unimplemented instruction at %p: 0x%08" PRIx32 "\n",
         reinterpret_cast<const void*>(instr), instr->InstructionBits());
  VIXL_UNIMPLEMENTED();
}


void Simulator::VisitUnallocated(const Instruction* instr) {
  printf("Unallocated instruction at %p: 0x%08" PRIx32 "\n",
         reinterpret_cast<const void*>(instr), instr->InstructionBits());
  VIXL_UNIMPLEMENTED();
}


void Simulator::VisitPCRelAddressing(const Instruction* instr) {
  VIXL_ASSERT((instr->Mask(PCRelAddressingMask) == ADR) ||
              (instr->Mask(PCRelAddressingMask) == ADRP));

  set_reg(instr->Rd(), instr->ImmPCOffsetTarget());
}


void Simulator::VisitUnconditionalBranch(const Instruction* instr) {
  switch (instr->Mask(UnconditionalBranchMask)) {
    case BL:
      set_lr(instr->NextInstruction());
      
    case B:
      set_pc(instr->ImmPCOffsetTarget());
      break;
    default: VIXL_UNREACHABLE();
  }
}


void Simulator::VisitConditionalBranch(const Instruction* instr) {
  VIXL_ASSERT(instr->Mask(ConditionalBranchMask) == B_cond);
  if (ConditionPassed(instr->ConditionBranch())) {
    set_pc(instr->ImmPCOffsetTarget());
  }
}


void Simulator::VisitUnconditionalBranchToRegister(const Instruction* instr) {
  const Instruction* target = Instruction::Cast(xreg(instr->Rn()));

  switch (instr->Mask(UnconditionalBranchToRegisterMask)) {
    case BLR:
      set_lr(instr->NextInstruction());
      
    case BR:
    case RET: set_pc(target); break;
    default: VIXL_UNREACHABLE();
  }
}


void Simulator::VisitTestBranch(const Instruction* instr) {
  unsigned bit_pos = (instr->ImmTestBranchBit5() << 5) |
                     instr->ImmTestBranchBit40();
  bool bit_zero = ((xreg(instr->Rt()) >> bit_pos) & 1) == 0;
  bool take_branch = false;
  switch (instr->Mask(TestBranchMask)) {
    case TBZ: take_branch = bit_zero; break;
    case TBNZ: take_branch = !bit_zero; break;
    default: VIXL_UNIMPLEMENTED();
  }
  if (take_branch) {
    set_pc(instr->ImmPCOffsetTarget());
  }
}


void Simulator::VisitCompareBranch(const Instruction* instr) {
  unsigned rt = instr->Rt();
  bool take_branch = false;
  switch (instr->Mask(CompareBranchMask)) {
    case CBZ_w: take_branch = (wreg(rt) == 0); break;
    case CBZ_x: take_branch = (xreg(rt) == 0); break;
    case CBNZ_w: take_branch = (wreg(rt) != 0); break;
    case CBNZ_x: take_branch = (xreg(rt) != 0); break;
    default: VIXL_UNIMPLEMENTED();
  }
  if (take_branch) {
    set_pc(instr->ImmPCOffsetTarget());
  }
}


void Simulator::AddSubHelper(const Instruction* instr, int64_t op2) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  bool set_flags = instr->FlagsUpdate();
  int64_t new_val = 0;
  Instr operation = instr->Mask(AddSubOpMask);

  switch (operation) {
    case ADD:
    case ADDS: {
      new_val = AddWithCarry(reg_size,
                             set_flags,
                             reg(reg_size, instr->Rn(), instr->RnMode()),
                             op2);
      break;
    }
    case SUB:
    case SUBS: {
      new_val = AddWithCarry(reg_size,
                             set_flags,
                             reg(reg_size, instr->Rn(), instr->RnMode()),
                             ~op2,
                             1);
      break;
    }
    default: VIXL_UNREACHABLE();
  }

  set_reg(reg_size, instr->Rd(), new_val, LogRegWrites, instr->RdMode());
}


void Simulator::VisitAddSubShifted(const Instruction* instr) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  int64_t op2 = ShiftOperand(reg_size,
                             reg(reg_size, instr->Rm()),
                             static_cast<Shift>(instr->ShiftDP()),
                             instr->ImmDPShift());
  AddSubHelper(instr, op2);
}


void Simulator::VisitAddSubImmediate(const Instruction* instr) {
  int64_t op2 = instr->ImmAddSub() << ((instr->ShiftAddSub() == 1) ? 12 : 0);
  AddSubHelper(instr, op2);
}


void Simulator::VisitAddSubExtended(const Instruction* instr) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  int64_t op2 = ExtendValue(reg_size,
                            reg(reg_size, instr->Rm()),
                            static_cast<Extend>(instr->ExtendMode()),
                            instr->ImmExtendShift());
  AddSubHelper(instr, op2);
}


void Simulator::VisitAddSubWithCarry(const Instruction* instr) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  int64_t op2 = reg(reg_size, instr->Rm());
  int64_t new_val;

  if ((instr->Mask(AddSubOpMask) == SUB) || instr->Mask(AddSubOpMask) == SUBS) {
    op2 = ~op2;
  }

  new_val = AddWithCarry(reg_size,
                         instr->FlagsUpdate(),
                         reg(reg_size, instr->Rn()),
                         op2,
                         C());

  set_reg(reg_size, instr->Rd(), new_val);
}


void Simulator::VisitLogicalShifted(const Instruction* instr) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  Shift shift_type = static_cast<Shift>(instr->ShiftDP());
  unsigned shift_amount = instr->ImmDPShift();
  int64_t op2 = ShiftOperand(reg_size, reg(reg_size, instr->Rm()), shift_type,
                             shift_amount);
  if (instr->Mask(NOT) == NOT) {
    op2 = ~op2;
  }
  LogicalHelper(instr, op2);
}


void Simulator::VisitLogicalImmediate(const Instruction* instr) {
  LogicalHelper(instr, instr->ImmLogical());
}


void Simulator::LogicalHelper(const Instruction* instr, int64_t op2) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  int64_t op1 = reg(reg_size, instr->Rn());
  int64_t result = 0;
  bool update_flags = false;

  
  
  switch (instr->Mask(LogicalOpMask & ~NOT)) {
    case ANDS: update_flags = true;  
    case AND: result = op1 & op2; break;
    case ORR: result = op1 | op2; break;
    case EOR: result = op1 ^ op2; break;
    default:
      VIXL_UNIMPLEMENTED();
  }

  if (update_flags) {
    nzcv().SetN(CalcNFlag(result, reg_size));
    nzcv().SetZ(CalcZFlag(result));
    nzcv().SetC(0);
    nzcv().SetV(0);
    LogSystemRegister(NZCV);
  }

  set_reg(reg_size, instr->Rd(), result, LogRegWrites, instr->RdMode());
}


void Simulator::VisitConditionalCompareRegister(const Instruction* instr) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  ConditionalCompareHelper(instr, reg(reg_size, instr->Rm()));
}


void Simulator::VisitConditionalCompareImmediate(const Instruction* instr) {
  ConditionalCompareHelper(instr, instr->ImmCondCmp());
}


void Simulator::ConditionalCompareHelper(const Instruction* instr,
                                         int64_t op2) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  int64_t op1 = reg(reg_size, instr->Rn());

  if (ConditionPassed(instr->Condition())) {
    
    
    if (instr->Mask(ConditionalCompareMask) == CCMP) {
      AddWithCarry(reg_size, true, op1, ~op2, 1);
    } else {
      VIXL_ASSERT(instr->Mask(ConditionalCompareMask) == CCMN);
      AddWithCarry(reg_size, true, op1, op2, 0);
    }
  } else {
    
    nzcv().SetFlags(instr->Nzcv());
    LogSystemRegister(NZCV);
  }
}


void Simulator::VisitLoadStoreUnsignedOffset(const Instruction* instr) {
  int offset = instr->ImmLSUnsigned() << instr->SizeLS();
  LoadStoreHelper(instr, offset, Offset);
}


void Simulator::VisitLoadStoreUnscaledOffset(const Instruction* instr) {
  LoadStoreHelper(instr, instr->ImmLS(), Offset);
}


void Simulator::VisitLoadStorePreIndex(const Instruction* instr) {
  LoadStoreHelper(instr, instr->ImmLS(), PreIndex);
}


void Simulator::VisitLoadStorePostIndex(const Instruction* instr) {
  LoadStoreHelper(instr, instr->ImmLS(), PostIndex);
}


void Simulator::VisitLoadStoreRegisterOffset(const Instruction* instr) {
  Extend ext = static_cast<Extend>(instr->ExtendMode());
  VIXL_ASSERT((ext == UXTW) || (ext == UXTX) || (ext == SXTW) || (ext == SXTX));
  unsigned shift_amount = instr->ImmShiftLS() * instr->SizeLS();

  int64_t offset = ExtendValue(kXRegSize, xreg(instr->Rm()), ext,
                               shift_amount);
  LoadStoreHelper(instr, offset, Offset);
}


void Simulator::LoadStoreHelper(const Instruction* instr,
                                int64_t offset,
                                AddrMode addrmode) {
  unsigned srcdst = instr->Rt();
  uintptr_t address = AddressModeHelper(instr->Rn(), offset, addrmode);

  LoadStoreOp op = static_cast<LoadStoreOp>(instr->Mask(LoadStoreOpMask));
  switch (op) {
    
    
    case LDRB_w:
      set_wreg(srcdst, MemoryRead<uint8_t>(address), NoRegLog);
      break;
    case LDRH_w:
      set_wreg(srcdst, MemoryRead<uint16_t>(address), NoRegLog);
      break;
    case LDR_w:
      set_wreg(srcdst, MemoryRead<uint32_t>(address), NoRegLog);
      break;
    case LDR_x:
      set_xreg(srcdst, MemoryRead<uint64_t>(address), NoRegLog);
      break;
    case LDRSB_w:
      set_wreg(srcdst, MemoryRead<int8_t>(address), NoRegLog);
      break;
    case LDRSH_w:
      set_wreg(srcdst, MemoryRead<int16_t>(address), NoRegLog);
      break;
    case LDRSB_x:
      set_xreg(srcdst, MemoryRead<int8_t>(address), NoRegLog);
      break;
    case LDRSH_x:
      set_xreg(srcdst, MemoryRead<int16_t>(address), NoRegLog);
      break;
    case LDRSW_x:
      set_xreg(srcdst, MemoryRead<int32_t>(address), NoRegLog);
      break;
    case LDR_s:
      set_sreg(srcdst, MemoryRead<float>(address), NoRegLog);
      break;
    case LDR_d:
      set_dreg(srcdst, MemoryRead<double>(address), NoRegLog);
      break;

    case STRB_w:  MemoryWrite<uint8_t>(address, wreg(srcdst)); break;
    case STRH_w:  MemoryWrite<uint16_t>(address, wreg(srcdst)); break;
    case STR_w:   MemoryWrite<uint32_t>(address, wreg(srcdst)); break;
    case STR_x:   MemoryWrite<uint64_t>(address, xreg(srcdst)); break;
    case STR_s:   MemoryWrite<float>(address, sreg(srcdst)); break;
    case STR_d:   MemoryWrite<double>(address, dreg(srcdst)); break;

    
    case PRFM: break;

    default: VIXL_UNIMPLEMENTED();
  }

  size_t access_size = 1 << instr->SizeLS();
  if (instr->IsLoad()) {
    if ((op == LDR_s) || (op == LDR_d)) {
      LogReadFP(address, access_size, srcdst);
    } else {
      LogRead(address, access_size, srcdst);
    }
  } else {
    if ((op == STR_s) || (op == STR_d)) {
      LogWriteFP(address, access_size, srcdst);
    } else {
      LogWrite(address, access_size, srcdst);
    }
  }

  local_monitor_.MaybeClear();
}


void Simulator::VisitLoadStorePairOffset(const Instruction* instr) {
  LoadStorePairHelper(instr, Offset);
}


void Simulator::VisitLoadStorePairPreIndex(const Instruction* instr) {
  LoadStorePairHelper(instr, PreIndex);
}


void Simulator::VisitLoadStorePairPostIndex(const Instruction* instr) {
  LoadStorePairHelper(instr, PostIndex);
}


void Simulator::VisitLoadStorePairNonTemporal(const Instruction* instr) {
  LoadStorePairHelper(instr, Offset);
}


void Simulator::LoadStorePairHelper(const Instruction* instr,
                                    AddrMode addrmode) {
  unsigned rt = instr->Rt();
  unsigned rt2 = instr->Rt2();
  size_t access_size = 1 << instr->SizeLSPair();
  int64_t offset = instr->ImmLSPair() * access_size;
  uintptr_t address = AddressModeHelper(instr->Rn(), offset, addrmode);
  uintptr_t address2 = address + access_size;

  LoadStorePairOp op =
    static_cast<LoadStorePairOp>(instr->Mask(LoadStorePairMask));

  
  VIXL_ASSERT(((op & LoadStorePairLBit) == 0) || (rt != rt2));

  switch (op) {
    
    
    case LDP_w: {
      VIXL_ASSERT(access_size == kWRegSizeInBytes);
      set_wreg(rt, MemoryRead<uint32_t>(address), NoRegLog);
      set_wreg(rt2, MemoryRead<uint32_t>(address2), NoRegLog);
      break;
    }
    case LDP_s: {
      VIXL_ASSERT(access_size == kSRegSizeInBytes);
      set_sreg(rt, MemoryRead<float>(address), NoRegLog);
      set_sreg(rt2, MemoryRead<float>(address2), NoRegLog);
      break;
    }
    case LDP_x: {
      VIXL_ASSERT(access_size == kXRegSizeInBytes);
      set_xreg(rt, MemoryRead<uint64_t>(address), NoRegLog);
      set_xreg(rt2, MemoryRead<uint64_t>(address2), NoRegLog);
      break;
    }
    case LDP_d: {
      VIXL_ASSERT(access_size == kDRegSizeInBytes);
      set_dreg(rt, MemoryRead<double>(address), NoRegLog);
      set_dreg(rt2, MemoryRead<double>(address2), NoRegLog);
      break;
    }
    case LDPSW_x: {
      VIXL_ASSERT(access_size == kWRegSizeInBytes);
      set_xreg(rt, MemoryRead<int32_t>(address), NoRegLog);
      set_xreg(rt2, MemoryRead<int32_t>(address2), NoRegLog);
      break;
    }
    case STP_w: {
      VIXL_ASSERT(access_size == kWRegSizeInBytes);
      MemoryWrite<uint32_t>(address, wreg(rt));
      MemoryWrite<uint32_t>(address2, wreg(rt2));
      break;
    }
    case STP_s: {
      VIXL_ASSERT(access_size == kSRegSizeInBytes);
      MemoryWrite<float>(address, sreg(rt));
      MemoryWrite<float>(address2, sreg(rt2));
      break;
    }
    case STP_x: {
      VIXL_ASSERT(access_size == kXRegSizeInBytes);
      MemoryWrite<uint64_t>(address, xreg(rt));
      MemoryWrite<uint64_t>(address2, xreg(rt2));
      break;
    }
    case STP_d: {
      VIXL_ASSERT(access_size == kDRegSizeInBytes);
      MemoryWrite<double>(address, dreg(rt));
      MemoryWrite<double>(address2, dreg(rt2));
      break;
    }
    default: VIXL_UNREACHABLE();
  }

  
  
  if (instr->IsLoad()) {
    if ((op == LDP_s) || (op == LDP_d)) {
      LogReadFP(address, access_size, rt);
      LogReadFP(address2, access_size, rt2);
    } else {
      LogRead(address, access_size, rt);
      LogRead(address2, access_size, rt2);
    }
  } else {
    if ((op == STP_s) || (op == STP_d)) {
      LogWriteFP(address, access_size, rt);
      LogWriteFP(address2, access_size, rt2);
    } else {
      LogWrite(address, access_size, rt);
      LogWrite(address2, access_size, rt2);
    }
  }

  local_monitor_.MaybeClear();
}


void Simulator::PrintExclusiveAccessWarning() {
  if (print_exclusive_access_warning_) {
    fprintf(
        stderr,
        "%sWARNING:%s VIXL simulator support for load-/store-/clear-exclusive "
        "instructions is limited. Refer to the README for details.%s\n",
        clr_warning, clr_warning_message, clr_normal);
    print_exclusive_access_warning_ = false;
  }
}


void Simulator::VisitLoadStoreExclusive(const Instruction* instr) {
  PrintExclusiveAccessWarning();

  unsigned rs = instr->Rs();
  unsigned rt = instr->Rt();
  unsigned rt2 = instr->Rt2();
  unsigned rn = instr->Rn();

  LoadStoreExclusive op =
      static_cast<LoadStoreExclusive>(instr->Mask(LoadStoreExclusiveMask));

  bool is_acquire_release = instr->LdStXAcquireRelease();
  bool is_exclusive = !instr->LdStXNotExclusive();
  bool is_load = instr->LdStXLoad();
  bool is_pair = instr->LdStXPair();

  size_t element_size = 1 << instr->LdStXSizeLog2();
  size_t access_size = is_pair ? element_size * 2 : element_size;
  uint64_t address = reg<uint64_t>(rn, Reg31IsStackPointer);

  
  VIXL_ASSERT(address == static_cast<uintptr_t>(address));

  
  if (AlignDown(address, access_size) != address) {
    VIXL_ALIGNMENT_EXCEPTION();
  }

  
  if ((rn == 31) && (AlignDown(address, 16) != address)) {
    VIXL_ALIGNMENT_EXCEPTION();
  }

  if (is_load) {
    if (is_exclusive) {
      local_monitor_.MarkExclusive(address, access_size);
    } else {
      
      
      local_monitor_.Clear();
    }

    
    
    switch (op) {
      case LDXRB_w:
      case LDAXRB_w:
      case LDARB_w:
        set_wreg(rt, MemoryRead<uint8_t>(address), NoRegLog);
        break;
      case LDXRH_w:
      case LDAXRH_w:
      case LDARH_w:
        set_wreg(rt, MemoryRead<uint16_t>(address), NoRegLog);
        break;
      case LDXR_w:
      case LDAXR_w:
      case LDAR_w:
        set_wreg(rt, MemoryRead<uint32_t>(address), NoRegLog);
        break;
      case LDXR_x:
      case LDAXR_x:
      case LDAR_x:
        set_xreg(rt, MemoryRead<uint64_t>(address), NoRegLog);
        break;
      case LDXP_w:
      case LDAXP_w:
        set_wreg(rt, MemoryRead<uint32_t>(address), NoRegLog);
        set_wreg(rt2, MemoryRead<uint32_t>(address + element_size), NoRegLog);
        break;
      case LDXP_x:
      case LDAXP_x:
        set_xreg(rt, MemoryRead<uint64_t>(address), NoRegLog);
        set_xreg(rt2, MemoryRead<uint64_t>(address + element_size), NoRegLog);
        break;
      default:
        VIXL_UNREACHABLE();
    }

    if (is_acquire_release) {
      
      __sync_synchronize();
    }

    LogRead(address, access_size, rt);
    if (is_pair) {
      LogRead(address + element_size, access_size, rt2);
    }
  } else {
    if (is_acquire_release) {
      
      __sync_synchronize();
    }

    bool do_store = true;
    if (is_exclusive) {
      do_store = local_monitor_.IsExclusive(address, access_size) &&
                 global_monitor_.IsExclusive(address, access_size);
      set_wreg(rs, do_store ? 0 : 1);

      
      local_monitor_.Clear();
    } else {
      
      local_monitor_.MaybeClear();
    }

    if (do_store) {
      switch (op) {
        case STXRB_w:
        case STLXRB_w:
        case STLRB_w:
          MemoryWrite<uint8_t>(address, wreg(rt));
          break;
        case STXRH_w:
        case STLXRH_w:
        case STLRH_w:
          MemoryWrite<uint16_t>(address, wreg(rt));
          break;
        case STXR_w:
        case STLXR_w:
        case STLR_w:
          MemoryWrite<uint32_t>(address, wreg(rt));
          break;
        case STXR_x:
        case STLXR_x:
        case STLR_x:
          MemoryWrite<uint64_t>(address, xreg(rt));
          break;
        case STXP_w:
        case STLXP_w:
          MemoryWrite<uint32_t>(address, wreg(rt));
          MemoryWrite<uint32_t>(address + element_size, wreg(rt2));
          break;
        case STXP_x:
        case STLXP_x:
          MemoryWrite<uint64_t>(address, xreg(rt));
          MemoryWrite<uint64_t>(address + element_size, xreg(rt2));
          break;
        default:
          VIXL_UNREACHABLE();
      }

      LogWrite(address, element_size, rt);
      if (is_pair) {
        LogWrite(address + element_size, element_size, rt2);
      }
    }
  }
}


void Simulator::VisitLoadLiteral(const Instruction* instr) {
  unsigned rt = instr->Rt();
  uint64_t address = instr->LiteralAddress<uint64_t>();

  
  VIXL_ASSERT(address == static_cast<uintptr_t>(address));

  switch (instr->Mask(LoadLiteralMask)) {
    
    
    case LDR_w_lit:
      set_wreg(rt, MemoryRead<uint32_t>(address), NoRegLog);
      LogRead(address, kWRegSizeInBytes, rt);
      break;
    case LDR_x_lit:
      set_xreg(rt, MemoryRead<uint64_t>(address), NoRegLog);
      LogRead(address, kXRegSizeInBytes, rt);
      break;
    case LDR_s_lit:
      set_sreg(rt, MemoryRead<float>(address), NoRegLog);
      LogReadFP(address, kSRegSizeInBytes, rt);
      break;
    case LDR_d_lit:
      set_dreg(rt, MemoryRead<double>(address), NoRegLog);
      LogReadFP(address, kDRegSizeInBytes, rt);
      break;
    case LDRSW_x_lit:
      set_xreg(rt, MemoryRead<int32_t>(address), NoRegLog);
      LogRead(address, kWRegSizeInBytes, rt);
      break;

    
    case PRFM_lit: break;

    default: VIXL_UNREACHABLE();
  }

  local_monitor_.MaybeClear();
}


uintptr_t Simulator::AddressModeHelper(unsigned addr_reg,
                                       int64_t offset,
                                       AddrMode addrmode) {
  uint64_t address = xreg(addr_reg, Reg31IsStackPointer);

  if ((addr_reg == 31) && ((address % 16) != 0)) {
    
    
    
    VIXL_ALIGNMENT_EXCEPTION();
  }

  if ((addrmode == PreIndex) || (addrmode == PostIndex)) {
    VIXL_ASSERT(offset != 0);
    set_xreg(addr_reg, address + offset, LogRegWrites, Reg31IsStackPointer);
  }

  if ((addrmode == Offset) || (addrmode == PreIndex)) {
    address += offset;
  }

  
  VIXL_ASSERT(address == static_cast<uintptr_t>(address));

  return static_cast<uintptr_t>(address);
}


void Simulator::VisitMoveWideImmediate(const Instruction* instr) {
  MoveWideImmediateOp mov_op =
    static_cast<MoveWideImmediateOp>(instr->Mask(MoveWideImmediateMask));
  int64_t new_xn_val = 0;

  bool is_64_bits = instr->SixtyFourBits() == 1;
  
  VIXL_ASSERT(is_64_bits || (instr->ShiftMoveWide() < 2));

  
  int64_t shift = instr->ShiftMoveWide() * 16;
  int64_t shifted_imm16 = instr->ImmMoveWide() << shift;

  
  switch (mov_op) {
    case MOVN_w:
    case MOVN_x: {
        new_xn_val = ~shifted_imm16;
        if (!is_64_bits) new_xn_val &= kWRegMask;
      break;
    }
    case MOVK_w:
    case MOVK_x: {
        unsigned reg_code = instr->Rd();
        int64_t prev_xn_val = is_64_bits ? xreg(reg_code)
                                         : wreg(reg_code);
        new_xn_val =
            (prev_xn_val & ~(INT64_C(0xffff) << shift)) | shifted_imm16;
      break;
    }
    case MOVZ_w:
    case MOVZ_x: {
        new_xn_val = shifted_imm16;
      break;
    }
    default:
      VIXL_UNREACHABLE();
  }

  
  set_xreg(instr->Rd(), new_xn_val);
}


void Simulator::VisitConditionalSelect(const Instruction* instr) {
  uint64_t new_val = xreg(instr->Rn());

  if (ConditionFailed(static_cast<Condition>(instr->Condition()))) {
    new_val = xreg(instr->Rm());
    switch (instr->Mask(ConditionalSelectMask)) {
      case CSEL_w:
      case CSEL_x: break;
      case CSINC_w:
      case CSINC_x: new_val++; break;
      case CSINV_w:
      case CSINV_x: new_val = ~new_val; break;
      case CSNEG_w:
      case CSNEG_x: new_val = -new_val; break;
      default: VIXL_UNIMPLEMENTED();
    }
  }
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  set_reg(reg_size, instr->Rd(), new_val);
}


void Simulator::VisitDataProcessing1Source(const Instruction* instr) {
  unsigned dst = instr->Rd();
  unsigned src = instr->Rn();

  switch (instr->Mask(DataProcessing1SourceMask)) {
    case RBIT_w: set_wreg(dst, ReverseBits(wreg(src), kWRegSize)); break;
    case RBIT_x: set_xreg(dst, ReverseBits(xreg(src), kXRegSize)); break;
    case REV16_w: set_wreg(dst, ReverseBytes(wreg(src), Reverse16)); break;
    case REV16_x: set_xreg(dst, ReverseBytes(xreg(src), Reverse16)); break;
    case REV_w: set_wreg(dst, ReverseBytes(wreg(src), Reverse32)); break;
    case REV32_x: set_xreg(dst, ReverseBytes(xreg(src), Reverse32)); break;
    case REV_x: set_xreg(dst, ReverseBytes(xreg(src), Reverse64)); break;
    case CLZ_w: set_wreg(dst, CountLeadingZeros(wreg(src), kWRegSize)); break;
    case CLZ_x: set_xreg(dst, CountLeadingZeros(xreg(src), kXRegSize)); break;
    case CLS_w: {
      set_wreg(dst, CountLeadingSignBits(wreg(src), kWRegSize));
      break;
    }
    case CLS_x: {
      set_xreg(dst, CountLeadingSignBits(xreg(src), kXRegSize));
      break;
    }
    default: VIXL_UNIMPLEMENTED();
  }
}


uint64_t Simulator::ReverseBits(uint64_t value, unsigned num_bits) {
  VIXL_ASSERT((num_bits == kWRegSize) || (num_bits == kXRegSize));
  uint64_t result = 0;
  for (unsigned i = 0; i < num_bits; i++) {
    result = (result << 1) | (value & 1);
    value >>= 1;
  }
  return result;
}


uint64_t Simulator::ReverseBytes(uint64_t value, ReverseByteMode mode) {
  
  
  uint8_t bytes[8];
  uint64_t mask = 0xff00000000000000;
  for (int i = 7; i >= 0; i--) {
    bytes[i] = (value & mask) >> (i * 8);
    mask >>= 8;
  }

  
  
  
  
  VIXL_STATIC_ASSERT((Reverse16 == 0) && (Reverse32 == 1) && (Reverse64 == 2));
  static const uint8_t permute_table[3][8] = { {6, 7, 4, 5, 2, 3, 0, 1},
                                               {4, 5, 6, 7, 0, 1, 2, 3},
                                               {0, 1, 2, 3, 4, 5, 6, 7} };
  uint64_t result = 0;
  for (int i = 0; i < 8; i++) {
    result <<= 8;
    result |= bytes[permute_table[mode][i]];
  }
  return result;
}


void Simulator::VisitDataProcessing2Source(const Instruction* instr) {
  Shift shift_op = NO_SHIFT;
  int64_t result = 0;
  switch (instr->Mask(DataProcessing2SourceMask)) {
    case SDIV_w: {
      int32_t rn = wreg(instr->Rn());
      int32_t rm = wreg(instr->Rm());
      if ((rn == kWMinInt) && (rm == -1)) {
        result = kWMinInt;
      } else if (rm == 0) {
        
        result = 0;
      } else {
        result = rn / rm;
      }
      break;
    }
    case SDIV_x: {
      int64_t rn = xreg(instr->Rn());
      int64_t rm = xreg(instr->Rm());
      if ((rn == kXMinInt) && (rm == -1)) {
        result = kXMinInt;
      } else if (rm == 0) {
        
        result = 0;
      } else {
        result = rn / rm;
      }
      break;
    }
    case UDIV_w: {
      uint32_t rn = static_cast<uint32_t>(wreg(instr->Rn()));
      uint32_t rm = static_cast<uint32_t>(wreg(instr->Rm()));
      if (rm == 0) {
        
        result = 0;
      } else {
        result = rn / rm;
      }
      break;
    }
    case UDIV_x: {
      uint64_t rn = static_cast<uint64_t>(xreg(instr->Rn()));
      uint64_t rm = static_cast<uint64_t>(xreg(instr->Rm()));
      if (rm == 0) {
        
        result = 0;
      } else {
        result = rn / rm;
      }
      break;
    }
    case LSLV_w:
    case LSLV_x: shift_op = LSL; break;
    case LSRV_w:
    case LSRV_x: shift_op = LSR; break;
    case ASRV_w:
    case ASRV_x: shift_op = ASR; break;
    case RORV_w:
    case RORV_x: shift_op = ROR; break;
    default: VIXL_UNIMPLEMENTED();
  }

  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  if (shift_op != NO_SHIFT) {
    
    
    int mask = (instr->SixtyFourBits() == 1) ? 0x3f : 0x1f;
    unsigned shift = wreg(instr->Rm()) & mask;
    result = ShiftOperand(reg_size, reg(reg_size, instr->Rn()), shift_op,
                          shift);
  }
  set_reg(reg_size, instr->Rd(), result);
}





static int64_t MultiplyHighSigned(int64_t u, int64_t v) {
  uint64_t u0, v0, w0;
  int64_t u1, v1, w1, w2, t;

  u0 = u & 0xffffffff;
  u1 = u >> 32;
  v0 = v & 0xffffffff;
  v1 = v >> 32;

  w0 = u0 * v0;
  t = u1 * v0 + (w0 >> 32);
  w1 = t & 0xffffffff;
  w2 = t >> 32;
  w1 = u0 * v1 + w1;

  return u1 * v1 + w2 + (w1 >> 32);
}


void Simulator::VisitDataProcessing3Source(const Instruction* instr) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;

  int64_t result = 0;
  
  uint64_t rn_u32 = reg<uint32_t>(instr->Rn());
  uint64_t rm_u32 = reg<uint32_t>(instr->Rm());
  int64_t rn_s32 = reg<int32_t>(instr->Rn());
  int64_t rm_s32 = reg<int32_t>(instr->Rm());
  switch (instr->Mask(DataProcessing3SourceMask)) {
    case MADD_w:
    case MADD_x:
      result = xreg(instr->Ra()) + (xreg(instr->Rn()) * xreg(instr->Rm()));
      break;
    case MSUB_w:
    case MSUB_x:
      result = xreg(instr->Ra()) - (xreg(instr->Rn()) * xreg(instr->Rm()));
      break;
    case SMADDL_x: result = xreg(instr->Ra()) + (rn_s32 * rm_s32); break;
    case SMSUBL_x: result = xreg(instr->Ra()) - (rn_s32 * rm_s32); break;
    case UMADDL_x: result = xreg(instr->Ra()) + (rn_u32 * rm_u32); break;
    case UMSUBL_x: result = xreg(instr->Ra()) - (rn_u32 * rm_u32); break;
    case SMULH_x:
      result = MultiplyHighSigned(xreg(instr->Rn()), xreg(instr->Rm()));
      break;
    default: VIXL_UNIMPLEMENTED();
  }
  set_reg(reg_size, instr->Rd(), result);
}


void Simulator::VisitBitfield(const Instruction* instr) {
  unsigned reg_size = instr->SixtyFourBits() ? kXRegSize : kWRegSize;
  int64_t reg_mask = instr->SixtyFourBits() ? kXRegMask : kWRegMask;
  int64_t R = instr->ImmR();
  int64_t S = instr->ImmS();
  int64_t diff = S - R;
  int64_t mask;
  if (diff >= 0) {
    mask = (diff < (reg_size - 1)) ? (INT64_C(1) << (diff + 1)) - 1
                                   : reg_mask;
  } else {
    mask = (INT64_C(1) << (S + 1)) - 1;
    mask = (static_cast<uint64_t>(mask) >> R) | (mask << (reg_size - R));
    diff += reg_size;
  }

  
  
  
  bool inzero = false;
  bool extend = false;
  switch (instr->Mask(BitfieldMask)) {
    case BFM_x:
    case BFM_w:
      break;
    case SBFM_x:
    case SBFM_w:
      inzero = true;
      extend = true;
      break;
    case UBFM_x:
    case UBFM_w:
      inzero = true;
      break;
    default:
      VIXL_UNIMPLEMENTED();
  }

  int64_t dst = inzero ? 0 : reg(reg_size, instr->Rd());
  int64_t src = reg(reg_size, instr->Rn());
  
  int64_t result = (static_cast<uint64_t>(src) >> R) | (src << (reg_size - R));
  
  int64_t topbits = ((INT64_C(1) << (reg_size - diff - 1)) - 1) << (diff + 1);
  int64_t signbits = extend && ((src >> S) & 1) ? topbits : 0;

  
  result = signbits | (result & mask) | (dst & ~mask);

  set_reg(reg_size, instr->Rd(), result);
}


void Simulator::VisitExtract(const Instruction* instr) {
  unsigned lsb = instr->ImmS();
  unsigned reg_size = (instr->SixtyFourBits() == 1) ? kXRegSize
                                                    : kWRegSize;
  uint64_t low_res = static_cast<uint64_t>(reg(reg_size, instr->Rm())) >> lsb;
  uint64_t high_res =
      (lsb == 0) ? 0 : reg(reg_size, instr->Rn()) << (reg_size - lsb);
  set_reg(reg_size, instr->Rd(), low_res | high_res);
}


void Simulator::VisitFPImmediate(const Instruction* instr) {
  AssertSupportedFPCR();

  unsigned dest = instr->Rd();
  switch (instr->Mask(FPImmediateMask)) {
    case FMOV_s_imm: set_sreg(dest, instr->ImmFP32()); break;
    case FMOV_d_imm: set_dreg(dest, instr->ImmFP64()); break;
    default: VIXL_UNREACHABLE();
  }
}


void Simulator::VisitFPIntegerConvert(const Instruction* instr) {
  AssertSupportedFPCR();

  unsigned dst = instr->Rd();
  unsigned src = instr->Rn();

  FPRounding round = RMode();

  switch (instr->Mask(FPIntegerConvertMask)) {
    case FCVTAS_ws: set_wreg(dst, FPToInt32(sreg(src), FPTieAway)); break;
    case FCVTAS_xs: set_xreg(dst, FPToInt64(sreg(src), FPTieAway)); break;
    case FCVTAS_wd: set_wreg(dst, FPToInt32(dreg(src), FPTieAway)); break;
    case FCVTAS_xd: set_xreg(dst, FPToInt64(dreg(src), FPTieAway)); break;
    case FCVTAU_ws: set_wreg(dst, FPToUInt32(sreg(src), FPTieAway)); break;
    case FCVTAU_xs: set_xreg(dst, FPToUInt64(sreg(src), FPTieAway)); break;
    case FCVTAU_wd: set_wreg(dst, FPToUInt32(dreg(src), FPTieAway)); break;
    case FCVTAU_xd: set_xreg(dst, FPToUInt64(dreg(src), FPTieAway)); break;
    case FCVTMS_ws:
      set_wreg(dst, FPToInt32(sreg(src), FPNegativeInfinity));
      break;
    case FCVTMS_xs:
      set_xreg(dst, FPToInt64(sreg(src), FPNegativeInfinity));
      break;
    case FCVTMS_wd:
      set_wreg(dst, FPToInt32(dreg(src), FPNegativeInfinity));
      break;
    case FCVTMS_xd:
      set_xreg(dst, FPToInt64(dreg(src), FPNegativeInfinity));
      break;
    case FCVTMU_ws:
      set_wreg(dst, FPToUInt32(sreg(src), FPNegativeInfinity));
      break;
    case FCVTMU_xs:
      set_xreg(dst, FPToUInt64(sreg(src), FPNegativeInfinity));
      break;
    case FCVTMU_wd:
      set_wreg(dst, FPToUInt32(dreg(src), FPNegativeInfinity));
      break;
    case FCVTMU_xd:
      set_xreg(dst, FPToUInt64(dreg(src), FPNegativeInfinity));
      break;
    case FCVTNS_ws: set_wreg(dst, FPToInt32(sreg(src), FPTieEven)); break;
    case FCVTNS_xs: set_xreg(dst, FPToInt64(sreg(src), FPTieEven)); break;
    case FCVTNS_wd: set_wreg(dst, FPToInt32(dreg(src), FPTieEven)); break;
    case FCVTNS_xd: set_xreg(dst, FPToInt64(dreg(src), FPTieEven)); break;
    case FCVTNU_ws: set_wreg(dst, FPToUInt32(sreg(src), FPTieEven)); break;
    case FCVTNU_xs: set_xreg(dst, FPToUInt64(sreg(src), FPTieEven)); break;
    case FCVTNU_wd: set_wreg(dst, FPToUInt32(dreg(src), FPTieEven)); break;
    case FCVTNU_xd: set_xreg(dst, FPToUInt64(dreg(src), FPTieEven)); break;
    case FCVTZS_ws: set_wreg(dst, FPToInt32(sreg(src), FPZero)); break;
    case FCVTZS_xs: set_xreg(dst, FPToInt64(sreg(src), FPZero)); break;
    case FCVTZS_wd: set_wreg(dst, FPToInt32(dreg(src), FPZero)); break;
    case FCVTZS_xd: set_xreg(dst, FPToInt64(dreg(src), FPZero)); break;
    case FCVTZU_ws: set_wreg(dst, FPToUInt32(sreg(src), FPZero)); break;
    case FCVTZU_xs: set_xreg(dst, FPToUInt64(sreg(src), FPZero)); break;
    case FCVTZU_wd: set_wreg(dst, FPToUInt32(dreg(src), FPZero)); break;
    case FCVTZU_xd: set_xreg(dst, FPToUInt64(dreg(src), FPZero)); break;
    case FMOV_ws: set_wreg(dst, sreg_bits(src)); break;
    case FMOV_xd: set_xreg(dst, dreg_bits(src)); break;
    case FMOV_sw: set_sreg_bits(dst, wreg(src)); break;
    case FMOV_dx: set_dreg_bits(dst, xreg(src)); break;

    
    
    case SCVTF_dx: set_dreg(dst, FixedToDouble(xreg(src), 0, round)); break;
    case SCVTF_dw: set_dreg(dst, FixedToDouble(wreg(src), 0, round)); break;
    case UCVTF_dx: set_dreg(dst, UFixedToDouble(xreg(src), 0, round)); break;
    case UCVTF_dw: {
      set_dreg(dst, UFixedToDouble(static_cast<uint32_t>(wreg(src)), 0, round));
      break;
    }
    case SCVTF_sx: set_sreg(dst, FixedToFloat(xreg(src), 0, round)); break;
    case SCVTF_sw: set_sreg(dst, FixedToFloat(wreg(src), 0, round)); break;
    case UCVTF_sx: set_sreg(dst, UFixedToFloat(xreg(src), 0, round)); break;
    case UCVTF_sw: {
      set_sreg(dst, UFixedToFloat(static_cast<uint32_t>(wreg(src)), 0, round));
      break;
    }

    default: VIXL_UNREACHABLE();
  }
}


void Simulator::VisitFPFixedPointConvert(const Instruction* instr) {
  AssertSupportedFPCR();

  unsigned dst = instr->Rd();
  unsigned src = instr->Rn();
  int fbits = 64 - instr->FPScale();

  FPRounding round = RMode();

  switch (instr->Mask(FPFixedPointConvertMask)) {
    
    
    case SCVTF_dx_fixed:
      set_dreg(dst, FixedToDouble(xreg(src), fbits, round));
      break;
    case SCVTF_dw_fixed:
      set_dreg(dst, FixedToDouble(wreg(src), fbits, round));
      break;
    case UCVTF_dx_fixed:
      set_dreg(dst, UFixedToDouble(xreg(src), fbits, round));
      break;
    case UCVTF_dw_fixed: {
      set_dreg(dst,
               UFixedToDouble(static_cast<uint32_t>(wreg(src)), fbits, round));
      break;
    }
    case SCVTF_sx_fixed:
      set_sreg(dst, FixedToFloat(xreg(src), fbits, round));
      break;
    case SCVTF_sw_fixed:
      set_sreg(dst, FixedToFloat(wreg(src), fbits, round));
      break;
    case UCVTF_sx_fixed:
      set_sreg(dst, UFixedToFloat(xreg(src), fbits, round));
      break;
    case UCVTF_sw_fixed: {
      set_sreg(dst,
               UFixedToFloat(static_cast<uint32_t>(wreg(src)), fbits, round));
      break;
    }
    default: VIXL_UNREACHABLE();
  }
}


int32_t Simulator::FPToInt32(double value, FPRounding rmode) {
  value = FPRoundInt(value, rmode);
  if (value >= kWMaxInt) {
    return kWMaxInt;
  } else if (value < kWMinInt) {
    return kWMinInt;
  }
  return isnan(value) ? 0 : static_cast<int32_t>(value);
}


int64_t Simulator::FPToInt64(double value, FPRounding rmode) {
  value = FPRoundInt(value, rmode);
  if (value >= kXMaxInt) {
    return kXMaxInt;
  } else if (value < kXMinInt) {
    return kXMinInt;
  }
  return isnan(value) ? 0 : static_cast<int64_t>(value);
}


uint32_t Simulator::FPToUInt32(double value, FPRounding rmode) {
  value = FPRoundInt(value, rmode);
  if (value >= kWMaxUInt) {
    return kWMaxUInt;
  } else if (value < 0.0) {
    return 0;
  }
  return isnan(value) ? 0 : static_cast<uint32_t>(value);
}


uint64_t Simulator::FPToUInt64(double value, FPRounding rmode) {
  value = FPRoundInt(value, rmode);
  if (value >= kXMaxUInt) {
    return kXMaxUInt;
  } else if (value < 0.0) {
    return 0;
  }
  return isnan(value) ? 0 : static_cast<uint64_t>(value);
}


void Simulator::VisitFPCompare(const Instruction* instr) {
  AssertSupportedFPCR();

  switch (instr->Mask(FPCompareMask)) {
    case FCMP_s: FPCompare(sreg(instr->Rn()), sreg(instr->Rm())); break;
    case FCMP_d: FPCompare(dreg(instr->Rn()), dreg(instr->Rm())); break;
    case FCMP_s_zero: FPCompare(sreg(instr->Rn()), 0.0f); break;
    case FCMP_d_zero: FPCompare(dreg(instr->Rn()), 0.0); break;
    default: VIXL_UNIMPLEMENTED();
  }
}


void Simulator::VisitFPConditionalCompare(const Instruction* instr) {
  AssertSupportedFPCR();

  switch (instr->Mask(FPConditionalCompareMask)) {
    case FCCMP_s:
      if (ConditionPassed(instr->Condition())) {
        FPCompare(sreg(instr->Rn()), sreg(instr->Rm()));
      } else {
        nzcv().SetFlags(instr->Nzcv());
        LogSystemRegister(NZCV);
      }
      break;
    case FCCMP_d:
      if (ConditionPassed(instr->Condition())) {
        FPCompare(dreg(instr->Rn()), dreg(instr->Rm()));
      } else {
        nzcv().SetFlags(instr->Nzcv());
        LogSystemRegister(NZCV);
      }
      break;
    default: VIXL_UNIMPLEMENTED();
  }
}


void Simulator::VisitFPConditionalSelect(const Instruction* instr) {
  AssertSupportedFPCR();

  Instr selected;
  if (ConditionPassed(instr->Condition())) {
    selected = instr->Rn();
  } else {
    selected = instr->Rm();
  }

  switch (instr->Mask(FPConditionalSelectMask)) {
    case FCSEL_s: set_sreg(instr->Rd(), sreg(selected)); break;
    case FCSEL_d: set_dreg(instr->Rd(), dreg(selected)); break;
    default: VIXL_UNIMPLEMENTED();
  }
}


void Simulator::VisitFPDataProcessing1Source(const Instruction* instr) {
  AssertSupportedFPCR();
  const FPRounding fpcr_rounding = static_cast<FPRounding>(fpcr().RMode());

  unsigned fd = instr->Rd();
  unsigned fn = instr->Rn();

  switch (instr->Mask(FPDataProcessing1SourceMask)) {
    case FMOV_s: set_sreg(fd, sreg(fn)); break;
    case FMOV_d: set_dreg(fd, dreg(fn)); break;
    case FABS_s: set_sreg(fd, fabsf(sreg(fn))); break;
    case FABS_d: set_dreg(fd, fabs(dreg(fn))); break;
    case FNEG_s: set_sreg(fd, -sreg(fn)); break;
    case FNEG_d: set_dreg(fd, -dreg(fn)); break;
    case FSQRT_s: set_sreg(fd, FPSqrt(sreg(fn))); break;
    case FSQRT_d: set_dreg(fd, FPSqrt(dreg(fn))); break;
    case FRINTA_s: set_sreg(fd, FPRoundInt(sreg(fn), FPTieAway)); break;
    case FRINTA_d: set_dreg(fd, FPRoundInt(dreg(fn), FPTieAway)); break;
    case FRINTI_s: set_sreg(fd, FPRoundInt(sreg(fn), fpcr_rounding)); break;
    case FRINTI_d: set_dreg(fd, FPRoundInt(dreg(fn), fpcr_rounding)); break;
    case FRINTM_s:
        set_sreg(fd, FPRoundInt(sreg(fn), FPNegativeInfinity)); break;
    case FRINTM_d:
        set_dreg(fd, FPRoundInt(dreg(fn), FPNegativeInfinity)); break;
    case FRINTN_s: set_sreg(fd, FPRoundInt(sreg(fn), FPTieEven)); break;
    case FRINTN_d: set_dreg(fd, FPRoundInt(dreg(fn), FPTieEven)); break;
    case FRINTP_s:
        set_sreg(fd, FPRoundInt(sreg(fn), FPPositiveInfinity)); break;
    case FRINTP_d:
        set_dreg(fd, FPRoundInt(dreg(fn), FPPositiveInfinity)); break;
    case FRINTX_s: {
      float input = sreg(fn);
      float rounded = FPRoundInt(input, fpcr_rounding);
      set_sreg(fd, rounded);
      if (!isnan(input) && (input != rounded)) FPProcessException();
      break;
    }
    case FRINTX_d: {
      double input = dreg(fn);
      double rounded = FPRoundInt(input, fpcr_rounding);
      set_dreg(fd, rounded);
      if (!isnan(input) && (input != rounded)) FPProcessException();
      break;
    }
    case FRINTZ_s: set_sreg(fd, FPRoundInt(sreg(fn), FPZero)); break;
    case FRINTZ_d: set_dreg(fd, FPRoundInt(dreg(fn), FPZero)); break;
    case FCVT_ds: set_dreg(fd, FPToDouble(sreg(fn))); break;
    case FCVT_sd: set_sreg(fd, FPToFloat(dreg(fn), FPTieEven)); break;
    default: VIXL_UNIMPLEMENTED();
  }
}

















template <class T, int ebits, int mbits>
static T FPRound(int64_t sign, int64_t exponent, uint64_t mantissa,
                 FPRounding round_mode) {
  VIXL_ASSERT((sign == 0) || (sign == 1));

  
  VIXL_ASSERT(round_mode == FPTieEven);
  USE(round_mode);

  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  static const int mantissa_offset = 0;
  static const int exponent_offset = mantissa_offset + mbits;
  static const int sign_offset = exponent_offset + ebits;
  VIXL_ASSERT(sign_offset == (sizeof(T) * 8 - 1));

  
  if (mantissa == 0) {
    return sign << sign_offset;
  }

  
  
  static const int infinite_exponent = (1 << ebits) - 1;
  static const int max_normal_exponent = infinite_exponent - 1;

  
  
  exponent += max_normal_exponent >> 1;

  if (exponent > max_normal_exponent) {
    
    
    exponent = infinite_exponent;
    mantissa = 0;
    return (sign << sign_offset) |
           (exponent << exponent_offset) |
           (mantissa << mantissa_offset);
  }

  
  
  const int highest_significant_bit = 63 - CountLeadingZeros(mantissa, 64);
  int shift = highest_significant_bit - mbits;

  if (exponent <= 0) {
    

    
    
    
    shift += -exponent + 1;

    
    
    
    
    
    if (shift > (highest_significant_bit + 1)) {
      
      return sign << sign_offset;
    }

    
    exponent = 0;
  } else {
    
    
    mantissa &= ~(UINT64_C(1) << highest_significant_bit);
  }

  if (shift > 0) {
    
    
    uint64_t onebit_mantissa = (mantissa >> (shift)) & 1;
    uint64_t halfbit_mantissa = (mantissa >> (shift-1)) & 1;
    uint64_t adjusted = mantissa - (halfbit_mantissa & ~onebit_mantissa);
    T halfbit_adjusted = (adjusted >> (shift-1)) & 1;

    T result = (sign << sign_offset) |
               (exponent << exponent_offset) |
               ((mantissa >> shift) << mantissa_offset);

    
    
    
    
    
    
    
    
    return result + halfbit_adjusted;
  } else {
    
    
    
    return (sign << sign_offset) |
           (exponent << exponent_offset) |
           ((mantissa << -shift) << mantissa_offset);
  }
}



static inline double FPRoundToDouble(int64_t sign, int64_t exponent,
                                     uint64_t mantissa, FPRounding round_mode) {
  int64_t bits =
      FPRound<int64_t, kDoubleExponentBits, kDoubleMantissaBits>(sign,
                                                                 exponent,
                                                                 mantissa,
                                                                 round_mode);
  return rawbits_to_double(bits);
}



static inline float FPRoundToFloat(int64_t sign, int64_t exponent,
                                   uint64_t mantissa, FPRounding round_mode) {
  int32_t bits =
      FPRound<int32_t, kFloatExponentBits, kFloatMantissaBits>(sign,
                                                               exponent,
                                                               mantissa,
                                                               round_mode);
  return rawbits_to_float(bits);
}


double Simulator::FixedToDouble(int64_t src, int fbits, FPRounding round) {
  if (src >= 0) {
    return UFixedToDouble(src, fbits, round);
  } else {
    
    return -UFixedToDouble(-src, fbits, round);
  }
}


double Simulator::UFixedToDouble(uint64_t src, int fbits, FPRounding round) {
  
  
  if (src == 0) {
    return 0.0;
  }

  
  
  const int highest_significant_bit = 63 - CountLeadingZeros(src, 64);
  const int64_t exponent = highest_significant_bit - fbits;

  return FPRoundToDouble(0, exponent, src, round);
}


float Simulator::FixedToFloat(int64_t src, int fbits, FPRounding round) {
  if (src >= 0) {
    return UFixedToFloat(src, fbits, round);
  } else {
    
    return -UFixedToFloat(-src, fbits, round);
  }
}


float Simulator::UFixedToFloat(uint64_t src, int fbits, FPRounding round) {
  
  
  if (src == 0) {
    return 0.0f;
  }

  
  
  const int highest_significant_bit = 63 - CountLeadingZeros(src, 64);
  const int32_t exponent = highest_significant_bit - fbits;

  return FPRoundToFloat(0, exponent, src, round);
}


double Simulator::FPRoundInt(double value, FPRounding round_mode) {
  if ((value == 0.0) || (value == kFP64PositiveInfinity) ||
      (value == kFP64NegativeInfinity)) {
    return value;
  } else if (isnan(value)) {
    return FPProcessNaN(value);
  }

  double int_result = floor(value);
  double error = value - int_result;
  switch (round_mode) {
    case FPTieAway: {
      
      
      if ((-0.5 < value) && (value < 0.0)) {
        int_result = -0.0;

      } else if ((error > 0.5) || ((error == 0.5) && (int_result >= 0.0))) {
        
        
        int_result++;
      }
      break;
    }
    case FPTieEven: {
      
      
      if ((-0.5 <= value) && (value < 0.0)) {
        int_result = -0.0;

      
      
      } else if ((error > 0.5) ||
          ((error == 0.5) && (fmod(int_result, 2) != 0))) {
        int_result++;
      }
      break;
    }
    case FPZero: {
      
      
      if (value < 0) {
         int_result = ceil(value);
      }
      break;
    }
    case FPNegativeInfinity: {
      
      break;
    }
    case FPPositiveInfinity: {
      
      
      if ((-1.0 < value) && (value < 0.0)) {
        int_result = -0.0;

      
      } else if (error > 0.0) {
        int_result++;
      }
      break;
    }
    default: VIXL_UNIMPLEMENTED();
  }
  return int_result;
}


double Simulator::FPToDouble(float value) {
  switch (std::fpclassify(value)) {
    case FP_NAN: {
      if (DN()) return kFP64DefaultNaN;

      
      
      
      
      
      uint32_t raw = float_to_rawbits(value);

      uint64_t sign = raw >> 31;
      uint64_t exponent = (1 << 11) - 1;
      uint64_t payload = unsigned_bitextract_64(21, 0, raw);
      payload <<= (52 - 23);  
      payload |= (UINT64_C(1) << 51);  

      return rawbits_to_double((sign << 63) | (exponent << 52) | payload);
    }

    case FP_ZERO:
    case FP_NORMAL:
    case FP_SUBNORMAL:
    case FP_INFINITE: {
      
      
      
      return static_cast<double>(value);
    }
  }

  VIXL_UNREACHABLE();
  return static_cast<double>(value);
}


float Simulator::FPToFloat(double value, FPRounding round_mode) {
  
  VIXL_ASSERT(round_mode == FPTieEven);
  USE(round_mode);

  switch (std::fpclassify(value)) {
    case FP_NAN: {
      if (DN()) return kFP32DefaultNaN;

      
      
      
      
      uint64_t raw = double_to_rawbits(value);

      uint32_t sign = raw >> 63;
      uint32_t exponent = (1 << 8) - 1;
      uint32_t payload = unsigned_bitextract_64(50, 52 - 23, raw);
      payload |= (1 << 22);   

      return rawbits_to_float((sign << 31) | (exponent << 23) | payload);
    }

    case FP_ZERO:
    case FP_INFINITE: {
      
      
      return static_cast<float>(value);
    }

    case FP_NORMAL:
    case FP_SUBNORMAL: {
      
      
      uint64_t raw = double_to_rawbits(value);
      
      uint32_t sign = raw >> 63;
      
      int32_t exponent = unsigned_bitextract_64(62, 52, raw) - 1023;
      
      uint64_t mantissa = unsigned_bitextract_64(51, 0, raw);
      if (std::fpclassify(value) == FP_NORMAL) {
        mantissa |= (UINT64_C(1) << 52);
      }
      return FPRoundToFloat(sign, exponent, mantissa, round_mode);
    }
  }

  VIXL_UNREACHABLE();
  return value;
}


void Simulator::VisitFPDataProcessing2Source(const Instruction* instr) {
  AssertSupportedFPCR();

  unsigned fd = instr->Rd();
  unsigned fn = instr->Rn();
  unsigned fm = instr->Rm();

  
  switch (instr->Mask(FPDataProcessing2SourceMask)) {
    case FMAXNM_s: set_sreg(fd, FPMaxNM(sreg(fn), sreg(fm))); return;
    case FMAXNM_d: set_dreg(fd, FPMaxNM(dreg(fn), dreg(fm))); return;
    case FMINNM_s: set_sreg(fd, FPMinNM(sreg(fn), sreg(fm))); return;
    case FMINNM_d: set_dreg(fd, FPMinNM(dreg(fn), dreg(fm))); return;
    default:
      break;    
  }

  if (FPProcessNaNs(instr)) return;

  switch (instr->Mask(FPDataProcessing2SourceMask)) {
    case FADD_s: set_sreg(fd, FPAdd(sreg(fn), sreg(fm))); break;
    case FADD_d: set_dreg(fd, FPAdd(dreg(fn), dreg(fm))); break;
    case FSUB_s: set_sreg(fd, FPSub(sreg(fn), sreg(fm))); break;
    case FSUB_d: set_dreg(fd, FPSub(dreg(fn), dreg(fm))); break;
    case FMUL_s: set_sreg(fd, FPMul(sreg(fn), sreg(fm))); break;
    case FMUL_d: set_dreg(fd, FPMul(dreg(fn), dreg(fm))); break;
    case FDIV_s: set_sreg(fd, FPDiv(sreg(fn), sreg(fm))); break;
    case FDIV_d: set_dreg(fd, FPDiv(dreg(fn), dreg(fm))); break;
    case FMAX_s: set_sreg(fd, FPMax(sreg(fn), sreg(fm))); break;
    case FMAX_d: set_dreg(fd, FPMax(dreg(fn), dreg(fm))); break;
    case FMIN_s: set_sreg(fd, FPMin(sreg(fn), sreg(fm))); break;
    case FMIN_d: set_dreg(fd, FPMin(dreg(fn), dreg(fm))); break;
    case FMAXNM_s:
    case FMAXNM_d:
    case FMINNM_s:
    case FMINNM_d:
      
      VIXL_UNREACHABLE();
    default: VIXL_UNIMPLEMENTED();
  }
}


void Simulator::VisitFPDataProcessing3Source(const Instruction* instr) {
  AssertSupportedFPCR();

  unsigned fd = instr->Rd();
  unsigned fn = instr->Rn();
  unsigned fm = instr->Rm();
  unsigned fa = instr->Ra();

  switch (instr->Mask(FPDataProcessing3SourceMask)) {
    
    case FMADD_s: set_sreg(fd, FPMulAdd(sreg(fa), sreg(fn), sreg(fm))); break;
    case FMSUB_s: set_sreg(fd, FPMulAdd(sreg(fa), -sreg(fn), sreg(fm))); break;
    case FMADD_d: set_dreg(fd, FPMulAdd(dreg(fa), dreg(fn), dreg(fm))); break;
    case FMSUB_d: set_dreg(fd, FPMulAdd(dreg(fa), -dreg(fn), dreg(fm))); break;
    
    case FNMADD_s:
      set_sreg(fd, FPMulAdd(-sreg(fa), -sreg(fn), sreg(fm)));
      break;
    case FNMSUB_s:
      set_sreg(fd, FPMulAdd(-sreg(fa), sreg(fn), sreg(fm)));
      break;
    case FNMADD_d:
      set_dreg(fd, FPMulAdd(-dreg(fa), -dreg(fn), dreg(fm)));
      break;
    case FNMSUB_d:
      set_dreg(fd, FPMulAdd(-dreg(fa), dreg(fn), dreg(fm)));
      break;
    default: VIXL_UNIMPLEMENTED();
  }
}


template <typename T>
T Simulator::FPAdd(T op1, T op2) {
  
  VIXL_ASSERT(!isnan(op1) && !isnan(op2));

  if (isinf(op1) && isinf(op2) && (op1 != op2)) {
    
    FPProcessException();
    return FPDefaultNaN<T>();
  } else {
    
    return op1 + op2;
  }
}


template <typename T>
T Simulator::FPDiv(T op1, T op2) {
  
  VIXL_ASSERT(!isnan(op1) && !isnan(op2));

  if ((isinf(op1) && isinf(op2)) || ((op1 == 0.0) && (op2 == 0.0))) {
    
    FPProcessException();
    return FPDefaultNaN<T>();
  } else {
    if (op2 == 0.0) FPProcessException();

    
    return op1 / op2;
  }
}


template <typename T>
T Simulator::FPMax(T a, T b) {
  
  VIXL_ASSERT(!isnan(a) && !isnan(b));

  if ((a == 0.0) && (b == 0.0) &&
      (copysign(1.0, a) != copysign(1.0, b))) {
    
    return 0.0;
  } else {
    return (a > b) ? a : b;
  }
}


template <typename T>
T Simulator::FPMaxNM(T a, T b) {
  if (IsQuietNaN(a) && !IsQuietNaN(b)) {
    a = kFP64NegativeInfinity;
  } else if (!IsQuietNaN(a) && IsQuietNaN(b)) {
    b = kFP64NegativeInfinity;
  }

  T result = FPProcessNaNs(a, b);
  return isnan(result) ? result : FPMax(a, b);
}


template <typename T>
T Simulator::FPMin(T a, T b) {
  
  VIXL_ASSERT(!isnan(a) && !isnan(b));

  if ((a == 0.0) && (b == 0.0) &&
      (copysign(1.0, a) != copysign(1.0, b))) {
    
    return -0.0;
  } else {
    return (a < b) ? a : b;
  }
}


template <typename T>
T Simulator::FPMinNM(T a, T b) {
  if (IsQuietNaN(a) && !IsQuietNaN(b)) {
    a = kFP64PositiveInfinity;
  } else if (!IsQuietNaN(a) && IsQuietNaN(b)) {
    b = kFP64PositiveInfinity;
  }

  T result = FPProcessNaNs(a, b);
  return isnan(result) ? result : FPMin(a, b);
}


template <typename T>
T Simulator::FPMul(T op1, T op2) {
  
  VIXL_ASSERT(!isnan(op1) && !isnan(op2));

  if ((isinf(op1) && (op2 == 0.0)) || (isinf(op2) && (op1 == 0.0))) {
    
    FPProcessException();
    return FPDefaultNaN<T>();
  } else {
    
    return op1 * op2;
  }
}


template<typename T>
T Simulator::FPMulAdd(T a, T op1, T op2) {
  T result = FPProcessNaNs3(a, op1, op2);

  T sign_a = copysign(1.0, a);
  T sign_prod = copysign(1.0, op1) * copysign(1.0, op2);
  bool isinf_prod = isinf(op1) || isinf(op2);
  bool operation_generates_nan =
      (isinf(op1) && (op2 == 0.0)) ||                     
      (isinf(op2) && (op1 == 0.0)) ||                     
      (isinf(a) && isinf_prod && (sign_a != sign_prod));  

  if (isnan(result)) {
    
    if (operation_generates_nan && IsQuietNaN(a)) {
      FPProcessException();
      return FPDefaultNaN<T>();
    } else {
      return result;
    }
  }

  
  if (operation_generates_nan) {
    FPProcessException();
    return FPDefaultNaN<T>();
  }

  
  
  if (((op1 == 0.0) || (op2 == 0.0)) && (a == 0.0)) {
    return ((sign_a < 0) && (sign_prod < 0)) ? -0.0 : 0.0;
  }

  result = FusedMultiplyAdd(op1, op2, a);
  VIXL_ASSERT(!isnan(result));

  
  
  if ((a == 0.0) && (result == 0.0)) {
    return copysign(0.0, sign_prod);
  }

  return result;
}


template <typename T>
T Simulator::FPSub(T op1, T op2) {
  
  VIXL_ASSERT(!isnan(op1) && !isnan(op2));

  if (isinf(op1) && isinf(op2) && (op1 == op2)) {
    
    FPProcessException();
    return FPDefaultNaN<T>();
  } else {
    
    return op1 - op2;
  }
}


template <typename T>
T Simulator::FPSqrt(T op) {
  if (isnan(op)) {
    return FPProcessNaN(op);
  } else if (op < 0.0) {
    FPProcessException();
    return FPDefaultNaN<T>();
  } else {
    return sqrt(op);
  }
}


template <typename T>
T Simulator::FPProcessNaN(T op) {
  VIXL_ASSERT(isnan(op));
  if (IsSignallingNaN(op)) {
    FPProcessException();
  }
  return DN() ? FPDefaultNaN<T>() : ToQuietNaN(op);
}


template <typename T>
T Simulator::FPProcessNaNs(T op1, T op2) {
  if (IsSignallingNaN(op1)) {
    return FPProcessNaN(op1);
  } else if (IsSignallingNaN(op2)) {
    return FPProcessNaN(op2);
  } else if (isnan(op1)) {
    VIXL_ASSERT(IsQuietNaN(op1));
    return FPProcessNaN(op1);
  } else if (isnan(op2)) {
    VIXL_ASSERT(IsQuietNaN(op2));
    return FPProcessNaN(op2);
  } else {
    return 0.0;
  }
}


template <typename T>
T Simulator::FPProcessNaNs3(T op1, T op2, T op3) {
  if (IsSignallingNaN(op1)) {
    return FPProcessNaN(op1);
  } else if (IsSignallingNaN(op2)) {
    return FPProcessNaN(op2);
  } else if (IsSignallingNaN(op3)) {
    return FPProcessNaN(op3);
  } else if (isnan(op1)) {
    VIXL_ASSERT(IsQuietNaN(op1));
    return FPProcessNaN(op1);
  } else if (isnan(op2)) {
    VIXL_ASSERT(IsQuietNaN(op2));
    return FPProcessNaN(op2);
  } else if (isnan(op3)) {
    VIXL_ASSERT(IsQuietNaN(op3));
    return FPProcessNaN(op3);
  } else {
    return 0.0;
  }
}


bool Simulator::FPProcessNaNs(const Instruction* instr) {
  unsigned fd = instr->Rd();
  unsigned fn = instr->Rn();
  unsigned fm = instr->Rm();
  bool done = false;

  if (instr->Mask(FP64) == FP64) {
    double result = FPProcessNaNs(dreg(fn), dreg(fm));
    if (isnan(result)) {
      set_dreg(fd, result);
      done = true;
    }
  } else {
    float result = FPProcessNaNs(sreg(fn), sreg(fm));
    if (isnan(result)) {
      set_sreg(fd, result);
      done = true;
    }
  }

  return done;
}


void Simulator::VisitSystem(const Instruction* instr) {
  
  
  
  if (instr->Mask(SystemExclusiveMonitorFMask) == SystemExclusiveMonitorFixed) {
    VIXL_ASSERT(instr->Mask(SystemExclusiveMonitorMask) == CLREX);
    switch (instr->Mask(SystemExclusiveMonitorMask)) {
      case CLREX: {
        PrintExclusiveAccessWarning();
        ClearLocalMonitor();
        break;
      }
    }
  } else if (instr->Mask(SystemSysRegFMask) == SystemSysRegFixed) {
    switch (instr->Mask(SystemSysRegMask)) {
      case MRS: {
        switch (instr->ImmSystemRegister()) {
          case NZCV: set_xreg(instr->Rt(), nzcv().RawValue()); break;
          case FPCR: set_xreg(instr->Rt(), fpcr().RawValue()); break;
          default: VIXL_UNIMPLEMENTED();
        }
        break;
      }
      case MSR: {
        switch (instr->ImmSystemRegister()) {
          case NZCV:
            nzcv().SetRawValue(xreg(instr->Rt()));
            LogSystemRegister(NZCV);
            break;
          case FPCR:
            fpcr().SetRawValue(xreg(instr->Rt()));
            LogSystemRegister(FPCR);
            break;
          default: VIXL_UNIMPLEMENTED();
        }
        break;
      }
    }
  } else if (instr->Mask(SystemHintFMask) == SystemHintFixed) {
    VIXL_ASSERT(instr->Mask(SystemHintMask) == HINT);
    switch (instr->ImmHint()) {
      case NOP: break;
      default: VIXL_UNIMPLEMENTED();
    }
  } else if (instr->Mask(MemBarrierFMask) == MemBarrierFixed) {
    __sync_synchronize();
  } else {
    VIXL_UNIMPLEMENTED();
  }
}


void Simulator::DoUnreachable(const Instruction* instr) {
  VIXL_ASSERT((instr->Mask(ExceptionMask) == HLT) &&
              (instr->ImmException() == kUnreachableOpcode));

  fprintf(stream_, "Hit UNREACHABLE marker at pc=%p.\n",
          reinterpret_cast<const void*>(instr));
  abort();
}


void Simulator::DoTrace(const Instruction* instr) {
  VIXL_ASSERT((instr->Mask(ExceptionMask) == HLT) &&
              (instr->ImmException() == kTraceOpcode));

  
  uint32_t parameters;
  uint32_t command;

  VIXL_STATIC_ASSERT(sizeof(*instr) == 1);
  memcpy(&parameters, instr + kTraceParamsOffset, sizeof(parameters));
  memcpy(&command, instr + kTraceCommandOffset, sizeof(command));

  switch (command) {
    case TRACE_ENABLE:
      set_trace_parameters(trace_parameters() | parameters);
      break;
    case TRACE_DISABLE:
      set_trace_parameters(trace_parameters() & ~parameters);
      break;
    default:
      VIXL_UNREACHABLE();
  }

  set_pc(instr->InstructionAtOffset(kTraceLength));
}


void Simulator::DoLog(const Instruction* instr) {
  VIXL_ASSERT((instr->Mask(ExceptionMask) == HLT) &&
              (instr->ImmException() == kLogOpcode));

  
  uint32_t parameters;

  VIXL_STATIC_ASSERT(sizeof(*instr) == 1);
  memcpy(&parameters, instr + kTraceParamsOffset, sizeof(parameters));

  
  VIXL_ASSERT((parameters & LOG_DISASM) == 0);
  
  if (parameters & LOG_SYS_REGS) PrintSystemRegisters();
  if (parameters & LOG_REGS) PrintRegisters();
  if (parameters & LOG_FP_REGS) PrintFPRegisters();

  set_pc(instr->InstructionAtOffset(kLogLength));
}


void Simulator::DoPrintf(const Instruction* instr) {
  VIXL_ASSERT((instr->Mask(ExceptionMask) == HLT) &&
              (instr->ImmException() == kPrintfOpcode));

  
  uint32_t arg_count;
  uint32_t arg_pattern_list;
  VIXL_STATIC_ASSERT(sizeof(*instr) == 1);
  memcpy(&arg_count,
         instr + kPrintfArgCountOffset,
         sizeof(arg_count));
  memcpy(&arg_pattern_list,
         instr + kPrintfArgPatternListOffset,
         sizeof(arg_pattern_list));

  VIXL_ASSERT(arg_count <= kPrintfMaxArgCount);
  VIXL_ASSERT((arg_pattern_list >> (kPrintfArgPatternBits * arg_count)) == 0);

  
  
  
  
  

  
  
  
  const char * format_base = reg<const char *>(0);
  VIXL_ASSERT(format_base != NULL);
  size_t length = strlen(format_base) + 1;
  char * const format = new char[length + arg_count];

  
  const char * chunks[kPrintfMaxArgCount];

  
  uint32_t placeholder_count = 0;
  char * format_scratch = format;
  for (size_t i = 0; i < length; i++) {
    if (format_base[i] != '%') {
      *format_scratch++ = format_base[i];
    } else {
      if (format_base[i + 1] == '%') {
        
        *format_scratch++ = format_base[i];
        i++;
        
        
        if (placeholder_count > 0) *format_scratch++ = format_base[i];
      } else {
        VIXL_CHECK(placeholder_count < arg_count);
        
        *format_scratch++ = '\0';
        chunks[placeholder_count++] = format_scratch;
        *format_scratch++ = format_base[i];
      }
    }
  }
  VIXL_CHECK(placeholder_count == arg_count);

  
  
  
  

  printf("%s", clr_printf);

  
  
  int result = printf("%s", format);
  int pcs_r = 1;      
  int pcs_f = 0;      
  if (result >= 0) {
    for (uint32_t i = 0; i < placeholder_count; i++) {
      int part_result = -1;

      uint32_t arg_pattern = arg_pattern_list >> (i * kPrintfArgPatternBits);
      arg_pattern &= (1 << kPrintfArgPatternBits) - 1;
      switch (arg_pattern) {
        case kPrintfArgW: part_result = printf(chunks[i], wreg(pcs_r++)); break;
        case kPrintfArgX: part_result = printf(chunks[i], xreg(pcs_r++)); break;
        case kPrintfArgD: part_result = printf(chunks[i], dreg(pcs_f++)); break;
        default: VIXL_UNREACHABLE();
      }

      if (part_result < 0) {
        
        result = part_result;
        break;
      }

      result += part_result;
    }
  }

  printf("%s", clr_normal);

  
  set_xreg(0, result);

  
  set_pc(instr->InstructionAtOffset(kPrintfLength));

  
  set_lr(pc());

  delete[] format;
}

}  

#endif  
