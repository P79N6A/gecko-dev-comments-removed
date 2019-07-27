



#include "sandbox/win/src/policy_engine_processor.h"

namespace sandbox {

void PolicyProcessor::SetInternalState(size_t index, EvalResult result) {
  state_.current_index_ = index;
  state_.current_result_ = result;
}

EvalResult PolicyProcessor::GetAction() const {
  return state_.current_result_;
}






bool SkipOpcode(const PolicyOpcode& opcode, MatchContext* context,
                bool* keep_skipping) {
  if (opcode.IsAction()) {
    uint32 options = context->options;
    context->Clear();
    *keep_skipping = false;
    return (kPolUseOREval != options);
  }
  *keep_skipping = true;
  return true;
}

PolicyResult PolicyProcessor::Evaluate(uint32 options,
                                       ParameterSet* parameters,
                                       size_t param_count) {
  if (NULL == policy_) {
    return NO_POLICY_MATCH;
  }
  if (0 == policy_->opcode_count) {
    return NO_POLICY_MATCH;
  }
  if (!(kShortEval & options)) {
    return POLICY_ERROR;
  }

  MatchContext context;
  bool evaluation = false;
  bool skip_group = false;
  SetInternalState(0, EVAL_FALSE);
  size_t count = policy_->opcode_count;

  
  
  
  
  
  
  
  

  for (size_t ix = 0; ix != count; ++ix) {
    PolicyOpcode& opcode = policy_->opcodes[ix];
    
    if (skip_group) {
      if (SkipOpcode(opcode, &context, &skip_group)) {
        continue;
      }
    }
    
    EvalResult result = opcode.Evaluate(parameters, param_count, &context);
    switch (result) {
      case EVAL_FALSE:
        evaluation = false;
        if (kPolUseOREval != context.options) {
          skip_group = true;
        }
        break;
      case EVAL_ERROR:
        if (kStopOnErrors & options) {
          return POLICY_ERROR;
        }
        break;
      case EVAL_TRUE:
        evaluation = true;
        if (kPolUseOREval == context.options) {
          skip_group = true;
        }
        break;
      default:
        
        SetInternalState(ix, result);
        return POLICY_MATCH;
    }
  }

  if (evaluation) {
    
    
    return POLICY_ERROR;
  }
  return NO_POLICY_MATCH;
}


}  
