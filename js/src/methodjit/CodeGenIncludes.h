







































#if !defined jsjaeger_codegenincs_h__ && defined JS_METHODJIT
#define jsjaeger_codegenincs_h__

#if defined JS_NUNBOX32
# include "NunboxAssembler.h"
#elif defined JS_PUNBOX64
# include "PunboxAssembler.h"
#else
# error "Neither JS_NUNBOX32 nor JS_PUNBOX32 is defined."
#endif

#endif

