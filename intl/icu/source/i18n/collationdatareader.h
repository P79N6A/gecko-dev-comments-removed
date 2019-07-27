










#ifndef __COLLATIONDATAREADER_H__
#define __COLLATIONDATAREADER_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/udata.h"

struct UDataMemory;

U_NAMESPACE_BEGIN

struct CollationTailoring;




struct U_I18N_API CollationDataReader  {
    
    
    enum {
        






        IX_INDEXES_LENGTH,  
        




        IX_OPTIONS,
        IX_RESERVED2,
        IX_RESERVED3,

        
        IX_JAMO_CE32S_START,  

        
        
        
        
        
        
        
        
        IX_REORDER_CODES_OFFSET,
        




        IX_REORDER_TABLE_OFFSET,
        
        IX_TRIE_OFFSET,

        IX_RESERVED8_OFFSET,  
        
        IX_CES_OFFSET,
        IX_RESERVED10_OFFSET,
        
        IX_CE32S_OFFSET,

        
        IX_ROOT_ELEMENTS_OFFSET,  
        
        IX_CONTEXTS_OFFSET,
        
        IX_UNSAFE_BWD_OFFSET,
        
        IX_FAST_LATIN_TABLE_OFFSET,

        
        IX_SCRIPTS_OFFSET,  
        




        IX_COMPRESSIBLE_BYTES_OFFSET,
        IX_RESERVED18_OFFSET,
        IX_TOTAL_SIZE
    };

    static void read(const CollationTailoring *base, const uint8_t *inBytes, int32_t inLength,
                     CollationTailoring &tailoring, UErrorCode &errorCode);

    static UBool U_CALLCONV
    isAcceptable(void *context, const char *type, const char *name, const UDataInfo *pInfo);

private:
    CollationDataReader();  
};











































































































































U_NAMESPACE_END

#endif  
#endif  
