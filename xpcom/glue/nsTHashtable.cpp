




#include "nsTHashtable.h"

PLDHashOperator
PL_DHashStubEnumRemove(PLDHashTable*    aTable,
                       PLDHashEntryHdr* aEntry,
                       uint32_t         aOrdinal,
                       void*            aUserarg)
{
  return PL_DHASH_REMOVE;
}
