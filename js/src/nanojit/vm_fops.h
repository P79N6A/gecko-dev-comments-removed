




































#define BUILTIN1(op, at0,      atr, tr, t0)              F_##op,
#define BUILTIN2(op, at0, at1, atr, tr, t0, t1)          F_##op,
#define BUILTIN3(op, at0, at1, at2, atr, tr, t0, t1, t2) F_##op,

INTERP_FOPCODE_LIST_BEGIN
#include "builtins.tbl"
INTERP_FOPCODE_LIST_END

#undef BUILTIN1
#undef BUILTIN2
#undef BUILTIN3
