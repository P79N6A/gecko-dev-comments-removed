





























#ifndef nptest_utils_h_
#define nptest_utils_h_

#include "npipctest.h"

NPUTF8* createCStringFromNPVariant(const NPVariant* variant);

NPIdentifier variantToIdentifier(NPVariant variant);
NPIdentifier stringVariantToIdentifier(NPVariant variant);
NPIdentifier int32VariantToIdentifier(NPVariant variant);
NPIdentifier doubleVariantToIdentifier(NPVariant variant);

PRUint32 parseHexColor(char* color);

#endif 
