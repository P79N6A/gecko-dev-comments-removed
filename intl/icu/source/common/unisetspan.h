















#ifndef __UNISETSPAN_H__
#define __UNISETSPAN_H__

#include "unicode/utypes.h"
#include "unicode/uniset.h"

U_NAMESPACE_BEGIN






class UnicodeSetStringSpan : public UMemory {
public:
    




    enum {
        FWD             = 0x20,
        BACK            = 0x10,
        UTF16           = 8,
        UTF8            = 4,
        CONTAINED       = 2,
        NOT_CONTAINED   = 1,

        ALL             = 0x3f,

        FWD_UTF16_CONTAINED     = FWD  | UTF16 |     CONTAINED,
        FWD_UTF16_NOT_CONTAINED = FWD  | UTF16 | NOT_CONTAINED,
        FWD_UTF8_CONTAINED      = FWD  | UTF8  |     CONTAINED,
        FWD_UTF8_NOT_CONTAINED  = FWD  | UTF8  | NOT_CONTAINED,
        BACK_UTF16_CONTAINED    = BACK | UTF16 |     CONTAINED,
        BACK_UTF16_NOT_CONTAINED= BACK | UTF16 | NOT_CONTAINED,
        BACK_UTF8_CONTAINED     = BACK | UTF8  |     CONTAINED,
        BACK_UTF8_NOT_CONTAINED = BACK | UTF8  | NOT_CONTAINED
    };

    UnicodeSetStringSpan(const UnicodeSet &set, const UVector &setStrings, uint32_t which);

    
    UnicodeSetStringSpan(const UnicodeSetStringSpan &otherStringSpan, const UVector &newParentSetStrings);

    ~UnicodeSetStringSpan();

    




    inline UBool needsStringSpanUTF16();
    inline UBool needsStringSpanUTF8();

    
    inline UBool contains(UChar32 c) const;

    int32_t span(const UChar *s, int32_t length, USetSpanCondition spanCondition) const;

    int32_t spanBack(const UChar *s, int32_t length, USetSpanCondition spanCondition) const;

    int32_t spanUTF8(const uint8_t *s, int32_t length, USetSpanCondition spanCondition) const;

    int32_t spanBackUTF8(const uint8_t *s, int32_t length, USetSpanCondition spanCondition) const;

private:
    
    enum {
        
        LONG_SPAN=0xfe,
        
        ALL_CP_CONTAINED=0xff
    };

    
    
    void addToSpanNotSet(UChar32 c);

    int32_t spanNot(const UChar *s, int32_t length) const;
    int32_t spanNotBack(const UChar *s, int32_t length) const;
    int32_t spanNotUTF8(const uint8_t *s, int32_t length) const;
    int32_t spanNotBackUTF8(const uint8_t *s, int32_t length) const;

    
    UnicodeSet spanSet;

    
    
    UnicodeSet *pSpanNotSet;

    
    const UVector &strings;

    
    
    
    int32_t *utf8Lengths;

    
    
    uint8_t *spanLengths;

    
    
    uint8_t *utf8;

    
    int32_t utf8Length;

    
    int32_t maxLength16;
    int32_t maxLength8;

    
    UBool all;

    
    
    
    
    int32_t staticLengths[32];
};

UBool UnicodeSetStringSpan::needsStringSpanUTF16() {
    return (UBool)(maxLength16!=0);
}

UBool UnicodeSetStringSpan::needsStringSpanUTF8() {
    return (UBool)(maxLength8!=0);
}

UBool UnicodeSetStringSpan::contains(UChar32 c) const {
    return spanSet.contains(c);
}

U_NAMESPACE_END

#endif
