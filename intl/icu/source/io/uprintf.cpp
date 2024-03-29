



















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_CONVERSION

#include "unicode/ustdio.h"
#include "unicode/ustring.h"
#include "unicode/unum.h"
#include "unicode/udat.h"
#include "unicode/putil.h"

#include "cmemory.h"
#include "locbund.h"
#include "mutex.h"
#include "uassert.h"
#include "uprintf.h"
#include "ufile.h"
#include "ucln_io.h"

U_NAMESPACE_USE

static UFILE *gStdOut = NULL;
static UInitOnce gStdOutInitOnce = U_INITONCE_INITIALIZER;

static UBool U_CALLCONV uprintf_cleanup(void)
{
    if (gStdOut != NULL) {
        u_fclose(gStdOut);
        gStdOut = NULL;
    }
    gStdOutInitOnce.reset();
    return TRUE;
}

static void U_CALLCONV u_stdout_init() {
    U_ASSERT(gStdOut ==  NULL);
    gStdOut = u_finit(stdout, NULL, NULL);
    ucln_io_registerCleanup(UCLN_IO_PRINTF, &uprintf_cleanup);
}

U_CAPI UFILE * U_EXPORT2
u_get_stdout()
{
    umtx_initOnce(gStdOutInitOnce, &u_stdout_init);
    return gStdOut;
}

static int32_t U_EXPORT2
u_printf_write(void          *context,
               const UChar   *str,
               int32_t       count)
{
    return u_file_write(str, count, (UFILE *)context);
}

static int32_t
u_printf_pad_and_justify(void                        *context,
                         const u_printf_spec_info    *info,
                         const UChar                 *result,
                         int32_t                     resultLen)
{
    UFILE   *output = (UFILE *)context;
    int32_t written, i;

    
    if(info->fWidth != -1 && resultLen < info->fWidth) {
        
        if(info->fLeft) {
            written = u_file_write(result, resultLen, output);
            for(i = 0; i < info->fWidth - resultLen; ++i) {
                written += u_file_write(&info->fPadChar, 1, output);
            }
        }
        
        else {
            written = 0;
            for(i = 0; i < info->fWidth - resultLen; ++i) {
                written += u_file_write(&info->fPadChar, 1, output);
            }
            written += u_file_write(result, resultLen, output);
        }
    }
    
    else {
        written = u_file_write(result, resultLen, output);
    }

    return written;
}

U_CAPI int32_t U_EXPORT2 
u_fprintf(    UFILE        *f,
          const char    *patternSpecification,
          ... )
{
    va_list ap;
    int32_t count;

    va_start(ap, patternSpecification);
    count = u_vfprintf(f, patternSpecification, ap);
    va_end(ap);

    return count;
}

U_CAPI int32_t U_EXPORT2
u_printf(const char *patternSpecification,
         ...)
{
    va_list ap;
    int32_t count;
    va_start(ap, patternSpecification);
    count = u_vfprintf(u_get_stdout(), patternSpecification, ap);
    va_end(ap);
    return count;
}

U_CAPI int32_t U_EXPORT2 
u_fprintf_u(    UFILE        *f,
            const UChar    *patternSpecification,
            ... )
{
    va_list ap;
    int32_t count;

    va_start(ap, patternSpecification);
    count = u_vfprintf_u(f, patternSpecification, ap);
    va_end(ap);

    return count;
}

U_CAPI int32_t U_EXPORT2
u_printf_u(const UChar *patternSpecification,
           ...)
{
    va_list ap;
    int32_t count;
    va_start(ap, patternSpecification);
    count = u_vfprintf_u(u_get_stdout(), patternSpecification, ap);
    va_end(ap);
    return count;
}

U_CAPI int32_t  U_EXPORT2 
u_vfprintf(    UFILE        *f,
           const char    *patternSpecification,
           va_list        ap)
{
    int32_t count;
    UChar *pattern;
    UChar buffer[UFMT_DEFAULT_BUFFER_SIZE];
    size_t size = strlen(patternSpecification) + 1;

    
    if (size >= MAX_UCHAR_BUFFER_SIZE(buffer)) {
        pattern = (UChar *)uprv_malloc(size * sizeof(UChar));
        if(pattern == 0) {
            return 0;
        }
    }
    else {
        pattern = buffer;
    }
    u_charsToUChars(patternSpecification, pattern, size);

    
    count = u_vfprintf_u(f, pattern, ap);

    
    if (pattern != buffer) {
        uprv_free(pattern);
    }

    return count;
}

static const u_printf_stream_handler g_stream_handler = {
    u_printf_write,
    u_printf_pad_and_justify
};

U_CAPI int32_t  U_EXPORT2 
u_vfprintf_u(    UFILE        *f,
             const UChar    *patternSpecification,
             va_list        ap)
{
    int32_t          written = 0;   

    
    u_printf_parse(&g_stream_handler, patternSpecification, f, NULL, &f->str.fBundle, &written, ap);

    
    return written;
}

#endif 

