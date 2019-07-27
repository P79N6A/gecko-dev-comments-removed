






#ifndef prefapi_private_data_h
#define prefapi_private_data_h

#include "mozilla/MemoryReporting.h"

extern PLDHashTable			gHashTable;
extern bool                 gDirty;

namespace mozilla {
namespace dom {
class PrefSetting;
}
}

enum pref_SaveTypes { SAVE_NONSHARED, SAVE_SHARED, SAVE_ALL, SAVE_ALL_AND_DEFAULTS };


struct pref_saveArgs {
  char **prefArray;
  pref_SaveTypes saveTypes;
};

PLDHashOperator
pref_savePref(PLDHashTable *table, PLDHashEntryHdr *heh, uint32_t i, void *arg);

PLDHashOperator
pref_GetPrefs(PLDHashTable *table,
              PLDHashEntryHdr *heh, uint32_t i, void *arg);

nsresult
pref_SetPref(const mozilla::dom::PrefSetting& aPref);

int pref_CompareStrings(const void *v1, const void *v2, void* unused);
PrefHashEntry* pref_HashTableLookup(const void *key);

void pref_GetPrefFromEntry(PrefHashEntry *aHashEntry,
                           mozilla::dom::PrefSetting* aPref);

size_t
pref_SizeOfPrivateData(mozilla::MallocSizeOf aMallocSizeOf);

#endif
