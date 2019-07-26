
























#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/udata.h"
#include "unicode/ustring.h"
#include "unicode/uiter.h"
#include "unicode/unorm.h"
#include "unicode/unorm2.h"
#include "normalizer2impl.h"
#include "unormimp.h"
#include "uprops.h"
#include "ustr_imp.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

U_NAMESPACE_USE



U_CAPI UNormalizationCheckResult U_EXPORT2
unorm_quickCheck(const UChar *src,
                 int32_t srcLength, 
                 UNormalizationMode mode,
                 UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    return unorm2_quickCheck((const UNormalizer2 *)n2, src, srcLength, pErrorCode);
}

U_CAPI UNormalizationCheckResult U_EXPORT2
unorm_quickCheckWithOptions(const UChar *src, int32_t srcLength, 
                            UNormalizationMode mode, int32_t options,
                            UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    if(options&UNORM_UNICODE_3_2) {
        FilteredNormalizer2 fn2(*n2, *uniset_getUnicode32Instance(*pErrorCode));
        return unorm2_quickCheck(
            reinterpret_cast<const UNormalizer2 *>(static_cast<Normalizer2 *>(&fn2)),
            src, srcLength, pErrorCode);
    } else {
        return unorm2_quickCheck((const UNormalizer2 *)n2, src, srcLength, pErrorCode);
    }
}

U_CAPI UBool U_EXPORT2
unorm_isNormalized(const UChar *src, int32_t srcLength,
                   UNormalizationMode mode,
                   UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    return unorm2_isNormalized((const UNormalizer2 *)n2, src, srcLength, pErrorCode);
}

U_CAPI UBool U_EXPORT2
unorm_isNormalizedWithOptions(const UChar *src, int32_t srcLength,
                              UNormalizationMode mode, int32_t options,
                              UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    if(options&UNORM_UNICODE_3_2) {
        FilteredNormalizer2 fn2(*n2, *uniset_getUnicode32Instance(*pErrorCode));
        return unorm2_isNormalized(
            reinterpret_cast<const UNormalizer2 *>(static_cast<Normalizer2 *>(&fn2)),
            src, srcLength, pErrorCode);
    } else {
        return unorm2_isNormalized((const UNormalizer2 *)n2, src, srcLength, pErrorCode);
    }
}




U_CAPI int32_t U_EXPORT2
unorm_normalize(const UChar *src, int32_t srcLength,
                UNormalizationMode mode, int32_t options,
                UChar *dest, int32_t destCapacity,
                UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    if(options&UNORM_UNICODE_3_2) {
        FilteredNormalizer2 fn2(*n2, *uniset_getUnicode32Instance(*pErrorCode));
        return unorm2_normalize(
            reinterpret_cast<const UNormalizer2 *>(static_cast<Normalizer2 *>(&fn2)),
            src, srcLength, dest, destCapacity, pErrorCode);
    } else {
        return unorm2_normalize((const UNormalizer2 *)n2,
            src, srcLength, dest, destCapacity, pErrorCode);
    }
}




static int32_t
_iterate(UCharIterator *src, UBool forward,
              UChar *dest, int32_t destCapacity,
              const Normalizer2 *n2,
              UBool doNormalize, UBool *pNeededToNormalize,
              UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0) || src==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(pNeededToNormalize!=NULL) {
        *pNeededToNormalize=FALSE;
    }
    if(!(forward ? src->hasNext(src) : src->hasPrevious(src))) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    }

    UnicodeString buffer;
    UChar32 c;
    if(forward) {
        
        buffer.append(uiter_next32(src));
        
        while((c=uiter_next32(src))>=0) {
            if(n2->hasBoundaryBefore(c)) {
                
                src->move(src, -U16_LENGTH(c), UITER_CURRENT);
                break;
            } else {
                buffer.append(c);
            }
        }
    } else {
        while((c=uiter_previous32(src))>=0) {
            
            buffer.insert(0, c);
            
            if(n2->hasBoundaryBefore(c)) {
                break;
            }
        }
    }

    UnicodeString destString(dest, 0, destCapacity);
    if(buffer.length()>0 && doNormalize) {
        n2->normalize(buffer, destString, *pErrorCode).extract(dest, destCapacity, *pErrorCode);
        if(pNeededToNormalize!=NULL && U_SUCCESS(*pErrorCode)) {
            *pNeededToNormalize= destString!=buffer;
        }
        return destString.length();
    } else {
        
        return buffer.extract(dest, destCapacity, *pErrorCode);
    }
}

static int32_t
unorm_iterate(UCharIterator *src, UBool forward,
              UChar *dest, int32_t destCapacity,
              UNormalizationMode mode, int32_t options,
              UBool doNormalize, UBool *pNeededToNormalize,
              UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    if(options&UNORM_UNICODE_3_2) {
        const UnicodeSet *uni32 = uniset_getUnicode32Instance(*pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            return 0;
        }
        FilteredNormalizer2 fn2(*n2, *uni32);
        return _iterate(src, forward, dest, destCapacity,
            &fn2, doNormalize, pNeededToNormalize, pErrorCode);
    }
    return _iterate(src, forward, dest, destCapacity,
            n2, doNormalize, pNeededToNormalize, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
unorm_previous(UCharIterator *src,
               UChar *dest, int32_t destCapacity,
               UNormalizationMode mode, int32_t options,
               UBool doNormalize, UBool *pNeededToNormalize,
               UErrorCode *pErrorCode) {
    return unorm_iterate(src, FALSE,
                         dest, destCapacity,
                         mode, options,
                         doNormalize, pNeededToNormalize,
                         pErrorCode);
}

U_CAPI int32_t U_EXPORT2
unorm_next(UCharIterator *src,
           UChar *dest, int32_t destCapacity,
           UNormalizationMode mode, int32_t options,
           UBool doNormalize, UBool *pNeededToNormalize,
           UErrorCode *pErrorCode) {
    return unorm_iterate(src, TRUE,
                         dest, destCapacity,
                         mode, options,
                         doNormalize, pNeededToNormalize,
                         pErrorCode);
}



static int32_t
_concatenate(const UChar *left, int32_t leftLength,
                  const UChar *right, int32_t rightLength,
                  UChar *dest, int32_t destCapacity,
                  const Normalizer2 *n2,
                  UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0) ||
        left==NULL || leftLength<-1 || right==NULL || rightLength<-1) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if( dest!=NULL &&
        ((right>=dest && right<(dest+destCapacity)) ||
         (rightLength>0 && dest>=right && dest<(right+rightLength)))
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    UnicodeString destString;
    if(left==dest) {
        destString.setTo(dest, leftLength, destCapacity);
    } else {
        destString.setTo(dest, 0, destCapacity);
        destString.append(left, leftLength);
    }
    return n2->append(destString, UnicodeString(rightLength<0, right, rightLength), *pErrorCode).
           extract(dest, destCapacity, *pErrorCode);
}

U_CAPI int32_t U_EXPORT2
unorm_concatenate(const UChar *left, int32_t leftLength,
                  const UChar *right, int32_t rightLength,
                  UChar *dest, int32_t destCapacity,
                  UNormalizationMode mode, int32_t options,
                  UErrorCode *pErrorCode) {
    const Normalizer2 *n2=Normalizer2Factory::getInstance(mode, *pErrorCode);
    if(options&UNORM_UNICODE_3_2) {
        const UnicodeSet *uni32 = uniset_getUnicode32Instance(*pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            return 0;
        }
        FilteredNormalizer2 fn2(*n2, *uni32);
        return _concatenate(left, leftLength, right, rightLength,
            dest, destCapacity, &fn2, pErrorCode);
    }
    return _concatenate(left, leftLength, right, rightLength,
        dest, destCapacity, n2, pErrorCode);
}

#endif 
