





































#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_CPU_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_CPU_H__

#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stack_frame.h"

namespace google_breakpad {

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

  
  
  
  
  enum FrameTrust {
    FRAME_TRUST_NONE,     
    FRAME_TRUST_SCAN,     
    FRAME_TRUST_CFI_SCAN, 
    FRAME_TRUST_FP,       
    FRAME_TRUST_CFI,      
    FRAME_TRUST_CONTEXT   
  };

 StackFrameX86()
     : context(),
       context_validity(CONTEXT_VALID_NONE),
       trust(FRAME_TRUST_NONE) {}

  
  
  
  
  MDRawContextX86 context;

  
  
  
  int context_validity;
  
  
  
  FrameTrust trust;
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

struct StackFrameAMD64 : public StackFrame {
  
  
  
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_RIP  = 1 << 0,
    CONTEXT_VALID_RSP  = 1 << 1,
    CONTEXT_VALID_RBP  = 1 << 2,
    CONTEXT_VALID_ALL  = -1
  };

  StackFrameAMD64() : context(), context_validity(CONTEXT_VALID_NONE) {}

  
  
  
  
  MDRawContextAMD64 context;

  
  
  
  int context_validity;
};

struct StackFrameSPARC : public StackFrame {
  
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_PC   = 1 << 0,
    CONTEXT_VALID_SP   = 1 << 1,
    CONTEXT_VALID_FP   = 1 << 2,
    CONTEXT_VALID_ALL  = -1
  };

  StackFrameSPARC() : context(), context_validity(CONTEXT_VALID_NONE) {}

  
  
  
  
  MDRawContextSPARC context;

  
  
  
  int context_validity;
};

}  

#endif  
