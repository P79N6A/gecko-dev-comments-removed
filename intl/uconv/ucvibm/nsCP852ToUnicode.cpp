


















#include "nsUCConstructors.h"
#include "nsCP852ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp852.ut"
};

nsresult
nsCP852ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

