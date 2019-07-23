




























#ifndef GOOGLE_AIRBAG_PROCESSOR_MINIDUMP_PROCESSOR_H__
#define GOOGLE_AIRBAG_PROCESSOR_MINIDUMP_PROCESSOR_H__

#include <string>

namespace google_airbag {

using std::string;

class Minidump;
class ProcessState;
class SymbolSupplier;

class MinidumpProcessor {
 public:
  
  
  explicit MinidumpProcessor(SymbolSupplier *supplier);
  ~MinidumpProcessor();

  
  
  
  ProcessState* Process(const string &minidump_file);

  
  
  
  
  
  static string GetCPUInfo(Minidump *dump, string *cpu_info);

  
  
  
  
  
  static string GetOSInfo(Minidump *dump, string *os_version);

  
  
  
  
  
  
  
  
  static string GetCrashReason(Minidump *dump, u_int64_t *address);

 private:
  SymbolSupplier *supplier_;
};

}  

#endif  
