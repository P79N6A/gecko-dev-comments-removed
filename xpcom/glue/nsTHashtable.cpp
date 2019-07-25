




#include "nsTHashtable.h"

PLDHashOperator
PL_DHashStubEnumRemove(PLDHashTable    *table,
                                       PLDHashEntryHdr *entry,
                                       uint32_t         ordinal,
                                       void            *userarg)
{
  return PL_DHASH_REMOVE;
}
