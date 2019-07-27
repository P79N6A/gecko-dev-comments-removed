




#ifndef nsXREAppData_h
#define nsXREAppData_h

#include <stdint.h>

class nsIFile;








struct nsXREAppData
{
  




  uint32_t size;

  



  nsIFile* directory;

  




  const char* vendor;

  




  const char* name;

  




  const char* remotingName;

  




  const char* version;

  


  const char* buildID;

  









  const char* ID;

  



  const char* copyright;

  


  uint32_t flags;

  



  nsIFile* xreDirectory;

  


  const char* minVersion;
  const char* maxVersion;

  


  const char* crashReporterURL;

  














  const char* profile;

  


  const char* UAName;
};





#define NS_XRE_ENABLE_PROFILE_MIGRATOR (1 << 1)




#define NS_XRE_ENABLE_CRASH_REPORTER (1 << 3)

#endif 
