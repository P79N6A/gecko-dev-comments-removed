






























#ifndef CLIENT_LINUX_HANDLER_MINIDUMP_GENERATOR_H__
#define CLIENT_LINUX_HANDLER_MINIDUMP_GENERATOR_H__

#include <stdint.h>

#include "google_breakpad/common/breakpad_types.h"
#include "processor/scoped_ptr.h"

struct sigcontext;

namespace google_breakpad {







class MinidumpGenerator {
  public:
   MinidumpGenerator();

   ~MinidumpGenerator();

   
   bool WriteMinidumpToFile(const char *file_pathname,
                            int signo,
                            uintptr_t sighandler_ebp,
                            struct sigcontext **sig_ctx) const;
  private:
   
   void AllocateStack();

  private:
   
   static const int kStackSize = 1024 * 1024;
   scoped_array<char> stack_;
};

}  

#endif   
