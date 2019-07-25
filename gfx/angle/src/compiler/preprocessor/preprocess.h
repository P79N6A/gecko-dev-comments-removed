











































#include "compiler/preprocessor/slglobals.h"
extern CPPStruct *cpp;
int InitCPPStruct(void);
int InitScanner(CPPStruct *cpp);
int InitAtomTable(AtomTable *atable, int htsize);
char* GetStringOfAtom(AtomTable *atable, int atom);
