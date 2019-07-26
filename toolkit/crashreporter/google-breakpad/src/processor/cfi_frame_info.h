




































#ifndef PROCESSOR_CFI_FRAME_INFO_H_
#define PROCESSOR_CFI_FRAME_INFO_H_

#include <map>
#include <string>

#include "common/using_std_string.h"
#include "common/unique_string.h"
#include "google_breakpad/common/breakpad_types.h"
#include "common/module.h"

namespace google_breakpad {

using std::map;

class MemoryRegion;














class CFIFrameInfo {
 public:
  
  template<typename ValueType> class RegisterValueMap:
    public UniqueStringMap<ValueType> { };

  
  
  
  void SetCFARule(const Module::Expr& rule) { cfa_rule_ = rule; }
  void SetRARule(const Module::Expr& rule)  { ra_rule_ = rule; }
  void SetRegisterRule(const UniqueString* register_name,
                       const Module::Expr& rule) {
    register_rules_[register_name] = rule;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  template<typename ValueType>
  bool FindCallerRegs(const RegisterValueMap<ValueType> &registers,
                      const MemoryRegion &memory,
                      RegisterValueMap<ValueType> *caller_registers) const;

  
  
  string Serialize() const;

 private:

  
  typedef map<const UniqueString*, Module::Expr> RuleMap;

  
  
  
  
  
  Module::Expr cfa_rule_;

  
  
  
  

  
  Module::Expr ra_rule_;

  
  
  
  RuleMap register_rules_;
};






class CFIRuleParser {
 public:

  class Handler {
   public:
    Handler() { }
    virtual ~Handler() { }

    
    virtual void CFARule(const string &expression) = 0;
    virtual void RARule(const string &expression) = 0;

    
    virtual void RegisterRule(const UniqueString* name,
                              const string &expression) = 0;
  };
    
  
  CFIRuleParser(Handler *handler) : handler_(handler) { }

  
  
  
  
  bool Parse(const string &rule_set);

 private:
  
  bool Report();

  
  Handler *handler_;

  
  const UniqueString* name_;
  string expression_;
};



class CFIFrameInfoParseHandler: public CFIRuleParser::Handler {
 public:
  
  CFIFrameInfoParseHandler(CFIFrameInfo *frame_info)
      : frame_info_(frame_info) { }

  void CFARule(const string &expression);
  void RARule(const string &expression);
  void RegisterRule(const UniqueString* name, const string &expression);

 private:
  CFIFrameInfo *frame_info_;
};
















template <typename RegisterType, class RawContextType>
class SimpleCFIWalker {
 public:
  
  struct RegisterSet {
    
    const UniqueString* name;

    
    
    
    
    
    const UniqueString* alternate_name;

    
    
    
    
    
    bool callee_saves;

    
    int validity_flag;

    
    
    RegisterType RawContextType::*context_member;
  };

  
  
  
  
  SimpleCFIWalker(const RegisterSet *register_map, size_t map_size)
      : register_map_(register_map), map_size_(map_size) { }

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool FindCallerRegisters(const MemoryRegion &memory,
                           const CFIFrameInfo &cfi_frame_info,
                           const RawContextType &callee_context,
                           int callee_validity,
                           RawContextType *caller_context,
                           int *caller_validity) const;

 private:
  const RegisterSet *register_map_;
  size_t map_size_;
};

}  

#include "cfi_frame_info-inl.h"

#endif  
