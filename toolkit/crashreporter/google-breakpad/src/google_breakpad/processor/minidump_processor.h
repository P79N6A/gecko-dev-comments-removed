




























#ifndef GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_PROCESSOR_H__
#define GOOGLE_BREAKPAD_PROCESSOR_MINIDUMP_PROCESSOR_H__

#include <assert.h>
#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

class Minidump;
class ProcessState;
class StackFrameSymbolizer;
class SourceLineResolverInterface;
class SymbolSupplier;
struct SystemInfo;

enum ProcessResult {
  PROCESS_OK,                                 
                                              
                                              

  PROCESS_ERROR_MINIDUMP_NOT_FOUND,           
                                              

  PROCESS_ERROR_NO_MINIDUMP_HEADER,           
                                              

  PROCESS_ERROR_NO_THREAD_LIST,               
                                              

  PROCESS_ERROR_GETTING_THREAD,               
                                              
                                              
                                              

  PROCESS_ERROR_GETTING_THREAD_ID,            
                                              
                                              
                                              

  PROCESS_ERROR_DUPLICATE_REQUESTING_THREADS, 
                                              
                                              

  PROCESS_SYMBOL_SUPPLIER_INTERRUPTED         
                                              
                                              
                                              
                                              
};

class MinidumpProcessor {
 public:
  
  
  MinidumpProcessor(SymbolSupplier* supplier,
                    SourceLineResolverInterface* resolver);

  
  
  
  MinidumpProcessor(SymbolSupplier* supplier,
                    SourceLineResolverInterface* resolver,
                    bool enable_exploitability);

  
  
  
  
  MinidumpProcessor(StackFrameSymbolizer* stack_frame_symbolizer,
                    bool enable_exploitability);

  ~MinidumpProcessor();

  
  ProcessResult Process(const string &minidump_file,
                        ProcessState* process_state);

  
  
  ProcessResult Process(Minidump* minidump,
                        ProcessState* process_state);
  
  
  
  
  static bool GetCPUInfo(Minidump* dump, SystemInfo* info);

  
  
  
  
  static bool GetOSInfo(Minidump* dump, SystemInfo* info);

  
  
  
  
  
  
  
  
  static string GetCrashReason(Minidump* dump, uint64_t* address);

  
  
  
  
  
  
  
  
  
  
  static bool IsErrorUnrecoverable(ProcessResult p) {
    assert(p !=  PROCESS_OK);
    return (p != PROCESS_SYMBOL_SUPPLIER_INTERRUPTED);
  }

  
  
  
  static string GetAssertion(Minidump* dump);

 private:
  StackFrameSymbolizer* frame_symbolizer_;
  
  bool own_frame_symbolizer_;

  
  
  
  bool enable_exploitability_;
};

}  

#endif  
