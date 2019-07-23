




































#include "nsUnicodeToJISx0208.h"
#include "nsUCVJADll.h"
#include "nsUCConstructors.h"




NS_METHOD
nsUnicodeToJISx0208Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult)
{
  return CreateTableEncoder(u2BytesCharset,
                            (uMappingTable*) g_uf0208Mapping,
                            2 ,
                            aOuter, aIID, aResult);
}

