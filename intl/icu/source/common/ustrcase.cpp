



















#include "unicode/utypes.h"
#include "unicode/brkiter.h"
#include "unicode/ustring.h"
#include "unicode/ucasemap.h"
#include "unicode/ubrk.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "ucase.h"
#include "ustr_imp.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

U_NAMESPACE_USE




static inline int32_t
appendResult(UChar *dest, int32_t destIndex, int32_t destCapacity,
             int32_t result, const UChar *s) {
    UChar32 c;
    int32_t length;

    
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
            U16_APPEND(dest, destIndex, destCapacity, c, isError);
            if(isError) {
                
                destIndex+=U16_LENGTH(c);
            }
        } else {
            
            if((destIndex+length)<=destCapacity) {
                while(length>0) {
                    dest[destIndex++]=*s++;
                    --length;
                }
            } else {
                
                destIndex+=length;
            }
        }
    } else {
        
        if(length<0) {
            destIndex+=U16_LENGTH(c);
        } else {
            destIndex+=length;
        }
    }
    return destIndex;
}

static UChar32 U_CALLCONV
utf16_caseContextIterator(void *context, int8_t dir) {
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
            U16_PREV((const UChar *)csc->p, csc->start, csc->index, c);
            return c;
        }
    } else {
        if(csc->index<csc->limit) {
            U16_NEXT((const UChar *)csc->p, csc->index, csc->limit, c);
            return c;
        }
    }
    return U_SENTINEL;
}





static int32_t
_caseMap(const UCaseMap *csm, UCaseMapFull *map,
         UChar *dest, int32_t destCapacity,
         const UChar *src, UCaseContext *csc,
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
        U16_NEXT(src, srcIndex, srcLimit, c);
        csc->cpLimit=srcIndex;
        c=map(csm->csp, c, utf16_caseContextIterator, csc, &s, csm->locale, &locCache);
        if((destIndex<destCapacity) && (c<0 ? (c2=~c)<=0xffff : UCASE_MAX_STRING_LENGTH<c && (c2=c)<=0xffff)) {
            
            dest[destIndex++]=(UChar)c2;
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
ustrcase_internalToTitle(const UCaseMap *csm,
                         UChar *dest, int32_t destCapacity,
                         const UChar *src, int32_t srcLength,
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
            U16_NEXT(src, titleLimit, idx, c);
            if((csm->options&U_TITLECASE_NO_BREAK_ADJUSTMENT)==0 && UCASE_NONE==ucase_getType(csm->csp, c)) {
                
                for(;;) {
                    titleStart=titleLimit;
                    if(titleLimit==idx) {
                        



                        break;
                    }
                    U16_NEXT(src, titleLimit, idx, c);
                    if(UCASE_NONE!=ucase_getType(csm->csp, c)) {
                        break; 
                    }
                }
                length=titleStart-prev;
                if(length>0) {
                    if((destIndex+length)<=destCapacity) {
                        uprv_memcpy(dest+destIndex, src+prev, length*U_SIZEOF_UCHAR);
                    }
                    destIndex+=length;
                }
            }

            if(titleStart<titleLimit) {
                
                csc.cpStart=titleStart;
                csc.cpLimit=titleLimit;
                c=ucase_toFullTitle(csm->csp, c, utf16_caseContextIterator, &csc, &s, csm->locale, &locCache);
                destIndex=appendResult(dest, destIndex, destCapacity, c, s); 

                
                if ( titleStart+1 < idx && 
                     ucase_getCaseLocale(csm->locale,&locCache) == UCASE_LOC_DUTCH &&
                     ( src[titleStart] == (UChar32) 0x0049 || src[titleStart] == (UChar32) 0x0069 ) &&
                     ( src[titleStart+1] == (UChar32) 0x004A || src[titleStart+1] == (UChar32) 0x006A )) { 
                            c=(UChar32) 0x004A;
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
                            uprv_memcpy(dest+destIndex, src+titleLimit, length*U_SIZEOF_UCHAR);
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



U_CFUNC int32_t U_CALLCONV
ustrcase_internalToLower(const UCaseMap *csm,
                         UChar *dest, int32_t destCapacity,
                         const UChar *src, int32_t srcLength,
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

U_CFUNC int32_t U_CALLCONV
ustrcase_internalToUpper(const UCaseMap *csm,
                         UChar *dest, int32_t destCapacity,
                         const UChar *src, int32_t srcLength,
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
ustr_foldCase(const UCaseProps *csp,
              UChar *dest, int32_t destCapacity,
              const UChar *src, int32_t srcLength,
              uint32_t options,
              UErrorCode *pErrorCode) {
    int32_t srcIndex, destIndex;

    const UChar *s;
    UChar32 c, c2 = 0;

    
    srcIndex=destIndex=0;
    while(srcIndex<srcLength) {
        U16_NEXT(src, srcIndex, srcLength, c);
        c=ucase_toFullFolding(csp, c, &s, options);
        if((destIndex<destCapacity) && (c<0 ? (c2=~c)<=0xffff : UCASE_MAX_STRING_LENGTH<c && (c2=c)<=0xffff)) {
            
            dest[destIndex++]=(UChar)c2;
        } else {
            destIndex=appendResult(dest, destIndex, destCapacity, c, s);
        }
    }

    if(destIndex>destCapacity) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return destIndex;
}

U_CFUNC int32_t U_CALLCONV
ustrcase_internalFold(const UCaseMap *csm,
                      UChar *dest, int32_t destCapacity,
                      const UChar *src, int32_t srcLength,
                      UErrorCode *pErrorCode) {
    return ustr_foldCase(csm->csp, dest, destCapacity, src, srcLength, csm->options, pErrorCode);
}

U_CFUNC int32_t
ustrcase_map(const UCaseMap *csm,
             UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             UStringCaseMapper *stringCaseMapper,
             UErrorCode *pErrorCode) {
    UChar buffer[300];
    UChar *temp;

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
        srcLength=u_strlen(src);
    }

    
    if( dest!=NULL &&
        ((src>=dest && src<(dest+destCapacity)) ||
         (dest>=src && dest<(src+srcLength)))
    ) {
        
        if(destCapacity<=LENGTHOF(buffer)) {
            
            temp=buffer;
        } else {
            
            temp=(UChar *)uprv_malloc(destCapacity*U_SIZEOF_UCHAR);
            if(temp==NULL) {
                *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }
        }
    } else {
        temp=dest;
    }

    destLength=stringCaseMapper(csm, temp, destCapacity, src, srcLength, pErrorCode);
    if(temp!=dest) {
        
        if(destLength>0) {
            int32_t copyLength= destLength<=destCapacity ? destLength : destCapacity;
            if(copyLength>0) {
                uprv_memmove(dest, temp, copyLength*U_SIZEOF_UCHAR);
            }
        }
        if(temp!=buffer) {
            uprv_free(temp);
        }
    }

    return u_terminateUChars(dest, destCapacity, destLength, pErrorCode);
}



U_CAPI int32_t U_EXPORT2
u_strFoldCase(UChar *dest, int32_t destCapacity,
              const UChar *src, int32_t srcLength,
              uint32_t options,
              UErrorCode *pErrorCode) {
    UCaseMap csm=UCASEMAP_INITIALIZER;
    csm.csp=ucase_getSingleton();
    csm.options=options;
    return ustrcase_map(
        &csm,
        dest, destCapacity,
        src, srcLength,
        ustrcase_internalFold, pErrorCode);
}













struct CmpEquivLevel {
    const UChar *start, *s, *limit;
};
typedef struct CmpEquivLevel CmpEquivLevel;


U_CFUNC int32_t
u_strcmpFold(const UChar *s1, int32_t length1,
             const UChar *s2, int32_t length2,
             uint32_t options,
             UErrorCode *pErrorCode) {
    const UCaseProps *csp;

    
    const UChar *start1, *start2, *limit1, *limit2;

    
    const UChar *p;
    int32_t length;

    
    CmpEquivLevel stack1[2], stack2[2];

    
    UChar fold1[UCASE_MAX_STRING_LENGTH+1], fold2[UCASE_MAX_STRING_LENGTH+1];

    
    int32_t level1, level2;

    
    UChar32 c1, c2, cp1, cp2;

    

    



    csp=ucase_getSingleton();
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    start1=s1;
    if(length1==-1) {
        limit1=NULL;
    } else {
        limit1=s1+length1;
    }

    start2=s2;
    if(length2==-1) {
        limit2=NULL;
    } else {
        limit2=s2+length2;
    }

    level1=level2=0;
    c1=c2=-1;

    
    for(;;) {
        




        if(c1<0) {
            
            for(;;) {
                if(s1==limit1 || ((c1=*s1)==0 && (limit1==NULL || (options&_STRNCMP_STYLE)))) {
                    if(level1==0) {
                        c1=-1;
                        break;
                    }
                } else {
                    ++s1;
                    break;
                }

                
                do {
                    --level1;
                    start1=stack1[level1].start;    
                } while(start1==NULL);
                s1=stack1[level1].s;                
                limit1=stack1[level1].limit;        
            }
        }

        if(c2<0) {
            
            for(;;) {
                if(s2==limit2 || ((c2=*s2)==0 && (limit2==NULL || (options&_STRNCMP_STYLE)))) {
                    if(level2==0) {
                        c2=-1;
                        break;
                    }
                } else {
                    ++s2;
                    break;
                }

                
                do {
                    --level2;
                    start2=stack2[level2].start;    
                } while(start2==NULL);
                s2=stack2[level2].s;                
                limit2=stack2[level2].limit;        
            }
        }

        



        if(c1==c2) {
            if(c1<0) {
                return 0;   
            }
            c1=c2=-1;       
            continue;
        } else if(c1<0) {
            return -1;      
        } else if(c2<0) {
            return 1;       
        }
        

        
        cp1=c1;
        if(U_IS_SURROGATE(c1)) {
            UChar c;

            if(U_IS_SURROGATE_LEAD(c1)) {
                if(s1!=limit1 && U16_IS_TRAIL(c=*s1)) {
                    
                    cp1=U16_GET_SUPPLEMENTARY(c1, c);
                }
            } else  {
                if(start1<=(s1-2) && U16_IS_LEAD(c=*(s1-2))) {
                    cp1=U16_GET_SUPPLEMENTARY(c, c1);
                }
            }
        }

        cp2=c2;
        if(U_IS_SURROGATE(c2)) {
            UChar c;

            if(U_IS_SURROGATE_LEAD(c2)) {
                if(s2!=limit2 && U16_IS_TRAIL(c=*s2)) {
                    
                    cp2=U16_GET_SUPPLEMENTARY(c2, c);
                }
            } else  {
                if(start2<=(s2-2) && U16_IS_LEAD(c=*(s2-2))) {
                    cp2=U16_GET_SUPPLEMENTARY(c, c2);
                }
            }
        }

        




        if( level1==0 &&
            (length=ucase_toFullFolding(csp, (UChar32)cp1, &p, options))>=0
        ) {
            
            if(U_IS_SURROGATE(c1)) {
                if(U_IS_SURROGATE_LEAD(c1)) {
                    
                    ++s1;
                } else  {
                    






                    --s2;
                    c2=*(s2-1);
                }
            }

            
            stack1[0].start=start1;
            stack1[0].s=s1;
            stack1[0].limit=limit1;
            ++level1;

            
            if(length<=UCASE_MAX_STRING_LENGTH) {
                u_memcpy(fold1, p, length);
            } else {
                int32_t i=0;
                U16_APPEND_UNSAFE(fold1, i, length);
                length=i;
            }

            
            start1=s1=fold1;
            limit1=fold1+length;

            
            c1=-1;
            continue;
        }

        if( level2==0 &&
            (length=ucase_toFullFolding(csp, (UChar32)cp2, &p, options))>=0
        ) {
            
            if(U_IS_SURROGATE(c2)) {
                if(U_IS_SURROGATE_LEAD(c2)) {
                    
                    ++s2;
                } else  {
                    






                    --s1;
                    c1=*(s1-1);
                }
            }

            
            stack2[0].start=start2;
            stack2[0].s=s2;
            stack2[0].limit=limit2;
            ++level2;

            
            if(length<=UCASE_MAX_STRING_LENGTH) {
                u_memcpy(fold2, p, length);
            } else {
                int32_t i=0;
                U16_APPEND_UNSAFE(fold2, i, length);
                length=i;
            }

            
            start2=s2=fold2;
            limit2=fold2+length;

            
            c2=-1;
            continue;
        }

        
















        if(c1>=0xd800 && c2>=0xd800 && (options&U_COMPARE_CODE_POINT_ORDER)) {
            
            if(
                (c1<=0xdbff && s1!=limit1 && U16_IS_TRAIL(*s1)) ||
                (U16_IS_TRAIL(c1) && start1!=(s1-1) && U16_IS_LEAD(*(s1-2)))
            ) {
                
            } else {
                
                c1-=0x2800;
            }

            if(
                (c2<=0xdbff && s2!=limit2 && U16_IS_TRAIL(*s2)) ||
                (U16_IS_TRAIL(c2) && start2!=(s2-1) && U16_IS_LEAD(*(s2-2)))
            ) {
                
            } else {
                
                c2-=0x2800;
            }
        }

        return c1-c2;
    }
}



U_CAPI int32_t U_EXPORT2
u_strCaseCompare(const UChar *s1, int32_t length1,
                 const UChar *s2, int32_t length2,
                 uint32_t options,
                 UErrorCode *pErrorCode) {
    
    if(pErrorCode==0 || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(s1==NULL || length1<-1 || s2==NULL || length2<-1) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    return u_strcmpFold(s1, length1, s2, length2,
                        options|U_COMPARE_IGNORE_CASE,
                        pErrorCode);
}

U_CAPI int32_t U_EXPORT2
u_strcasecmp(const UChar *s1, const UChar *s2, uint32_t options) {
    UErrorCode errorCode=U_ZERO_ERROR;
    return u_strcmpFold(s1, -1, s2, -1,
                        options|U_COMPARE_IGNORE_CASE,
                        &errorCode);
}

U_CAPI int32_t U_EXPORT2
u_memcasecmp(const UChar *s1, const UChar *s2, int32_t length, uint32_t options) {
    UErrorCode errorCode=U_ZERO_ERROR;
    return u_strcmpFold(s1, length, s2, length,
                        options|U_COMPARE_IGNORE_CASE,
                        &errorCode);
}

U_CAPI int32_t U_EXPORT2
u_strncasecmp(const UChar *s1, const UChar *s2, int32_t n, uint32_t options) {
    UErrorCode errorCode=U_ZERO_ERROR;
    return u_strcmpFold(s1, n, s2, n,
                        options|(U_COMPARE_IGNORE_CASE|_STRNCMP_STYLE),
                        &errorCode);
}
