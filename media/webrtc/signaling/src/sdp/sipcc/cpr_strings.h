



#ifndef _CPR_STRINGS_H_
#define _CPR_STRINGS_H_

#include "cpr_types.h"


#include <string.h>

#if defined(_MSC_VER)
#define cpr_strcasecmp _stricmp
#define cpr_strncasecmp _strnicmp
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#else 

#define cpr_strcasecmp  strcasecmp
#define cpr_strncasecmp strncasecmp

#endif 

#endif
