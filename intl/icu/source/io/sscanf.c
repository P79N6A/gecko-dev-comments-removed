
















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_CONVERSION

#include "unicode/putil.h"
#include "unicode/ustdio.h"
#include "unicode/ustring.h"
#include "uscanf.h"
#include "ufile.h"
#include "ufmt_cmn.h"

#include "cmemory.h"
#include "cstring.h"


U_CAPI int32_t U_EXPORT2
u_sscanf(const UChar   *buffer,
         const char    *patternSpecification,
         ... )
{
    va_list ap;
    int32_t converted;

    va_start(ap, patternSpecification);
    converted = u_vsscanf(buffer, patternSpecification, ap);
    va_end(ap);

    return converted;
}

U_CAPI int32_t U_EXPORT2
u_sscanf_u(const UChar    *buffer,
           const UChar    *patternSpecification,
           ... )
{
    va_list ap;
    int32_t converted;

    va_start(ap, patternSpecification);
    converted = u_vsscanf_u(buffer, patternSpecification, ap);
    va_end(ap);

    return converted;
}

U_CAPI int32_t  U_EXPORT2 
u_vsscanf(const UChar   *buffer,
          const char    *patternSpecification,
          va_list        ap)
{
    int32_t converted;
    UChar *pattern;
    UChar patBuffer[UFMT_DEFAULT_BUFFER_SIZE];
    int32_t size = (int32_t)uprv_strlen(patternSpecification) + 1;

    
    if (size >= MAX_UCHAR_BUFFER_SIZE(patBuffer)) {
        pattern = (UChar *)uprv_malloc(size * sizeof(UChar));
        if(pattern == 0) {
            return 0;
        }
    }
    else {
        pattern = patBuffer;
    }
    u_charsToUChars(patternSpecification, pattern, size);

    
    converted = u_vsscanf_u(buffer, pattern, ap);

    
    if (pattern != patBuffer) {
        uprv_free(pattern);
    }

    return converted;
}

U_CAPI int32_t U_EXPORT2 
u_vsscanf_u(const UChar *buffer,
            const UChar *patternSpecification,
            va_list     ap)
{
    int32_t         converted;
    UFILE           inStr;

    inStr.fConverter = NULL;
    inStr.fFile = NULL;
    inStr.fOwnFile = FALSE;
#if !UCONFIG_NO_TRANSLITERATION
    inStr.fTranslit = NULL;
#endif
    inStr.fUCBuffer[0] = 0;
    inStr.str.fBuffer = (UChar *)buffer;
    inStr.str.fPos = (UChar *)buffer;
    inStr.str.fLimit = buffer + u_strlen(buffer);

    if(u_locbund_init(&inStr.str.fBundle, "en_US_POSIX") == 0) {
        return 0;
    }

    converted = u_scanf_parse(&inStr, patternSpecification, ap);

    u_locbund_close(&inStr.str.fBundle);

    
    return converted;
}

#endif 

