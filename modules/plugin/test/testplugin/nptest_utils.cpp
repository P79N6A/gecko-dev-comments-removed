





























#include "nptest_utils.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
  int32_t integer = NPVARIANT_TO_INT32(variant);
  return NPN_GetIntIdentifier(integer);
}

NPIdentifier
doubleVariantToIdentifier(NPVariant variant)
{
  assert(NPVARIANT_IS_DOUBLE(variant));
  double value = NPVARIANT_TO_DOUBLE(variant);
  
  int32_t integer = static_cast<int32_t>(value);
  return NPN_GetIntIdentifier(integer);
}






PRUint32
parseHexColor(const char* color, int len)
{
  PRUint8 bgra[4] = { 0, 0, 0, 0xFF };
  int i = 0;

  
  while (len > 0) {
    char byte[3];
    if (len > 1) {
      
      byte[0] = color[len - 2];
      byte[1] = color[len - 1];
    }
    else {
      
      byte[0] = '0';
      byte[1] = color[len - 1];
    }
    byte[2] = '\0';

    bgra[i] = (PRUint8)(strtoul(byte, NULL, 16) & 0xFF);
    i++;
    len -= 2;
  }
  return (bgra[3] << 24) | (bgra[2] << 16) | (bgra[1] << 8) | bgra[0];
}
