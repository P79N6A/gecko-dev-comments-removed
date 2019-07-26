



#ifndef SANDBOX_SRC_POLICY_ENGINE_PROCESSOR_H__
#define SANDBOX_SRC_POLICY_ENGINE_PROCESSOR_H__

#include "base/basictypes.h"
#include "sandbox/win/src/policy_engine_params.h"
#include "sandbox/win/src/policy_engine_opcodes.h"

namespace sandbox {














































enum PolicyResult {
  NO_POLICY_MATCH,
  POLICY_MATCH,
  POLICY_ERROR
};





const uint32 kStopOnErrors = 0;

const uint32 kIgnoreErrors = 1;


const uint32 kShortEval = 2;


const uint32 kRankedEval = 4;






























class PolicyProcessor {
 public:
  
  

  
  explicit PolicyProcessor(PolicyBuffer* policy)
      : policy_(policy) {
    SetInternalState(0, EVAL_FALSE);
  }

  
  
  
  PolicyResult Evaluate(uint32 options,
                        ParameterSet* parameters,
                        size_t parameter_count);

  
  
  EvalResult GetAction() const;

 private:
  struct {
    size_t current_index_;
    EvalResult current_result_;
  } state_;

  
  void SetInternalState(size_t index, EvalResult result);

  PolicyBuffer* policy_;
  DISALLOW_COPY_AND_ASSIGN(PolicyProcessor);
};

}  

#endif  
