







































#if !defined jsjaeger_codegenincs_h__ && defined JS_METHODJIT
#define jsjaeger_codegenincs_h__

#if defined JS_NUNBOX32
# include "NunboxAssembler.h"
#elif defined JS_PUNBOX64
# include "PunboxAssembler.h"
#else
# error "Neither JS_NUNBOX32 nor JS_PUNBOX64 is defined."
#endif


#ifdef DEBUG
# define DBGLABEL(name) Label name = masm.label();
# define DBGLABEL_ASSIGN(name) name = masm.label();
#else
# define DBGLABEL(name)
# define DBGLABEL_ASSIGN(name)
#endif

#endif 

