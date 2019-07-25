




































#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "prbit.h"
#include "mozilla/HashFunctions.h"

using namespace mozilla;

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
    code = AddToHash(code, *begin);
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
    code = AddToHash(code, *begin);
    ++begin;
  }

  return code;
}

PRUint32
HashString(const char *str)
{
  PRUint32 code = 0;

  while (*str) {
    code = AddToHash(code, *str);
    ++str;
  }

  return code;
}

PRUint32
HashString(const PRUnichar *str)
{
  PRUint32 code = 0;

  while (*str) {
    code = AddToHash(code, *str);
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

  h = PR_ROTATE_LEFT32(h, 4) ^ id->m1;
  h = PR_ROTATE_LEFT32(h, 4) ^ id->m2;

  for (i = 0; i < 8; i++)
    h = PR_ROTATE_LEFT32(h, 4) ^ id->m3[i];

  return h;
}
