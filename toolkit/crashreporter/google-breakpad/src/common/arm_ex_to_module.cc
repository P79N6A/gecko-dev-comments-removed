
























































#include "common/unique_string.h"
#include "common/arm_ex_to_module.h"

#include <stdio.h>
#include <assert.h>




#define ARM_EXBUF_START(x) (((x) >> 4) & 0x0f)
#define ARM_EXBUF_COUNT(x) ((x) & 0x0f)
#define ARM_EXBUF_END(x)   (ARM_EXBUF_START(x) + ARM_EXBUF_COUNT(x))

using google_breakpad::ustr__pc;
using google_breakpad::ustr__lr;
using google_breakpad::ustr__sp;
using google_breakpad::Module;
using google_breakpad::ToUniqueString;
using google_breakpad::UniqueString;

namespace arm_ex_to_module {


int ARMExToModule::TranslateCmd(const struct extab_data* edata,
                                Module::StackFrameEntry* entry, string& vsp) {
  int ret = 0;
  switch (edata->cmd) {
    case ARM_EXIDX_CMD_FINISH:
      
      if (entry->initial_rules.find(ustr__pc())
          == entry->initial_rules.end()) {
        if (entry->initial_rules.find(ustr__lr())
            == entry->initial_rules.end()) {
          entry->initial_rules[ustr__pc()] = Module::Expr("lr");
        } else {
          entry->initial_rules[ustr__pc()] = entry->initial_rules[ustr__lr()];
        }
      }
      break;
    case ARM_EXIDX_CMD_SUB_FROM_VSP:
      {
        char c[16];
        sprintf(c, " %d -", edata->data);
        vsp += c;
      }
      break;
    case ARM_EXIDX_CMD_ADD_TO_VSP:
      {
        char c[16];
        sprintf(c, " %d +", edata->data);
        vsp += c;
      }
      break;
    case ARM_EXIDX_CMD_REG_POP:
      for (unsigned int i = 0; i < 16; i++) {
        if (edata->data & (1 << i)) {
          entry->initial_rules[ToUniqueString(regnames[i])]
            = Module::Expr(vsp + " ^");
          vsp += " 4 +";
        }
      }
      
      if (edata->data & (1 << 13)) {
        Module::Expr& vsp_expr = entry->initial_rules[ustr__sp()];
        
        
        if (!vsp_expr.isExprPostfix()) {
          ret = -1;
          break;
        };
        vsp = vsp_expr.getExprPostfix();
      }
      break;
    case ARM_EXIDX_CMD_REG_TO_SP: {
      assert (edata->data < 16);
      const char* const regname = regnames[edata->data];
      const UniqueString* regname_us = ToUniqueString(regname);
      if (entry->initial_rules.find(regname_us) == entry->initial_rules.end()) {
        entry->initial_rules[ustr__sp()] = Module::Expr(regname);
      } else {
        entry->initial_rules[ustr__sp()] = entry->initial_rules[regname_us];
      }
      Module::Expr& vsp_expr = entry->initial_rules[ustr__sp()];
      if (!vsp_expr.isExprPostfix()) {
        ret = -1;
        break;
      };
      vsp = vsp_expr.getExprPostfix();
      break;
    }
    case ARM_EXIDX_CMD_VFP_POP:
      

      for (unsigned int i = ARM_EXBUF_START(edata->data);
           i <= ARM_EXBUF_END(edata->data); i++) {
        vsp += " 8 +";
      }
      if (!(edata->data & ARM_EXIDX_VFP_FSTMD)) {
        vsp += " 4 +";
      }
      break;
    case ARM_EXIDX_CMD_WREG_POP:
      for (unsigned int i = ARM_EXBUF_START(edata->data);
           i <= ARM_EXBUF_END(edata->data); i++) {
        vsp += " 8 +";
      }
      break;
    case ARM_EXIDX_CMD_WCGR_POP:
      
      for (unsigned int i = 0; i < 4; i++) {
        if (edata->data & (1 << i)) {
          vsp += " 4 +";
        }
      }
      break;
    case ARM_EXIDX_CMD_REFUSED:
    case ARM_EXIDX_CMD_RESERVED:
      ret = -1;
      break;
  }
  return ret;
}

void ARMExToModule::AddStackFrame(uintptr_t addr, size_t size) {
  stack_frame_entry_ = new Module::StackFrameEntry;
  stack_frame_entry_->address = addr;
  stack_frame_entry_->size = size;
  stack_frame_entry_->initial_rules[ToUniqueString(kCFA)] = Module::Expr("sp");
  vsp_ = "sp";
}

int ARMExToModule::ImproveStackFrame(const struct extab_data* edata) {
  return TranslateCmd(edata, stack_frame_entry_, vsp_) ;
}

void ARMExToModule::DeleteStackFrame() {
  delete stack_frame_entry_;
}

void ARMExToModule::SubmitStackFrame() {
  
  stack_frame_entry_->initial_rules[ToUniqueString(kRA)]
    = stack_frame_entry_->initial_rules[ustr__pc()];
  
  stack_frame_entry_->initial_rules[ustr__sp()] = vsp_;
  module_->AddStackFrameEntry(stack_frame_entry_);
}

} 
