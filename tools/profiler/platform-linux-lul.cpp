




#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "platform.h"
#include "PlatformMacros.h"
#include "LulMain.h"
#include "shared-libraries.h"
#include "AutoObjectMapper.h"







void
read_procmaps(lul::LUL* aLUL)
{
  MOZ_ASSERT(aLUL->CountMappings() == 0);

# if defined(SPS_OS_linux) || defined(SPS_OS_android) || defined(SPS_OS_darwin)
  SharedLibraryInfo info = SharedLibraryInfo::GetInfoForSelf();

  for (size_t i = 0; i < info.GetSize(); i++) {
    const SharedLibrary& lib = info.GetEntry(i);

#   if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
    
    AutoObjectMapperFaultyLib mapper(aLUL->mLog);
#   else
    
    AutoObjectMapperPOSIX mapper(aLUL->mLog);
#   endif

    
    
    void*  image = nullptr;
    size_t size  = 0;
    bool ok = mapper.Map(&image, &size, lib.GetName());
    if (ok && image && size > 0) {
      aLUL->NotifyAfterMap(lib.GetStart(), lib.GetEnd()-lib.GetStart(),
                           lib.GetName().c_str(), image);
    } else if (!ok && lib.GetName() == "") {
      
      
      
      
      
      
      
      
      
      
      aLUL->NotifyExecutableArea(lib.GetStart(), lib.GetEnd()-lib.GetStart());
    }

    
    
  }

# else
#  error "Unknown platform"
# endif
}



void
logging_sink_for_LUL(const char* str) {
  
  size_t n = strlen(str);
  if (n > 0 && str[n-1] == '\n') {
    char* tmp = strdup(str);
    tmp[n-1] = 0;
    LOG(tmp);
    free(tmp);
  } else {
    LOG(str);
  }
}
