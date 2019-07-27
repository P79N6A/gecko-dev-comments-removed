










#ifndef __COLLATION_H__
#define __COLLATION_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

U_NAMESPACE_BEGIN







class U_I18N_API Collation {
public:
    
    static const uint8_t TERMINATOR_BYTE = 0;
    static const uint8_t LEVEL_SEPARATOR_BYTE = 1;

    
    static const uint32_t BEFORE_WEIGHT16 = 0x0100;

    





    static const uint8_t MERGE_SEPARATOR_BYTE = 2;
    static const uint32_t MERGE_SEPARATOR_PRIMARY = 0x02000000;  
    static const uint32_t MERGE_SEPARATOR_CE32 = 0x02000505;  

    




    static const uint8_t PRIMARY_COMPRESSION_LOW_BYTE = 3;
    




    static const uint8_t PRIMARY_COMPRESSION_HIGH_BYTE = 0xff;

    
    static const uint8_t COMMON_BYTE = 5;
    static const uint32_t COMMON_WEIGHT16 = 0x0500;
    
    static const uint32_t COMMON_SECONDARY_CE = 0x05000000;
    
    static const uint32_t COMMON_TERTIARY_CE = 0x0500;
    
    static const uint32_t COMMON_SEC_AND_TER_CE = 0x05000500;

    static const uint32_t SECONDARY_MASK = 0xffff0000;
    static const uint32_t CASE_MASK = 0xc000;
    static const uint32_t SECONDARY_AND_CASE_MASK = SECONDARY_MASK | CASE_MASK;
    
    static const uint32_t ONLY_TERTIARY_MASK = 0x3f3f;
    
    static const uint32_t ONLY_SEC_TER_MASK = SECONDARY_MASK | ONLY_TERTIARY_MASK;
    
    static const uint32_t CASE_AND_TERTIARY_MASK = CASE_MASK | ONLY_TERTIARY_MASK;
    static const uint32_t QUATERNARY_MASK = 0xc0;
    
    static const uint32_t CASE_AND_QUATERNARY_MASK = CASE_MASK | QUATERNARY_MASK;

    static const uint8_t UNASSIGNED_IMPLICIT_BYTE = 0xfe;  
    







    static const uint32_t FIRST_UNASSIGNED_PRIMARY = 0xfe040200;

    static const uint8_t TRAIL_WEIGHT_BYTE = 0xff;  
    static const uint32_t FIRST_TRAILING_PRIMARY = 0xff020200;  
    static const uint32_t MAX_PRIMARY = 0xffff0000;  
    static const uint32_t MAX_REGULAR_CE32 = 0xffff0505;  

    
    
    static const uint32_t FFFD_PRIMARY = MAX_PRIMARY - 0x20000;
    static const uint32_t FFFD_CE32 = MAX_REGULAR_CE32 - 0x20000;

    




    static const uint8_t SPECIAL_CE32_LOW_BYTE = 0xc0;
    static const uint32_t FALLBACK_CE32 = SPECIAL_CE32_LOW_BYTE;
    


    static const uint8_t LONG_PRIMARY_CE32_LOW_BYTE = 0xc1;  

    static const uint32_t UNASSIGNED_CE32 = 0xffffffff;  

    static const uint32_t NO_CE32 = 1;

    
    static const uint32_t NO_CE_PRIMARY = 1;  
    static const uint32_t NO_CE_WEIGHT16 = 0x0100;  
    static const int64_t NO_CE = INT64_C(0x101000100);  

    
    enum Level {
        
        NO_LEVEL,
        PRIMARY_LEVEL,
        SECONDARY_LEVEL,
        CASE_LEVEL,
        TERTIARY_LEVEL,
        QUATERNARY_LEVEL,
        IDENTICAL_LEVEL,
        
        ZERO_LEVEL
    };

    



    static const uint32_t NO_LEVEL_FLAG = 1;
    static const uint32_t PRIMARY_LEVEL_FLAG = 2;
    static const uint32_t SECONDARY_LEVEL_FLAG = 4;
    static const uint32_t CASE_LEVEL_FLAG = 8;
    static const uint32_t TERTIARY_LEVEL_FLAG = 0x10;
    static const uint32_t QUATERNARY_LEVEL_FLAG = 0x20;
    static const uint32_t IDENTICAL_LEVEL_FLAG = 0x40;
    static const uint32_t ZERO_LEVEL_FLAG = 0x80;

    




    enum {
        




        FALLBACK_TAG = 0,
        



        LONG_PRIMARY_TAG = 1,
        




        LONG_SECONDARY_TAG = 2,
        





        RESERVED_TAG_3 = 3,
        





        LATIN_EXPANSION_TAG = 4,
        




        EXPANSION32_TAG = 5,
        




        EXPANSION_TAG = 6,
        











        BUILDER_DATA_TAG = 7,
        




        PREFIX_TAG = 8,
        







        CONTRACTION_TAG = 9,
        





        DIGIT_TAG = 10,
        




        U0000_TAG = 11,
        




        HANGUL_TAG = 12,
        







        LEAD_SURROGATE_TAG = 13,
        










        OFFSET_TAG = 14,
        



        IMPLICIT_TAG = 15
    };

    static UBool isAssignedCE32(uint32_t ce32) {
        return ce32 != FALLBACK_CE32 && ce32 != UNASSIGNED_CE32;
    }

    




    static const int32_t MAX_EXPANSION_LENGTH = 31;
    static const int32_t MAX_INDEX = 0x7ffff;

    






    static const uint32_t CONTRACT_SINGLE_CP_NO_MATCH = 0x100;
    
    static const uint32_t CONTRACT_NEXT_CCC = 0x200;
    
    static const uint32_t CONTRACT_TRAILING_CCC = 0x400;

    
    static const uint32_t HANGUL_NO_SPECIAL_JAMO = 0x100;

    static const uint32_t LEAD_ALL_UNASSIGNED = 0;
    static const uint32_t LEAD_ALL_FALLBACK = 0x100;
    static const uint32_t LEAD_MIXED = 0x200;
    static const uint32_t LEAD_TYPE_MASK = 0x300;

    static uint32_t makeLongPrimaryCE32(uint32_t p) { return p | LONG_PRIMARY_CE32_LOW_BYTE; }

    
    static inline uint32_t primaryFromLongPrimaryCE32(uint32_t ce32) {
        return ce32 & 0xffffff00;
    }
    static inline int64_t ceFromLongPrimaryCE32(uint32_t ce32) {
        return ((int64_t)(ce32 & 0xffffff00) << 32) | COMMON_SEC_AND_TER_CE;
    }

    static uint32_t makeLongSecondaryCE32(uint32_t lower32) {
        return lower32 | SPECIAL_CE32_LOW_BYTE | LONG_SECONDARY_TAG;
    }
    static inline int64_t ceFromLongSecondaryCE32(uint32_t ce32) {
        return ce32 & 0xffffff00;
    }

    
    static uint32_t makeCE32FromTagIndexAndLength(int32_t tag, int32_t index, int32_t length) {
        return (index << 13) | (length << 8) | SPECIAL_CE32_LOW_BYTE | tag;
    }
    
    static uint32_t makeCE32FromTagAndIndex(int32_t tag, int32_t index) {
        return (index << 13) | SPECIAL_CE32_LOW_BYTE | tag;
    }

    static inline UBool isSpecialCE32(uint32_t ce32) {
        return (ce32 & 0xff) >= SPECIAL_CE32_LOW_BYTE;
    }

    static inline int32_t tagFromCE32(uint32_t ce32) {
        return (int32_t)(ce32 & 0xf);
    }

    static inline UBool hasCE32Tag(uint32_t ce32, int32_t tag) {
        return isSpecialCE32(ce32) && tagFromCE32(ce32) == tag;
    }

    static inline UBool isLongPrimaryCE32(uint32_t ce32) {
        return hasCE32Tag(ce32, LONG_PRIMARY_TAG);
    }

    static UBool isSimpleOrLongCE32(uint32_t ce32) {
        return !isSpecialCE32(ce32) ||
                tagFromCE32(ce32) == LONG_PRIMARY_TAG ||
                tagFromCE32(ce32) == LONG_SECONDARY_TAG;
    }

    


    static UBool isSelfContainedCE32(uint32_t ce32) {
        return !isSpecialCE32(ce32) ||
                tagFromCE32(ce32) == LONG_PRIMARY_TAG ||
                tagFromCE32(ce32) == LONG_SECONDARY_TAG ||
                tagFromCE32(ce32) == LATIN_EXPANSION_TAG;
    }

    static inline UBool isPrefixCE32(uint32_t ce32) {
        return hasCE32Tag(ce32, PREFIX_TAG);
    }

    static inline UBool isContractionCE32(uint32_t ce32) {
        return hasCE32Tag(ce32, CONTRACTION_TAG);
    }

    static inline UBool ce32HasContext(uint32_t ce32) {
        return isSpecialCE32(ce32) &&
                (tagFromCE32(ce32) == PREFIX_TAG ||
                tagFromCE32(ce32) == CONTRACTION_TAG);
    }

    



    static inline int64_t latinCE0FromCE32(uint32_t ce32) {
        return ((int64_t)(ce32 & 0xff000000) << 32) | COMMON_SECONDARY_CE | ((ce32 & 0xff0000) >> 8);
    }

    



    static inline int64_t latinCE1FromCE32(uint32_t ce32) {
        return ((ce32 & 0xff00) << 16) | COMMON_TERTIARY_CE;
    }

    


    static inline int32_t indexFromCE32(uint32_t ce32) {
        return (int32_t)(ce32 >> 13);
    }

    


    static inline int32_t lengthFromCE32(uint32_t ce32) {
        return (ce32 >> 8) & 31;
    }

    


    static inline char digitFromCE32(uint32_t ce32) {
        return (char)((ce32 >> 8) & 0xf);
    }

    
    static inline int64_t ceFromSimpleCE32(uint32_t ce32) {
        
        
        return ((int64_t)(ce32 & 0xffff0000) << 32) | ((ce32 & 0xff00) << 16) | ((ce32 & 0xff) << 8);
    }

    
    static inline int64_t ceFromCE32(uint32_t ce32) {
        uint32_t tertiary = ce32 & 0xff;
        if(tertiary < SPECIAL_CE32_LOW_BYTE) {
            
            return ((int64_t)(ce32 & 0xffff0000) << 32) | ((ce32 & 0xff00) << 16) | (tertiary << 8);
        } else {
            ce32 -= tertiary;
            if((tertiary & 0xf) == LONG_PRIMARY_TAG) {
                
                return ((int64_t)ce32 << 32) | COMMON_SEC_AND_TER_CE;
            } else {
                
                
                return ce32;
            }
        }
    }

    
    static inline int64_t makeCE(uint32_t p) {
        return ((int64_t)p << 32) | COMMON_SEC_AND_TER_CE;
    }
    



    static inline int64_t makeCE(uint32_t p, uint32_t s, uint32_t t, uint32_t q) {
        return ((int64_t)p << 32) | (s << 16) | t | (q << 6);
    }

    


    static uint32_t incTwoBytePrimaryByOffset(uint32_t basePrimary, UBool isCompressible,
                                              int32_t offset);

    


    static uint32_t incThreeBytePrimaryByOffset(uint32_t basePrimary, UBool isCompressible,
                                                int32_t offset);

    


    static uint32_t decTwoBytePrimaryByOneStep(uint32_t basePrimary, UBool isCompressible, int32_t step);

    


    static uint32_t decThreeBytePrimaryByOneStep(uint32_t basePrimary, UBool isCompressible, int32_t step);

    


    static uint32_t getThreeBytePrimaryForOffsetData(UChar32 c, int64_t dataCE);

    


    static uint32_t unassignedPrimaryFromCodePoint(UChar32 c);

    static inline int64_t unassignedCEFromCodePoint(UChar32 c) {
        return makeCE(unassignedPrimaryFromCodePoint(c));
    }

private:
    Collation();  
};

U_NAMESPACE_END

#endif  
#endif  
