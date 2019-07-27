



#include <string>
#include <map>

#include "sandbox/win/src/policy_low_level.h"
#include "base/basictypes.h"

namespace {

  
  const size_t kRuleBufferSize = 1024*4;

  
  enum {
    PENDING_NONE,
    PENDING_ASTERISK,    
    PENDING_QMARK,       
  };

  
  
  const uint32 kLastCharIsNone = 0;
  const uint32 kLastCharIsAlpha = 1;
  const uint32 kLastCharIsWild = 2;
  const uint32 kLastCharIsAsterisk = kLastCharIsWild + 4;
  const uint32 kLastCharIsQuestionM = kLastCharIsWild + 8;
}

namespace sandbox {




bool LowLevelPolicy::AddRule(int service, PolicyRule* rule) {
  if (!rule->Done()) {
    return false;
  }

  PolicyRule* local_rule = new PolicyRule(*rule);
  RuleNode node = {local_rule, service};
  rules_.push_back(node);
  return true;
}

LowLevelPolicy::~LowLevelPolicy() {
  
  typedef std::list<RuleNode> RuleNodes;
  for (RuleNodes::iterator it = rules_.begin(); it != rules_.end(); ++it) {
    delete it->rule;
  }
}








bool LowLevelPolicy::Done() {
  typedef std::list<RuleNode> RuleNodes;
  typedef std::list<const PolicyRule*> RuleList;
  typedef std::map<uint32, RuleList> Mmap;
  Mmap mmap;

  for (RuleNodes::iterator it = rules_.begin(); it != rules_.end(); ++it) {
    mmap[it->service].push_back(it->rule);
  }

  PolicyBuffer* current_buffer = &policy_store_->data[0];
  char* buffer_end = reinterpret_cast<char*>(current_buffer) +
    policy_store_->data_size;
  size_t avail_size =  policy_store_->data_size;

  for (Mmap::iterator it = mmap.begin(); it != mmap.end(); ++it) {
    uint32 service = (*it).first;
    if (service >= kMaxServiceCount) {
      return false;
    }
    policy_store_->entry[service] = current_buffer;

    RuleList::iterator rules_it = (*it).second.begin();
    RuleList::iterator rules_it_end = (*it).second.end();

    size_t svc_opcode_count = 0;

    for (; rules_it != rules_it_end; ++rules_it) {
      const PolicyRule* rule = (*rules_it);
      size_t op_count = rule->GetOpcodeCount();

      size_t opcodes_size = op_count * sizeof(PolicyOpcode);
      if (avail_size < opcodes_size) {
        return false;
      }
      size_t data_size = avail_size - opcodes_size;
      PolicyOpcode* opcodes_start = &current_buffer->opcodes[svc_opcode_count];
      if (!rule->RebindCopy(opcodes_start, opcodes_size,
                            buffer_end, &data_size)) {
        return false;
      }
      size_t used = avail_size - data_size;
      buffer_end -= used;
      avail_size -= used;
      svc_opcode_count += op_count;
    }

    current_buffer->opcode_count += svc_opcode_count;
    size_t policy_byte_count = (svc_opcode_count * sizeof(PolicyOpcode))
                                / sizeof(current_buffer[0]);
    current_buffer = &current_buffer[policy_byte_count + 1];
  }

  return true;
}

PolicyRule::PolicyRule(EvalResult action)
    : action_(action), done_(false) {
  char* memory = new char[sizeof(PolicyBuffer) + kRuleBufferSize];
  buffer_ = reinterpret_cast<PolicyBuffer*>(memory);
  buffer_->opcode_count = 0;
  opcode_factory_ = new OpcodeFactory(buffer_,
                                      kRuleBufferSize + sizeof(PolicyOpcode));
}

PolicyRule::PolicyRule(const PolicyRule& other) {
  if (this == &other)
    return;
  action_ = other.action_;
  done_ = other.done_;
  size_t buffer_size = sizeof(PolicyBuffer) + kRuleBufferSize;
  char* memory = new char[buffer_size];
  buffer_ = reinterpret_cast<PolicyBuffer*>(memory);
  memcpy(buffer_, other.buffer_, buffer_size);

  char* opcode_buffer = reinterpret_cast<char*>(&buffer_->opcodes[0]);
  char* next_opcode = &opcode_buffer[GetOpcodeCount() * sizeof(PolicyOpcode)];
  opcode_factory_ =
      new OpcodeFactory(next_opcode, other.opcode_factory_->memory_size());
}








bool PolicyRule::GenStringOpcode(RuleType rule_type,
                                 StringMatchOptions match_opts,
                                 uint16 parameter, int state, bool last_call,
                                 int* skip_count, base::string16* fragment) {

  
  
  
  
  uint32 options = kPolNone;
  if (last_call) {
    if (IF_NOT == rule_type) {
      options = kPolClearContext | kPolNegateEval;
    } else {
      options = kPolClearContext;
    }
  } else if (IF_NOT == rule_type) {
    options = kPolUseOREval | kPolNegateEval;
  }

  PolicyOpcode* op = NULL;

  
  
  
  if (fragment->empty()) {
    
    
    
    if (last_call && (buffer_->opcode_count > 0)) {
      op = &buffer_->opcodes[buffer_->opcode_count - 1];
      op->SetOptions(options);
    }
    return true;
  }

  if (PENDING_ASTERISK == state) {
    if (last_call) {
      op = opcode_factory_->MakeOpWStringMatch(parameter, fragment->c_str(),
                                               kSeekToEnd, match_opts,
                                               options);
    } else {
      op = opcode_factory_->MakeOpWStringMatch(parameter, fragment->c_str(),
                                               kSeekForward, match_opts,
                                               options);
    }

  } else if (PENDING_QMARK == state) {
    op = opcode_factory_->MakeOpWStringMatch(parameter, fragment->c_str(),
                                             *skip_count, match_opts, options);
    *skip_count = 0;
  } else {
    if (last_call) {
      match_opts = static_cast<StringMatchOptions>(EXACT_LENGHT | match_opts);
    }
    op = opcode_factory_->MakeOpWStringMatch(parameter, fragment->c_str(), 0,
                                             match_opts, options);
  }
  if (NULL == op) {
    return false;
  }
  ++buffer_->opcode_count;
  fragment->clear();
  return true;
}

bool PolicyRule::AddStringMatch(RuleType rule_type, int16 parameter,
                                const wchar_t* string,
                                StringMatchOptions match_opts) {
  if (done_) {
    
    return false;
  }

  const wchar_t* current_char = string;
  uint32 last_char = kLastCharIsNone;
  int state = PENDING_NONE;
  int skip_count = 0;       
  base::string16 fragment;  

  while (L'\0' != *current_char) {
    switch (*current_char) {
      case L'*':
        if (kLastCharIsWild & last_char) {
          
          return false;
        }
        if (!GenStringOpcode(rule_type, match_opts, parameter,
                             state, false, &skip_count, &fragment)) {
          return false;
        }
        last_char = kLastCharIsAsterisk;
        state = PENDING_ASTERISK;
        break;
      case L'?':
        if (kLastCharIsAsterisk == last_char) {
          
          return false;
        }
        if (!GenStringOpcode(rule_type, match_opts, parameter,
                             state, false, &skip_count, &fragment)) {
          return false;
        }
        ++skip_count;
        last_char = kLastCharIsQuestionM;
        state = PENDING_QMARK;
        break;
      case L'/':
        
        if (L'?' == current_char[1]) {
          ++current_char;
        }
      default:
        fragment += *current_char;
        last_char = kLastCharIsAlpha;
    }
    ++current_char;
  }

  if (!GenStringOpcode(rule_type, match_opts, parameter,
                       state, true, &skip_count, &fragment)) {
    return false;
  }
  return true;
}

bool PolicyRule::AddNumberMatch(RuleType rule_type,
                                int16 parameter,
                                uint32 number,
                                RuleOp comparison_op) {
  if (done_) {
    
    return false;
  }
  uint32 opts = (rule_type == IF_NOT)? kPolNegateEval : kPolNone;

  if (EQUAL == comparison_op) {
    if (NULL == opcode_factory_->MakeOpNumberMatch(parameter, number, opts)) {
      return false;
    }
  } else if (AND == comparison_op) {
    if (NULL == opcode_factory_->MakeOpNumberAndMatch(parameter, number,
                                                      opts)) {
      return false;
    }
  }
  ++buffer_->opcode_count;
  return true;
}

bool PolicyRule::Done() {
  if (done_) {
    return true;
  }
  if (NULL == opcode_factory_->MakeOpAction(action_, kPolNone)) {
    return false;
  }
  ++buffer_->opcode_count;
  done_ = true;
  return true;
}

bool PolicyRule::RebindCopy(PolicyOpcode* opcode_start, size_t opcode_size,
                            char* data_start, size_t* data_size) const {
  size_t count = buffer_->opcode_count;
  for (size_t ix = 0; ix != count; ++ix) {
    if (opcode_size < sizeof(PolicyOpcode)) {
      return false;
    }
    PolicyOpcode& opcode = buffer_->opcodes[ix];
    *opcode_start = opcode;
    if (OP_WSTRING_MATCH == opcode.GetID()) {
      
      
      const wchar_t* str = opcode.GetRelativeString(0);
      size_t str_len;
      opcode.GetArgument(1, &str_len);
      str_len = str_len * sizeof(wchar_t);
      if ((*data_size) < str_len) {
        return false;
      }
      *data_size -= str_len;
      data_start -= str_len;
      memcpy(data_start, str, str_len);
      
      ptrdiff_t delta = data_start - reinterpret_cast<char*>(opcode_start);
      opcode_start->SetArgument(0, delta);
    }
    ++opcode_start;
    opcode_size -= sizeof(PolicyOpcode);
  }

  return true;
}

PolicyRule::~PolicyRule() {
  delete [] reinterpret_cast<char*>(buffer_);
  delete opcode_factory_;
}

}  
