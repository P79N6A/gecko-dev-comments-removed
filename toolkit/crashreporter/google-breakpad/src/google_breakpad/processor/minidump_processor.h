




























#ifndef GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_PROCESSOR_H__
#define GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_PROCESSOR_H__

#include <string>
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

using std::string;

class Minidump;
class ProcessState;
class SourceLineResolverInterface;
class SymbolSupplier;
class SystemInfo;

class MinidumpProcessor {
 public:
  
  enum ProcessResult {
    PROCESS_OK,           
    PROCESS_ERROR,        
    PROCESS_INTERRUPTED   
  };

  
  
  MinidumpProcessor(SymbolSupplier *supplier,
                    SourceLineResolverInterface *resolver);
  ~MinidumpProcessor();

  
  ProcessResult Process(const string &minidump_file,
                        ProcessState *process_state);

  
  
  
  
  static bool GetCPUInfo(Minidump *dump, SystemInfo *info);

  
  
  
  
  static bool GetOSInfo(Minidump *dump, SystemInfo *info);

  
  
  
  
  
  
  
  
  static string GetCrashReason(Minidump *dump, u_int64_t *address);

 private:
  SymbolSupplier *supplier_;
  SourceLineResolverInterface *resolver_;
};

}  

#endif
