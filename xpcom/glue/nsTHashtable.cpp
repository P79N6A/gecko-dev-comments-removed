




































#include "nsTHashtable.h"

PLDHashOperator
PL_DHashStubEnumRemove(PLDHashTable    *table,
                                       PLDHashEntryHdr *entry,
                                       PRUint32         ordinal,
                                       void            *userarg)
{
  return PL_DHASH_REMOVE;
}
