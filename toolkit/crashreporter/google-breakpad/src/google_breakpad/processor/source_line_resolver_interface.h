
































#ifndef GOOGLE_BREAKPAD_PROCESSOR_SOURCE_LINE_RESOLVER_INTERFACE_H__
#define GOOGLE_BREAKPAD_PROCESSOR_SOURCE_LINE_RESOLVER_INTERFACE_H__

#include <string>
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

using std::string;

struct StackFrame;
struct WindowsFrameInfo;
struct CFIFrameInfo;

class SourceLineResolverInterface {
 public:
  typedef u_int64_t MemAddr;

  virtual ~SourceLineResolverInterface() {}

  
  
  
  
  
  
  virtual bool LoadModule(const string &module_name,
                          const string &map_file) = 0;
  
  virtual bool LoadModuleUsingMapBuffer(const string &module_name,
                                        const string &map_buffer) = 0;

  
  virtual bool HasModule(const string &module_name) const = 0;

  
  
  
  virtual void FillSourceLineInfo(StackFrame *frame) const = 0;

  
  
  
  
  
  virtual WindowsFrameInfo *FindWindowsFrameInfo(const StackFrame *frame) 
    const = 0; 

  
  
  
  
  virtual CFIFrameInfo *FindCFIFrameInfo(const StackFrame *frame) const = 0;

 protected:
  
  SourceLineResolverInterface() {}
};

}  

#endif  
