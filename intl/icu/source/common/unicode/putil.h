






















#ifndef PUTIL_H
#define PUTIL_H

#include "unicode/utypes.h"
 






































U_STABLE const char* U_EXPORT2 u_getDataDirectory(void);





















U_STABLE void U_EXPORT2 u_setDataDirectory(const char *directory);

#ifndef U_HIDE_INTERNAL_API







 
U_INTERNAL const char * U_EXPORT2 u_getTimeZoneFilesDirectory(UErrorCode *status);









U_INTERNAL void U_EXPORT2 u_setTimeZoneFilesDirectory(const char *path, UErrorCode *status);
#endif  








#if U_PLATFORM_USES_ONLY_WIN32_API
#   define U_FILE_SEP_CHAR '\\'
#   define U_FILE_ALT_SEP_CHAR '/'
#   define U_PATH_SEP_CHAR ';'
#   define U_FILE_SEP_STRING "\\"
#   define U_FILE_ALT_SEP_STRING "/"
#   define U_PATH_SEP_STRING ";"
#else
#   define U_FILE_SEP_CHAR '/'
#   define U_FILE_ALT_SEP_CHAR '/'
#   define U_PATH_SEP_CHAR ':'
#   define U_FILE_SEP_STRING "/"
#   define U_FILE_ALT_SEP_STRING "/"
#   define U_PATH_SEP_STRING ":"
#endif





















U_STABLE void U_EXPORT2
u_charsToUChars(const char *cs, UChar *us, int32_t length);




















U_STABLE void U_EXPORT2
u_UCharsToChars(const UChar *us, char *cs, int32_t length);

#endif
