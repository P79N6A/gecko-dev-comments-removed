







































#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACKWALKER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACKWALKER_H__

#include <set>
#include <string>
#include <vector>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/stack_frame_symbolizer.h"

namespace google_breakpad {

class CallStack;
class MinidumpContext;
class StackFrameSymbolizer;

using std::set;
using std::vector;

class Stackwalker {
 public:
  virtual ~Stackwalker() {}

  
  
  
  
  
  
  
  
  
  
  
  bool Walk(CallStack* stack,
            vector<const CodeModule*>* modules_without_symbols);

  
  
  
  static Stackwalker* StackwalkerForCPU(
     const SystemInfo* system_info,
     MinidumpContext* context,
     MemoryRegion* memory,
     const CodeModules* modules,
     StackFrameSymbolizer* resolver_helper);

  static void set_max_frames(uint32_t max_frames) {
    max_frames_ = max_frames;
    max_frames_set_ = true;
  }
  static uint32_t max_frames() { return max_frames_; }

  static void set_max_frames_scanned(uint32_t max_frames_scanned) {
    max_frames_scanned_ = max_frames_scanned;
  }

 protected:
  
  
  
  
  
  
  
  
  
  Stackwalker(const SystemInfo* system_info,
              MemoryRegion* memory,
              const CodeModules* modules,
              StackFrameSymbolizer* frame_symbolizer);

  
  
  
  
  
  
  
  
  bool InstructionAddressSeemsValid(uint64_t address);

  
  
  static const int kRASearchWords;

  template<typename InstructionType>
  bool ScanForReturnAddress(InstructionType location_start,
                            InstructionType* location_found,
                            InstructionType* ip_found) {
    return ScanForReturnAddress(location_start, location_found, ip_found,
                                kRASearchWords);
  }

  
  
  
  
  
  
  
  
  
  template<typename InstructionType>
  bool ScanForReturnAddress(InstructionType location_start,
                            InstructionType* location_found,
                            InstructionType* ip_found,
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

  
  
  const SystemInfo* system_info_;

  
  
  MemoryRegion* memory_;

  
  
  const CodeModules* modules_;

 protected:
  
  StackFrameSymbolizer* frame_symbolizer_;

 private:
  
  
  
  
  virtual StackFrame* GetContextFrame() = 0;

  
  
  
  
  
  
  
  
  
  
  virtual StackFrame* GetCallerFrame(const CallStack* stack,
                                     bool stack_scan_allowed) = 0;

  
  
  static uint32_t max_frames_;

  
  
  
  static bool max_frames_set_;

  
  
  
  
  static uint32_t max_frames_scanned_;
};

}  


#endif  
