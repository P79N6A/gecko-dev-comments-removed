




































#include "nsUnicodeToCNS11643p4.h"
#include "nsUCvTW2Dll.h"
#include "nsUCConstructors.h"





NS_METHOD
nsUnicodeToCNS11643p4Constructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult)
{
  return CreateTableEncoder(u2BytesCharset,
                            (uMappingTable*) &g_ufCNS4MappingTable,
                            2 ,
                            aOuter, aIID, aResult);
}

