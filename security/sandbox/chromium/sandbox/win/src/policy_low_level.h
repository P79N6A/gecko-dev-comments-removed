



#ifndef SANDBOX_SRC_POLICY_LOW_LEVEL_H__
#define SANDBOX_SRC_POLICY_LOW_LEVEL_H__

#include <list>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_engine_params.h"
#include "sandbox/win/src/policy_engine_opcodes.h"

























namespace sandbox {


const size_t kMaxServiceCount = 32;
COMPILE_ASSERT(IPC_LAST_TAG <= kMaxServiceCount, kMaxServiceCount_is_too_low);


























struct PolicyGlobal {
  PolicyBuffer* entry[kMaxServiceCount];
  size_t data_size;
  PolicyBuffer data[1];
};

class PolicyRule;


class LowLevelPolicy {
 public:
  
  
  explicit LowLevelPolicy(PolicyGlobal* policy_store)
      : policy_store_(policy_store) {
  }

  
  ~LowLevelPolicy();

  
  
  
  
  bool AddRule(int service, PolicyRule* rule);

  
  
  bool Done();

 private:
  struct RuleNode {
    const PolicyRule* rule;
    int service;
  };
  std::list<RuleNode> rules_;
  PolicyGlobal* policy_store_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(LowLevelPolicy);
};


enum RuleType {
  IF = 0,
  IF_NOT = 1,
};


enum RuleOp {
  EQUAL,
  AND,
  RANGE   
};



class PolicyRule {
  friend class LowLevelPolicy;

 public:
  explicit PolicyRule(EvalResult action);
  PolicyRule(const PolicyRule& other);
  ~PolicyRule();

  
  
  
  
  
  
  bool AddStringMatch(RuleType rule_type, int16 parameter,
                      const wchar_t* string, StringMatchOptions match_opts);

  
  
  
  
  
  bool AddNumberMatch(RuleType rule_type,
                      int16 parameter,
                      uint32 number,
                      RuleOp comparison_op);

  
  size_t GetOpcodeCount() const {
    return buffer_->opcode_count;
  }

  
  
  bool Done();

 private:
  void operator=(const PolicyRule&);
  
  
  
  bool GenStringOpcode(RuleType rule_type, StringMatchOptions match_opts,
                       uint16 parameter, int state, bool last_call,
                       int* skip_count, base::string16* fragment);

  
  
  
  
  bool RebindCopy(PolicyOpcode* opcode_start, size_t opcode_size,
                  char* data_start, size_t* data_size) const;
  PolicyBuffer* buffer_;
  OpcodeFactory* opcode_factory_;
  EvalResult action_;
  bool done_;
};

}  

#endif  
