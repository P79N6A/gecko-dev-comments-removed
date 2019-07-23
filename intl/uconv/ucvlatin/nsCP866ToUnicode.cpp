




































#include "nsUCConstructors.h"
#include "nsCP866ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp866.ut"
};

NS_METHOD
nsCP866ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}


