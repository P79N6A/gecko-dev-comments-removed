




























#ifndef GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_PROCESSOR_H__
#define GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_PROCESSOR_H__

#include <cassert>
#include <string>
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

using std::string;

class Minidump;
class ProcessState;
class SourceLineResolverInterface;
class SymbolSupplier;
class SystemInfo;

enum ProcessResult {
  PROCESS_OK,                                 
                                              
                                              

  PROCESS_ERROR_MINIDUMP_NOT_FOUND,           
                                              

  PROCESS_ERROR_NO_MINIDUMP_HEADER,           
                                              

  PROCESS_ERROR_NO_THREAD_LIST,               
                                              

  PROCESS_ERROR_GETTING_THREAD,               
                                              
                                              
                                              

  PROCESS_ERROR_GETTING_THREAD_ID,            
                                              
                                              
                                              

  PROCESS_ERROR_DUPLICATE_REQUESTING_THREADS, 
                                              
                                              

  PROCESS_ERROR_NO_MEMORY_FOR_THREAD,         
                                              

  PROCESS_ERROR_NO_STACKWALKER_FOR_THREAD,    
                                              
                                              
                                              
                                              

  PROCESS_SYMBOL_SUPPLIER_INTERRUPTED         
                                              
                                              
                                              
                                              
};

class MinidumpProcessor {
 public:
  
  
  MinidumpProcessor(SymbolSupplier *supplier,
                    SourceLineResolverInterface *resolver);
  ~MinidumpProcessor();

  
  ProcessResult Process(const string &minidump_file,
                        ProcessState *process_state);

  
  
  ProcessResult Process(Minidump *minidump,
                        ProcessState *process_state);
  
  
  
  
  static bool GetCPUInfo(Minidump *dump, SystemInfo *info);

  
  
  
  
  static bool GetOSInfo(Minidump *dump, SystemInfo *info);

  
  
  
  
  
  
  
  
  static string GetCrashReason(Minidump *dump, u_int64_t *address);

  
  
  
  
  
  
  
  
  
  
  static bool IsErrorUnrecoverable(ProcessResult p) {
    assert(p !=  PROCESS_OK);
    return (p != PROCESS_SYMBOL_SUPPLIER_INTERRUPTED);
  }

  
  
  
  static string GetAssertion(Minidump *dump);

 private:
  SymbolSupplier *supplier_;
  SourceLineResolverInterface *resolver_;
};

}  

#endif  
