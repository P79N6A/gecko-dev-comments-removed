


















#include "unicode/utypes.h"
#include "unicont.h"

#define UTRIE_GET8_LATIN1(trie) ((const uint8_t *)(trie)->data32+UTRIE_DATA_BLOCK_LENGTH)

#define UTRIE_GET8_FROM_LEAD(trie, c16) \
    ((const uint8_t *)(trie)->data32)[ \
        ((int32_t)((trie)->index[(c16)>>UTRIE_SHIFT])<<UTRIE_INDEX_SHIFT)+ \
        ((c16)&UTRIE_MASK) \
    ]

class TrieSet : public UObject, public UnicodeContainable {
public:
    TrieSet(const UnicodeSet &set, UErrorCode &errorCode)
            : trieData(NULL), latin1(NULL), restSet(set.clone()) {
        if(U_FAILURE(errorCode)) {
            return;
        }
        if(restSet==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        UNewTrie *newTrie=utrie_open(NULL, NULL, 0x11000, 0, 0, TRUE);
        UChar32 start, end;

        UnicodeSetIterator iter(set);

        while(iter.nextRange() && !iter.isString()) {
            start=iter.getCodepoint();
            end=iter.getCodepointEnd();
            if(start>0xffff) {
                break;
            }
            if(end>0xffff) {
                end=0xffff;
            }
            if(!utrie_setRange32(newTrie, start, end+1, TRUE, TRUE)) {
                errorCode=U_INTERNAL_PROGRAM_ERROR;
                return;
            }
        }

        
        int32_t length=utrie_serialize(newTrie, NULL, 0, NULL, 8, &errorCode);
        if(errorCode!=U_BUFFER_OVERFLOW_ERROR) {
            return;
        }

        trieData=(uint32_t *)uprv_malloc(length);
        if(trieData==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        errorCode=U_ZERO_ERROR;
        utrie_serialize(newTrie, trieData, length, NULL, 8, &errorCode);
        utrie_unserialize(&trie, trieData, length, &errorCode);  

        if(U_SUCCESS(errorCode)) {
            
            
            uprv_memcpy((uint16_t *)trie.index+(0xd800>>UTRIE_SHIFT),
                        trie.index+UTRIE_BMP_INDEX_LENGTH,
                        (0x800>>UTRIE_SHIFT)*2);
            latin1=UTRIE_GET8_LATIN1(&trie);
        }

        restSet.remove(0, 0xffff);
    }

    ~TrieSet() {
        uprv_free(trieData);
        delete restSet;
    }

    UBool contains(UChar32 c) const {
        if((uint32_t)c<=0xff) {
            return (UBool)latin1[c];
        } else if((uint32_t)c<0xffff) {
            return (UBool)UTRIE_GET8_FROM_LEAD(&trie, c);
        } else {
            return restSet->contains(c);
        }
    }

private:
    uint32_t *trieData;
    const uint8_t *latin1;
    UTrie trie;
    UnicodeSet *restSet;
};
