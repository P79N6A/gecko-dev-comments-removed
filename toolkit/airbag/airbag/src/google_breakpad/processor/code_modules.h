

































#ifndef GOOGLE_BREAKPAD_PROCESSOR_CODE_MODULES_H__
#define GOOGLE_BREAKPAD_PROCESSOR_CODE_MODULES_H__

#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

class CodeModule;

class CodeModules {
 public:
  virtual ~CodeModules() {}

  
  virtual unsigned int module_count() const = 0;

  
  
  
  
  
  virtual const CodeModule* GetModuleForAddress(u_int64_t address) const = 0;

  
  
  
  
  
  virtual const CodeModule* GetMainModule() const = 0;

  
  
  
  
  
  virtual const CodeModule* GetModuleAtSequence(
      unsigned int sequence) const = 0;

  
  
  
  
  
  
  
  
  
  
  virtual const CodeModule* GetModuleAtIndex(unsigned int index) const = 0;

  
  
  
  
  
  
  
  
  virtual const CodeModules* Copy() const = 0;
};

}  

#endif  
