













#ifndef UMISC_H
#define UMISC_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING



extern const UChar  *g_umsgTypeList[];
extern const UChar  *g_umsgModifierList[];
extern const UChar  *g_umsgDateModifierList[];
extern const int32_t g_umsgListLength;

extern const UChar g_umsg_number[];
extern const UChar g_umsg_date[];
extern const UChar g_umsg_time[];
extern const UChar g_umsg_choice[];

extern const UChar g_umsg_currency[];
extern const UChar g_umsg_percent[];
extern const UChar g_umsg_integer[];

extern const UChar g_umsg_short[];
extern const UChar g_umsg_medium[];
extern const UChar g_umsg_long[];
extern const UChar g_umsg_full[];

#endif 

#endif
