






#ifndef ULISTFORMATTER_H
#define ULISTFORMATTER_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#ifndef U_HIDE_DRAFT_API

#include "unicode/localpointer.h"















struct UListFormatter;
typedef struct UListFormatter UListFormatter;  

















U_DRAFT UListFormatter* U_EXPORT2
ulistfmt_open(const char*  locale,
              UErrorCode*  status);







U_DRAFT void U_EXPORT2
ulistfmt_close(UListFormatter *listfmt);


#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUListFormatterPointer, UListFormatter, ulistfmt_close);

U_NAMESPACE_END

#endif



































U_DRAFT int32_t U_EXPORT2
ulistfmt_format(const UListFormatter* listfmt,
                const UChar* const strings[],
                const int32_t *    stringLengths,
                int32_t            stringCount,
                UChar*             result,
                int32_t            resultCapacity,
                UErrorCode*        status);

#endif 
#endif 

#endif
