

























#ifndef VIXL_A64_DEBUGGER_A64_H_
#define VIXL_A64_DEBUGGER_A64_H_

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <vector>

#include "jit/arm64/vixl/Constants-vixl.h"
#include "jit/arm64/vixl/Globals-vixl.h"
#include "jit/arm64/vixl/Simulator-vixl.h"
#include "jit/arm64/vixl/Utils-vixl.h"

namespace vixl {


enum DebugParameters {
  DBG_INACTIVE = 0,
  DBG_ACTIVE = 1 << 0,  
  DBG_BREAK  = 1 << 1   
};


class DebugCommand;
class Token;
class FormatToken;

class Debugger : public Simulator {
 public:
  explicit Debugger(Decoder* decoder, FILE* stream = stdout);

  virtual void Run();
  virtual void VisitException(const Instruction* instr);

  int debug_parameters() const { return debug_parameters_; }
  void set_debug_parameters(int parameters) {
    debug_parameters_ = parameters;

    update_pending_request();
  }

  
  
  int steps() const { return steps_; }
  void set_steps(int value) {
    VIXL_ASSERT(value > 1);
    steps_ = value;
  }

  bool IsDebuggerRunning() const {
    return (debug_parameters_ & DBG_ACTIVE) != 0;
  }

  bool pending_request() const { return pending_request_; }
  void update_pending_request() {
    pending_request_ = IsDebuggerRunning();
  }

  void PrintInstructions(const void* address, int64_t count = 1);
  void PrintMemory(const uint8_t* address,
                   const FormatToken* format,
                   int64_t count = 1);
  void PrintRegister(const Register& target_reg,
                     const char* name,
                     const FormatToken* format);
  void PrintFPRegister(const FPRegister& target_fpreg,
                       const FormatToken* format);

 private:
  char* ReadCommandLine(const char* prompt, char* buffer, int length);
  void RunDebuggerShell();
  void DoBreakpoint(const Instruction* instr);

  int debug_parameters_;
  bool pending_request_;
  int steps_;
  DebugCommand* last_command_;
  PrintDisassembler* disasm_;
  Decoder* printer_;

  
  static const int kMaxDebugShellLine = 256;
};

}  

#endif  
