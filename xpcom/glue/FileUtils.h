




#ifndef mozilla_FileUtils_h
#define mozilla_FileUtils_h

#include "nscore.h" 

#if defined(XP_UNIX) || defined(XP_OS2)
# include <unistd.h>
#elif defined(XP_WIN)
# include <io.h>
#endif
#include "prio.h"

#include "mozilla/Scoped.h"
#include <errno.h>

namespace mozilla {






struct ScopedClosePRFDTraits
{
  typedef PRFileDesc* type;
  static type empty() { return NULL; }
  static void release(type fd) {
    if (fd != NULL) {
      PR_Close(fd);
    }
  }
};
typedef Scoped<ScopedClosePRFDTraits> AutoFDClose;






struct ScopedCloseFDTraits
{
  typedef int type;
  static type empty() { return -1; }
  static void release(type fd) {
    if (fd != -1) {
      while ((close(fd) == -1) && (errno == EINTR)) {
        ;
      }
    }
  }
};
typedef Scoped<ScopedCloseFDTraits> ScopedClose;










NS_COM_GLUE bool fallocate(PRFileDesc *aFD, int64_t aLength);

} 
#endif
