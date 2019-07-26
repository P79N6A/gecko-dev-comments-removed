







































#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACKWALKER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACKWALKER_H__

#include <set>
#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/memory_region.h"

namespace google_breakpad {

class CallStack;
class MinidumpContext;
class SourceLineResolverInterface;
struct StackFrame;
class SymbolSupplier;
struct SystemInfo;

using std::set;


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

  static void set_max_frames(u_int32_t max_frames) { max_frames_ = max_frames; }
  static u_int32_t max_frames() { return max_frames_; }

 protected:
  
  
  
  
  
  
  
  
  
  Stackwalker(const SystemInfo *system_info,
              MemoryRegion *memory,
              const CodeModules *modules,
              SymbolSupplier *supplier,
              SourceLineResolverInterface *resolver);

  
  
  
  
  
  
  
  
  bool InstructionAddressSeemsValid(u_int64_t address);

  template<typename InstructionType>
  bool ScanForReturnAddress(InstructionType location_start,
                            InstructionType *location_found,
                            InstructionType *ip_found) {
    const int kRASearchWords = 30;
    return ScanForReturnAddress(location_start, location_found, ip_found,
                                kRASearchWords);
  }

  
  
  
  
  
  
  
  
  
  template<typename InstructionType>
  bool ScanForReturnAddress(InstructionType location_start,
                            InstructionType *location_found,
                            InstructionType *ip_found,
                            int searchwords) {
    for (InstructionType location = location_start;
         location <= location_start + searchwords * sizeof(InstructionType);
         location += sizeof(InstructionType)) {
      InstructionType ip;
      if (!memory_->GetMemoryAtAddress(location, &ip))
        break;

      if (modules_ && modules_->GetModuleForAddress(ip) &&
          InstructionAddressSeemsValid(ip)) {

        *ip_found = ip;
        *location_found = location;
        return true;
      }
    }
    
    return false;
  }

  
  
  const SystemInfo *system_info_;

  
  
  MemoryRegion *memory_;

  
  
  const CodeModules *modules_;

 protected:
  
  SourceLineResolverInterface *resolver_;

 private:
  
  
  
  
  virtual StackFrame* GetContextFrame() = 0;

  
  
  
  
  
  
  
  
  virtual StackFrame* GetCallerFrame(const CallStack *stack) = 0;

  
  SymbolSupplier *supplier_;

  
  
  
  set<string> no_symbol_modules_;

  
  
  static u_int32_t max_frames_;
};


}  


#endif  
