






#ifndef UDATEINTERVALFORMAT_H
#define UDATEINTERVALFORMAT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/umisc.h"
#include "unicode/localpointer.h"






























































struct UDateIntervalFormat;
typedef struct UDateIntervalFormat UDateIntervalFormat;  

























U_STABLE UDateIntervalFormat* U_EXPORT2
udtitvfmt_open(const char*  locale,
              const UChar* skeleton,
              int32_t      skeletonLength,
              const UChar* tzID,
              int32_t      tzIDLength,
              UErrorCode*  status);







U_STABLE void U_EXPORT2
udtitvfmt_close(UDateIntervalFormat *formatter);


#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUDateIntervalFormatPointer, UDateIntervalFormat, udtitvfmt_close);

U_NAMESPACE_END

#endif




























U_STABLE int32_t U_EXPORT2
udtitvfmt_format(const UDateIntervalFormat* formatter,
                UDate           fromDate,
                UDate           toDate,
                UChar*          result,
                int32_t         resultCapacity,
                UFieldPosition* position,
                UErrorCode*     status);

#endif 

#endif
