






































#ifndef nsXREAppData_h
#define nsXREAppData_h

#include "mozilla/StandardInteger.h"

class nsILocalFile;








struct nsXREAppData
{
  




  uint32_t size;

  



  nsILocalFile* directory;

  




  const char *vendor;

  




  const char *name;

  




  const char *version;

  


  const char *buildID;

  









  const char *ID;

  



  const char *copyright;

  


  uint32_t flags;

  



  nsILocalFile* xreDirectory;

  


  const char *minVersion;
  const char *maxVersion;

  


  const char *crashReporterURL;

  














  const char *profile;
};





#define NS_XRE_ENABLE_PROFILE_MIGRATOR (1 << 1)





#define NS_XRE_ENABLE_EXTENSION_MANAGER (1 << 2)




#define NS_XRE_ENABLE_CRASH_REPORTER (1 << 3)

#endif 
