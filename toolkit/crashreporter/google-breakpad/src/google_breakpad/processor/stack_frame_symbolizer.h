

































#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_SYMBOLIZER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_SYMBOLIZER_H__

#include <set>
#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/code_module.h"

namespace google_breakpad {
class CFIFrameInfo;
class CodeModules;
class SymbolSupplier;
class SourceLineResolverInterface;
struct StackFrame;
struct SystemInfo;
struct WindowsFrameInfo;

class StackFrameSymbolizer {
 public:
  enum SymbolizerResult {
    
    
    kNoError,
    
    
    kError,
    
    
    kInterrupt
  };

  StackFrameSymbolizer(SymbolSupplier* supplier,
                       SourceLineResolverInterface* resolver);

  virtual ~StackFrameSymbolizer() { }

  
  
  virtual SymbolizerResult FillSourceLineInfo(const CodeModules* modules,
                                              const SystemInfo* system_info,
                                              StackFrame* stack_frame);

  virtual WindowsFrameInfo* FindWindowsFrameInfo(const StackFrame* frame);

  virtual CFIFrameInfo* FindCFIFrameInfo(const StackFrame* frame);

  
  
  
  
  virtual void Reset() { no_symbol_modules_.clear(); }

  
  virtual bool HasImplementation() { return resolver_ && supplier_; }

  SourceLineResolverInterface* resolver() { return resolver_; }
  SymbolSupplier* supplier() { return supplier_; }

 protected:
  SymbolSupplier* supplier_;
  SourceLineResolverInterface* resolver_;
  
  
  std::set<string> no_symbol_modules_;
};

}  

#endif
