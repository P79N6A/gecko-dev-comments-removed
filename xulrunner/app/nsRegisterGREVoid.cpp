




































#include "nsRegisterGRE.h"

#include <stdlib.h>
#include <stdio.h>

int
RegisterXULRunner(PRBool aRegisterGlobally, nsIFile* aLocation,
                  const GREProperty *aProperties, PRUint32 aPropertiesLen)
{
  fprintf(stderr, "Registration not implemented on this platform!\n");
  return 1;
}

void
UnregisterXULRunner(PRBool aUnregisterGlobally, nsIFile* aLocation)
{
  fprintf(stderr, "Registration not implemented on this platform!\n");
}
