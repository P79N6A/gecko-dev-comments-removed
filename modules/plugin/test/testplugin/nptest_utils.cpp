





























#include "nptest_utils.h"

#include <string.h>

NPUTF8*
createCStringFromNPVariant(const NPVariant* variant)
{
  size_t length = NPVARIANT_TO_STRING(*variant).UTF8Length;
  NPUTF8* result = (NPUTF8*)malloc(length + 1);
  memcpy(result, NPVARIANT_TO_STRING(*variant).UTF8Characters, length);
  result[length] = '\0';
  return result;
}

NPIdentifier
variantToIdentifier(NPVariant variant)
{
  if (NPVARIANT_IS_STRING(variant))
    return stringVariantToIdentifier(variant);
  else if (NPVARIANT_IS_INT32(variant))
    return int32VariantToIdentifier(variant);
  else if (NPVARIANT_IS_DOUBLE(variant))
    return doubleVariantToIdentifier(variant);
  return 0;
}

NPIdentifier
stringVariantToIdentifier(NPVariant variant)
{
  assert(NPVARIANT_IS_STRING(variant));
  NPUTF8* utf8String = createCStringFromNPVariant(&variant);
  NPIdentifier identifier = NPN_GetStringIdentifier(utf8String);
  free(utf8String);
  return identifier;
}

NPIdentifier
int32VariantToIdentifier(NPVariant variant)
{
  assert(NPVARIANT_IS_INT32(variant));
  int32 integer = NPVARIANT_TO_INT32(variant);
  return NPN_GetIntIdentifier(integer);
}

NPIdentifier
doubleVariantToIdentifier(NPVariant variant)
{
  assert(NPVARIANT_IS_DOUBLE(variant));
  double value = NPVARIANT_TO_DOUBLE(variant);
  
  int32 integer = static_cast<int32>(value);
  return NPN_GetIntIdentifier(integer);
}
