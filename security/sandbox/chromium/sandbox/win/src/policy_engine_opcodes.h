



#ifndef SANDBOX_WIN_SRC_POLICY_ENGINE_OPCODES_H_
#define SANDBOX_WIN_SRC_POLICY_ENGINE_OPCODES_H_

#include "base/basictypes.h"
#include "base/numerics/safe_conversions.h"
#include "sandbox/win/src/policy_engine_params.h"




































namespace sandbox {





enum EvalResult {
  
  EVAL_TRUE,   
  EVAL_FALSE,  
  EVAL_ERROR,  
  
  ASK_BROKER,  
               
  DENY_ACCESS,   
  GIVE_READONLY,  
  GIVE_ALLACCESS,  
  GIVE_CACHED,  
  GIVE_FIRST,  
  SIGNAL_ALARM,  
  FAKE_SUCCESS,  
  FAKE_ACCESS_DENIED,  
                       
  TERMINATE_PROCESS,  
};


enum OpcodeID {
  OP_ALWAYS_FALSE,  
  OP_ALWAYS_TRUE,  
  OP_NUMBER_MATCH,  
  OP_NUMBER_MATCH_RANGE,  
  OP_NUMBER_AND_MATCH,  
  OP_WSTRING_MATCH,  
  OP_ACTION  
};




const uint32 kPolNone = 0;



const uint32 kPolNegateEval = 1;



const uint32 kPolClearContext = 2;






const uint32 kPolUseOREval = 4;






struct MatchContext {
  size_t position;
  uint32 options;

  MatchContext() {
    Clear();
  }

  void Clear() {
    position = 0;
    options = 0;
  }
};
















class PolicyOpcode {
  friend class OpcodeFactory;
 public:
  
  
  
  
  
  
  
  
  
  EvalResult Evaluate(const ParameterSet* parameters, size_t count,
                      MatchContext* match);

  
  
  template <typename T>
  void GetArgument(size_t index, T* argument) const {
    COMPILE_ASSERT(sizeof(T) <= sizeof(arguments_[0]), invalid_size);
    *argument = *reinterpret_cast<const T*>(&arguments_[index].mem);
  }

  
  
  template <typename T>
  void SetArgument(size_t index, const T& argument) {
    COMPILE_ASSERT(sizeof(T) <= sizeof(arguments_[0]), invalid_size);
    *reinterpret_cast<T*>(&arguments_[index].mem) = argument;
  }

  
  
  
  
  
  const wchar_t* GetRelativeString(size_t index) const {
    ptrdiff_t str_delta = 0;
    GetArgument(index, &str_delta);
    const char* delta = reinterpret_cast<const char*>(this) + str_delta;
    return reinterpret_cast<const wchar_t*>(delta);
  }

  
  
  bool IsAction() const {
    return (OP_ACTION == opcode_id_);
  };

  
  OpcodeID GetID() const {
    return opcode_id_;
  }

  
  uint32 GetOptions() const {
    return options_;
  }

  
  void SetOptions(uint32 options) {
    options_ = base::checked_cast<uint16>(options);
  }

 private:

  static const size_t kArgumentCount = 4;  

  struct OpcodeArgument {
    UINT_PTR mem;
  };

  
  
  void* operator new(size_t, void* location) {
    return location;
  }

  
  
  EvalResult EvaluateHelper(const ParameterSet* parameters,
                           MatchContext* match);
  OpcodeID opcode_id_;
  int16 parameter_;
  
  
  
  uint16 options_;
  OpcodeArgument arguments_[PolicyOpcode::kArgumentCount];
};

enum StringMatchOptions {
  CASE_SENSITIVE = 0,      
  CASE_INSENSITIVE = 1,    
  EXACT_LENGHT = 2         
};







const int  kSeekForward = -1;

const int  kSeekToEnd = 0xfffff;




struct PolicyBuffer {
  size_t opcode_count;
  PolicyOpcode opcodes[1];
};


























class OpcodeFactory {
 public:
  
  
  OpcodeFactory(char* memory, size_t memory_size)
      : memory_top_(memory) {
    memory_bottom_ = &memory_top_[memory_size];
  }

  
  
  OpcodeFactory(PolicyBuffer* policy, size_t memory_size) {
    memory_top_ = reinterpret_cast<char*>(&policy->opcodes[0]);
    memory_bottom_ = &memory_top_[memory_size];
  }

  
  size_t memory_size() const {
    return memory_bottom_ - memory_top_;
  }

  
  PolicyOpcode* MakeOpAlwaysFalse(uint32 options);

  
  PolicyOpcode* MakeOpAlwaysTrue(uint32 options);

  
  
  PolicyOpcode* MakeOpAction(EvalResult action, uint32 options);

  
  
  
  
  PolicyOpcode* MakeOpNumberMatch(int16 selected_param,
                                  uint32 match,
                                  uint32 options);

  
  
  
  
  PolicyOpcode* MakeOpVoidPtrMatch(int16 selected_param,
                                   const void* match,
                                   uint32 options);

  
  
  
  
  PolicyOpcode* MakeOpNumberMatchRange(int16 selected_param,
                                       uint32 lower_bound,
                                       uint32 upper_bound,
                                       uint32 options);

  
  
  
  
  
  
  
  
  
  
  
  
  
  PolicyOpcode* MakeOpWStringMatch(int16 selected_param,
                                   const wchar_t* match_str,
                                   int start_position,
                                   StringMatchOptions match_opts,
                                   uint32 options);

  
  
  
  
  PolicyOpcode* MakeOpNumberAndMatch(int16 selected_param,
                                     uint32 match,
                                     uint32 options);

 private:
  
  
  
  PolicyOpcode* MakeBase(OpcodeID opcode_id, uint32 options,
                         int16 selected_param);

  
  
  ptrdiff_t AllocRelative(void* start, const wchar_t* str, size_t lenght);

  
  
  char* memory_top_;

  
  
  
  char* memory_bottom_;

  DISALLOW_COPY_AND_ASSIGN(OpcodeFactory);
};

}  

#endif  
