
























































#ifndef COMMON_ARM_EX_TO_MODULE__
#define COMMON_ARM_EX_TO_MODULE__

#include "common/module.h"

#include <string.h>

namespace arm_ex_to_module {

using google_breakpad::Module;

typedef enum extab_cmd {
  ARM_EXIDX_CMD_FINISH,
  ARM_EXIDX_CMD_SUB_FROM_VSP,
  ARM_EXIDX_CMD_ADD_TO_VSP,
  ARM_EXIDX_CMD_REG_POP,
  ARM_EXIDX_CMD_REG_TO_SP,
  ARM_EXIDX_CMD_VFP_POP,
  ARM_EXIDX_CMD_WREG_POP,
  ARM_EXIDX_CMD_WCGR_POP,
  ARM_EXIDX_CMD_RESERVED,
  ARM_EXIDX_CMD_REFUSED,
} extab_cmd_t;

struct exidx_entry {
  uint32_t addr;
  uint32_t data;
};

struct extab_data {
  extab_cmd_t cmd;
  uint32_t data;
};

enum extab_cmd_flags {
  ARM_EXIDX_VFP_SHIFT_16 = 1 << 16,
  ARM_EXIDX_VFP_FSTMD = 1 << 17, 
};

static const char* const regnames[] = {
 "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
 "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc",
 "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
 "fps", "cpsr"
};



class ARMExToModule {
 public:
  ARMExToModule(Module* module)
      : module_(module) { }
  ~ARMExToModule() { }
  bool HasStackFrame(uintptr_t addr, size_t size);
  void AddStackFrame(uintptr_t addr, size_t size);
  int ImproveStackFrame(const struct extab_data* edata);
  void DeleteStackFrame();
  void SubmitStackFrame();
 private:
  Module* module_;
  Module::StackFrameEntry* stack_frame_entry_;
  Module::Expr vsp_;
  int TranslateCmd(const struct extab_data* edata,
                   Module::StackFrameEntry* entry,
                   Module::Expr& vsp);
};

} 

#endif 
