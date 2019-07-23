





































#include "nsUnicodeToKSC5601.h"
#include "nsUCvKODll.h"
#include "nsUCConstructors.h"




NS_METHOD
nsUnicodeToKSC5601Constructor(nsISupports *aOuter, REFNSIID aIID,
                              void **aResult)
{
  return CreateTableEncoder(u2BytesCharset,
                             (uMappingTable*) g_ufKSC5601Mapping,
                             2 ,
                             aOuter, aIID, aResult);
}

