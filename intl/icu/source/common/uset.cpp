



















#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/uset.h"
#include "unicode/uniset.h"
#include "cmemory.h"
#include "unicode/ustring.h"
#include "unicode/parsepos.h"

U_NAMESPACE_USE

U_CAPI USet* U_EXPORT2
uset_openEmpty() {
    return (USet*) new UnicodeSet();
}

U_CAPI USet* U_EXPORT2
uset_open(UChar32 start, UChar32 end) {
    return (USet*) new UnicodeSet(start, end);
}

U_CAPI void U_EXPORT2
uset_close(USet* set) {
    delete (UnicodeSet*) set;
}

U_CAPI USet * U_EXPORT2
uset_clone(const USet *set) {
    return (USet*) (((UnicodeSet*) set)->UnicodeSet::clone());
}

U_CAPI UBool U_EXPORT2
uset_isFrozen(const USet *set) {
    return ((UnicodeSet*) set)->UnicodeSet::isFrozen();
}

U_CAPI void U_EXPORT2
uset_freeze(USet *set) {
    ((UnicodeSet*) set)->UnicodeSet::freeze();
}

U_CAPI USet * U_EXPORT2
uset_cloneAsThawed(const USet *set) {
    return (USet*) (((UnicodeSet*) set)->UnicodeSet::cloneAsThawed());
}

U_CAPI void U_EXPORT2
uset_set(USet* set,
     UChar32 start, UChar32 end) {
    ((UnicodeSet*) set)->UnicodeSet::set(start, end);
}

U_CAPI void U_EXPORT2
uset_addAll(USet* set, const USet *additionalSet) {
    ((UnicodeSet*) set)->UnicodeSet::addAll(*((const UnicodeSet*)additionalSet));
}

U_CAPI void U_EXPORT2
uset_add(USet* set, UChar32 c) {
    ((UnicodeSet*) set)->UnicodeSet::add(c);
}

U_CAPI void U_EXPORT2
uset_addRange(USet* set, UChar32 start, UChar32 end) {
    ((UnicodeSet*) set)->UnicodeSet::add(start, end);    
}

U_CAPI void U_EXPORT2
uset_addString(USet* set, const UChar* str, int32_t strLen) {
    
    UnicodeString s(strLen<0, str, strLen);
    ((UnicodeSet*) set)->UnicodeSet::add(s);
}

U_CAPI void U_EXPORT2
uset_addAllCodePoints(USet* set, const UChar *str, int32_t strLen) {
    
    UnicodeString s(str, strLen);
    ((UnicodeSet*) set)->UnicodeSet::addAll(s);
}

U_CAPI void U_EXPORT2
uset_remove(USet* set, UChar32 c) {
    ((UnicodeSet*) set)->UnicodeSet::remove(c);
}

U_CAPI void U_EXPORT2
uset_removeRange(USet* set, UChar32 start, UChar32 end) {
    ((UnicodeSet*) set)->UnicodeSet::remove(start, end);
}

U_CAPI void U_EXPORT2
uset_removeString(USet* set, const UChar* str, int32_t strLen) {
    UnicodeString s(strLen==-1, str, strLen);
    ((UnicodeSet*) set)->UnicodeSet::remove(s);
}

U_CAPI void U_EXPORT2
uset_removeAll(USet* set, const USet* remove) {
    ((UnicodeSet*) set)->UnicodeSet::removeAll(*(const UnicodeSet*)remove);
}

U_CAPI void U_EXPORT2
uset_retain(USet* set, UChar32 start, UChar32 end) {
    ((UnicodeSet*) set)->UnicodeSet::retain(start, end);
}

U_CAPI void U_EXPORT2
uset_retainAll(USet* set, const USet* retain) {
    ((UnicodeSet*) set)->UnicodeSet::retainAll(*(const UnicodeSet*)retain);
}

U_CAPI void U_EXPORT2
uset_compact(USet* set) {
    ((UnicodeSet*) set)->UnicodeSet::compact();
}

U_CAPI void U_EXPORT2
uset_complement(USet* set) {
    ((UnicodeSet*) set)->UnicodeSet::complement();
}

U_CAPI void U_EXPORT2
uset_complementAll(USet* set, const USet* complement) {
    ((UnicodeSet*) set)->UnicodeSet::complementAll(*(const UnicodeSet*)complement);
}

U_CAPI void U_EXPORT2
uset_clear(USet* set) {
    ((UnicodeSet*) set)->UnicodeSet::clear();
}

U_CAPI void U_EXPORT2
uset_removeAllStrings(USet* set) {
    ((UnicodeSet*) set)->UnicodeSet::removeAllStrings();
}

U_CAPI UBool U_EXPORT2
uset_isEmpty(const USet* set) {
    return ((const UnicodeSet*) set)->UnicodeSet::isEmpty();
}

U_CAPI UBool U_EXPORT2
uset_contains(const USet* set, UChar32 c) {
    return ((const UnicodeSet*) set)->UnicodeSet::contains(c);
}

U_CAPI UBool U_EXPORT2
uset_containsRange(const USet* set, UChar32 start, UChar32 end) {
    return ((const UnicodeSet*) set)->UnicodeSet::contains(start, end);
}

U_CAPI UBool U_EXPORT2
uset_containsString(const USet* set, const UChar* str, int32_t strLen) {
    UnicodeString s(strLen==-1, str, strLen);
    return ((const UnicodeSet*) set)->UnicodeSet::contains(s);
}

U_CAPI UBool U_EXPORT2
uset_containsAll(const USet* set1, const USet* set2) {
    return ((const UnicodeSet*) set1)->UnicodeSet::containsAll(* (const UnicodeSet*) set2);
}

U_CAPI UBool U_EXPORT2
uset_containsAllCodePoints(const USet* set, const UChar *str, int32_t strLen) {
    
    UnicodeString s(strLen==-1, str, strLen);
    return ((const UnicodeSet*) set)->UnicodeSet::containsAll(s);
}

U_CAPI UBool U_EXPORT2
uset_containsNone(const USet* set1, const USet* set2) {
    return ((const UnicodeSet*) set1)->UnicodeSet::containsNone(* (const UnicodeSet*) set2);
}

U_CAPI UBool U_EXPORT2
uset_containsSome(const USet* set1, const USet* set2) {
    return ((const UnicodeSet*) set1)->UnicodeSet::containsSome(* (const UnicodeSet*) set2);
}

U_CAPI int32_t U_EXPORT2
uset_span(const USet *set, const UChar *s, int32_t length, USetSpanCondition spanCondition) {
    return ((UnicodeSet*) set)->UnicodeSet::span(s, length, spanCondition);
}

U_CAPI int32_t U_EXPORT2
uset_spanBack(const USet *set, const UChar *s, int32_t length, USetSpanCondition spanCondition) {
    return ((UnicodeSet*) set)->UnicodeSet::spanBack(s, length, spanCondition);
}

U_CAPI int32_t U_EXPORT2
uset_spanUTF8(const USet *set, const char *s, int32_t length, USetSpanCondition spanCondition) {
    return ((UnicodeSet*) set)->UnicodeSet::spanUTF8(s, length, spanCondition);
}

U_CAPI int32_t U_EXPORT2
uset_spanBackUTF8(const USet *set, const char *s, int32_t length, USetSpanCondition spanCondition) {
    return ((UnicodeSet*) set)->UnicodeSet::spanBackUTF8(s, length, spanCondition);
}

U_CAPI UBool U_EXPORT2
uset_equals(const USet* set1, const USet* set2) {
    return *(const UnicodeSet*)set1 == *(const UnicodeSet*)set2;
}

U_CAPI int32_t U_EXPORT2
uset_indexOf(const USet* set, UChar32 c) {
    return ((UnicodeSet*) set)->UnicodeSet::indexOf(c);
}

U_CAPI UChar32 U_EXPORT2
uset_charAt(const USet* set, int32_t index) {
    return ((UnicodeSet*) set)->UnicodeSet::charAt(index);
}

U_CAPI int32_t U_EXPORT2
uset_size(const USet* set) {
    return ((const UnicodeSet*) set)->UnicodeSet::size();
}

U_NAMESPACE_BEGIN





class USetAccess  {
public:
    
    inline static int32_t getStringCount(const UnicodeSet& set) {
        return set.getStringCount();
    }
    inline static const UnicodeString* getString(const UnicodeSet& set,
                                                 int32_t i) {
        return set.getString(i);
    }
private:
    
    USetAccess();
};
U_NAMESPACE_END

U_CAPI int32_t U_EXPORT2
uset_getItemCount(const USet* uset) {
    const UnicodeSet& set = *(const UnicodeSet*)uset;
    return set.getRangeCount() + USetAccess::getStringCount(set);
}

U_CAPI int32_t U_EXPORT2
uset_getItem(const USet* uset, int32_t itemIndex,
             UChar32* start, UChar32* end,
             UChar* str, int32_t strCapacity,
             UErrorCode* ec) {
    if (U_FAILURE(*ec)) return 0;
    const UnicodeSet& set = *(const UnicodeSet*)uset;
    int32_t rangeCount;

    if (itemIndex < 0) {
        *ec = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    } else if (itemIndex < (rangeCount = set.getRangeCount())) {
        *start = set.getRangeStart(itemIndex);
        *end = set.getRangeEnd(itemIndex);
        return 0;
    } else {
        itemIndex -= rangeCount;
        if (itemIndex < USetAccess::getStringCount(set)) {
            const UnicodeString* s = USetAccess::getString(set, itemIndex);
            return s->extract(str, strCapacity, *ec);
        } else {
            *ec = U_INDEX_OUTOFBOUNDS_ERROR;
            return -1;
        }
    }
}




































U_CAPI int32_t U_EXPORT2
uset_serialize(const USet* set, uint16_t* dest, int32_t destCapacity, UErrorCode* ec) {
    if (ec==NULL || U_FAILURE(*ec)) {
        return 0;
    }

    return ((const UnicodeSet*) set)->UnicodeSet::serialize(dest, destCapacity,* ec);
}

U_CAPI UBool U_EXPORT2
uset_getSerializedSet(USerializedSet* fillSet, const uint16_t* src, int32_t srcLength) {
    int32_t length;

    if(fillSet==NULL) {
        return FALSE;
    }
    if(src==NULL || srcLength<=0) {
        fillSet->length=fillSet->bmpLength=0;
        return FALSE;
    }

    length=*src++;
    if(length&0x8000) {
        
        length&=0x7fff;
        if(srcLength<(2+length)) {
            fillSet->length=fillSet->bmpLength=0;
            return FALSE;
        }
        fillSet->bmpLength=*src++;
    } else {
        
        if(srcLength<(1+length)) {
            fillSet->length=fillSet->bmpLength=0;
            return FALSE;
        }
        fillSet->bmpLength=length;
    }
    fillSet->array=src;
    fillSet->length=length;
    return TRUE;
}

U_CAPI void U_EXPORT2
uset_setSerializedToOne(USerializedSet* fillSet, UChar32 c) {
    if(fillSet==NULL || (uint32_t)c>0x10ffff) {
        return;
    }

    fillSet->array=fillSet->staticArray;
    if(c<0xffff) {
        fillSet->bmpLength=fillSet->length=2;
        fillSet->staticArray[0]=(uint16_t)c;
        fillSet->staticArray[1]=(uint16_t)c+1;
    } else if(c==0xffff) {
        fillSet->bmpLength=1;
        fillSet->length=3;
        fillSet->staticArray[0]=0xffff;
        fillSet->staticArray[1]=1;
        fillSet->staticArray[2]=0;
    } else if(c<0x10ffff) {
        fillSet->bmpLength=0;
        fillSet->length=4;
        fillSet->staticArray[0]=(uint16_t)(c>>16);
        fillSet->staticArray[1]=(uint16_t)c;
        ++c;
        fillSet->staticArray[2]=(uint16_t)(c>>16);
        fillSet->staticArray[3]=(uint16_t)c;
    } else  {
        fillSet->bmpLength=0;
        fillSet->length=2;
        fillSet->staticArray[0]=0x10;
        fillSet->staticArray[1]=0xffff;
    }
}

U_CAPI UBool U_EXPORT2
uset_serializedContains(const USerializedSet* set, UChar32 c) {
    const uint16_t* array;

    if(set==NULL || (uint32_t)c>0x10ffff) {
        return FALSE;
    }

    array=set->array;
    if(c<=0xffff) {
        
        int32_t lo = 0;
        int32_t hi = set->bmpLength-1;
        if (c < array[0]) {
            hi = 0;
        } else if (c < array[hi]) {
            for(;;) {
                int32_t i = (lo + hi) >> 1;
                if (i == lo) {
                    break;  
                } else if (c < array[i]) {
                    hi = i;
                } else {
                    lo = i;
                }
            }
        } else {
            hi += 1;
        }
        return (UBool)(hi&1);
    } else {
        
        uint16_t high=(uint16_t)(c>>16), low=(uint16_t)c;
        int32_t base = set->bmpLength;
        int32_t lo = 0;
        int32_t hi = set->length - 2 - base;
        if (high < array[base] || (high==array[base] && low<array[base+1])) {
            hi = 0;
        } else if (high < array[base+hi] || (high==array[base+hi] && low<array[base+hi+1])) {
            for (;;) {
                int32_t i = ((lo + hi) >> 1) & ~1;  
                int32_t iabs = i + base;
                if (i == lo) {
                    break;  
                } else if (high < array[iabs] || (high==array[iabs] && low<array[iabs+1])) {
                    hi = i;
                } else {
                    lo = i;
                }
            }
        } else {
            hi += 2;
        }
        
        return (UBool)(((hi+(base<<1))&2)!=0);
    }
}

U_CAPI int32_t U_EXPORT2
uset_getSerializedRangeCount(const USerializedSet* set) {
    if(set==NULL) {
        return 0;
    }

    return (set->bmpLength+(set->length-set->bmpLength)/2+1)/2;
}

U_CAPI UBool U_EXPORT2
uset_getSerializedRange(const USerializedSet* set, int32_t rangeIndex,
                        UChar32* pStart, UChar32* pEnd) {
    const uint16_t* array;
    int32_t bmpLength, length;

    if(set==NULL || rangeIndex<0 || pStart==NULL || pEnd==NULL) {
        return FALSE;
    }

    array=set->array;
    length=set->length;
    bmpLength=set->bmpLength;

    rangeIndex*=2; 
    if(rangeIndex<bmpLength) {
        *pStart=array[rangeIndex++];
        if(rangeIndex<bmpLength) {
            *pEnd=array[rangeIndex]-1;
        } else if(rangeIndex<length) {
            *pEnd=((((int32_t)array[rangeIndex])<<16)|array[rangeIndex+1])-1;
        } else {
            *pEnd=0x10ffff;
        }
        return TRUE;
    } else {
        rangeIndex-=bmpLength;
        rangeIndex*=2; 
        length-=bmpLength;
        if(rangeIndex<length) {
            array+=bmpLength;
            *pStart=(((int32_t)array[rangeIndex])<<16)|array[rangeIndex+1];
            rangeIndex+=2;
            if(rangeIndex<length) {
                *pEnd=((((int32_t)array[rangeIndex])<<16)|array[rangeIndex+1])-1;
            } else {
                *pEnd=0x10ffff;
            }
            return TRUE;
        } else {
            return FALSE;
        }
    }
}


























































































































