






























#ifndef CLIENT_SOLARIS_HANDLER_MINIDUMP_GENERATOR_H__
#define CLIENT_SOLARIS_HANDLER_MINIDUMP_GENERATOR_H__

#include "client/minidump_file_writer.h"
#include "client/solaris/handler/solaris_lwp.h"
#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {






class MinidumpGenerator {
  
  friend bool LwpInformationCallback(lwpstatus_t *lsp, void *context);

  
  friend bool ModuleInfoCallback(const ModuleInfo &module_info, void *context);

 public:
  MinidumpGenerator();

  ~MinidumpGenerator();

  
  bool WriteMinidumpToFile(const char *file_pathname,
                           int signo);

 private:
  
  bool WriteCVRecord(MDRawModule *module, const char *module_path);

  
  bool WriteLwpStack(uintptr_t last_esp, UntypedMDRVA *memory,
                     MDMemoryDescriptor *loc);

  
  bool WriteContext(MDRawContextX86 *context, prgregset_t regs,
                    prfpregset_t *fp_regs);

  
  
  bool WriteLwpStream(lwpstatus_t *lsp, MDRawThread *lwp);

  
  bool WriteCPUInformation(MDRawSystemInfo *sys_info);

  
  bool WriteOSInformation(MDRawSystemInfo *sys_info);

  typedef bool (MinidumpGenerator::*WriteStreamFN)(MDRawDirectory *);

  
  void *Write();

  
  bool WriteLwpListStream(MDRawDirectory *dir);
  bool WriteModuleListStream(MDRawDirectory *dir);
  bool WriteSystemInfoStream(MDRawDirectory *dir);
  bool WriteExceptionStream(MDRawDirectory *dir);
  bool WriteMiscInfoStream(MDRawDirectory *dir);
  bool WriteBreakpadInfoStream(MDRawDirectory *dir);

 private:
  MinidumpFileWriter writer_;

  
  int requester_pid_;

  
  int signo_;

  
  SolarisLwp *lwp_lister_;
};

}  

#endif   
