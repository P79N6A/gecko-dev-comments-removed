




































#define BUILTIN1(op, at0,      atr, tr, t0, cse, fold)              F_##op,
#define BUILTIN2(op, at0, at1, atr, tr, t0, t1, cse, fold)          F_##op,
#define BUILTIN3(op, at0, at1, at2, atr, tr, t0, t1, t2, cse, fold) F_##op,
#define BUILTIN4(op, at0, at1, at2, at3, atr, tr, t0, t1, t2, t3, cse, fold) F_##op,
#define BUILTIN5(op, at0, at1, at2, at3, at4, atr, tr, t0, t1, t2, t3, t4, cse, fold) F_##op,

INTERP_FOPCODE_LIST_BEGIN
#include "builtins.tbl"
INTERP_FOPCODE_LIST_END

#undef BUILTIN1
#undef BUILTIN2
#undef BUILTIN3
#undef BUILTIN4
#undef BUILTIN5
