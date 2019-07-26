



















#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/unorm.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "normalizer2impl.h"
#include "ucase.h"
#include "uprops.h"
#include "ustr_imp.h"

U_NAMESPACE_USE

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))






























































































struct CmpEquivLevel {
    const UChar *start, *s, *limit;
};
typedef struct CmpEquivLevel CmpEquivLevel;





#define _COMPARE_EQUIV 0x80000


static int32_t
unorm_cmpEquivFold(const UChar *s1, int32_t length1,
                   const UChar *s2, int32_t length2,
                   uint32_t options,
                   UErrorCode *pErrorCode) {
    const Normalizer2Impl *nfcImpl;
    const UCaseProps *csp;

    
    const UChar *start1, *start2, *limit1, *limit2;

    
    const UChar *p;
    int32_t length;

    
    CmpEquivLevel stack1[2], stack2[2];

    
    UChar decomp1[4], decomp2[4];

    
    UChar fold1[UCASE_MAX_STRING_LENGTH+1], fold2[UCASE_MAX_STRING_LENGTH+1];

    
    int32_t level1, level2;

    
    UChar32 c1, c2, cp1, cp2;

    

    





    
    if((options&_COMPARE_EQUIV)!=0) {
        nfcImpl=Normalizer2Factory::getNFCImpl(*pErrorCode);
    } else {
        nfcImpl=NULL;
    }
    if((options&U_COMPARE_IGNORE_CASE)!=0) {
        csp=ucase_getSingleton();
    } else {
        csp=NULL;
    }
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

        




        if( level1==0 && (options&U_COMPARE_IGNORE_CASE) &&
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

        if( level2==0 && (options&U_COMPARE_IGNORE_CASE) &&
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

        if( level1<2 && (options&_COMPARE_EQUIV) &&
            0!=(p=nfcImpl->getDecomposition((UChar32)cp1, decomp1, length))
        ) {
            
            if(U_IS_SURROGATE(c1)) {
                if(U_IS_SURROGATE_LEAD(c1)) {
                    
                    ++s1;
                } else  {
                    






                    --s2;
                    c2=*(s2-1);
                }
            }

            
            stack1[level1].start=start1;
            stack1[level1].s=s1;
            stack1[level1].limit=limit1;
            ++level1;

            
            if(level1<2) {
                stack1[level1++].start=NULL;
            }

            
            start1=s1=p;
            limit1=p+length;

            
            c1=-1;
            continue;
        }

        if( level2<2 && (options&_COMPARE_EQUIV) &&
            0!=(p=nfcImpl->getDecomposition((UChar32)cp2, decomp2, length))
        ) {
            
            if(U_IS_SURROGATE(c2)) {
                if(U_IS_SURROGATE_LEAD(c2)) {
                    
                    ++s2;
                } else  {
                    






                    --s1;
                    c1=*(s1-1);
                }
            }

            
            stack2[level2].start=start2;
            stack2[level2].s=s2;
            stack2[level2].limit=limit2;
            ++level2;

            
            if(level2<2) {
                stack2[level2++].start=NULL;
            }

            
            start2=s2=p;
            limit2=p+length;

            
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

static
UBool _normalize(const Normalizer2 *n2, const UChar *s, int32_t length,
                UnicodeString &normalized, UErrorCode *pErrorCode) {
    UnicodeString str(length<0, s, length);

    
    int32_t spanQCYes=n2->spanQuickCheckYes(str, *pErrorCode);
    if (U_FAILURE(*pErrorCode)) {
        return FALSE;
    }
    







    if(spanQCYes<str.length()) {
        UnicodeString unnormalized=str.tempSubString(spanQCYes);
        normalized.setTo(FALSE, str.getBuffer(), spanQCYes);
        n2->normalizeSecondAndAppend(normalized, unnormalized, *pErrorCode);
        if (U_SUCCESS(*pErrorCode)) {
            return TRUE;
        }
    }
    return FALSE;
}

U_CAPI int32_t U_EXPORT2
unorm_compare(const UChar *s1, int32_t length1,
              const UChar *s2, int32_t length2,
              uint32_t options,
              UErrorCode *pErrorCode) {
    
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(s1==0 || length1<-1 || s2==0 || length2<-1) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    UnicodeString fcd1, fcd2;
    int32_t normOptions=(int32_t)(options>>UNORM_COMPARE_NORM_OPTIONS_SHIFT);
    options|=_COMPARE_EQUIV;

    




















    if(!(options&UNORM_INPUT_IS_FCD) || (options&U_FOLD_CASE_EXCLUDE_SPECIAL_I)) {
        const Normalizer2 *n2;
        if(options&U_FOLD_CASE_EXCLUDE_SPECIAL_I) {
            n2=Normalizer2Factory::getNFDInstance(*pErrorCode);
        } else {
            n2=Normalizer2Factory::getFCDInstance(*pErrorCode);
        }
        if (U_FAILURE(*pErrorCode)) {
            return 0;
        }

        if(normOptions&UNORM_UNICODE_3_2) {
            const UnicodeSet *uni32=uniset_getUnicode32Instance(*pErrorCode);
            FilteredNormalizer2 fn2(*n2, *uni32);
            if(_normalize(&fn2, s1, length1, fcd1, pErrorCode)) {
                s1=fcd1.getBuffer();
                length1=fcd1.length();
            }
            if(_normalize(&fn2, s2, length2, fcd2, pErrorCode)) {
                s2=fcd2.getBuffer();
                length2=fcd2.length();
            }
        } else {
            if(_normalize(n2, s1, length1, fcd1, pErrorCode)) {
                s1=fcd1.getBuffer();
                length1=fcd1.length();
            }
            if(_normalize(n2, s2, length2, fcd2, pErrorCode)) {
                s2=fcd2.getBuffer();
                length2=fcd2.length();
            }
        }
    }

    if(U_SUCCESS(*pErrorCode)) {
        return unorm_cmpEquivFold(s1, length1, s2, length2, options, pErrorCode);
    } else {
        return 0;
    }
}

#endif 
