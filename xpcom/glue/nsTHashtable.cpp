




































#include "nsTHashtable.h"
#include "nsHashKeys.h"

PRUint32
HashString( const nsAString& aStr )
{
  PRUint32 code = 0;

#ifdef MOZILLA_INTERNAL_API
  nsAString::const_iterator begin, end;
  aStr.BeginReading(begin);
  aStr.EndReading(end);
#else
  const PRUnichar *begin, *end;
  PRUint32 len = NS_StringGetData(aStr, &begin);
  end = begin + len;
#endif

  while (begin != end) {
    code = (code>>28) ^ (code<<4) ^ PRUint32(*begin);
    ++begin;
  }

  return code;
}

PRUint32
HashString( const nsACString& aStr )
{
  PRUint32 code = 0;

#ifdef MOZILLA_INTERNAL_API
  nsACString::const_iterator begin, end;
  aStr.BeginReading(begin);
  aStr.EndReading(end);
#else
  const char *begin, *end;
  PRUint32 len = NS_CStringGetData(aStr, &begin);
  end = begin + len;
#endif

  while (begin != end) {
    code = (code>>28) ^ (code<<4) ^ PRUint32(*begin);
    ++begin;
  }

  return code;
}

PRUint32
HashString(const char *str)
{
  PRUint32 code = 0;

  while (*str) {
    code = (code>>28) ^ (code<<4) ^ PRUint32(*str);
    ++str;
  }

  return code;
}

PRUint32
HashString(const PRUnichar *str)
{
  PRUint32 code = 0;

  while (*str) {
    code = (code>>28) ^ (code<<4) ^ PRUint32(*str);
    ++str;
  }

  return code;
}

PLDHashOperator
PL_DHashStubEnumRemove(PLDHashTable    *table,
                                       PLDHashEntryHdr *entry,
                                       PRUint32         ordinal,
                                       void            *userarg)
{
  return PL_DHASH_REMOVE;
}

PRUint32 nsIDHashKey::HashKey(const nsID* id)
{
  PRUint32 h = id->m0;
  PRUint32 i;

  h = (h>>28) ^ (h<<4) ^ id->m1;
  h = (h>>28) ^ (h<<4) ^ id->m2;

  for (i = 0; i < 8; i++)
    h = (h>>28) ^ (h<<4) ^ id->m3[i];

  return h;
}
