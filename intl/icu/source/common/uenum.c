















#include "unicode/putil.h"
#include "uenumimp.h"
#include "cmemory.h"


typedef struct {
    int32_t len;  
    char    data; 
} _UEnumBuffer;


static const int32_t PAD = 8;



static void* _getBuffer(UEnumeration* en, int32_t capacity) {

    if (en->baseContext != NULL) {
        if (((_UEnumBuffer*) en->baseContext)->len < capacity) {
            capacity += PAD;
            en->baseContext = uprv_realloc(en->baseContext,
                                           sizeof(int32_t) + capacity);
            if (en->baseContext == NULL) {
                return NULL;
            }
            ((_UEnumBuffer*) en->baseContext)->len = capacity;
        }
    } else {
        capacity += PAD;
        en->baseContext = uprv_malloc(sizeof(int32_t) + capacity);
        if (en->baseContext == NULL) {
            return NULL;
        }
        ((_UEnumBuffer*) en->baseContext)->len = capacity;
    }
    
    return (void*) & ((_UEnumBuffer*) en->baseContext)->data;
}

U_CAPI void U_EXPORT2
uenum_close(UEnumeration* en)
{
    if (en) {
        if (en->close != NULL) {
            if (en->baseContext) {
                uprv_free(en->baseContext);
            }
            en->close(en);
        } else { 
            uprv_free(en);
        }
    }
}

U_CAPI int32_t U_EXPORT2
uenum_count(UEnumeration* en, UErrorCode* status)
{
    if (!en || U_FAILURE(*status)) {
        return -1;
    }
    if (en->count != NULL) {
        return en->count(en, status);
    } else {
        *status = U_UNSUPPORTED_ERROR;
        return -1;
    }
}


U_CAPI const UChar* U_EXPORT2
uenum_unextDefault(UEnumeration* en,
            int32_t* resultLength,
            UErrorCode* status)
{
    UChar *ustr = NULL;
    int32_t len = 0;
    if (en->next != NULL) {
        const char *cstr = en->next(en, &len, status);
        if (cstr != NULL) {
            ustr = (UChar*) _getBuffer(en, (len+1) * sizeof(UChar));
            if (ustr == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
            } else {
                u_charsToUChars(cstr, ustr, len+1);
            }
        }
    } else {
        *status = U_UNSUPPORTED_ERROR;
    }
    if (resultLength) {
        *resultLength = len;
    }
    return ustr;
}


U_CAPI const char* U_EXPORT2
uenum_nextDefault(UEnumeration* en,
            int32_t* resultLength,
            UErrorCode* status)
{
    if (en->uNext != NULL) {
        char *tempCharVal;
        const UChar *tempUCharVal = en->uNext(en, resultLength, status);
        if (tempUCharVal == NULL) {
            return NULL;
        }
        tempCharVal = (char*)
            _getBuffer(en, (*resultLength+1) * sizeof(char));
        if (!tempCharVal) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        u_UCharsToChars(tempUCharVal, tempCharVal, *resultLength + 1);
        return tempCharVal;
    } else {
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }
}

U_CAPI const UChar* U_EXPORT2
uenum_unext(UEnumeration* en,
            int32_t* resultLength,
            UErrorCode* status)
{
    if (!en || U_FAILURE(*status)) {
        return NULL;
    }
    if (en->uNext != NULL) {
        return en->uNext(en, resultLength, status);
    } else {
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }
}

U_CAPI const char* U_EXPORT2
uenum_next(UEnumeration* en,
          int32_t* resultLength,
          UErrorCode* status)
{
    if (!en || U_FAILURE(*status)) {
        return NULL;
    }
    if (en->next != NULL) {
        if (resultLength != NULL) {
            return en->next(en, resultLength, status);
        }
        else {
            int32_t dummyLength=0;
            return en->next(en, &dummyLength, status);
        }
    } else {
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }
}

U_CAPI void U_EXPORT2
uenum_reset(UEnumeration* en, UErrorCode* status)
{
    if (!en || U_FAILURE(*status)) {
        return;
    }
    if (en->reset != NULL) {
        en->reset(en, status);
    } else {
        *status = U_UNSUPPORTED_ERROR;
    }
}
