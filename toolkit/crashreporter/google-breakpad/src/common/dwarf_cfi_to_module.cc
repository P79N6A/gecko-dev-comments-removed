



































#include <sstream>

#include "common/dwarf_cfi_to_module.h"

namespace google_breakpad {

using std::ostringstream;

vector<string> DwarfCFIToModule::RegisterNames::MakeVector(
    const char * const *strings,
    size_t size) {
  vector<string> names(strings, strings + size);
  return names;
}

vector<string> DwarfCFIToModule::RegisterNames::I386() {
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

vector<string> DwarfCFIToModule::RegisterNames::X86_64() {
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

vector<string> DwarfCFIToModule::RegisterNames::ARM() {
  static const char *const names[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "sp",  "lr",  "pc",
    "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
    "fps", "cpsr"
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
    entry_->initial_rules[ra_name_] = register_names_[return_address_];

  return true;
}

string DwarfCFIToModule::RegisterName(int i) {
  assert(entry_);
  if (i < 0) {
    assert(i == kCFARegister);
    return cfa_name_;
  }
  unsigned reg = i;
  if (reg == return_address_)
    return ra_name_;

  if (0 <= reg && reg < register_names_.size())
    return register_names_[reg];

  reporter_->UnnamedRegister(entry_offset_, reg);
  char buf[30];
  sprintf(buf, "unnamed_register%u", reg);
  return buf;
}

void DwarfCFIToModule::Record(Module::Address address, int reg,
                              const string &rule) {
  assert(entry_);

  
  
  
  
  
  
  string shared_rule = *common_strings_.insert(rule).first;

  
  if (address == entry_->address)
    entry_->initial_rules[RegisterName(reg)] = shared_rule;
  
  else
    entry_->rule_changes[address][RegisterName(reg)] = shared_rule;
}

bool DwarfCFIToModule::UndefinedRule(uint64 address, int reg) {
  reporter_->UndefinedNotSupported(entry_offset_, RegisterName(reg));
  
  return true;
}

bool DwarfCFIToModule::SameValueRule(uint64 address, int reg) {
  ostringstream s;
  s << RegisterName(reg);
  Record(address, reg, s.str());
  return true;
}

bool DwarfCFIToModule::OffsetRule(uint64 address, int reg,
                                  int base_register, long offset) {
  ostringstream s;
  s << RegisterName(base_register) << " " << offset << " + ^";
  Record(address, reg, s.str());
  return true;
}

bool DwarfCFIToModule::ValOffsetRule(uint64 address, int reg,
                                     int base_register, long offset) {
  ostringstream s;
  s << RegisterName(base_register) << " " << offset << " +";
  Record(address, reg, s.str());
  return true;
}

bool DwarfCFIToModule::RegisterRule(uint64 address, int reg,
                                    int base_register) {
  ostringstream s;
  s << RegisterName(base_register);
  Record(address, reg, s.str());
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
  fprintf(stderr, "%s, section '%s': "
          "the call frame entry at offset 0x%zx refers to register %d,"
          " whose name we don't know\n",
          file_.c_str(), section_.c_str(), offset, reg);
}

void DwarfCFIToModule::Reporter::UndefinedNotSupported(size_t offset,
                                                       const string &reg) {
  fprintf(stderr, "%s, section '%s': "
          "the call frame entry at offset 0x%zx sets the rule for "
          "register '%s' to 'undefined', but the Breakpad symbol file format"
          " cannot express this\n",
          file_.c_str(), section_.c_str(), offset, reg.c_str());
}

void DwarfCFIToModule::Reporter::ExpressionsNotSupported(size_t offset,
                                                         const string &reg) {
  fprintf(stderr, "%s, section '%s': "
          "the call frame entry at offset 0x%zx uses a DWARF expression to"
          " describe how to recover register '%s', "
          " but this translator cannot yet translate DWARF expressions to"
          " Breakpad postfix expressions\n",
          file_.c_str(), section_.c_str(), offset, reg.c_str());
}

} 
