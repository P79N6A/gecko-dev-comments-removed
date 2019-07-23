





































#ifndef nsCSSPropertySet_h__
#define nsCSSPropertySet_h__

#include "nsCSSProperty.h"






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
        mProperties[aProperty / kBitsInChunk] |=
            property_set_type(1 << (aProperty % kBitsInChunk));
    }

    void RemoveProperty(nsCSSProperty aProperty) {
        AssertInSetRange(aProperty);
        mProperties[aProperty / kBitsInChunk] &=
            ~property_set_type(1 << (aProperty % kBitsInChunk));
    }

    PRBool HasProperty(nsCSSProperty aProperty) const {
        AssertInSetRange(aProperty);
        return (mProperties[aProperty / kBitsInChunk] &
                (1 << (aProperty % kBitsInChunk))) != 0;
    }

    void Empty() {
        memset(mProperties, 0, sizeof(mProperties));
    }

    void AssertIsEmpty(const char* aText) const {
        for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(mProperties); ++i) {
            NS_ASSERTION(mProperties[i] == 0, aText);
        }
    }

private:
    typedef PRUint8 property_set_type;
public:
    enum { kBitsInChunk = 8 }; 
                                          
    
    enum { kChunkCount =
             (eCSSProperty_COUNT_no_shorthands + (kBitsInChunk-1)) /
             kBitsInChunk };

    




    PRBool HasPropertyInChunk(PRUint32 aChunk) const {
        return mProperties[aChunk] != 0;
    }
    PRBool HasPropertyAt(PRUint32 aChunk, PRInt32 aBit) const {
        return (mProperties[aChunk] & (1 << aBit)) != 0;
    }
    static nsCSSProperty CSSPropertyAt(PRUint32 aChunk, PRInt32 aBit) {
        return nsCSSProperty(aChunk * kBitsInChunk + aBit);
    }

private:
    property_set_type mProperties[kChunkCount];
};

#endif
