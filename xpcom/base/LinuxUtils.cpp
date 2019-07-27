





#include "LinuxUtils.h"

#if defined(XP_LINUX)

#include <ctype.h>
#include <stdio.h>

#include "nsPrintfCString.h"

namespace mozilla {

void
LinuxUtils::GetThreadName(pid_t aTid, nsACString& aName)
{
  aName.Truncate();
  if (aTid <= 0) {
    return;
  }

  const size_t kBuffSize = 16; 
  char buf[kBuffSize];
  nsPrintfCString path("/proc/%d/comm", aTid);
  FILE* fp = fopen(path.get(), "r");
  if (!fp) {
    
    return;
  }

  size_t len = fread(buf, 1, kBuffSize, fp);
  fclose(fp);

  
  while (len > 0 &&
         (isspace(buf[len - 1]) || isdigit(buf[len - 1]) ||
          buf[len - 1] == '#' || buf[len - 1] == '_')) {
    --len;
  }

  aName.Assign(buf, len);
}

}

#endif 
