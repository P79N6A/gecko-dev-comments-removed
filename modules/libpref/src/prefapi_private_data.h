






































extern PLDHashTable			gHashTable;
extern bool                 gDirty;

struct PrefTuple;

enum pref_SaveTypes { SAVE_NONSHARED, SAVE_SHARED, SAVE_ALL, SAVE_ALL_AND_DEFAULTS };


struct pref_saveArgs {
  char **prefArray;
  pref_SaveTypes saveTypes;
};

PLDHashOperator
pref_savePref(PLDHashTable *table, PLDHashEntryHdr *heh, PRUint32 i, void *arg);

PLDHashOperator
pref_MirrorPrefs(PLDHashTable *table, PLDHashEntryHdr *heh, PRUint32 i, void *arg);

nsresult
pref_SetPrefTuple(const PrefTuple &aPref,bool set_default = false);

int pref_CompareStrings(const void *v1, const void *v2, void* unused);
PrefHashEntry* pref_HashTableLookup(const void *key);

void pref_GetTupleFromEntry(PrefHashEntry *aHashEntry, PrefTuple *aTuple);
