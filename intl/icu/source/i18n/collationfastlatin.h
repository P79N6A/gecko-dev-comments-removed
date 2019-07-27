










#ifndef __COLLATIONFASTLATIN_H__
#define __COLLATIONFASTLATIN_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

U_NAMESPACE_BEGIN

struct CollationData;
struct CollationSettings;

class U_I18N_API CollationFastLatin  {
public:
    







    static const uint16_t VERSION = 2;

    static const int32_t LATIN_MAX = 0x17f;
    static const int32_t LATIN_LIMIT = LATIN_MAX + 1;

    static const int32_t LATIN_MAX_UTF8_LEAD = 0xc5;  

    static const int32_t PUNCT_START = 0x2000;
    static const int32_t PUNCT_LIMIT = 0x2040;

    
    static const int32_t NUM_FAST_CHARS = LATIN_LIMIT + (PUNCT_LIMIT - PUNCT_START);

    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    

    static const uint32_t SHORT_PRIMARY_MASK = 0xfc00;  
    static const uint32_t INDEX_MASK = 0x3ff;  
    static const uint32_t SECONDARY_MASK = 0x3e0;  
    static const uint32_t CASE_MASK = 0x18;  
    static const uint32_t LONG_PRIMARY_MASK = 0xfff8;  
    static const uint32_t TERTIARY_MASK = 7;  
    static const uint32_t CASE_AND_TERTIARY_MASK = CASE_MASK | TERTIARY_MASK;

    static const uint32_t TWO_SHORT_PRIMARIES_MASK =
            (SHORT_PRIMARY_MASK << 16) | SHORT_PRIMARY_MASK;  
    static const uint32_t TWO_LONG_PRIMARIES_MASK =
            (LONG_PRIMARY_MASK << 16) | LONG_PRIMARY_MASK;  
    static const uint32_t TWO_SECONDARIES_MASK =
            (SECONDARY_MASK << 16) | SECONDARY_MASK;  
    static const uint32_t TWO_CASES_MASK =
            (CASE_MASK << 16) | CASE_MASK;  
    static const uint32_t TWO_TERTIARIES_MASK =
            (TERTIARY_MASK << 16) | TERTIARY_MASK;  

    












    static const uint32_t CONTRACTION = 0x400;
    






    static const uint32_t EXPANSION = 0x800;
    




    static const uint32_t MIN_LONG = 0xc00;
    static const uint32_t LONG_INC = 8;
    static const uint32_t MAX_LONG = 0xff8;
    




    static const uint32_t MIN_SHORT = 0x1000;
    static const uint32_t SHORT_INC = 0x400;
    
    static const uint32_t MAX_SHORT = SHORT_PRIMARY_MASK;

    static const uint32_t MIN_SEC_BEFORE = 0;  
    static const uint32_t SEC_INC = 0x20;
    static const uint32_t MAX_SEC_BEFORE = MIN_SEC_BEFORE + 4 * SEC_INC;  
    static const uint32_t COMMON_SEC = MAX_SEC_BEFORE + SEC_INC;
    static const uint32_t MIN_SEC_AFTER = COMMON_SEC + SEC_INC;
    static const uint32_t MAX_SEC_AFTER = MIN_SEC_AFTER + 5 * SEC_INC;  
    static const uint32_t MIN_SEC_HIGH = MAX_SEC_AFTER + SEC_INC;  
    static const uint32_t MAX_SEC_HIGH = SECONDARY_MASK;

    




    static const uint32_t SEC_OFFSET = SEC_INC;
    static const uint32_t COMMON_SEC_PLUS_OFFSET = COMMON_SEC + SEC_OFFSET;

    static const uint32_t TWO_SEC_OFFSETS =
            (SEC_OFFSET << 16) | SEC_OFFSET;  
    static const uint32_t TWO_COMMON_SEC_PLUS_OFFSET =
            (COMMON_SEC_PLUS_OFFSET << 16) | COMMON_SEC_PLUS_OFFSET;

    static const uint32_t LOWER_CASE = 8;  
    static const uint32_t TWO_LOWER_CASES = (LOWER_CASE << 16) | LOWER_CASE;  

    static const uint32_t COMMON_TER = 0;  
    static const uint32_t MAX_TER_AFTER = 7;  

    






    static const uint32_t TER_OFFSET = SEC_OFFSET;
    static const uint32_t COMMON_TER_PLUS_OFFSET = COMMON_TER + TER_OFFSET;

    static const uint32_t TWO_TER_OFFSETS = (TER_OFFSET << 16) | TER_OFFSET;
    static const uint32_t TWO_COMMON_TER_PLUS_OFFSET =
            (COMMON_TER_PLUS_OFFSET << 16) | COMMON_TER_PLUS_OFFSET;

    static const uint32_t MERGE_WEIGHT = 3;
    static const uint32_t EOS = 2;  
    static const uint32_t BAIL_OUT = 1;

    




    static const uint32_t CONTR_CHAR_MASK = 0x1ff;
    



    static const uint32_t CONTR_LENGTH_SHIFT = 9;

    



    static const int32_t BAIL_OUT_RESULT = -2;

    static inline int32_t getCharIndex(UChar c) {
        if(c <= LATIN_MAX) {
            return c;
        } else if(PUNCT_START <= c && c < PUNCT_LIMIT) {
            return c - (PUNCT_START - LATIN_LIMIT);
        } else {
            
            
            
            return -1;
        }
    }

    





    static int32_t getOptions(const CollationData *data, const CollationSettings &settings,
                              uint16_t *primaries, int32_t capacity);

    static int32_t compareUTF16(const uint16_t *table, const uint16_t *primaries, int32_t options,
                                const UChar *left, int32_t leftLength,
                                const UChar *right, int32_t rightLength);

    static int32_t compareUTF8(const uint16_t *table, const uint16_t *primaries, int32_t options,
                               const uint8_t *left, int32_t leftLength,
                               const uint8_t *right, int32_t rightLength);

private:
    static uint32_t lookup(const uint16_t *table, UChar32 c);
    static uint32_t lookupUTF8(const uint16_t *table, UChar32 c,
                               const uint8_t *s8, int32_t &sIndex, int32_t sLength);
    static uint32_t lookupUTF8Unsafe(const uint16_t *table, UChar32 c,
                                     const uint8_t *s8, int32_t &sIndex);

    static uint32_t nextPair(const uint16_t *table, UChar32 c, uint32_t ce,
                             const UChar *s16, const uint8_t *s8, int32_t &sIndex, int32_t &sLength);

    static inline uint32_t getPrimaries(uint32_t variableTop, uint32_t pair) {
        uint32_t ce = pair & 0xffff;
        if(ce >= MIN_SHORT) { return pair & TWO_SHORT_PRIMARIES_MASK; }
        if(ce > variableTop) { return pair & TWO_LONG_PRIMARIES_MASK; }
        if(ce >= MIN_LONG) { return 0; }  
        return pair;  
    }
    static inline uint32_t getSecondariesFromOneShortCE(uint32_t ce) {
        ce &= SECONDARY_MASK;
        if(ce < MIN_SEC_HIGH) {
            return ce + SEC_OFFSET;
        } else {
            return ((ce + SEC_OFFSET) << 16) | COMMON_SEC_PLUS_OFFSET;
        }
    }
    static uint32_t getSecondaries(uint32_t variableTop, uint32_t pair);
    static uint32_t getCases(uint32_t variableTop, UBool strengthIsPrimary, uint32_t pair);
    static uint32_t getTertiaries(uint32_t variableTop, UBool withCaseBits, uint32_t pair);
    static uint32_t getQuaternaries(uint32_t variableTop, uint32_t pair);

private:
    CollationFastLatin();  
};






























































U_NAMESPACE_END

#endif  
#endif  
