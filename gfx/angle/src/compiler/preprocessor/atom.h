



















































#if !defined(__ATOM_H)
#define __ATOM_H 1

typedef struct AtomTable_Rec AtomTable;

extern AtomTable *atable;

int InitAtomTable(AtomTable *atable, int htsize);
void FreeAtomTable(AtomTable *atable);
int AddAtom(AtomTable *atable, const char *s);
void PrintAtomTable(AtomTable *atable);
int LookUpAddString(AtomTable *atable, const char *s);
const char *GetAtomString(AtomTable *atable, int atom);
int GetReversedAtom(AtomTable *atable, int atom);
char* GetStringOfAtom(AtomTable *atable, int atom);
#endif 
