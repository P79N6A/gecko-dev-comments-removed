






















#if defined(__GNUC__) && defined(__STRICT_ANSI__)
#undef __STRICT_ANSI__
#endif

#include "locmap.h"
#include "unicode/ustdio.h"

#if !UCONFIG_NO_CONVERSION

#include "ufile.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "cmemory.h"

#if U_PLATFORM_USES_ONLY_WIN32_API && !defined(fileno)

#define fileno _fileno
#endif

static UFILE*
finit_owner(FILE         *f,
              const char *locale,
              const char *codepage,
              UBool       takeOwnership
              )
{
    UErrorCode status = U_ZERO_ERROR;
    UFILE     *result;
    if(f == NULL) {
        return 0;
    }
    result = (UFILE*) uprv_malloc(sizeof(UFILE));
    if(result == NULL) {
        return 0;
    }

    uprv_memset(result, 0, sizeof(UFILE));
    result->fFileno = fileno(f);

#if U_PLATFORM_USES_ONLY_WIN32_API
    if (0 <= result->fFileno && result->fFileno <= 2) {
        
#if _MSC_VER >= 1400
        result->fFile = &__iob_func()[_fileno(f)];
#else
        result->fFile = &_iob[_fileno(f)];
#endif
    }
    else
#endif
    {
        result->fFile = f;
    }

    result->str.fBuffer = result->fUCBuffer;
    result->str.fPos    = result->fUCBuffer;
    result->str.fLimit  = result->fUCBuffer;

#if !UCONFIG_NO_FORMATTING
        
        if(u_locbund_init(&result->str.fBundle, locale) == 0) {
            
            uprv_free(result);
            return 0;
        }
#endif

    
    if(codepage == NULL || *codepage != '\0') {
        result->fConverter = ucnv_open(codepage, &status);
    }
    

    if(U_SUCCESS(status)) {
        result->fOwnFile = takeOwnership;
    }
    else {
#if !UCONFIG_NO_FORMATTING
        u_locbund_close(&result->str.fBundle);
#endif
        
        uprv_free(result);
        result = NULL;
    }

    return result;
}

U_CAPI UFILE* U_EXPORT2 
u_finit(FILE          *f,
        const char    *locale,
        const char    *codepage)
{
    return finit_owner(f, locale, codepage, FALSE);
}

U_CAPI UFILE* U_EXPORT2
u_fadopt(FILE         *f,
        const char    *locale,
        const char    *codepage)
{
    return finit_owner(f, locale, codepage, TRUE);
}

U_CAPI UFILE* U_EXPORT2 
u_fopen(const char    *filename,
        const char    *perm,
        const char    *locale,
        const char    *codepage)
{
    UFILE     *result;
    FILE     *systemFile = fopen(filename, perm);
    if(systemFile == 0) {
        return 0;
    }

    result = finit_owner(systemFile, locale, codepage, TRUE);

    if (!result) {
        

        fclose(systemFile);
    }

    return result; 
}

U_CAPI UFILE* U_EXPORT2
u_fopen_u(const UChar   *filename,
        const char    *perm,
        const char    *locale,
        const char    *codepage)
{
    UFILE     *result;
    char buffer[256];

    u_austrcpy(buffer, filename);

    result = u_fopen(buffer, perm, locale, codepage);
#if U_PLATFORM_USES_ONLY_WIN32_API
    
    if (!result) {
        FILE *systemFile = _wfopen(filename, (UChar*)perm);
        if (systemFile) {
            result = finit_owner(systemFile, locale, codepage, TRUE);
        }
        if (!result) {
            

            fclose(systemFile);
        }
    }
#endif
    return result; 
}

U_CAPI UFILE* U_EXPORT2
u_fstropen(UChar *stringBuf,
           int32_t      capacity,
           const char  *locale)
{
    UFILE *result;

    if (capacity < 0) {
        return NULL;
    }

    result = (UFILE*) uprv_malloc(sizeof(UFILE));
    
    if (result == NULL) {
    	return NULL; 
    }
    uprv_memset(result, 0, sizeof(UFILE));
    result->str.fBuffer = stringBuf;
    result->str.fPos    = stringBuf;
    result->str.fLimit  = stringBuf+capacity;

#if !UCONFIG_NO_FORMATTING
    
    if(u_locbund_init(&result->str.fBundle, locale) == 0) {
        
        uprv_free(result);
        return 0;
    }
#endif

    return result;
}

U_CAPI UBool U_EXPORT2
u_feof(UFILE  *f)
{
    UBool endOfBuffer;
    if (f == NULL) {
        return TRUE;
    }
    endOfBuffer = (UBool)(f->str.fPos >= f->str.fLimit);
    if (f->fFile != NULL) {
        return endOfBuffer && feof(f->fFile);
    }
    return endOfBuffer;
}

U_CAPI void U_EXPORT2
u_fflush(UFILE *file)
{
    ufile_flush_translit(file);
    ufile_flush_io(file);
    if (file->fFile) {
        fflush(file->fFile);
    }
    else if (file->str.fPos < file->str.fLimit) {
        *(file->str.fPos++) = 0;
    }
    
}

U_CAPI void
u_frewind(UFILE *file)
{
    u_fflush(file);
    ucnv_reset(file->fConverter);
    if (file->fFile) {
        rewind(file->fFile);
        file->str.fLimit = file->fUCBuffer;
        file->str.fPos   = file->fUCBuffer;
    }
    else {
        file->str.fPos = file->str.fBuffer;
    }
}

U_CAPI void U_EXPORT2 
u_fclose(UFILE *file)
{
    if (file) {
        u_fflush(file);
        ufile_close_translit(file);

        if(file->fOwnFile)
            fclose(file->fFile);

#if !UCONFIG_NO_FORMATTING
        u_locbund_close(&file->str.fBundle);
#endif

        ucnv_close(file->fConverter);
        uprv_free(file);
    }
}

U_CAPI FILE* U_EXPORT2 
u_fgetfile(    UFILE         *f)
{
    return f->fFile;
}

#if !UCONFIG_NO_FORMATTING

U_CAPI const char*  U_EXPORT2 
u_fgetlocale(    UFILE        *file)
{
    return file->str.fBundle.fLocale;
}

U_CAPI int32_t U_EXPORT2 
u_fsetlocale(UFILE      *file,
             const char *locale)
{
    u_locbund_close(&file->str.fBundle);

    return u_locbund_init(&file->str.fBundle, locale) == 0 ? -1 : 0;
}

#endif

U_CAPI const char* U_EXPORT2 
u_fgetcodepage(UFILE        *file)
{
    UErrorCode     status = U_ZERO_ERROR;
    const char     *codepage = NULL;

    if (file->fConverter) {
        codepage = ucnv_getName(file->fConverter, &status);
        if(U_FAILURE(status))
            return 0;
    }
    return codepage;
}

U_CAPI int32_t U_EXPORT2 
u_fsetcodepage(    const char    *codepage,
               UFILE        *file)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t retVal = -1;

    
    if ((file->str.fPos == file->str.fBuffer) && (file->str.fLimit == file->str.fBuffer)) {
        ucnv_close(file->fConverter);
        file->fConverter = ucnv_open(codepage, &status);
        if(U_SUCCESS(status)) {
            retVal = 0;
        }
    }
    return retVal;
}


U_CAPI UConverter * U_EXPORT2 
u_fgetConverter(UFILE *file)
{
    return file->fConverter;
}
#if !UCONFIG_NO_FORMATTING
U_CAPI const UNumberFormat* U_EXPORT2 u_fgetNumberFormat(UFILE *file)
{
    return u_locbund_getNumberFormat(&file->str.fBundle, UNUM_DECIMAL);
}
#endif

#endif
