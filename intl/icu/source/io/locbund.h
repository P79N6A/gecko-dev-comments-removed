

















#ifndef LOCBUND_H
#define LOCBUND_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unum.h"

#define ULOCALEBUNDLE_NUMBERFORMAT_COUNT ((int32_t)UNUM_SPELLOUT)

typedef struct ULocaleBundle {
    char            *fLocale;

    UNumberFormat   *fNumberFormat[ULOCALEBUNDLE_NUMBERFORMAT_COUNT];
    UBool           isInvariantLocale;
} ULocaleBundle;








U_CAPI ULocaleBundle *
u_locbund_init(ULocaleBundle *result, const char *loc);





















U_CAPI void
u_locbund_close(ULocaleBundle *bundle);






U_CAPI UNumberFormat *
u_locbund_getNumberFormat(ULocaleBundle *bundle, UNumberFormatStyle style);

#endif 

#endif
