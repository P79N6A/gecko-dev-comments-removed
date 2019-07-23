

































#ifndef GOOGLE_BREAKPAD_PROCESSOR_CODE_MODULE_H__
#define GOOGLE_BREAKPAD_PROCESSOR_CODE_MODULE_H__

#include <string>

namespace google_breakpad {

using std::string;

class CodeModule {
 public:
  virtual ~CodeModule() {}

  
  
  virtual u_int64_t base_address() const = 0;

  
  virtual u_int64_t size() const = 0;

  
  
  virtual string code_file() const = 0;

  
  
  
  
  virtual string code_identifier() const = 0;

  
  
  
  
  
  
  virtual string debug_file() const = 0;

  
  
  
  
  
  virtual string debug_identifier() const = 0;

  
  
  virtual string version() const = 0;

  
  
  
  
  virtual const CodeModule* Copy() const = 0;
};

}  

#endif  
