







































#if !defined jsjaeger_codegenincs_h__ && defined JS_METHODJIT
#define jsjaeger_codegenincs_h__

#if defined JS_32BIT
# include "NunboxAssembler.h"
#elif defined JS_64BIT
# include "PunboxAssembler.h"
#else
# error "Neither JS_32BIT or JS_64BIT is defined."
#endif

#endif

