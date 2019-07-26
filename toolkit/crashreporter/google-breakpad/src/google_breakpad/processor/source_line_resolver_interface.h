
































#ifndef GOOGLE_BREAKPAD_PROCESSOR_SOURCE_LINE_RESOLVER_INTERFACE_H__
#define GOOGLE_BREAKPAD_PROCESSOR_SOURCE_LINE_RESOLVER_INTERFACE_H__

#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/code_module.h"

namespace google_breakpad {

struct StackFrame;
struct WindowsFrameInfo;
class CFIFrameInfo;

class SourceLineResolverInterface {
 public:
  typedef uint64_t MemAddr;

  virtual ~SourceLineResolverInterface() {}

  
  
  
  
  
  
  virtual bool LoadModule(const CodeModule *module,
                          const string &map_file) = 0;
  
  virtual bool LoadModuleUsingMapBuffer(const CodeModule *module,
                                        const string &map_buffer) = 0;

  
  
  
  
  virtual bool LoadModuleUsingMemoryBuffer(const CodeModule *module,
                                           char *memory_buffer) = 0;

  
  
  
  virtual bool ShouldDeleteMemoryBufferAfterLoadModule() = 0;

  
  
  virtual void UnloadModule(const CodeModule *module) = 0;

  
  virtual bool HasModule(const CodeModule *module) = 0;

  
  
  
  virtual void FillSourceLineInfo(StackFrame *frame) = 0;

  
  
  
  
  
  virtual WindowsFrameInfo *FindWindowsFrameInfo(const StackFrame *frame) = 0;

  
  
  
  
  virtual CFIFrameInfo *FindCFIFrameInfo(const StackFrame *frame) = 0;

 protected:
  
  SourceLineResolverInterface() {}
};

}  

#endif  
