




































#include "nsUCConstructors.h"
#include "nsUnicodeToISO885913.h"




static const PRUint16 g_ufMappingTable[] = {
#include "8859-13.uf"
};

NS_METHOD
nsUnicodeToISO885913Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
