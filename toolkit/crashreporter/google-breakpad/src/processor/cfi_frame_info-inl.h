


































#ifndef PROCESSOR_CFI_FRAME_INFO_INL_H_
#define PROCESSOR_CFI_FRAME_INFO_INL_H_

#include <string.h>

namespace google_breakpad {

template <typename RegisterValueType, class RawContextType>
bool SimpleCFIWalker<RegisterValueType, RawContextType>::FindCallerRegisters(
    const MemoryRegion &memory,
    const CFIFrameInfo &cfi_frame_info,
    const RawContextType &callee_context,
    int callee_validity,
    RawContextType *caller_context,
    int *caller_validity) const {
  typedef CFIFrameInfo::RegisterValueMap<RegisterValueType> ValueMap;
  ValueMap callee_registers;
  ValueMap caller_registers;

  
  for (size_t i = 0; i < map_size_; i++) {
    const RegisterSet &r = register_map_[i];
    if (callee_validity & r.validity_flag)
      callee_registers.set(r.name, callee_context.*r.context_member);
  }

  
  if (!cfi_frame_info
       .FindCallerRegs<RegisterValueType>(callee_registers, memory,
                                          &caller_registers))
    return false;

  
  
  memset(caller_context, 0xda, sizeof(*caller_context));
  *caller_validity = 0;
  for (size_t i = 0; i < map_size_; i++) {
    const RegisterSet &r = register_map_[i];

    
    bool found = false;
    RegisterValueType v = caller_registers.get(&found, r.name);
    if (found) {
      caller_context->*r.context_member = v;
      *caller_validity |= r.validity_flag;
      continue;
    }

    
    
    if (r.alternate_name) {
      found = false;
      v = caller_registers.get(&found, r.alternate_name);
      if (found) {
        caller_context->*r.context_member = v;
        *caller_validity |= r.validity_flag;
        continue;
      }
    }

    
    
    
    
    
    
    
    
    
    if (r.callee_saves && (callee_validity & r.validity_flag) != 0) {
      caller_context->*r.context_member = callee_context.*r.context_member;
      *caller_validity |= r.validity_flag;
      continue;
    }

    
  }

  return true;
}

} 

#endif 
