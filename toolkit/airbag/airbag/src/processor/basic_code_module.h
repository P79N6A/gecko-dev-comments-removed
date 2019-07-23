







































#ifndef PROCESSOR_BASIC_CODE_MODULE_H__
#define PROCESSOR_BASIC_CODE_MODULE_H__

#include <string>

#include "google_breakpad/processor/code_module.h"

namespace google_breakpad {

using std::string;

class BasicCodeModule : public CodeModule {
 public:
  
  
  
  
  explicit BasicCodeModule(const CodeModule *that)
      : base_address_(that->base_address()),
        size_(that->size()),
        code_file_(that->code_file()),
        code_identifier_(that->code_identifier()),
        debug_file_(that->debug_file()),
        debug_identifier_(that->debug_identifier()),
        version_(that->version()) {}
  virtual ~BasicCodeModule() {}

  
  
  virtual u_int64_t base_address() const { return base_address_; }
  virtual u_int64_t size() const { return size_; }
  virtual string code_file() const { return code_file_; }
  virtual string code_identifier() const { return code_identifier_; }
  virtual string debug_file() const { return debug_file_; }
  virtual string debug_identifier() const { return debug_identifier_; }
  virtual string version() const { return version_; }
  virtual const CodeModule* Copy() const { return new BasicCodeModule(this); }

 private:
  u_int64_t base_address_;
  u_int64_t size_;
  string code_file_;
  string code_identifier_;
  string debug_file_;
  string debug_identifier_;
  string version_;

  
  BasicCodeModule(const BasicCodeModule &that);
  void operator=(const BasicCodeModule &that);
};

}  

#endif  
