










































#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <map>
#include <stack>
#include <string>

#include "mozilla/Assertions.h"

#include "LulCommonExt.h"
#include "LulDwarfInt.h"


#define DEBUG_DWARF 0


namespace lul {

using std::string;

ByteReader::ByteReader(enum Endianness endian)
    :offset_reader_(NULL), address_reader_(NULL), endian_(endian),
     address_size_(0), offset_size_(0),
     have_section_base_(), have_text_base_(), have_data_base_(),
     have_function_base_() { }

ByteReader::~ByteReader() { }

void ByteReader::SetOffsetSize(uint8 size) {
  offset_size_ = size;
  MOZ_ASSERT(size == 4 || size == 8);
  if (size == 4) {
    this->offset_reader_ = &ByteReader::ReadFourBytes;
  } else {
    this->offset_reader_ = &ByteReader::ReadEightBytes;
  }
}

void ByteReader::SetAddressSize(uint8 size) {
  address_size_ = size;
  MOZ_ASSERT(size == 4 || size == 8);
  if (size == 4) {
    this->address_reader_ = &ByteReader::ReadFourBytes;
  } else {
    this->address_reader_ = &ByteReader::ReadEightBytes;
  }
}

uint64 ByteReader::ReadInitialLength(const char* start, size_t* len) {
  const uint64 initial_length = ReadFourBytes(start);
  start += 4;

  
  
  if (initial_length == 0xffffffff) {
    SetOffsetSize(8);
    *len = 12;
    return ReadOffset(start);
  } else {
    SetOffsetSize(4);
    *len = 4;
  }
  return initial_length;
}

bool ByteReader::ValidEncoding(DwarfPointerEncoding encoding) const {
  if (encoding == DW_EH_PE_omit) return true;
  if (encoding == DW_EH_PE_aligned) return true;
  if ((encoding & 0x7) > DW_EH_PE_udata8)
    return false;
  if ((encoding & 0x70) > DW_EH_PE_funcrel)
    return false;
  return true;
}

bool ByteReader::UsableEncoding(DwarfPointerEncoding encoding) const {
  switch (encoding & 0x70) {
    case DW_EH_PE_absptr:  return true;
    case DW_EH_PE_pcrel:   return have_section_base_;
    case DW_EH_PE_textrel: return have_text_base_;
    case DW_EH_PE_datarel: return have_data_base_;
    case DW_EH_PE_funcrel: return have_function_base_;
    default:               return false;
  }
}

uint64 ByteReader::ReadEncodedPointer(const char *buffer,
                                      DwarfPointerEncoding encoding,
                                      size_t *len) const {
  
  
  MOZ_ASSERT(encoding != DW_EH_PE_omit);

  
  
  
  
  if (encoding == DW_EH_PE_aligned) {
    MOZ_ASSERT(have_section_base_);

    
    
    
    
    
    

    
    
    uint64 skew = section_base_ & (AddressSize() - 1);
    
    uint64 offset = skew + (buffer - buffer_base_);
    
    uint64 aligned = (offset + AddressSize() - 1) & -AddressSize();
    
    const char *aligned_buffer = buffer_base_ + (aligned - skew);
    
    *len = aligned_buffer - buffer + AddressSize();
    return ReadAddress(aligned_buffer);
  }

  
  
  uint64 offset;
  switch (encoding & 0x0f) {
    case DW_EH_PE_absptr:
      
      
      
      
      
      
      
      
      
      offset = ReadAddress(buffer);
      *len = AddressSize();
      break;

    case DW_EH_PE_uleb128:
      offset = ReadUnsignedLEB128(buffer, len);
      break;

    case DW_EH_PE_udata2:
      offset = ReadTwoBytes(buffer);
      *len = 2;
      break;

    case DW_EH_PE_udata4:
      offset = ReadFourBytes(buffer);
      *len = 4;
      break;

    case DW_EH_PE_udata8:
      offset = ReadEightBytes(buffer);
      *len = 8;
      break;

    case DW_EH_PE_sleb128:
      offset = ReadSignedLEB128(buffer, len);
      break;

    case DW_EH_PE_sdata2:
      offset = ReadTwoBytes(buffer);
      
      offset = (offset ^ 0x8000) - 0x8000;
      *len = 2;
      break;

    case DW_EH_PE_sdata4:
      offset = ReadFourBytes(buffer);
      
      offset = (offset ^ 0x80000000ULL) - 0x80000000ULL;
      *len = 4;
      break;

    case DW_EH_PE_sdata8:
      
      offset = ReadEightBytes(buffer);
      *len = 8;
      break;

    default:
      abort();
  }

  
  uint64 base;
  switch (encoding & 0x70) {
    case DW_EH_PE_absptr:
      base = 0;
      break;

    case DW_EH_PE_pcrel:
      MOZ_ASSERT(have_section_base_);
      base = section_base_ + (buffer - buffer_base_);
      break;

    case DW_EH_PE_textrel:
      MOZ_ASSERT(have_text_base_);
      base = text_base_;
      break;

    case DW_EH_PE_datarel:
      MOZ_ASSERT(have_data_base_);
      base = data_base_;
      break;

    case DW_EH_PE_funcrel:
      MOZ_ASSERT(have_function_base_);
      base = function_base_;
      break;

    default:
      abort();
  }

  uint64 pointer = base + offset;

  
  if (AddressSize() == 4)
    pointer = pointer & 0xffffffff;
  else
    MOZ_ASSERT(AddressSize() == sizeof(uint64));

  return pointer;
}
















class CallFrameInfo::Rule {
 public:
  virtual ~Rule() { }

  
  
  
  
  virtual bool Handle(Handler *handler,
                      uint64 address, int register) const = 0;

  
  
  virtual bool operator==(const Rule &rhs) const = 0;

  bool operator!=(const Rule &rhs) const { return ! (*this == rhs); }

  
  virtual Rule *Copy() const = 0;

  
  
  virtual void SetBaseRegister(unsigned reg) { }

  
  
  virtual void SetOffset(long long offset) { }

  
  
  enum CFIRTag {
    CFIR_UNDEFINED_RULE,
    CFIR_SAME_VALUE_RULE,
    CFIR_OFFSET_RULE,
    CFIR_VAL_OFFSET_RULE,
    CFIR_REGISTER_RULE,
    CFIR_EXPRESSION_RULE,
    CFIR_VAL_EXPRESSION_RULE
  };

  
  virtual CFIRTag getTag() const = 0;
};


class CallFrameInfo::UndefinedRule: public CallFrameInfo::Rule {
 public:
  UndefinedRule() { }
  ~UndefinedRule() { }
  CFIRTag getTag() const { return CFIR_UNDEFINED_RULE; }
  bool Handle(Handler *handler, uint64 address, int reg) const {
    return handler->UndefinedRule(address, reg);
  }
  bool operator==(const Rule &rhs) const {
    if (rhs.getTag() != CFIR_UNDEFINED_RULE) return false;
    return true;
  }
  Rule *Copy() const { return new UndefinedRule(*this); }
};


class CallFrameInfo::SameValueRule: public CallFrameInfo::Rule {
 public:
  SameValueRule() { }
  ~SameValueRule() { }
  CFIRTag getTag() const { return CFIR_SAME_VALUE_RULE; }
  bool Handle(Handler *handler, uint64 address, int reg) const {
    return handler->SameValueRule(address, reg);
  }
  bool operator==(const Rule &rhs) const {
    if (rhs.getTag() != CFIR_SAME_VALUE_RULE) return false;
    return true;
  }
  Rule *Copy() const { return new SameValueRule(*this); }
};



class CallFrameInfo::OffsetRule: public CallFrameInfo::Rule {
 public:
  OffsetRule(int base_register, long offset)
      : base_register_(base_register), offset_(offset) { }
  ~OffsetRule() { }
  CFIRTag getTag() const { return CFIR_OFFSET_RULE; }
  bool Handle(Handler *handler, uint64 address, int reg) const {
    return handler->OffsetRule(address, reg, base_register_, offset_);
  }
  bool operator==(const Rule &rhs) const {
    if (rhs.getTag() != CFIR_OFFSET_RULE) return false;
    const OffsetRule *our_rhs = static_cast<const OffsetRule *>(&rhs);
    return (base_register_ == our_rhs->base_register_ &&
            offset_ == our_rhs->offset_);
  }
  Rule *Copy() const { return new OffsetRule(*this); }
  
  
  
  
 private:
  int base_register_;
  long offset_;
};




class CallFrameInfo::ValOffsetRule: public CallFrameInfo::Rule {
 public:
  ValOffsetRule(int base_register, long offset)
      : base_register_(base_register), offset_(offset) { }
  ~ValOffsetRule() { }
  CFIRTag getTag() const { return CFIR_VAL_OFFSET_RULE; }
  bool Handle(Handler *handler, uint64 address, int reg) const {
    return handler->ValOffsetRule(address, reg, base_register_, offset_);
  }
  bool operator==(const Rule &rhs) const {
    if (rhs.getTag() != CFIR_VAL_OFFSET_RULE) return false;
    const ValOffsetRule *our_rhs = static_cast<const ValOffsetRule *>(&rhs);
    return (base_register_ == our_rhs->base_register_ &&
            offset_ == our_rhs->offset_);
  }
  Rule *Copy() const { return new ValOffsetRule(*this); }
  void SetBaseRegister(unsigned reg) { base_register_ = reg; }
  void SetOffset(long long offset) { offset_ = offset; }
 private:
  int base_register_;
  long offset_;
};


class CallFrameInfo::RegisterRule: public CallFrameInfo::Rule {
 public:
  explicit RegisterRule(int register_number)
      : register_number_(register_number) { }
  ~RegisterRule() { }
  CFIRTag getTag() const { return CFIR_REGISTER_RULE; }
  bool Handle(Handler *handler, uint64 address, int reg) const {
    return handler->RegisterRule(address, reg, register_number_);
  }
  bool operator==(const Rule &rhs) const {
    if (rhs.getTag() != CFIR_REGISTER_RULE) return false;
    const RegisterRule *our_rhs = static_cast<const RegisterRule *>(&rhs);
    return (register_number_ == our_rhs->register_number_);
  }
  Rule *Copy() const { return new RegisterRule(*this); }
 private:
  int register_number_;
};


class CallFrameInfo::ExpressionRule: public CallFrameInfo::Rule {
 public:
  explicit ExpressionRule(const string &expression)
      : expression_(expression) { }
  ~ExpressionRule() { }
  CFIRTag getTag() const { return CFIR_EXPRESSION_RULE; }
  bool Handle(Handler *handler, uint64 address, int reg) const {
    return handler->ExpressionRule(address, reg, expression_);
  }
  bool operator==(const Rule &rhs) const {
    if (rhs.getTag() != CFIR_EXPRESSION_RULE) return false;
    const ExpressionRule *our_rhs = static_cast<const ExpressionRule *>(&rhs);
    return (expression_ == our_rhs->expression_);
  }
  Rule *Copy() const { return new ExpressionRule(*this); }
 private:
  string expression_;
};


class CallFrameInfo::ValExpressionRule: public CallFrameInfo::Rule {
 public:
  explicit ValExpressionRule(const string &expression)
      : expression_(expression) { }
  ~ValExpressionRule() { }
  CFIRTag getTag() const { return CFIR_VAL_EXPRESSION_RULE; }
  bool Handle(Handler *handler, uint64 address, int reg) const {
    return handler->ValExpressionRule(address, reg, expression_);
  }
  bool operator==(const Rule &rhs) const {
    if (rhs.getTag() != CFIR_VAL_EXPRESSION_RULE) return false;
    const ValExpressionRule *our_rhs =
        static_cast<const ValExpressionRule *>(&rhs);
    return (expression_ == our_rhs->expression_);
  }
  Rule *Copy() const { return new ValExpressionRule(*this); }
 private:
  string expression_;
};


class CallFrameInfo::RuleMap {
 public:
  RuleMap() : cfa_rule_(NULL) { }
  RuleMap(const RuleMap &rhs) : cfa_rule_(NULL) { *this = rhs; }
  ~RuleMap() { Clear(); }

  RuleMap &operator=(const RuleMap &rhs);

  
  void SetCFARule(Rule *rule) { delete cfa_rule_; cfa_rule_ = rule; }

  
  
  
  
  Rule *CFARule() const { return cfa_rule_; }

  
  
  Rule *RegisterRule(int reg) const;

  
  void SetRegisterRule(int reg, Rule *rule);

  
  
  
  
  bool HandleTransitionTo(Handler *handler, uint64 address,
                          const RuleMap &new_rules) const;

 private:
  
  typedef std::map<int, Rule *> RuleByNumber;

  
  void Clear();

  
  
  Rule *cfa_rule_;

  
  
  RuleByNumber registers_;
};

CallFrameInfo::RuleMap &CallFrameInfo::RuleMap::operator=(const RuleMap &rhs) {
  Clear();
  
  if (rhs.cfa_rule_) cfa_rule_ = rhs.cfa_rule_->Copy();
  for (RuleByNumber::const_iterator it = rhs.registers_.begin();
       it != rhs.registers_.end(); it++)
    registers_[it->first] = it->second->Copy();
  return *this;
}

CallFrameInfo::Rule *CallFrameInfo::RuleMap::RegisterRule(int reg) const {
  MOZ_ASSERT(reg != Handler::kCFARegister);
  RuleByNumber::const_iterator it = registers_.find(reg);
  if (it != registers_.end())
    return it->second->Copy();
  else
    return NULL;
}

void CallFrameInfo::RuleMap::SetRegisterRule(int reg, Rule *rule) {
  MOZ_ASSERT(reg != Handler::kCFARegister);
  MOZ_ASSERT(rule);
  Rule **slot = &registers_[reg];
  delete *slot;
  *slot = rule;
}

bool CallFrameInfo::RuleMap::HandleTransitionTo(
    Handler *handler,
    uint64 address,
    const RuleMap &new_rules) const {
  
  if (cfa_rule_ && new_rules.cfa_rule_) {
    if (*cfa_rule_ != *new_rules.cfa_rule_ &&
        !new_rules.cfa_rule_->Handle(handler, address, Handler::kCFARegister))
      return false;
  } else if (cfa_rule_) {
    
    
    
    
  } else if (new_rules.cfa_rule_) {
    
    
    MOZ_ASSERT(0);
  } else {
    
  }

  
  
  RuleByNumber::const_iterator old_it = registers_.begin();
  RuleByNumber::const_iterator new_it = new_rules.registers_.begin();
  while (old_it != registers_.end() && new_it != new_rules.registers_.end()) {
    if (old_it->first < new_it->first) {
      
      
      
      
      
      
      
      if (!handler->SameValueRule(address, old_it->first))
        return false;
      old_it++;
    } else if (old_it->first > new_it->first) {
      
      
      
      MOZ_ASSERT(0);
    } else {
      
      
      if (*old_it->second != *new_it->second &&
          !new_it->second->Handle(handler, address, new_it->first))
        return false;
      new_it++, old_it++;
    }
  }
  
  while (old_it != registers_.end()) {
    if (!handler->SameValueRule(address, old_it->first))
      return false;
    old_it++;
  }
  
  
  
  MOZ_ASSERT(new_it == new_rules.registers_.end());

  return true;
}


void CallFrameInfo::RuleMap::Clear() {
  delete cfa_rule_;
  cfa_rule_ = NULL;
  for (RuleByNumber::iterator it = registers_.begin();
       it != registers_.end(); it++)
    delete it->second;
  registers_.clear();
}



class CallFrameInfo::State {
 public:
  
  
  State(ByteReader *reader, Handler *handler, Reporter *reporter,
        uint64 address)
      : reader_(reader), handler_(handler), reporter_(reporter),
        address_(address), entry_(NULL), cursor_(NULL),
        saved_rules_(NULL) { }

  ~State() {
    if (saved_rules_)
      delete saved_rules_;
  }

  
  
  
  bool InterpretCIE(const CIE &cie);

  
  
  bool InterpretFDE(const FDE &fde);

 private:
  
  struct Operands {
    unsigned register_number;  
    uint64 offset;             
    long signed_offset;        
    string expression;         
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool ParseOperands(const char *format, Operands *operands);

  
  
  
  bool DoInstruction();

  
  
  

  
  
  
  bool DoDefCFA(unsigned base_register, long offset);

  
  
  
  bool DoDefCFAOffset(long offset);

  
  
  bool DoRule(unsigned reg, Rule *rule);

  
  
  
  bool DoOffset(unsigned reg, long offset);

  
  
  
  bool DoValOffset(unsigned reg, long offset);

  
  
  
  bool DoRestore(unsigned reg);

  
  
  uint64 CursorOffset() { return entry_->offset + (cursor_ - entry_->start); }

  
  bool ReportIncomplete() {
    reporter_->Incomplete(entry_->offset, entry_->kind);
    return false;
  }

  
  ByteReader *reader_;

  
  Handler *handler_;

  
  Reporter *reporter_;

  
  uint64 address_;

  
  
  const Entry *entry_;

  
  const char *cursor_;

  
  RuleMap rules_;

  
  
  
  RuleMap cie_rules_;

  
  
  std::stack<RuleMap>* saved_rules_;
};

bool CallFrameInfo::State::InterpretCIE(const CIE &cie) {
  entry_ = &cie;
  cursor_ = entry_->instructions;
  while (cursor_ < entry_->end)
    if (!DoInstruction())
      return false;
  
  
  cie_rules_ = rules_;
  return true;
}

bool CallFrameInfo::State::InterpretFDE(const FDE &fde) {
  entry_ = &fde;
  cursor_ = entry_->instructions;
  while (cursor_ < entry_->end)
    if (!DoInstruction())
      return false;
  return true;
}

bool CallFrameInfo::State::ParseOperands(const char *format,
                                         Operands *operands) {
  size_t len;
  const char *operand;

  for (operand = format; *operand; operand++) {
    size_t bytes_left = entry_->end - cursor_;
    switch (*operand) {
      case 'r':
        operands->register_number = reader_->ReadUnsignedLEB128(cursor_, &len);
        if (len > bytes_left) return ReportIncomplete();
        cursor_ += len;
        break;

      case 'o':
        operands->offset = reader_->ReadUnsignedLEB128(cursor_, &len);
        if (len > bytes_left) return ReportIncomplete();
        cursor_ += len;
        break;

      case 's':
        operands->signed_offset = reader_->ReadSignedLEB128(cursor_, &len);
        if (len > bytes_left) return ReportIncomplete();
        cursor_ += len;
        break;

      case 'a':
        operands->offset =
          reader_->ReadEncodedPointer(cursor_, entry_->cie->pointer_encoding,
                                      &len);
        if (len > bytes_left) return ReportIncomplete();
        cursor_ += len;
        break;

      case '1':
        if (1 > bytes_left) return ReportIncomplete();
        operands->offset = static_cast<unsigned char>(*cursor_++);
        break;

      case '2':
        if (2 > bytes_left) return ReportIncomplete();
        operands->offset = reader_->ReadTwoBytes(cursor_);
        cursor_ += 2;
        break;

      case '4':
        if (4 > bytes_left) return ReportIncomplete();
        operands->offset = reader_->ReadFourBytes(cursor_);
        cursor_ += 4;
        break;

      case '8':
        if (8 > bytes_left) return ReportIncomplete();
        operands->offset = reader_->ReadEightBytes(cursor_);
        cursor_ += 8;
        break;

      case 'e': {
        size_t expression_length = reader_->ReadUnsignedLEB128(cursor_, &len);
        if (len > bytes_left || expression_length > bytes_left - len)
          return ReportIncomplete();
        cursor_ += len;
        operands->expression = string(cursor_, expression_length);
        cursor_ += expression_length;
        break;
      }

      default:
        MOZ_ASSERT(0);
    }
  }

  return true;
}

bool CallFrameInfo::State::DoInstruction() {
  CIE *cie = entry_->cie;
  Operands ops;

  
  MOZ_ASSERT(entry_->kind != kUnknown);

  
  
  MOZ_ASSERT(cursor_ < entry_->end);

  unsigned opcode = *cursor_++;
  if ((opcode & 0xc0) != 0) {
    switch (opcode & 0xc0) {
      
      case DW_CFA_advance_loc: {
        size_t code_offset = opcode & 0x3f;
        address_ += code_offset * cie->code_alignment_factor;
        break;
      }

      
      case DW_CFA_offset:
        if (!ParseOperands("o", &ops) ||
            !DoOffset(opcode & 0x3f, ops.offset * cie->data_alignment_factor))
          return false;
        break;

      
      case DW_CFA_restore:
        if (!DoRestore(opcode & 0x3f)) return false;
        break;

      
      default:
        MOZ_ASSERT(0);
    }

    
    return true;
  }

  switch (opcode) {
    
    case DW_CFA_set_loc:
      if (!ParseOperands("a", &ops)) return false;
      address_ = ops.offset;
      break;

    
    case DW_CFA_advance_loc1:
      if (!ParseOperands("1", &ops)) return false;
      address_ += ops.offset * cie->code_alignment_factor;
      break;

    
    case DW_CFA_advance_loc2:
      if (!ParseOperands("2", &ops)) return false;
      address_ += ops.offset * cie->code_alignment_factor;
      break;

    
    case DW_CFA_advance_loc4:
      if (!ParseOperands("4", &ops)) return false;
      address_ += ops.offset * cie->code_alignment_factor;
      break;

    
    case DW_CFA_MIPS_advance_loc8:
      if (!ParseOperands("8", &ops)) return false;
      address_ += ops.offset * cie->code_alignment_factor;
      break;

    
    case DW_CFA_def_cfa:
      if (!ParseOperands("ro", &ops) ||
          !DoDefCFA(ops.register_number, ops.offset))
        return false;
      break;

    
    case DW_CFA_def_cfa_sf:
      if (!ParseOperands("rs", &ops) ||
          !DoDefCFA(ops.register_number,
                    ops.signed_offset * cie->data_alignment_factor))
        return false;
      break;

    
    case DW_CFA_def_cfa_register: {
      Rule *cfa_rule = rules_.CFARule();
      if (!cfa_rule) {
        reporter_->NoCFARule(entry_->offset, entry_->kind, CursorOffset());
        return false;
      }
      if (!ParseOperands("r", &ops)) return false;
      cfa_rule->SetBaseRegister(ops.register_number);
      if (!cfa_rule->Handle(handler_, address_, Handler::kCFARegister))
        return false;
      break;
    }

    
    case DW_CFA_def_cfa_offset:
      if (!ParseOperands("o", &ops) ||
          !DoDefCFAOffset(ops.offset))
        return false;
      break;

    
    case DW_CFA_def_cfa_offset_sf:
      if (!ParseOperands("s", &ops) ||
          !DoDefCFAOffset(ops.signed_offset * cie->data_alignment_factor))
        return false;
      break;

    
    case DW_CFA_def_cfa_expression: {
      if (!ParseOperands("e", &ops))
        return false;
      Rule *rule = new ValExpressionRule(ops.expression);
      rules_.SetCFARule(rule);
      if (!rule->Handle(handler_, address_, Handler::kCFARegister))
        return false;
      break;
    }

    
    case DW_CFA_undefined: {
      if (!ParseOperands("r", &ops) ||
          !DoRule(ops.register_number, new UndefinedRule()))
        return false;
      break;
    }

    
    case DW_CFA_same_value: {
      if (!ParseOperands("r", &ops) ||
          !DoRule(ops.register_number, new SameValueRule()))
        return false;
      break;
    }

    
    case DW_CFA_offset_extended:
      if (!ParseOperands("ro", &ops) ||
          !DoOffset(ops.register_number,
                    ops.offset * cie->data_alignment_factor))
        return false;
      break;

    
    case DW_CFA_offset_extended_sf:
      if (!ParseOperands("rs", &ops) ||
          !DoOffset(ops.register_number,
                    ops.signed_offset * cie->data_alignment_factor))
        return false;
      break;

    
    case DW_CFA_GNU_negative_offset_extended:
      if (!ParseOperands("ro", &ops) ||
          !DoOffset(ops.register_number,
                    -ops.offset * cie->data_alignment_factor))
        return false;
      break;

    
    case DW_CFA_val_offset:
      if (!ParseOperands("ro", &ops) ||
          !DoValOffset(ops.register_number,
                       ops.offset * cie->data_alignment_factor))
        return false;
      break;

    
    case DW_CFA_val_offset_sf:
      if (!ParseOperands("rs", &ops) ||
          !DoValOffset(ops.register_number,
                       ops.signed_offset * cie->data_alignment_factor))
        return false;
      break;

    
    case DW_CFA_register: {
      if (!ParseOperands("ro", &ops) ||
          !DoRule(ops.register_number, new RegisterRule(ops.offset)))
        return false;
      break;
    }

    
    case DW_CFA_expression: {
      if (!ParseOperands("re", &ops) ||
          !DoRule(ops.register_number, new ExpressionRule(ops.expression)))
        return false;
      break;
    }

    
    case DW_CFA_val_expression: {
      if (!ParseOperands("re", &ops) ||
          !DoRule(ops.register_number, new ValExpressionRule(ops.expression)))
        return false;
      break;
    }

    
    case DW_CFA_restore_extended:
      if (!ParseOperands("r", &ops) ||
          !DoRestore( ops.register_number))
        return false;
      break;

    
    case DW_CFA_remember_state:
      if (!saved_rules_) {
        saved_rules_ = new std::stack<RuleMap>();
      }
      saved_rules_->push(rules_);
      break;

    
    case DW_CFA_restore_state: {
      if (!saved_rules_ || saved_rules_->empty()) {
        reporter_->EmptyStateStack(entry_->offset, entry_->kind,
                                   CursorOffset());
        return false;
      }
      const RuleMap &new_rules = saved_rules_->top();
      if (rules_.CFARule() && !new_rules.CFARule()) {
        reporter_->ClearingCFARule(entry_->offset, entry_->kind,
                                   CursorOffset());
        return false;
      }
      rules_.HandleTransitionTo(handler_, address_, new_rules);
      rules_ = new_rules;
      saved_rules_->pop();
      break;
    }

    
    case DW_CFA_nop:
      break;

    
    
    
    
    
    case DW_CFA_GNU_window_save: {
      
      for (int i = 8; i < 16; i++)
        if (!DoRule(i, new RegisterRule(i + 16)))
          return false;
      
      for (int i = 16; i < 32; i++)
        
        
        if (!DoRule(i, new OffsetRule(Handler::kCFARegister,
                                      (i - 16) * reader_->AddressSize())))
          return false;
      break;
    }

    
    case DW_CFA_GNU_args_size:
      if (!ParseOperands("o", &ops)) return false;
      break;

    
    default: {
      reporter_->BadInstruction(entry_->offset, entry_->kind, CursorOffset());
      return false;
    }
  }

  return true;
}

bool CallFrameInfo::State::DoDefCFA(unsigned base_register, long offset) {
  Rule *rule = new ValOffsetRule(base_register, offset);
  rules_.SetCFARule(rule);
  return rule->Handle(handler_, address_, Handler::kCFARegister);
}

bool CallFrameInfo::State::DoDefCFAOffset(long offset) {
  Rule *cfa_rule = rules_.CFARule();
  if (!cfa_rule) {
    reporter_->NoCFARule(entry_->offset, entry_->kind, CursorOffset());
    return false;
  }
  cfa_rule->SetOffset(offset);
  return cfa_rule->Handle(handler_, address_, Handler::kCFARegister);
}

bool CallFrameInfo::State::DoRule(unsigned reg, Rule *rule) {
  rules_.SetRegisterRule(reg, rule);
  return rule->Handle(handler_, address_, reg);
}

bool CallFrameInfo::State::DoOffset(unsigned reg, long offset) {
  if (!rules_.CFARule()) {
    reporter_->NoCFARule(entry_->offset, entry_->kind, CursorOffset());
    return false;
  }
  return DoRule(reg,
                new OffsetRule(Handler::kCFARegister, offset));
}

bool CallFrameInfo::State::DoValOffset(unsigned reg, long offset) {
  if (!rules_.CFARule()) {
    reporter_->NoCFARule(entry_->offset, entry_->kind, CursorOffset());
    return false;
  }
  return DoRule(reg,
                new ValOffsetRule(Handler::kCFARegister, offset));
}

bool CallFrameInfo::State::DoRestore(unsigned reg) {
  
  if (entry_->kind == kCIE) {
    reporter_->RestoreInCIE(entry_->offset, CursorOffset());
    return false;
  }
  Rule *rule = cie_rules_.RegisterRule(reg);
  if (!rule) {
    
    
    
    
    rule = new SameValueRule();
  }
  return DoRule(reg, rule);
}

bool CallFrameInfo::ReadEntryPrologue(const char *cursor, Entry *entry) {
  const char *buffer_end = buffer_ + buffer_length_;

  
  entry->offset = cursor - buffer_;
  entry->start = cursor;
  entry->kind = kUnknown;
  entry->end = NULL;

  
  size_t length_size;
  uint64 length = reader_->ReadInitialLength(cursor, &length_size);
  if (length_size > size_t(buffer_end - cursor))
    return ReportIncomplete(entry);
  cursor += length_size;

  
  
  if (length == 0 && eh_frame_) {
    entry->kind = kTerminator;
    entry->end = cursor;
    return true;
  }

  
  if (length > size_t(buffer_end - cursor))
    return ReportIncomplete(entry);

  
  
  
  
  
  entry->end = cursor + length;

  
  size_t offset_size = reader_->OffsetSize();
  if (offset_size > size_t(entry->end - cursor)) return ReportIncomplete(entry);
  entry->id = reader_->ReadOffset(cursor);

  
  

  
  if (eh_frame_) {
    
    
    
    if (entry->id == 0) {
      entry->kind = kCIE;
    } else {
      entry->kind = kFDE;
      
      entry->id = (cursor - buffer_) - entry->id;
    }
  } else {
    
    
    
    if (offset_size == 4)
      entry->kind = (entry->id == 0xffffffff) ? kCIE : kFDE;
    else {
      MOZ_ASSERT(offset_size == 8);
      entry->kind = (entry->id == 0xffffffffffffffffULL) ? kCIE : kFDE;
    }
  }

  
   cursor += offset_size;

  
  entry->fields = cursor;

  entry->cie = NULL;

  return true;
}

bool CallFrameInfo::ReadCIEFields(CIE *cie) {
  const char *cursor = cie->fields;
  size_t len;

  MOZ_ASSERT(cie->kind == kCIE);

  
  cie->version = 0;
  cie->augmentation.clear();
  cie->code_alignment_factor = 0;
  cie->data_alignment_factor = 0;
  cie->return_address_register = 0;
  cie->has_z_augmentation = false;
  cie->pointer_encoding = DW_EH_PE_absptr;
  cie->instructions = 0;

  
  if (cie->end - cursor < 1)
    return ReportIncomplete(cie);
  cie->version = reader_->ReadOneByte(cursor);
  cursor++;

  
  
  
  
  
  if (cie->version < 1 || cie->version > 3) {
    reporter_->UnrecognizedVersion(cie->offset, cie->version);
    return false;
  }

  const char *augmentation_start = cursor;
  const void *augmentation_end =
      memchr(augmentation_start, '\0', cie->end - augmentation_start);
  if (! augmentation_end) return ReportIncomplete(cie);
  cursor = static_cast<const char *>(augmentation_end);
  cie->augmentation = string(augmentation_start,
                                  cursor - augmentation_start);
  
  cursor++;

  
  if (!cie->augmentation.empty()) {
    
    if (cie->augmentation[0] == DW_Z_augmentation_start) {
      
      cie->has_z_augmentation = true;
    } else {
      
      
      reporter_->UnrecognizedAugmentation(cie->offset, cie->augmentation);
      return false;
    }
  }

  
  cie->code_alignment_factor = reader_->ReadUnsignedLEB128(cursor, &len);
  if (size_t(cie->end - cursor) < len) return ReportIncomplete(cie);
  cursor += len;

  
  cie->data_alignment_factor = reader_->ReadSignedLEB128(cursor, &len);
  if (size_t(cie->end - cursor) < len) return ReportIncomplete(cie);
  cursor += len;

  
  
  if (cie->version == 1) {
    if (cursor >= cie->end) return ReportIncomplete(cie);
    cie->return_address_register = uint8(*cursor++);
  } else {
    cie->return_address_register = reader_->ReadUnsignedLEB128(cursor, &len);
    if (size_t(cie->end - cursor) < len) return ReportIncomplete(cie);
    cursor += len;
  }

  
  
  if (cie->has_z_augmentation) {
    uint64_t data_size = reader_->ReadUnsignedLEB128(cursor, &len);
    if (size_t(cie->end - cursor) < len + data_size)
      return ReportIncomplete(cie);
    cursor += len;
    const char *data = cursor;
    cursor += data_size;
    const char *data_end = cursor;

    cie->has_z_lsda = false;
    cie->has_z_personality = false;
    cie->has_z_signal_frame = false;

    
    
    for (size_t i = 1; i < cie->augmentation.size(); i++) {
      switch (cie->augmentation[i]) {
        case DW_Z_has_LSDA:
          
          
          
          cie->has_z_lsda = true;
          
          if (data >= data_end) return ReportIncomplete(cie);
          cie->lsda_encoding = DwarfPointerEncoding(*data++);
          if (!reader_->ValidEncoding(cie->lsda_encoding)) {
            reporter_->InvalidPointerEncoding(cie->offset, cie->lsda_encoding);
            return false;
          }
          
          
          
          
          break;

        case DW_Z_has_personality_routine:
          
          
          cie->has_z_personality = true;
          
          
          if (data >= data_end) return ReportIncomplete(cie);
          cie->personality_encoding = DwarfPointerEncoding(*data++);
          if (!reader_->ValidEncoding(cie->personality_encoding)) {
            reporter_->InvalidPointerEncoding(cie->offset,
                                              cie->personality_encoding);
            return false;
          }
          if (!reader_->UsableEncoding(cie->personality_encoding)) {
            reporter_->UnusablePointerEncoding(cie->offset,
                                               cie->personality_encoding);
            return false;
          }
          
          cie->personality_address =
            reader_->ReadEncodedPointer(data, cie->personality_encoding,
                                        &len);
          if (len > size_t(data_end - data))
            return ReportIncomplete(cie);
          data += len;
          break;

        case DW_Z_has_FDE_address_encoding:
          
          
          if (data >= data_end) return ReportIncomplete(cie);
          cie->pointer_encoding = DwarfPointerEncoding(*data++);
          if (!reader_->ValidEncoding(cie->pointer_encoding)) {
            reporter_->InvalidPointerEncoding(cie->offset,
                                              cie->pointer_encoding);
            return false;
          }
          if (!reader_->UsableEncoding(cie->pointer_encoding)) {
            reporter_->UnusablePointerEncoding(cie->offset,
                                               cie->pointer_encoding);
            return false;
          }
          break;

        case DW_Z_is_signal_trampoline:
          
          cie->has_z_signal_frame = true;
          break;

        default:
          
          reporter_->UnrecognizedAugmentation(cie->offset, cie->augmentation);
          return false;
      }
    }
  }

  
  cie->instructions = cursor;

  return true;
}

bool CallFrameInfo::ReadFDEFields(FDE *fde) {
  const char *cursor = fde->fields;
  size_t size;

  fde->address = reader_->ReadEncodedPointer(cursor, fde->cie->pointer_encoding,
                                             &size);
  if (size > size_t(fde->end - cursor))
    return ReportIncomplete(fde);
  cursor += size;
  reader_->SetFunctionBase(fde->address);

  
  
  DwarfPointerEncoding length_encoding =
    DwarfPointerEncoding(fde->cie->pointer_encoding & 0x0f);
  fde->size = reader_->ReadEncodedPointer(cursor, length_encoding, &size);
  if (size > size_t(fde->end - cursor))
    return ReportIncomplete(fde);
  cursor += size;

  
  
  if (fde->cie->has_z_augmentation) {
    uint64_t data_size = reader_->ReadUnsignedLEB128(cursor, &size);
    if (size_t(fde->end - cursor) < size + data_size)
      return ReportIncomplete(fde);
    cursor += size;

    
    
    
    
    
    
    
    
    
    
    if (fde->cie->has_z_lsda) {
      
      
      
      
      if (!reader_->UsableEncoding(fde->cie->lsda_encoding)) {
        reporter_->UnusablePointerEncoding(fde->cie->offset,
                                           fde->cie->lsda_encoding);
        return false;
      }

      fde->lsda_address =
        reader_->ReadEncodedPointer(cursor, fde->cie->lsda_encoding, &size);
      if (size > data_size)
        return ReportIncomplete(fde);
      
      
    }

    cursor += data_size;
  }

  
  fde->instructions = cursor;

  return true;
}

bool CallFrameInfo::Start() {
  const char *buffer_end = buffer_ + buffer_length_;
  const char *cursor;
  bool all_ok = true;
  const char *entry_end;
  bool ok;

  
  
  for (cursor = buffer_; cursor < buffer_end;
       cursor = entry_end, all_ok = all_ok && ok) {
    FDE fde;

    
    
    
    ok = false;

    
    if (!ReadEntryPrologue(cursor, &fde)) {
      if (!fde.end) {
        
        
        all_ok = false;
        break;
      }
      entry_end = fde.end;
      continue;
    }

    
    entry_end = fde.end;

    
    if (fde.kind == kTerminator) {
      
      
      
      if (fde.end < buffer_end) reporter_->EarlyEHTerminator(fde.offset);
      break;
    }

    
    
    
    
    if (fde.kind != kFDE) {
      ok = true;
      continue;
    }

    
    if (fde.id > buffer_length_) {
      reporter_->CIEPointerOutOfRange(fde.offset, fde.id);
      continue;
    }

    CIE cie;

    
    if (!ReadEntryPrologue(buffer_ + fde.id, &cie))
      continue;
    
    if (cie.kind != kCIE) {
      reporter_->BadCIEId(fde.offset, fde.id);
      continue;
    }
    if (!ReadCIEFields(&cie))
      continue;

    
    cie.cie = &cie;
    fde.cie = &cie;

    
    if (!ReadFDEFields(&fde))
      continue;

    
    if (!handler_->Entry(fde.offset, fde.address, fde.size,
                         cie.version, cie.augmentation,
                         cie.return_address_register)) {
      
      ok = true;
      continue;
    }

    if (cie.has_z_augmentation) {
      
      if (cie.has_z_personality) {
        if (!handler_
            ->PersonalityRoutine(cie.personality_address,
                                 IsIndirectEncoding(cie.personality_encoding)))
          continue;
      }

      
      if (cie.has_z_lsda) {
        if (!handler_
            ->LanguageSpecificDataArea(fde.lsda_address,
                                       IsIndirectEncoding(cie.lsda_encoding)))
          continue;
      }

      
      if (cie.has_z_signal_frame) {
        if (!handler_->SignalHandler())
          continue;
      }
    }

    
    State state(reader_, handler_, reporter_, fde.address);
    ok = state.InterpretCIE(cie) && state.InterpretFDE(fde);

    
    
    reader_->ClearFunctionBase();

    
    handler_->End();
  }

  return all_ok;
}

const char *CallFrameInfo::KindName(EntryKind kind) {
  if (kind == CallFrameInfo::kUnknown)
    return "entry";
  else if (kind == CallFrameInfo::kCIE)
    return "common information entry";
  else if (kind == CallFrameInfo::kFDE)
    return "frame description entry";
  else {
    MOZ_ASSERT (kind == CallFrameInfo::kTerminator);
    return ".eh_frame sequence terminator";
  }
}

bool CallFrameInfo::ReportIncomplete(Entry *entry) {
  reporter_->Incomplete(entry->offset, entry->kind);
  return false;
}

void CallFrameInfo::Reporter::Incomplete(uint64 offset,
                                         CallFrameInfo::EntryKind kind) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI %s at offset 0x%llx in '%s': entry ends early\n",
           filename_.c_str(), CallFrameInfo::KindName(kind), offset,
           section_.c_str());
  log_(buf);
}

void CallFrameInfo::Reporter::EarlyEHTerminator(uint64 offset) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI at offset 0x%llx in '%s': saw end-of-data marker"
           " before end of section contents\n",
           filename_.c_str(), offset, section_.c_str());
  log_(buf);
}

void CallFrameInfo::Reporter::CIEPointerOutOfRange(uint64 offset,
                                                   uint64 cie_offset) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI frame description entry at offset 0x%llx in '%s':"
           " CIE pointer is out of range: 0x%llx\n",
           filename_.c_str(), offset, section_.c_str(), cie_offset);
  log_(buf);
}

void CallFrameInfo::Reporter::BadCIEId(uint64 offset, uint64 cie_offset) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI frame description entry at offset 0x%llx in '%s':"
           " CIE pointer does not point to a CIE: 0x%llx\n",
           filename_.c_str(), offset, section_.c_str(), cie_offset);
  log_(buf);
}

void CallFrameInfo::Reporter::UnrecognizedVersion(uint64 offset, int version) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI frame description entry at offset 0x%llx in '%s':"
           " CIE specifies unrecognized version: %d\n",
           filename_.c_str(), offset, section_.c_str(), version);
  log_(buf);
}

void CallFrameInfo::Reporter::UnrecognizedAugmentation(uint64 offset,
                                                       const string &aug) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI frame description entry at offset 0x%llx in '%s':"
           " CIE specifies unrecognized augmentation: '%s'\n",
           filename_.c_str(), offset, section_.c_str(), aug.c_str());
  log_(buf);
}

void CallFrameInfo::Reporter::InvalidPointerEncoding(uint64 offset,
                                                     uint8 encoding) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI common information entry at offset 0x%llx in '%s':"
           " 'z' augmentation specifies invalid pointer encoding: 0x%02x\n",
           filename_.c_str(), offset, section_.c_str(), encoding);
  log_(buf);
}

void CallFrameInfo::Reporter::UnusablePointerEncoding(uint64 offset,
                                                      uint8 encoding) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI common information entry at offset 0x%llx in '%s':"
           " 'z' augmentation specifies a pointer encoding for which"
           " we have no base address: 0x%02x\n",
           filename_.c_str(), offset, section_.c_str(), encoding);
  log_(buf);
}

void CallFrameInfo::Reporter::RestoreInCIE(uint64 offset, uint64 insn_offset) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI common information entry at offset 0x%llx in '%s':"
           " the DW_CFA_restore instruction at offset 0x%llx"
           " cannot be used in a common information entry\n",
           filename_.c_str(), offset, section_.c_str(), insn_offset);
  log_(buf);
}

void CallFrameInfo::Reporter::BadInstruction(uint64 offset,
                                             CallFrameInfo::EntryKind kind,
                                             uint64 insn_offset) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI %s at offset 0x%llx in section '%s':"
           " the instruction at offset 0x%llx is unrecognized\n",
           filename_.c_str(), CallFrameInfo::KindName(kind),
           offset, section_.c_str(), insn_offset);
  log_(buf);
}

void CallFrameInfo::Reporter::NoCFARule(uint64 offset,
                                        CallFrameInfo::EntryKind kind,
                                        uint64 insn_offset) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI %s at offset 0x%llx in section '%s':"
           " the instruction at offset 0x%llx assumes that a CFA rule has"
           " been set, but none has been set\n",
           filename_.c_str(), CallFrameInfo::KindName(kind), offset,
           section_.c_str(), insn_offset);
  log_(buf);
}

void CallFrameInfo::Reporter::EmptyStateStack(uint64 offset,
                                              CallFrameInfo::EntryKind kind,
                                              uint64 insn_offset) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI %s at offset 0x%llx in section '%s':"
           " the DW_CFA_restore_state instruction at offset 0x%llx"
           " should pop a saved state from the stack, but the stack is empty\n",
           filename_.c_str(), CallFrameInfo::KindName(kind), offset,
           section_.c_str(), insn_offset);
  log_(buf);
}

void CallFrameInfo::Reporter::ClearingCFARule(uint64 offset,
                                              CallFrameInfo::EntryKind kind,
                                              uint64 insn_offset) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "%s: CFI %s at offset 0x%llx in section '%s':"
           " the DW_CFA_restore_state instruction at offset 0x%llx"
           " would clear the CFA rule in effect\n",
           filename_.c_str(), CallFrameInfo::KindName(kind), offset,
           section_.c_str(), insn_offset);
  log_(buf);
}


const unsigned int DwarfCFIToModule::RegisterNames::I386() {
  










  return 8 + 3 + 8 + 2 + 8 + 8 + 3 + 8 + 2;
}

const unsigned int DwarfCFIToModule::RegisterNames::X86_64() {
  













  return 8 + 8 + 1 + 8 + 8 + 8 + 8 + 1 + 8 + 4 + 2 + 3;
}


const unsigned int DwarfCFIToModule::RegisterNames::ARM() {
  














  return 13 * 8;
}

bool DwarfCFIToModule::Entry(size_t offset, uint64 address, uint64 length,
                             uint8 version, const string &augmentation,
                             unsigned return_address) {
  if (DEBUG_DWARF)
    printf("LUL.DW DwarfCFIToModule::Entry 0x%llx,+%lld\n", address, length);

  summ_->Entry(address, length);

  
  
  

  
  return_address_ = return_address;

  
  
  
  
  
  if (return_address_ < num_dw_regs_) {
    summ_->Rule(address, return_address_, return_address, 0, false);
  }

  return true;
}

const UniqueString* DwarfCFIToModule::RegisterName(int i) {
  if (i < 0) {
    MOZ_ASSERT(i == kCFARegister);
    return usu_->ToUniqueString(".cfa");
  }
  unsigned reg = i;
  if (reg == return_address_)
    return usu_->ToUniqueString(".ra");

  char buf[30];
  sprintf(buf, "dwarf_reg_%u", reg);
  return usu_->ToUniqueString(buf);
}

bool DwarfCFIToModule::UndefinedRule(uint64 address, int reg) {
  reporter_->UndefinedNotSupported(entry_offset_, RegisterName(reg));
  
  return true;
}

bool DwarfCFIToModule::SameValueRule(uint64 address, int reg) {
  if (DEBUG_DWARF)
    printf("LUL.DW  0x%llx: old r%d = Same\n", address, reg);
  
  summ_->Rule(address, reg, reg, 0, false);
  return true;
}

bool DwarfCFIToModule::OffsetRule(uint64 address, int reg,
                                  int base_register, long offset) {
  if (DEBUG_DWARF)
    printf("LUL.DW  0x%llx: old r%d = *(r%d + %ld)\n",
           address, reg, base_register, offset);
  
  summ_->Rule(address, reg, base_register, offset, true);
  return true;
}

bool DwarfCFIToModule::ValOffsetRule(uint64 address, int reg,
                                     int base_register, long offset) {
  if (DEBUG_DWARF)
    printf("LUL.DW  0x%llx: old r%d = r%d + %ld\n",
           address, reg, base_register, offset);
  
  summ_->Rule(address, reg, base_register, offset, false);
  return true;
}

bool DwarfCFIToModule::RegisterRule(uint64 address, int reg,
                                    int base_register) {
  if (DEBUG_DWARF)
    printf("LUL.DW  0x%llx: old r%d = r%d\n", address, reg, base_register);
  
  summ_->Rule(address, reg, base_register, 0, false);
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
  
  if (DEBUG_DWARF)
    printf("LUL.DW DwarfCFIToModule::End()\n");
  summ_->End();
  return true;
}

void DwarfCFIToModule::Reporter::UndefinedNotSupported(
    size_t offset,
    const UniqueString* reg) {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "DwarfCFIToModule::Reporter::UndefinedNotSupported()\n");
  log_(buf);
  
  
  
  
  
  
}


static bool is_power_of_2(uint64_t n)
{
  int i, nSetBits = 0;
  for (i = 0; i < 8*(int)sizeof(n); i++) {
    if ((n & ((uint64_t)1) << i) != 0)
      nSetBits++;
  }
  return nSetBits <= 1;
}

void DwarfCFIToModule::Reporter::ExpressionsNotSupported(
    size_t offset,
    const UniqueString* reg) {
  static uint64_t n_complaints = 0; 
  n_complaints++;
  if (!is_power_of_2(n_complaints))
    return;
  char buf[300];
  snprintf(buf, sizeof(buf),
           "DwarfCFIToModule::Reporter::"
           "ExpressionsNotSupported(shown %llu times)\n",
           (unsigned long long int)n_complaints);
  log_(buf);
  
  
  
  
  
  
  
}

} 
