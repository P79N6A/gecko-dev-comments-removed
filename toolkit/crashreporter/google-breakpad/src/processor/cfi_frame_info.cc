

































#include <cstring>

#include "processor/cfi_frame_info.h"
#include "processor/postfix_evaluator-inl.h"
#include "processor/scoped_ptr.h"

namespace google_breakpad {

template<typename V>
bool CFIFrameInfo::FindCallerRegs(const RegisterValueMap<V> &registers,
                                  const MemoryRegion &memory,
                                  RegisterValueMap<V> *caller_registers) const {
  
  
  if (cfa_rule_.empty() || ra_rule_.empty())
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
  working[".cfa"] = cfa;
  if (!evaluator.EvaluateForValue(ra_rule_, &ra))
    return false;

  
  for (RuleMap::const_iterator it = register_rules_.begin();
       it != register_rules_.end(); it++) {
    V value;
    working = registers;
    working[".cfa"] = cfa;
    if (!evaluator.EvaluateForValue(it->second, &value))
      return false;
    (*caller_registers)[it->first] = value;
  }

  (*caller_registers)[".ra"] = ra;
  (*caller_registers)[".cfa"] = cfa;

  return true;
}


template bool CFIFrameInfo::FindCallerRegs<u_int32_t>(
    const RegisterValueMap<u_int32_t> &registers,
    const MemoryRegion &memory,
    RegisterValueMap<u_int32_t> *caller_registers) const;
template bool CFIFrameInfo::FindCallerRegs<u_int64_t>(
    const RegisterValueMap<u_int64_t> &registers,
    const MemoryRegion &memory,
    RegisterValueMap<u_int64_t> *caller_registers) const;

bool CFIRuleParser::Parse(const string &rule_set) {
  size_t rule_set_len = rule_set.size();
  scoped_array<char> working_copy(new char[rule_set_len + 1]);
  memcpy(working_copy.get(), rule_set.data(), rule_set_len);
  working_copy[rule_set_len] = '\0';

  name_.clear();
  expression_.clear();

  char *cursor;
  static const char token_breaks[] = " \t\r\n";
  char *token = strtok_r(working_copy.get(), token_breaks, &cursor);

  for (;;) {
    
    if (!token) return Report();

    
    size_t token_len = strlen(token);
    if (token_len >= 1 && token[token_len - 1] == ':') {
      
      if (token_len < 2) return false;
      
      if (!name_.empty() || !expression_.empty()) {
        if (!Report()) return false;
      }
      name_.assign(token, token_len - 1);
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
  if (name_.empty() || expression_.empty()) return false;
  if (name_ == ".cfa") handler_->CFARule(expression_);
  else if (name_ == ".ra") handler_->RARule(expression_);
  else handler_->RegisterRule(name_, expression_);
  return true;
}

void CFIFrameInfoParseHandler::CFARule(const string &expression) {
  frame_info_->SetCFARule(expression);
}

void CFIFrameInfoParseHandler::RARule(const string &expression) {
  frame_info_->SetRARule(expression);
}

void CFIFrameInfoParseHandler::RegisterRule(const string &name,
                                            const string &expression) {
  frame_info_->SetRegisterRule(name, expression);
}

} 
