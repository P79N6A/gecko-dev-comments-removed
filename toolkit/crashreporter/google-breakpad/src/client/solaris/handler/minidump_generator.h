






























#ifndef CLIENT_SOLARIS_HANDLER_MINIDUMP_GENERATOR_H__
#define CLIENT_SOLARIS_HANDLER_MINIDUMP_GENERATOR_H__

#if defined(sparc) || defined(__sparc__)
#define TARGET_CPU_SPARC 1
#elif defined(i386) || defined(__i386__)
#define TARGET_CPU_X86 1
#else
#error "cannot determine cpu type"
#endif

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

  
#if TARGET_CPU_SPARC
  bool WriteContext(MDRawContextSPARC *context, prgregset_t regs,
                    prfpregset_t *fp_regs);
#elif TARGET_CPU_X86
  bool WriteContext(MDRawContextX86 *context, prgregset_t regs,
                    prfpregset_t *fp_regs);
#endif 

  
  
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
