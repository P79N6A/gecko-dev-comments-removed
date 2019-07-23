





































#ifndef GOOGLE_AIRBAG_PROCESSOR_STACK_FRAME_CPU_H__
#define GOOGLE_AIRBAG_PROCESSOR_STACK_FRAME_CPU_H__

#include "google_airbag/common/minidump_format.h"
#include "google_airbag/processor/stack_frame.h"

namespace google_airbag {

struct StackFrameX86 : public StackFrame {
  
  
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_EIP  = 1 << 0,
    CONTEXT_VALID_ESP  = 1 << 1,
    CONTEXT_VALID_EBP  = 1 << 2,
    CONTEXT_VALID_EBX  = 1 << 3,
    CONTEXT_VALID_ESI  = 1 << 4,
    CONTEXT_VALID_EDI  = 1 << 5,
    CONTEXT_VALID_ALL  = -1
  };

  StackFrameX86() : context(), context_validity(CONTEXT_VALID_NONE) {}

  
  
  
  
  MDRawContextX86 context;

  
  
  
  int context_validity;
};

struct StackFramePPC : public StackFrame {
  
  
  
  
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_SRR0 = 1 << 0,
    CONTEXT_VALID_GPR1 = 1 << 1,
    CONTEXT_VALID_ALL  = -1
  };

  StackFramePPC() : context(), context_validity(CONTEXT_VALID_NONE) {}

  
  
  
  
  MDRawContextPPC context;

  
  
  
  int context_validity;
};

}  

#endif  
