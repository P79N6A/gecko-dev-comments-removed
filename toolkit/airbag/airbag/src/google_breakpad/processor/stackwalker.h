







































#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACKWALKER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACKWALKER_H__

#include <vector>

namespace google_breakpad {

class CallStack;
class CodeModules;
template<typename T> class linked_ptr;
class MemoryRegion;
class MinidumpContext;
class SourceLineResolverInterface;
struct StackFrame;
struct StackFrameInfo;
class SymbolSupplier;
class SystemInfo;

using std::vector;


class Stackwalker {
 public:
  virtual ~Stackwalker() {}

  
  
  
  
  bool Walk(CallStack *stack);

  
  
  
  static Stackwalker* StackwalkerForCPU(const SystemInfo *system_info,
                                        MinidumpContext *context,
                                        MemoryRegion *memory,
                                        const CodeModules *modules,
                                        SymbolSupplier *supplier,
                                        SourceLineResolverInterface *resolver);

 protected:
  
  
  
  
  
  
  
  
  
  Stackwalker(const SystemInfo *system_info,
              MemoryRegion *memory,
              const CodeModules *modules,
              SymbolSupplier *supplier,
              SourceLineResolverInterface *resolver);

  
  
  const SystemInfo *system_info_;

  
  
  MemoryRegion *memory_;

 private:
  
  
  
  
  virtual StackFrame* GetContextFrame() = 0;

  
  
  
  
  
  
  
  
  virtual StackFrame* GetCallerFrame(
      const CallStack *stack,
      const vector< linked_ptr<StackFrameInfo> > &stack_frame_info) = 0;

  
  
  const CodeModules *modules_;

  
  SymbolSupplier *supplier_;

  
  SourceLineResolverInterface *resolver_;
};


}  


#endif  
