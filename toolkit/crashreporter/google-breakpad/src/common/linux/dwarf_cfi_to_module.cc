



































#include <sstream>

#include "common/linux/dwarf_cfi_to_module.h"

namespace google_breakpad {

using std::ostringstream;

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
    entry_->initial_rules[".ra"] = register_names_[return_address_];

  return true;
}

string DwarfCFIToModule::RegisterName(int i) {
  assert(entry_);
  if (i < 0) {
    assert(i == kCFARegister);
    return ".cfa";
  }
  unsigned reg = i;
  if (reg == return_address_)
    return ".ra";

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
