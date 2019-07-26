



















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ustdio.h"
#include "unicode/ustring.h"
#include "unicode/putil.h"

#include "uprintf.h"
#include "locbund.h"

#include "cmemory.h"
#include <ctype.h>


static int32_t
u_sprintf_write(void        *context,
                const UChar *str,
                int32_t     count)
{
    u_localized_print_string *output = (u_localized_print_string *)context;
    int32_t size = ufmt_min(count, output->available);

    u_strncpy(output->str + (output->len - output->available), str, size);
    output->available -= size;
    return size;
}

static int32_t
u_sprintf_pad_and_justify(void                        *context,
                          const u_printf_spec_info    *info,
                          const UChar                 *result,
                          int32_t                     resultLen)
{
    u_localized_print_string *output = (u_localized_print_string *)context;
    int32_t written = 0;
    int32_t lengthOfResult = resultLen;

    resultLen = ufmt_min(resultLen, output->available);

    
    if(info->fWidth != -1 && resultLen < info->fWidth) {
        int32_t paddingLeft = info->fWidth - resultLen;
        int32_t outputPos = output->len - output->available;
  
        if (paddingLeft + resultLen > output->available) {
            paddingLeft = output->available - resultLen;
            if (paddingLeft < 0) {
                paddingLeft = 0;
            }
            
        }
        written += paddingLeft;

        
        if(info->fLeft) {
            written += u_sprintf_write(output, result, resultLen);
            u_memset(&output->str[outputPos + resultLen], info->fPadChar, paddingLeft);
            output->available -= paddingLeft;
        }
        
        else {
            u_memset(&output->str[outputPos], info->fPadChar, paddingLeft);
            output->available -= paddingLeft;
            written += u_sprintf_write(output, result, resultLen);
        }
    }
    
    else {
        written = u_sprintf_write(output, result, resultLen);
    }
    
    if (written >= 0 && lengthOfResult > written) {
    	return lengthOfResult;
    }

    return written;
}

U_CAPI int32_t U_EXPORT2
u_sprintf(UChar       *buffer,
          const char    *patternSpecification,
          ... )
{
    va_list ap;
    int32_t written;

    va_start(ap, patternSpecification);
    written = u_vsnprintf(buffer, INT32_MAX, patternSpecification, ap);
    va_end(ap);

    return written;
}

U_CAPI int32_t U_EXPORT2
u_sprintf_u(UChar     *buffer,
            const UChar    *patternSpecification,
            ... )
{
    va_list ap;
    int32_t written;

    va_start(ap, patternSpecification);
    written = u_vsnprintf_u(buffer, INT32_MAX, patternSpecification, ap);
    va_end(ap);

    return written;
}

U_CAPI int32_t U_EXPORT2 
u_vsprintf(UChar       *buffer,
           const char     *patternSpecification,
           va_list         ap)
{
    return u_vsnprintf(buffer, INT32_MAX, patternSpecification, ap);
}

U_CAPI int32_t U_EXPORT2
u_snprintf(UChar       *buffer,
           int32_t         count,
           const char    *patternSpecification,
           ... )
{
    va_list ap;
    int32_t written;

    va_start(ap, patternSpecification);
    written = u_vsnprintf(buffer, count, patternSpecification, ap);
    va_end(ap);

    return written;
}

U_CAPI int32_t U_EXPORT2
u_snprintf_u(UChar     *buffer,
             int32_t        count,
             const UChar    *patternSpecification,
             ... )
{
    va_list ap;
    int32_t written;

    va_start(ap, patternSpecification);
    written = u_vsnprintf_u(buffer, count, patternSpecification, ap);
    va_end(ap);

    return written;
}

U_CAPI int32_t  U_EXPORT2 
u_vsnprintf(UChar       *buffer,
            int32_t         count,
            const char     *patternSpecification,
            va_list         ap)
{
    int32_t written;
    UChar *pattern;
    UChar patBuffer[UFMT_DEFAULT_BUFFER_SIZE];
    int32_t size = (int32_t)strlen(patternSpecification) + 1;

    
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

    
    written = u_vsnprintf_u(buffer, count, pattern, ap);

    
    if (pattern != patBuffer) {
        uprv_free(pattern);
    }

    return written;
}

U_CAPI int32_t U_EXPORT2 
u_vsprintf_u(UChar       *buffer, 
             const UChar *patternSpecification, 
             va_list     ap) 
{ 
    return u_vsnprintf_u(buffer, INT32_MAX, patternSpecification, ap); 
} 

static const u_printf_stream_handler g_sprintf_stream_handler = {
    u_sprintf_write,
    u_sprintf_pad_and_justify
};

U_CAPI int32_t  U_EXPORT2 
u_vsnprintf_u(UChar    *buffer,
              int32_t        count,
              const UChar    *patternSpecification,
              va_list        ap)
{
    int32_t          written = 0;   
    int32_t			 result = 0; 

    u_localized_print_string outStr;

    if (count < 0) {
        count = INT32_MAX;
    }

    outStr.str = buffer;
    outStr.len = count;
    outStr.available = count;

    if(u_locbund_init(&outStr.fBundle, "en_US_POSIX") == 0) {
        return 0;
    }

    
    result = u_printf_parse(&g_sprintf_stream_handler, patternSpecification, &outStr, &outStr, &outStr.fBundle, &written, ap);
    
    
    if (outStr.available > 0) {
        buffer[outStr.len - outStr.available] = 0x0000;
    }

    
    u_locbund_close(&outStr.fBundle);

     
    if (result < 0) {
    	return result;
    }
    
    return written;
}

#endif 

