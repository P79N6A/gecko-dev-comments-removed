







































#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_CPU_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_CPU_H__

#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stack_frame.h"

namespace google_breakpad {

struct WindowsFrameInfo;
class CFIFrameInfo;

struct StackFrameX86 : public StackFrame {
  
  
  
  
  
  
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_EIP  = 1 << 0,
    CONTEXT_VALID_ESP  = 1 << 1,
    CONTEXT_VALID_EBP  = 1 << 2,
    CONTEXT_VALID_EAX  = 1 << 3,
    CONTEXT_VALID_EBX  = 1 << 4,
    CONTEXT_VALID_ECX  = 1 << 5,
    CONTEXT_VALID_EDX  = 1 << 6,
    CONTEXT_VALID_ESI  = 1 << 7,
    CONTEXT_VALID_EDI  = 1 << 8,
    CONTEXT_VALID_ALL  = -1
  };

  StackFrameX86()
     : context(),
       context_validity(CONTEXT_VALID_NONE),
       windows_frame_info(NULL),
       cfi_frame_info(NULL) {}
  ~StackFrameX86();

  
  virtual uint64_t ReturnAddress() const;

  
  
  
  
  MDRawContextX86 context;

  
  
  
  int context_validity;

  
  
  WindowsFrameInfo *windows_frame_info;
  CFIFrameInfo *cfi_frame_info;
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
    CONTEXT_VALID_NONE  = 0,
    CONTEXT_VALID_RAX   = 1 << 0,
    CONTEXT_VALID_RDX   = 1 << 1,
    CONTEXT_VALID_RCX   = 1 << 2,
    CONTEXT_VALID_RBX   = 1 << 3,
    CONTEXT_VALID_RSI   = 1 << 4,
    CONTEXT_VALID_RDI   = 1 << 5,
    CONTEXT_VALID_RBP   = 1 << 6,
    CONTEXT_VALID_RSP   = 1 << 7,
    CONTEXT_VALID_R8    = 1 << 8,
    CONTEXT_VALID_R9    = 1 << 9,
    CONTEXT_VALID_R10   = 1 << 10,
    CONTEXT_VALID_R11   = 1 << 11,
    CONTEXT_VALID_R12   = 1 << 12,
    CONTEXT_VALID_R13   = 1 << 13,
    CONTEXT_VALID_R14   = 1 << 14,
    CONTEXT_VALID_R15   = 1 << 15,
    CONTEXT_VALID_RIP   = 1 << 16,
    CONTEXT_VALID_ALL  = -1
  };

  StackFrameAMD64() : context(), context_validity(CONTEXT_VALID_NONE) {}

  
  virtual uint64_t ReturnAddress() const;

  
  
  
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

struct StackFrameARM : public StackFrame {
  
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_R0   = 1 << 0,
    CONTEXT_VALID_R1   = 1 << 1,
    CONTEXT_VALID_R2   = 1 << 2,
    CONTEXT_VALID_R3   = 1 << 3,
    CONTEXT_VALID_R4   = 1 << 4,
    CONTEXT_VALID_R5   = 1 << 5,
    CONTEXT_VALID_R6   = 1 << 6,
    CONTEXT_VALID_R7   = 1 << 7,
    CONTEXT_VALID_R8   = 1 << 8,
    CONTEXT_VALID_R9   = 1 << 9,
    CONTEXT_VALID_R10  = 1 << 10,
    CONTEXT_VALID_R11  = 1 << 11,
    CONTEXT_VALID_R12  = 1 << 12,
    CONTEXT_VALID_R13  = 1 << 13,
    CONTEXT_VALID_R14  = 1 << 14,
    CONTEXT_VALID_R15  = 1 << 15,
    CONTEXT_VALID_ALL  = ~CONTEXT_VALID_NONE,

    
    CONTEXT_VALID_FP   = CONTEXT_VALID_R11,
    CONTEXT_VALID_SP   = CONTEXT_VALID_R13,
    CONTEXT_VALID_LR   = CONTEXT_VALID_R14,
    CONTEXT_VALID_PC   = CONTEXT_VALID_R15
  };

  StackFrameARM() : context(), context_validity(CONTEXT_VALID_NONE) {}

  
  static ContextValidity RegisterValidFlag(int n) {
    return ContextValidity(1 << n);
  }

  
  
  
  
  MDRawContextARM context;

  
  
  
  
  
  
  
  int context_validity;
};

}  

#endif  
