




#include "nsUCConstructors.h"
#include "nsCP866ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp866.ut"
};

nsresult
nsCP866ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}


