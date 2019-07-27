






#ifndef prefapi_private_data_h
#define prefapi_private_data_h

#include "mozilla/MemoryReporting.h"

extern PLDHashTable* gHashTable;
extern bool gDirty;

namespace mozilla {
namespace dom {
class PrefSetting;
} 
} 

void
pref_savePrefs(PLDHashTable* aTable, char** aPrefArray);

nsresult
pref_SetPref(const mozilla::dom::PrefSetting& aPref);

int pref_CompareStrings(const void *v1, const void *v2, void* unused);
PrefHashEntry* pref_HashTableLookup(const void *key);

void pref_GetPrefFromEntry(PrefHashEntry *aHashEntry,
                           mozilla::dom::PrefSetting* aPref);

size_t
pref_SizeOfPrivateData(mozilla::MallocSizeOf aMallocSizeOf);

#endif
