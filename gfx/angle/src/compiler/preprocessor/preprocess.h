











































# include "compiler/preprocessor/slglobals.h"
extern CPPStruct *cpp;
int InitCPPStruct(void);
int InitScanner(CPPStruct *cpp);
int InitAtomTable(AtomTable *atable, int htsize);
int ScanFromString(const char *s);
char* GetStringOfAtom(AtomTable *atable, int atom);
