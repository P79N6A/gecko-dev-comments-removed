

































#include "processor/cfi_frame_info.h"

#include <string.h>

#include <algorithm>
#include <sstream>

#include "common/scoped_ptr.h"
#include "processor/postfix_evaluator-inl.h"

namespace google_breakpad {

#ifdef _WIN32
#define strtok_r strtok_s
#endif

template<typename V>
bool CFIFrameInfo::FindCallerRegs(const RegisterValueMap<V> &registers,
                                  const MemoryRegion &memory,
                                  RegisterValueMap<V> *caller_registers) const {
  
  
  if (cfa_rule_.isExprInvalid() || ra_rule_.isExprInvalid())
    return false;

  RegisterValueMap<V> working;
  PostfixEvaluator<V> evaluator(&working, &memory);

  caller_registers->clear();

  
  V cfa;
  working = registers;
  if (!evaluator.EvaluateForValue(cfa_rule_, &cfa))
    return false;

  
  V ra;
  working = registers;
  working.set(ustr__ZDcfa(), cfa);
  if (!evaluator.EvaluateForValue(ra_rule_, &ra))
    return false;

  
  for (RuleMap::const_iterator it = register_rules_.begin();
       it != register_rules_.end(); it++) {
    V value;
    working = registers;
    working.set(ustr__ZDcfa(), cfa);
    if (!evaluator.EvaluateForValue(it->second, &value))
      return false;
    caller_registers->set(it->first, value);
  }

  caller_registers->set(ustr__ZDra(), ra);
  caller_registers->set(ustr__ZDcfa(), cfa);

  return true;
}


template bool CFIFrameInfo::FindCallerRegs<uint32_t>(
    const RegisterValueMap<uint32_t> &registers,
    const MemoryRegion &memory,
    RegisterValueMap<uint32_t> *caller_registers) const;
template bool CFIFrameInfo::FindCallerRegs<uint64_t>(
    const RegisterValueMap<uint64_t> &registers,
    const MemoryRegion &memory,
    RegisterValueMap<uint64_t> *caller_registers) const;

string CFIFrameInfo::Serialize() const {
  std::ostringstream stream;

  if (!cfa_rule_.isExprInvalid()) {
    stream << ".cfa: " << cfa_rule_;
  }
  if (!ra_rule_.isExprInvalid()) {
    if (static_cast<std::streamoff>(stream.tellp()) != 0)
      stream << " ";
    stream << ".ra: " << ra_rule_;
  }

  
  
  
  
  std::vector<const UniqueString*> rr_names;
  for (RuleMap::const_iterator iter = register_rules_.begin();
       iter != register_rules_.end();
       ++iter) {
    rr_names.push_back(iter->first);
  }

  std::sort(rr_names.begin(), rr_names.end(), LessThan_UniqueString);

  
  for (std::vector<const UniqueString*>::const_iterator name = rr_names.begin();
       name != rr_names.end();
       ++name) {
    const UniqueString* nm = *name;
    Module::Expr rule = register_rules_.find(nm)->second;
    if (static_cast<std::streamoff>(stream.tellp()) != 0)
      stream << " ";
    stream << FromUniqueString(nm) << ": " << rule;
  }

  return stream.str();
}

bool CFIRuleParser::Parse(const string &rule_set) {
  size_t rule_set_len = rule_set.size();
  scoped_array<char> working_copy(new char[rule_set_len + 1]);
  memcpy(working_copy.get(), rule_set.data(), rule_set_len);
  working_copy[rule_set_len] = '\0';

  name_ = ustr__empty();
  expression_.clear();

  char *cursor;
  static const char token_breaks[] = " \t\r\n";
  char *token = strtok_r(working_copy.get(), token_breaks, &cursor);

  for (;;) {
    
    if (!token) return Report();

    
    size_t token_len = strlen(token);
    if (token_len >= 1 && token[token_len - 1] == ':') {
      
      if (token_len < 2) return false;
      
      if (name_ != ustr__empty() || !expression_.empty()) {
        if (!Report()) return false;
      }
      name_ = ToUniqueString_n(token, token_len - 1);
      expression_.clear();
    } else {
      
      assert(token_len > 0); 
      if (!expression_.empty())
        expression_ += ' ';
      expression_ += token;
    }
    token = strtok_r(NULL, token_breaks, &cursor);
  }
}

bool CFIRuleParser::Report() {
  if (name_ == ustr__empty() || expression_.empty()) return false;
  if (name_ == ustr__ZDcfa()) handler_->CFARule(expression_);
  else if (name_ == ustr__ZDra()) handler_->RARule(expression_);
  else handler_->RegisterRule(name_, expression_);
  return true;
}

void CFIFrameInfoParseHandler::CFARule(const string &expression) {
  
  frame_info_->SetCFARule(Module::Expr(expression));
}

void CFIFrameInfoParseHandler::RARule(const string &expression) {
  frame_info_->SetRARule(Module::Expr(expression));
}

void CFIFrameInfoParseHandler::RegisterRule(const UniqueString* name,
                                            const string &expression) {
  frame_info_->SetRegisterRule(name, Module::Expr(expression));
}

} 
