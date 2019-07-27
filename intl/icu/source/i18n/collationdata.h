










#ifndef __COLLATIONDATA_H__
#define __COLLATIONDATA_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uniset.h"
#include "collation.h"
#include "normalizer2impl.h"
#include "utrie2.h"

struct UDataMemory;

U_NAMESPACE_BEGIN

class UVector32;








struct U_I18N_API CollationData : public UMemory {
    
    
    
    enum {
        REORDER_RESERVED_BEFORE_LATIN = UCOL_REORDER_CODE_FIRST + 14,
        REORDER_RESERVED_AFTER_LATIN
    };

    enum {
        MAX_NUM_SPECIAL_REORDER_CODES = 8,
        
        MAX_NUM_SCRIPT_RANGES = 256
    };

    CollationData(const Normalizer2Impl &nfc)
            : trie(NULL),
              ce32s(NULL), ces(NULL), contexts(NULL), base(NULL),
              jamoCE32s(NULL),
              nfcImpl(nfc),
              numericPrimary(0x12000000),
              ce32sLength(0), cesLength(0), contextsLength(0),
              compressibleBytes(NULL),
              unsafeBackwardSet(NULL),
              fastLatinTable(NULL), fastLatinTableLength(0),
              numScripts(0), scriptsIndex(NULL), scriptStarts(NULL), scriptStartsLength(0),
              rootElements(NULL), rootElementsLength(0) {}

    uint32_t getCE32(UChar32 c) const {
        return UTRIE2_GET32(trie, c);
    }

    uint32_t getCE32FromSupplementary(UChar32 c) const {
        return UTRIE2_GET32_FROM_SUPP(trie, c);
    }

    UBool isDigit(UChar32 c) const {
        return c < 0x660 ? c <= 0x39 && 0x30 <= c :
                Collation::hasCE32Tag(getCE32(c), Collation::DIGIT_TAG);
    }

    UBool isUnsafeBackward(UChar32 c, UBool numeric) const {
        return unsafeBackwardSet->contains(c) || (numeric && isDigit(c));
    }

    UBool isCompressibleLeadByte(uint32_t b) const {
        return compressibleBytes[b];
    }

    inline UBool isCompressiblePrimary(uint32_t p) const {
        return isCompressibleLeadByte(p >> 24);
    }

    



    static uint32_t readCE32(const UChar *p) {
        return ((uint32_t)p[0] << 16) | p[1];
    }

    



    uint32_t getIndirectCE32(uint32_t ce32) const;
    



    uint32_t getFinalCE32(uint32_t ce32) const;

    


    int64_t getCEFromOffsetCE32(UChar32 c, uint32_t ce32) const {
        int64_t dataCE = ces[Collation::indexFromCE32(ce32)];
        return Collation::makeCE(Collation::getThreeBytePrimaryForOffsetData(c, dataCE));
    }

    



    int64_t getSingleCE(UChar32 c, UErrorCode &errorCode) const;

    


    uint16_t getFCD16(UChar32 c) const {
        return nfcImpl.getFCD16(c);
    }

    





    uint32_t getFirstPrimaryForGroup(int32_t script) const;

    





    uint32_t getLastPrimaryForGroup(int32_t script) const;

    



    int32_t getGroupForPrimary(uint32_t p) const;

    int32_t getEquivalentScripts(int32_t script,
                                 int32_t dest[], int32_t capacity, UErrorCode &errorCode) const;

    









    void makeReorderRanges(const int32_t *reorder, int32_t length,
                           UVector32 &ranges, UErrorCode &errorCode) const;

    
    static const int32_t JAMO_CE32S_LENGTH = 19 + 21 + 27;

    
    const UTrie2 *trie;
    




    const uint32_t *ce32s;
    
    const int64_t *ces;
    
    const UChar *contexts;
    
    const CollationData *base;
    




    const uint32_t *jamoCE32s;
    const Normalizer2Impl &nfcImpl;
    
    uint32_t numericPrimary;

    int32_t ce32sLength;
    int32_t cesLength;
    int32_t contextsLength;

    
    const UBool *compressibleBytes;
    



    const UnicodeSet *unsafeBackwardSet;

    



    const uint16_t *fastLatinTable;
    int32_t fastLatinTableLength;

    




    int32_t numScripts;
    








    const uint16_t *scriptsIndex;
    





    const uint16_t *scriptStarts;
    int32_t scriptStartsLength;

    




    const uint32_t *rootElements;
    int32_t rootElementsLength;

private:
    int32_t getScriptIndex(int32_t script) const;
    void makeReorderRanges(const int32_t *reorder, int32_t length,
                           UBool latinMustMove,
                           UVector32 &ranges, UErrorCode &errorCode) const;
    int32_t addLowScriptRange(uint8_t table[], int32_t index, int32_t lowStart) const;
    int32_t addHighScriptRange(uint8_t table[], int32_t index, int32_t highLimit) const;
};

U_NAMESPACE_END

#endif  
#endif  
