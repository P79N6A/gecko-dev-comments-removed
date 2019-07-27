





#ifndef mozilla_LinuxUtils_h
#define mozilla_LinuxUtils_h

#if defined(XP_LINUX)

#include <unistd.h>
#include "nsString.h"

namespace mozilla {

class LinuxUtils
{
public:
  
  
  
  
  
  
  
  static void GetThreadName(pid_t aTid, nsACString& aName);
};

}

#endif 

#endif
