










#ifndef __DICTIONARYDATA_H__
#define __DICTIONARYDATA_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/utext.h"
#include "unicode/udata.h"
#include "udataswp.h"
#include "unicode/uobject.h"
#include "unicode/ustringtrie.h"

U_NAMESPACE_BEGIN

class UCharsTrie;
class BytesTrie;

class U_COMMON_API DictionaryData : public UMemory {
public:
    static const int32_t TRIE_TYPE_BYTES = 0;
    static const int32_t TRIE_TYPE_UCHARS = 1;
    static const int32_t TRIE_TYPE_MASK = 7;
    static const int32_t TRIE_HAS_VALUES = 8;

    static const int32_t TRANSFORM_NONE = 0;
    static const int32_t TRANSFORM_TYPE_OFFSET = 0x1000000;
    static const int32_t TRANSFORM_TYPE_MASK = 0x7f000000;
    static const int32_t TRANSFORM_OFFSET_MASK = 0x1fffff;

    enum {
        
        IX_STRING_TRIE_OFFSET,
        IX_RESERVED1_OFFSET,
        IX_RESERVED2_OFFSET,
        IX_TOTAL_SIZE,

        
        IX_TRIE_TYPE,
        
        IX_TRANSFORM,

        IX_RESERVED6,
        IX_RESERVED7,
        IX_COUNT
    };
};








class U_COMMON_API DictionaryMatcher : public UMemory {
public:
    virtual ~DictionaryMatcher();
    
    virtual int32_t matches(UText *text, int32_t maxLength, int32_t *lengths, int32_t &count,
                            int32_t limit, int32_t *values = NULL) const = 0;
    
    virtual int32_t getType() const = 0;
};


class U_COMMON_API UCharsDictionaryMatcher : public DictionaryMatcher {
public:
    
    
    UCharsDictionaryMatcher(const UChar *c, UDataMemory *f) : characters(c), file(f) { }
    virtual ~UCharsDictionaryMatcher();
    virtual int32_t matches(UText *text, int32_t maxLength, int32_t *lengths, int32_t &count,
                            int32_t limit, int32_t *values = NULL) const;
    virtual int32_t getType() const;
private:
    const UChar *characters;
    UDataMemory *file;
};


class U_COMMON_API BytesDictionaryMatcher : public DictionaryMatcher {
public:
    
    
    
    BytesDictionaryMatcher(const char *c, int32_t t, UDataMemory *f)
            : characters(c), transformConstant(t), file(f) { }
    virtual ~BytesDictionaryMatcher();
    virtual int32_t matches(UText *text, int32_t maxLength, int32_t *lengths, int32_t &count,
                            int32_t limit, int32_t *values = NULL) const;
    virtual int32_t getType() const;
private:
    UChar32 transform(UChar32 c) const;

    const char *characters;
    int32_t transformConstant;
    UDataMemory *file;
};

U_NAMESPACE_END

U_CAPI int32_t U_EXPORT2
udict_swap(const UDataSwapper *ds, const void *inData, int32_t length, void *outData, UErrorCode *pErrorCode);
















































#endif  
#endif  
