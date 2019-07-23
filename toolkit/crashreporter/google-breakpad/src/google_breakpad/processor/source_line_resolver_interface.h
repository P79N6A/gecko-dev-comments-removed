






























#ifndef GOOGLE_BREAKPAD_PROCESSOR_SOURCE_LINE_RESOLVER_INTERFACE_H__
#define GOOGLE_BREAKPAD_PROCESSOR_SOURCE_LINE_RESOLVER_INTERFACE_H__

#include <string>
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

using std::string;

struct StackFrame;
struct StackFrameInfo;

class SourceLineResolverInterface {
 public:
  typedef u_int64_t MemAddr;

  virtual ~SourceLineResolverInterface() {}

  
  
  
  
  
  
  virtual bool LoadModule(const string &module_name,
                          const string &map_file) = 0;

  
  virtual bool HasModule(const string &module_name) const = 0;

  
  
  
  
  
  
  
  virtual StackFrameInfo* FillSourceLineInfo(StackFrame *frame) const = 0;

 protected:
  
  SourceLineResolverInterface() {}
};

}  

#endif  
