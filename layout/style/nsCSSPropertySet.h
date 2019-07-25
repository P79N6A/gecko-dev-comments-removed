





































#ifndef nsCSSPropertySet_h__
#define nsCSSPropertySet_h__

#include "mozilla/Util.h"

#include "nsCSSProperty.h"
#include <limits.h> 






class nsCSSPropertySet {
public:
    nsCSSPropertySet() { Empty(); }
    

    void AssertInSetRange(nsCSSProperty aProperty) const {
        NS_ASSERTION(0 <= aProperty &&
                     aProperty < eCSSProperty_COUNT_no_shorthands,
                     "out of bounds");
    }

    
    

    void AddProperty(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        size_t p = aProperty;
        mProperties[p / kBitsInChunk] |=
          property_set_type(1) << (p % kBitsInChunk);
    }

    void RemoveProperty(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        size_t p = aProperty;
        mProperties[p / kBitsInChunk] &=
            ~(property_set_type(1) << (p % kBitsInChunk));
    }

    bool HasProperty(nsCSSProperty aProperty) const {
        AssertInSetRange(aProperty);
        size_t p = aProperty;
        return (mProperties[p / kBitsInChunk] &
                (property_set_type(1) << (p % kBitsInChunk))) != 0;
    }

    void Empty() {
        memset(mProperties, 0, sizeof(mProperties));
    }

    void AssertIsEmpty(const char* aText) const {
        for (size_t i = 0; i < mozilla::ArrayLength(mProperties); ++i) {
            NS_ASSERTION(mProperties[i] == 0, aText);
        }
    }

private:
    typedef unsigned long property_set_type;
public:
    
    static const size_t kBitsInChunk = sizeof(property_set_type)*CHAR_BIT;
    
    static const size_t kChunkCount =
        (eCSSProperty_COUNT_no_shorthands + kBitsInChunk - 1) / kBitsInChunk;

    




    bool HasPropertyInChunk(size_t aChunk) const {
        return mProperties[aChunk] != 0;
    }
    bool HasPropertyAt(size_t aChunk, size_t aBit) const {
        return (mProperties[aChunk] & (property_set_type(1) << aBit)) != 0;
    }
    static nsCSSProperty CSSPropertyAt(size_t aChunk, size_t aBit) {
        return nsCSSProperty(aChunk * kBitsInChunk + aBit);
    }

private:
    property_set_type mProperties[kChunkCount];
};

#endif 
