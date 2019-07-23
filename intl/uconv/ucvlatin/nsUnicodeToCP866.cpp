




































#include "nsUCConstructors.h"
#include "nsUnicodeToCP866.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp866.uf"
};

NS_METHOD
nsUnicodeToCP866Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

