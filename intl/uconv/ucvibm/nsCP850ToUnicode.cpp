



















































#include "nsUCConstructors.h"
#include "nsCP850ToUnicode.h"




static const PRUint16 g_utMappingTable[] = {
#include "cp850.ut"
};




NS_METHOD
nsCP850ToUnicodeConstructor(nsISupports* aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
