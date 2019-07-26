




















#ifndef __FLAGPARSER_H__
#define __FLAGPARSER_H__

#include "unicode/utypes.h"

U_CAPI int32_t U_EXPORT2
parseFlagsFile(const char *fileName, char **flagBuffer, int32_t flagBufferSize, const char ** flagNames, int32_t numOfFlags, UErrorCode *status);

#endif
