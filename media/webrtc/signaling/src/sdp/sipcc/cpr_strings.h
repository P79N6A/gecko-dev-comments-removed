



#ifndef _CPR_STRINGS_H_
#define _CPR_STRINGS_H_

#include "cpr_types.h"

__BEGIN_DECLS


#include <string.h>

#ifdef _MSC_VER

#define cpr_strcasecmp _stricmp
#define cpr_strncasecmp _strnicmp
#define snprintf _snprintf

#else 

#define cpr_strcasecmp  strcasecmp
#define cpr_strncasecmp strncasecmp

#endif 

__END_DECLS

#endif
