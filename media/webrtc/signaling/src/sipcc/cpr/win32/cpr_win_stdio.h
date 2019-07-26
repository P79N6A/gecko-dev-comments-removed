



#ifndef _CPR_WIN_STDIO_H_
#define _CPR_WIN_STDIO_H_

#include "cpr_types.h"
#include <stdio.h>


#define snprintf cpr_win_snprintf
int cpr_win_snprintf(char *buffer, size_t n, const char *format, ...);

#endif
