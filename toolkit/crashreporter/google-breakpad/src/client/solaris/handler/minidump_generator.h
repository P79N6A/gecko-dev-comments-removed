






























#ifndef CLIENT_SOLARIS_HANDLER_MINIDUMP_GENERATOR_H__
#define CLIENT_SOLARIS_HANDLER_MINIDUMP_GENERATOR_H__

#include <ucontext.h>

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
                           int signo,
                           uintptr_t sighandler_ebp,
                           ucontext_t **sig_ctx) const;
};

}  

#endif   
