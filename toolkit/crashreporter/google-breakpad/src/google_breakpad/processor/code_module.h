

































#ifndef GOOGLE_BREAKPAD_PROCESSOR_CODE_MODULE_H__
#define GOOGLE_BREAKPAD_PROCESSOR_CODE_MODULE_H__

#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

class CodeModule {
 public:
  virtual ~CodeModule() {}

  
  
  virtual uint64_t base_address() const = 0;

  
  virtual uint64_t size() const = 0;

  
  
  virtual string code_file() const = 0;

  
  
  
  
  virtual string code_identifier() const = 0;

  
  
  
  
  
  
  virtual string debug_file() const = 0;

  
  
  
  
  
  virtual string debug_identifier() const = 0;

  
  
  virtual string version() const = 0;

  
  
  
  
  virtual const CodeModule* Copy() const = 0;
};

}  

#endif  
