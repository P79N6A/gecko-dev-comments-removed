

















#include "unicode/utypes.h"
#include "unicode/brkiter.h"
#include "unicode/ubrk.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"
#include "unicode/ucasemap.h"
#if !UCONFIG_NO_BREAK_ITERATION
#include "unicode/utext.h"
#endif
#include "unicode/utf.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "cstring.h"
#include "ucase.h"
#include "ustr_imp.h"

U_NAMESPACE_USE



U_CAPI UCaseMap * U_EXPORT2
ucasemap_open(const char *locale, uint32_t options, UErrorCode *pErrorCode) {
    UCaseMap *csm;

    if(U_FAILURE(*pErrorCode)) {
        return NULL;
    }

    csm=(UCaseMap *)uprv_malloc(sizeof(UCaseMap));
    if(csm==NULL) {
        return NULL;
    }
    uprv_memset(csm, 0, sizeof(UCaseMap));

    csm->csp=ucase_getSingleton();
    ucasemap_setLocale(csm, locale, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        uprv_free(csm);
        return NULL;
    }

    csm->options=options;
    return csm;
}

U_CAPI void U_EXPORT2
ucasemap_close(UCaseMap *csm) {
    if(csm!=NULL) {
#if !UCONFIG_NO_BREAK_ITERATION
        
        delete reinterpret_cast<BreakIterator *>(csm->iter);
#endif
        uprv_free(csm);
    }
}

U_CAPI const char * U_EXPORT2
ucasemap_getLocale(const UCaseMap *csm) {
    return csm->locale;
}

U_CAPI uint32_t U_EXPORT2
ucasemap_getOptions(const UCaseMap *csm) {
    return csm->options;
}

U_CAPI void U_EXPORT2
ucasemap_setLocale(UCaseMap *csm, const char *locale, UErrorCode *pErrorCode) {
    int32_t length;

    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    length=uloc_getName(locale, csm->locale, (int32_t)sizeof(csm->locale), pErrorCode);
    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR || length==sizeof(csm->locale)) {
        *pErrorCode=U_ZERO_ERROR;
        
        length=uloc_getLanguage(locale, csm->locale, (int32_t)sizeof(csm->locale), pErrorCode);
    }
    if(length==sizeof(csm->locale)) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    csm->locCache=0;
    if(U_SUCCESS(*pErrorCode)) {
        ucase_getCaseLocale(csm->locale, &csm->locCache);
    } else {
        csm->locale[0]=0;
    }
}

U_CAPI void U_EXPORT2
ucasemap_setOptions(UCaseMap *csm, uint32_t options, UErrorCode * ) {
    csm->options=options;
}






static inline int32_t
appendResult(uint8_t *dest, int32_t destIndex, int32_t destCapacity,
             int32_t result, const UChar *s) {
    UChar32 c;
    int32_t length, destLength;
    UErrorCode errorCode;

    
    if(result<0) {
        
        c=~result;
        length=-1;
    } else if(result<=UCASE_MAX_STRING_LENGTH) {
        c=U_SENTINEL;
        length=result;
    } else {
        c=result;
        length=-1;
    }

    if(destIndex<destCapacity) {
        
        if(length<0) {
            
            UBool isError=FALSE;
            U8_APPEND(dest, destIndex, destCapacity, c, isError);
            if(isError) {
                
                destIndex+=U8_LENGTH(c);
            }
        } else {
            
            errorCode=U_ZERO_ERROR;
            u_strToUTF8(
                (char *)(dest+destIndex), destCapacity-destIndex, &destLength,
                s, length,
                &errorCode);
            destIndex+=destLength;
            
        }
    } else {
        
        if(length<0) {
            destIndex+=U8_LENGTH(c);
        } else {
            errorCode=U_ZERO_ERROR;
            u_strToUTF8(
                NULL, 0, &destLength,
                s, length,
                &errorCode);
            destIndex+=destLength;
        }
    }
    return destIndex;
}

static UChar32 U_CALLCONV
utf8_caseContextIterator(void *context, int8_t dir) {
    UCaseContext *csc=(UCaseContext *)context;
    UChar32 c;

    if(dir<0) {
        
        csc->index=csc->cpStart;
        csc->dir=dir;
    } else if(dir>0) {
        
        csc->index=csc->cpLimit;
        csc->dir=dir;
    } else {
        
        dir=csc->dir;
    }

    if(dir<0) {
        if(csc->start<csc->index) {
            U8_PREV((const uint8_t *)csc->p, csc->start, csc->index, c);
            return c;
        }
    } else {
        if(csc->index<csc->limit) {
            U8_NEXT((const uint8_t *)csc->p, csc->index, csc->limit, c);
            return c;
        }
    }
    return U_SENTINEL;
}





static int32_t
_caseMap(const UCaseMap *csm, UCaseMapFull *map,
         uint8_t *dest, int32_t destCapacity,
         const uint8_t *src, UCaseContext *csc,
         int32_t srcStart, int32_t srcLimit,
         UErrorCode *pErrorCode) {
    const UChar *s;
    UChar32 c, c2 = 0;
    int32_t srcIndex, destIndex;
    int32_t locCache;

    locCache=csm->locCache;

    
    srcIndex=srcStart;
    destIndex=0;
    while(srcIndex<srcLimit) {
        csc->cpStart=srcIndex;
        U8_NEXT(src, srcIndex, srcLimit, c);
        csc->cpLimit=srcIndex;
        if(c<0) {
            int32_t i=csc->cpStart;
            while(destIndex<destCapacity && i<srcIndex) {
                dest[destIndex++]=src[i++];
            }
            continue;
        }
        c=map(csm->csp, c, utf8_caseContextIterator, csc, &s, csm->locale, &locCache);
        if((destIndex<destCapacity) && (c<0 ? (c2=~c)<=0x7f : UCASE_MAX_STRING_LENGTH<c && (c2=c)<=0x7f)) {
            
            dest[destIndex++]=(uint8_t)c2;
        } else {
            destIndex=appendResult(dest, destIndex, destCapacity, c, s);
        }
    }

    if(destIndex>destCapacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return destIndex;
}

#if !UCONFIG_NO_BREAK_ITERATION

U_CFUNC int32_t U_CALLCONV
ucasemap_internalUTF8ToTitle(const UCaseMap *csm,
         uint8_t *dest, int32_t destCapacity,
         const uint8_t *src, int32_t srcLength,
         UErrorCode *pErrorCode) {
    const UChar *s;
    UChar32 c;
    int32_t prev, titleStart, titleLimit, idx, destIndex, length;
    UBool isFirstIndex;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    
    BreakIterator *bi=reinterpret_cast<BreakIterator *>(csm->iter);

    
    int32_t locCache=csm->locCache;
    UCaseContext csc=UCASECONTEXT_INITIALIZER;
    csc.p=(void *)src;
    csc.limit=srcLength;
    destIndex=0;
    prev=0;
    isFirstIndex=TRUE;

    
    while(prev<srcLength) {
        
        if(isFirstIndex) {
            isFirstIndex=FALSE;
            idx=bi->first();
        } else {
            idx=bi->next();
        }
        if(idx==UBRK_DONE || idx>srcLength) {
            idx=srcLength;
        }

        












        if(prev<idx) {
            
            titleStart=titleLimit=prev;
            U8_NEXT(src, titleLimit, idx, c);
            if((csm->options&U_TITLECASE_NO_BREAK_ADJUSTMENT)==0 && UCASE_NONE==ucase_getType(csm->csp, c)) {
                
                for(;;) {
                    titleStart=titleLimit;
                    if(titleLimit==idx) {
                        



                        break;
                    }
                    U8_NEXT(src, titleLimit, idx, c);
                    if(UCASE_NONE!=ucase_getType(csm->csp, c)) {
                        break; 
                    }
                }
                length=titleStart-prev;
                if(length>0) {
                    if((destIndex+length)<=destCapacity) {
                        uprv_memcpy(dest+destIndex, src+prev, length);
                    }
                    destIndex+=length;
                }
            }

            if(titleStart<titleLimit) {
                
                csc.cpStart=titleStart;
                csc.cpLimit=titleLimit;
                c=ucase_toFullTitle(csm->csp, c, utf8_caseContextIterator, &csc, &s, csm->locale, &locCache);
                destIndex=appendResult(dest, destIndex, destCapacity, c, s);

                
                if ( titleStart+1 < idx && 
                     ucase_getCaseLocale(csm->locale, &locCache) == UCASE_LOC_DUTCH &&
                     ( src[titleStart] == 0x0049 || src[titleStart] == 0x0069 ) &&
                     ( src[titleStart+1] == 0x004A || src[titleStart+1] == 0x006A )) { 
                            c=0x004A;
                            destIndex=appendResult(dest, destIndex, destCapacity, c, s);
                            titleLimit++;
                }
                
                if(titleLimit<idx) {
                    if((csm->options&U_TITLECASE_NO_LOWERCASE)==0) {
                        
                        destIndex+=
                            _caseMap(
                                csm, ucase_toFullLower,
                                dest+destIndex, destCapacity-destIndex,
                                src, &csc,
                                titleLimit, idx,
                                pErrorCode);
                    } else {
                        
                        length=idx-titleLimit;
                        if((destIndex+length)<=destCapacity) {
                            uprv_memcpy(dest+destIndex, src+titleLimit, length);
                        }
                        destIndex+=length;
                    }
                }
            }
        }

        prev=idx;
    }

    if(destIndex>destCapacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return destIndex;
}

#endif

static int32_t U_CALLCONV
ucasemap_internalUTF8ToLower(const UCaseMap *csm,
                             uint8_t *dest, int32_t destCapacity,
                             const uint8_t *src, int32_t srcLength,
                             UErrorCode *pErrorCode) {
    UCaseContext csc=UCASECONTEXT_INITIALIZER;
    csc.p=(void *)src;
    csc.limit=srcLength;
    return _caseMap(
        csm, ucase_toFullLower,
        dest, destCapacity,
        src, &csc, 0, srcLength,
        pErrorCode);
}

static int32_t U_CALLCONV
ucasemap_internalUTF8ToUpper(const UCaseMap *csm,
                             uint8_t *dest, int32_t destCapacity,
                             const uint8_t *src, int32_t srcLength,
                             UErrorCode *pErrorCode) {
    UCaseContext csc=UCASECONTEXT_INITIALIZER;
    csc.p=(void *)src;
    csc.limit=srcLength;
    return _caseMap(
        csm, ucase_toFullUpper,
        dest, destCapacity,
        src, &csc, 0, srcLength,
        pErrorCode);
}

static int32_t
utf8_foldCase(const UCaseProps *csp,
              uint8_t *dest, int32_t destCapacity,
              const uint8_t *src, int32_t srcLength,
              uint32_t options,
              UErrorCode *pErrorCode) {
    int32_t srcIndex, destIndex;

    const UChar *s;
    UChar32 c, c2;
    int32_t start;

    
    srcIndex=destIndex=0;
    while(srcIndex<srcLength) {
        start=srcIndex;
        U8_NEXT(src, srcIndex, srcLength, c);
        if(c<0) {
            while(destIndex<destCapacity && start<srcIndex) {
                dest[destIndex++]=src[start++];
            }
            continue;
        }
        c=ucase_toFullFolding(csp, c, &s, options);
        if((destIndex<destCapacity) && (c<0 ? (c2=~c)<=0x7f : UCASE_MAX_STRING_LENGTH<c && (c2=c)<=0x7f)) {
            
            dest[destIndex++]=(uint8_t)c2;
        } else {
            destIndex=appendResult(dest, destIndex, destCapacity, c, s);
        }
    }

    if(destIndex>destCapacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return destIndex;
}

static int32_t U_CALLCONV
ucasemap_internalUTF8Fold(const UCaseMap *csm,
                          uint8_t *dest, int32_t destCapacity,
                          const uint8_t *src, int32_t srcLength,
                          UErrorCode *pErrorCode) {
    return utf8_foldCase(csm->csp, dest, destCapacity, src, srcLength, csm->options, pErrorCode);
}

U_CFUNC int32_t
ucasemap_mapUTF8(const UCaseMap *csm,
                 uint8_t *dest, int32_t destCapacity,
                 const uint8_t *src, int32_t srcLength,
                 UTF8CaseMapper *stringCaseMapper,
                 UErrorCode *pErrorCode) {
    int32_t destLength;

    
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if( destCapacity<0 ||
        (dest==NULL && destCapacity>0) ||
        src==NULL ||
        srcLength<-1
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if(srcLength==-1) {
        srcLength=(int32_t)uprv_strlen((const char *)src);
    }

    
    if( dest!=NULL &&
        ((src>=dest && src<(dest+destCapacity)) ||
         (dest>=src && dest<(src+srcLength)))
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    destLength=stringCaseMapper(csm, dest, destCapacity, src, srcLength, pErrorCode);
    return u_terminateChars((char *)dest, destCapacity, destLength, pErrorCode);
}



U_CAPI int32_t U_EXPORT2
ucasemap_utf8ToLower(const UCaseMap *csm,
                     char *dest, int32_t destCapacity,
                     const char *src, int32_t srcLength,
                     UErrorCode *pErrorCode) {
    return ucasemap_mapUTF8(csm,
                   (uint8_t *)dest, destCapacity,
                   (const uint8_t *)src, srcLength,
                   ucasemap_internalUTF8ToLower, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
ucasemap_utf8ToUpper(const UCaseMap *csm,
                     char *dest, int32_t destCapacity,
                     const char *src, int32_t srcLength,
                     UErrorCode *pErrorCode) {
    return ucasemap_mapUTF8(csm,
                   (uint8_t *)dest, destCapacity,
                   (const uint8_t *)src, srcLength,
                   ucasemap_internalUTF8ToUpper, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
ucasemap_utf8FoldCase(const UCaseMap *csm,
                      char *dest, int32_t destCapacity,
                      const char *src, int32_t srcLength,
                      UErrorCode *pErrorCode) {
    return ucasemap_mapUTF8(csm,
                   (uint8_t *)dest, destCapacity,
                   (const uint8_t *)src, srcLength,
                   ucasemap_internalUTF8Fold, pErrorCode);
}
