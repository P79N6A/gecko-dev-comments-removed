



































#include <sstream>
#include <iomanip>

#include "common/dwarf_cfi_to_module.h"
#include "common/logging.h"

namespace google_breakpad {

using std::ostringstream;

vector<const UniqueString*> DwarfCFIToModule::RegisterNames::MakeVector(
    const char* const* strings,
    size_t size) {
  vector<const UniqueString*> names(size, NULL);
  for (size_t i = 0; i < size; ++i) {
    names[i] = ToUniqueString(strings[i]);
  }

  return names;
}

vector<const UniqueString*> DwarfCFIToModule::RegisterNames::I386() {
  static const char *const names[] = {
    "$eax", "$ecx", "$edx", "$ebx", "$esp", "$ebp", "$esi", "$edi",
    "$eip", "$eflags", "$unused1",
    "$st0", "$st1", "$st2", "$st3", "$st4", "$st5", "$st6", "$st7",
    "$unused2", "$unused3",
    "$xmm0", "$xmm1", "$xmm2", "$xmm3", "$xmm4", "$xmm5", "$xmm6", "$xmm7",
    "$mm0", "$mm1", "$mm2", "$mm3", "$mm4", "$mm5", "$mm6", "$mm7",
    "$fcw", "$fsw", "$mxcsr",
    "$es", "$cs", "$ss", "$ds", "$fs", "$gs", "$unused4", "$unused5",
    "$tr", "$ldtr"
  };

  return MakeVector(names, sizeof(names) / sizeof(names[0]));
}

vector<const UniqueString*> DwarfCFIToModule::RegisterNames::X86_64() {
  static const char *const names[] = {
    "$rax", "$rdx", "$rcx", "$rbx", "$rsi", "$rdi", "$rbp", "$rsp",
    "$r8",  "$r9",  "$r10", "$r11", "$r12", "$r13", "$r14", "$r15",
    "$rip",
    "$xmm0","$xmm1","$xmm2", "$xmm3", "$xmm4", "$xmm5", "$xmm6", "$xmm7",
    "$xmm8","$xmm9","$xmm10","$xmm11","$xmm12","$xmm13","$xmm14","$xmm15",
    "$st0", "$st1", "$st2", "$st3", "$st4", "$st5", "$st6", "$st7",
    "$mm0", "$mm1", "$mm2", "$mm3", "$mm4", "$mm5", "$mm6", "$mm7",
    "$rflags",
    "$es", "$cs", "$ss", "$ds", "$fs", "$gs", "$unused1", "$unused2",
    "$fs.base", "$gs.base", "$unused3", "$unused4",
    "$tr", "$ldtr",
    "$mxcsr", "$fcw", "$fsw"
  };

  return MakeVector(names, sizeof(names) / sizeof(names[0]));
}


vector<const UniqueString*> DwarfCFIToModule::RegisterNames::ARM() {
  static const char *const names[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "sp",  "lr",  "pc",
    "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
    "fps", "cpsr", "",   "",    "",    "",    "",    "",
    "",    "",    "",    "",    "",    "",    "",    "",
    "",    "",    "",    "",    "",    "",    "",    "",
    "",    "",    "",    "",    "",    "",    "",    "",
    "",    "",    "",    "",    "",    "",    "",    "",
    "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
    "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
    "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
    "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31",
    "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7"
  };

  return MakeVector(names, sizeof(names) / sizeof(names[0]));
}

bool DwarfCFIToModule::Entry(size_t offset, uint64 address, uint64 length,
                             uint8 version, const string &augmentation,
                             unsigned return_address) {
  assert(!entry_);

  
  
  

  
  entry_ = new Module::StackFrameEntry;
  entry_->address = address;
  entry_->size = length;
  entry_offset_ = offset;
  return_address_ = return_address;

  
  
  
  
  
  if (return_address_ < register_names_.size())
    entry_->initial_rules[ustr__ZDra()]
      = Module::Expr(register_names_[return_address_], 0, false);

  return true;
}

const UniqueString* DwarfCFIToModule::RegisterName(int i) {
  assert(entry_);
  if (i < 0) {
    assert(i == kCFARegister);
    return ustr__ZDcfa();
  }
  unsigned reg = i;
  if (reg == return_address_)
    return ustr__ZDra();

  
  if (reg < register_names_.size() && register_names_[reg] != ustr__empty())
    return register_names_[reg];

  reporter_->UnnamedRegister(entry_offset_, reg);
  char buf[30];
  sprintf(buf, "unnamed_register%u", reg);
  return ToUniqueString(buf);
}

void DwarfCFIToModule::Record(Module::Address address, int reg,
                              const Module::Expr &rule) {
  assert(entry_);

  
  if (address == entry_->address)
    entry_->initial_rules[RegisterName(reg)] = rule;
  
  else
    entry_->rule_changes[address][RegisterName(reg)] = rule;
}

bool DwarfCFIToModule::UndefinedRule(uint64 address, int reg) {
  reporter_->UndefinedNotSupported(entry_offset_, RegisterName(reg));
  
  return true;
}

bool DwarfCFIToModule::SameValueRule(uint64 address, int reg) {
  
  Module::Expr rule
    = Module::Expr(RegisterName(reg), 0, false);
  Record(address, reg, rule);
  return true;
}

bool DwarfCFIToModule::OffsetRule(uint64 address, int reg,
                                  int base_register, long offset) {
  
  Module::Expr rule
    = Module::Expr(RegisterName(base_register), offset, true);
  Record(address, reg, rule);
  return true;
}

bool DwarfCFIToModule::ValOffsetRule(uint64 address, int reg,
                                     int base_register, long offset) {
  
  Module::Expr rule
    = Module::Expr(RegisterName(base_register), offset, false);
  Record(address, reg, rule);
  return true;
}

bool DwarfCFIToModule::RegisterRule(uint64 address, int reg,
                                    int base_register) {
  
  Module::Expr rule
    = Module::Expr(RegisterName(base_register), 0, false);
  Record(address, reg, rule);
  return true;
}

bool DwarfCFIToModule::ExpressionRule(uint64 address, int reg,
                                      const string &expression) {
  reporter_->ExpressionsNotSupported(entry_offset_, RegisterName(reg));
  
  return true;
}

bool DwarfCFIToModule::ValExpressionRule(uint64 address, int reg,
                                         const string &expression) {
  reporter_->ExpressionsNotSupported(entry_offset_, RegisterName(reg));
  
  return true;
}

bool DwarfCFIToModule::End() {
  module_->AddStackFrameEntry(entry_);
  entry_ = NULL;
  return true;
}

void DwarfCFIToModule::Reporter::UnnamedRegister(size_t offset, int reg) {
  BPLOG(INFO) << file_ << ", section '" << section_ 
    << "': the call frame entry at offset 0x" 
    << std::setbase(16) << offset << std::setbase(10)
    << " refers to register " << reg << ", whose name we don't know";
}

void DwarfCFIToModule::Reporter::UndefinedNotSupported(
    size_t offset,
    const UniqueString* reg) {
  BPLOG(INFO) << file_ << ", section '" << section_ 
    << "': the call frame entry at offset 0x" 
    << std::setbase(16) << offset << std::setbase(10)
    << " sets the rule for register '" << FromUniqueString(reg)
    << "' to 'undefined', but the Breakpad symbol file format cannot "
    << " express this";
}

void DwarfCFIToModule::Reporter::ExpressionsNotSupported(
    size_t offset,
    const UniqueString* reg) {
  static uint64_t n_complaints = 0; 
  n_complaints++;
  if (!is_power_of_2(n_complaints))
    return;
  BPLOG(INFO) << file_ << ", section '" << section_ 
    << "': the call frame entry at offset 0x" 
    << std::setbase(16) << offset << std::setbase(10)
    << " uses a DWARF expression to describe how to recover register '"
    << FromUniqueString(reg) << "', but this translator cannot yet "
    << "translate DWARF expressions to Breakpad postfix expressions (shown "
    << n_complaints << " times)";
}

} 
