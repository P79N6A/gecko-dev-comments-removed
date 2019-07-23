




































#ifndef nsRegisterGRE_h__
#define nsRegisterGRE_h__

#include "nscore.h"
#include "nsStringAPI.h"
class nsIFile;
struct GREProperty;




NS_HIDDEN_(PRBool)
RegisterXULRunner(PRBool aRegisterGlobally, nsIFile* aLocation,
                  const GREProperty *aProperties, PRUint32 aPropertiesLen);

NS_HIDDEN_(void)
UnregisterXULRunner(PRBool aUnregisterGlobally, nsIFile* aLocation);



NS_HIDDEN_(nsresult)
GetGREVersion(const char *argv0, nsACString *aMilestone, nsACString *aVersion);

#endif 
