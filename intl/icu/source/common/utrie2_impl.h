


















#ifndef __UTRIE2_IMPL_H__
#define __UTRIE2_IMPL_H__

#include "utrie2.h"












#define UTRIE_SIG       0x54726965
#define UTRIE_OE_SIG    0x65697254

#define UTRIE2_SIG      0x54726932
#define UTRIE2_OE_SIG   0x32697254









typedef struct UTrie2Header {
    
    uint32_t signature;

    




    uint16_t options;

    
    uint16_t indexLength;

    
    uint16_t shiftedDataLength;

    
    uint16_t index2NullOffset, dataNullOffset;

    



    uint16_t shiftedHighStart;
} UTrie2Header;





enum {
    
    UTRIE2_OPTIONS_VALUE_BITS_MASK=0xf
};








enum {
    





    UNEWTRIE2_INDEX_GAP_OFFSET=UTRIE2_INDEX_2_BMP_LENGTH,
    UNEWTRIE2_INDEX_GAP_LENGTH=
        ((UTRIE2_UTF8_2B_INDEX_2_LENGTH+UTRIE2_MAX_INDEX_1_LENGTH)+UTRIE2_INDEX_2_MASK)&
        ~UTRIE2_INDEX_2_MASK,

    






    UNEWTRIE2_MAX_INDEX_2_LENGTH=
        (0x110000>>UTRIE2_SHIFT_2)+
        UTRIE2_LSCP_INDEX_2_LENGTH+
        UNEWTRIE2_INDEX_GAP_LENGTH+
        UTRIE2_INDEX_2_BLOCK_LENGTH,

    UNEWTRIE2_INDEX_1_LENGTH=0x110000>>UTRIE2_SHIFT_1
};






#define UNEWTRIE2_MAX_DATA_LENGTH (0x110000+0x40+0x40+0x400)















struct UNewTrie2 {
    int32_t index1[UNEWTRIE2_INDEX_1_LENGTH];
    int32_t index2[UNEWTRIE2_MAX_INDEX_2_LENGTH];
    uint32_t *data;

    uint32_t initialValue, errorValue;
    int32_t index2Length, dataCapacity, dataLength;
    int32_t firstFreeBlock;
    int32_t index2NullOffset, dataNullOffset;
    UChar32 highStart;
    UBool isCompacted;

    














    int32_t map[UNEWTRIE2_MAX_DATA_LENGTH>>UTRIE2_SHIFT_2];
};

#endif
