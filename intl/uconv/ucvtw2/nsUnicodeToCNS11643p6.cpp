




































#include "nsUnicodeToCNS11643p6.h"
#include "nsUCvTW2Dll.h"
#include "nsUCConstructors.h"




NS_METHOD
nsUnicodeToCNS11643p6Constructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult) 
{
  return CreateTableEncoder(u2BytesCharset,
                            (uMappingTable*) &g_ufCNS6MappingTable,
                            2 ,
                            aOuter, aIID, aResult);
}

