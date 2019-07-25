







































#if !defined jsjaeger_codegenincs_h__ && defined JS_METHODJIT
#define jsjaeger_codegenincs_h__


#ifdef DEBUG
# define DBGLABEL(name) Label name = masm.label();
# define DBGLABEL_NOMASM(name) Label name = label();
# define DBGLABEL_ASSIGN(name) name = masm.label();
#else
# define DBGLABEL(name)
# define DBGLABEL_NOMASM(name)
# define DBGLABEL_ASSIGN(name)
#endif

#if defined JS_NUNBOX32
# include "NunboxAssembler.h"
#elif defined JS_PUNBOX64
# include "PunboxAssembler.h"
#else
# error "Neither JS_NUNBOX32 nor JS_PUNBOX64 is defined."
#endif

#include "BaseAssembler.h"

#endif 

