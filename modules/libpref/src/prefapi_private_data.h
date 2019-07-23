






































extern PLDHashTable			gHashTable;
extern PRBool               gDirty;

enum pref_SaveTypes { SAVE_NONSHARED, SAVE_SHARED, SAVE_ALL };


struct pref_saveArgs {
  char **prefArray;
  pref_SaveTypes saveTypes;
};

PLDHashOperator
pref_savePref(PLDHashTable *table, PLDHashEntryHdr *heh, PRUint32 i, void *arg);

int PR_CALLBACK pref_CompareStrings(const void *v1, const void *v2, void* unused);
