




































#include "nsUnicodeToCNS11643p2.h"
#include "nsUCvTW2Dll.h"
#include "nsUCConstructors.h"





NS_METHOD
nsUnicodeToCNS11643p2Constructor(nsISupports* aOuter, REFNSIID aIID,
                                 void **aResult)
{
  return CreateTableEncoder(u2BytesCharset,
                            (uMappingTable*) &g_ufCNS2MappingTable,
                            2 ,
                            aOuter, aIID, aResult);
}

