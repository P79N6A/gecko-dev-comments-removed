




































#ifndef nsRegisterGRE_h__
#define nsRegisterGRE_h__

#include "nscore.h"
#include "nsStringAPI.h"
class nsIFile;
struct GREProperty;




NS_HIDDEN_(PRBool)
RegisterXULRunner(PRBool aRegisterGlobally, nsIFile* aLocation,
                  const GREProperty *aProperties, PRUint32 aPropertiesLen,
                  const char *aGREMilestone);

NS_HIDDEN_(void)
UnregisterXULRunner(PRBool aUnregisterGlobally, nsIFile* aLocation,
                    const char *aGREMilestone);

#endif 
