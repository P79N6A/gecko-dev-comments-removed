










#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/utf8.h"
#include "charstr.h"
#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "collationfcd.h"
#include "collationiterator.h"
#include "normalizer2impl.h"
#include "uassert.h"
#include "utf8collationiterator.h"

U_NAMESPACE_BEGIN

UTF8CollationIterator::~UTF8CollationIterator() {}

void
UTF8CollationIterator::resetToOffset(int32_t newOffset) {
    reset();
    pos = newOffset;
}

int32_t
UTF8CollationIterator::getOffset() const {
    return pos;
}

uint32_t
UTF8CollationIterator::handleNextCE32(UChar32 &c, UErrorCode & ) {
    if(pos == length) {
        c = U_SENTINEL;
        return Collation::FALLBACK_CE32;
    }
    
    c = u8[pos++];
    if(c < 0xc0) {
        
        return trie->data32[c];
    }
    uint8_t t1, t2;
    if(c < 0xe0 && pos != length && (t1 = (u8[pos] - 0x80)) <= 0x3f) {
        
        uint32_t ce32 = trie->data32[trie->index[(UTRIE2_UTF8_2B_INDEX_2_OFFSET - 0xc0) + c] + t1];
        c = ((c & 0x1f) << 6) | t1;
        ++pos;
        return ce32;
    } else if(c <= 0xef &&
              ((pos + 1) < length || length < 0) &&
              (t1 = (u8[pos] - 0x80)) <= 0x3f && (c != 0xe0 || t1 >= 0x20) &&
              (t2 = (u8[pos + 1] - 0x80)) <= 0x3f
    ) {
        
        c = (UChar)((c << 12) | (t1 << 6) | t2);
        pos += 2;
        return UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie, c);
    } else {
        
        
        c = utf8_nextCharSafeBody(u8, &pos, length, c, -3);
        return data->getCE32(c);
    }
}

UBool
UTF8CollationIterator::foundNULTerminator() {
    if(length < 0) {
        length = --pos;
        return TRUE;
    } else {
        return FALSE;
    }
}

UBool
UTF8CollationIterator::forbidSurrogateCodePoints() const {
    return TRUE;
}

UChar32
UTF8CollationIterator::nextCodePoint(UErrorCode & ) {
    if(pos == length) {
        return U_SENTINEL;
    }
    if(u8[pos] == 0 && length < 0) {
        length = pos;
        return U_SENTINEL;
    }
    UChar32 c;
    U8_NEXT_OR_FFFD(u8, pos, length, c);
    return c;
}

UChar32
UTF8CollationIterator::previousCodePoint(UErrorCode & ) {
    if(pos == 0) {
        return U_SENTINEL;
    }
    UChar32 c;
    U8_PREV_OR_FFFD(u8, 0, pos, c);
    return c;
}

void
UTF8CollationIterator::forwardNumCodePoints(int32_t num, UErrorCode & ) {
    U8_FWD_N(u8, pos, length, num);
}

void
UTF8CollationIterator::backwardNumCodePoints(int32_t num, UErrorCode & ) {
    U8_BACK_N(u8, 0, pos, num);
}



FCDUTF8CollationIterator::~FCDUTF8CollationIterator() {}

void
FCDUTF8CollationIterator::resetToOffset(int32_t newOffset) {
    reset();
    start = pos = newOffset;
    state = CHECK_FWD;
}

int32_t
FCDUTF8CollationIterator::getOffset() const {
    if(state != IN_NORMALIZED) {
        return pos;
    } else if(pos == 0) {
        return start;
    } else {
        return limit;
    }
}

uint32_t
FCDUTF8CollationIterator::handleNextCE32(UChar32 &c, UErrorCode &errorCode) {
    for(;;) {
        if(state == CHECK_FWD) {
            
            if(pos == length) {
                c = U_SENTINEL;
                return Collation::FALLBACK_CE32;
            }
            c = u8[pos++];
            if(c < 0xc0) {
                
                return trie->data32[c];
            }
            uint8_t t1, t2;
            if(c < 0xe0 && pos != length && (t1 = (u8[pos] - 0x80)) <= 0x3f) {
                
                uint32_t ce32 = trie->data32[trie->index[(UTRIE2_UTF8_2B_INDEX_2_OFFSET - 0xc0) + c] + t1];
                c = ((c & 0x1f) << 6) | t1;
                ++pos;
                if(CollationFCD::hasTccc(c) && pos != length && nextHasLccc()) {
                    pos -= 2;
                } else {
                    return ce32;
                }
            } else if(c <= 0xef &&
                      ((pos + 1) < length || length < 0) &&
                      (t1 = (u8[pos] - 0x80)) <= 0x3f && (c != 0xe0 || t1 >= 0x20) &&
                      (t2 = (u8[pos + 1] - 0x80)) <= 0x3f
            ) {
                
                c = (UChar)((c << 12) | (t1 << 6) | t2);
                pos += 2;
                if(CollationFCD::hasTccc(c) &&
                        (CollationFCD::maybeTibetanCompositeVowel(c) ||
                            (pos != length && nextHasLccc()))) {
                    pos -= 3;
                } else {
                    break;  
                }
            } else {
                
                
                c = utf8_nextCharSafeBody(u8, &pos, length, c, -3);
                if(c == 0xfffd) {
                    return Collation::FFFD_CE32;
                } else {
                    U_ASSERT(c > 0xffff);
                    if(CollationFCD::hasTccc(U16_LEAD(c)) && pos != length && nextHasLccc()) {
                        pos -= 4;
                    } else {
                        return data->getCE32FromSupplementary(c);
                    }
                }
            }
            if(!nextSegment(errorCode)) {
                c = U_SENTINEL;
                return Collation::FALLBACK_CE32;
            }
            continue;
        } else if(state == IN_FCD_SEGMENT && pos != limit) {
            return UTF8CollationIterator::handleNextCE32(c, errorCode);
        } else if(state == IN_NORMALIZED && pos != normalized.length()) {
            c = normalized[pos++];
            break;
        } else {
            switchToForward();
        }
    }
    return UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie, c);
}

UBool
FCDUTF8CollationIterator::nextHasLccc() const {
    U_ASSERT(state == CHECK_FWD && pos != length);
    
    
    UChar32 c = u8[pos];
    if(c < 0xcc || (0xe4 <= c && c <= 0xed && c != 0xea)) { return FALSE; }
    int32_t i = pos;
    U8_NEXT_OR_FFFD(u8, i, length, c);
    if(c > 0xffff) { c = U16_LEAD(c); }
    return CollationFCD::hasLccc(c);
}

UBool
FCDUTF8CollationIterator::previousHasTccc() const {
    U_ASSERT(state == CHECK_BWD && pos != 0);
    UChar32 c = u8[pos - 1];
    if(c < 0x80) { return FALSE; }
    int32_t i = pos;
    U8_PREV_OR_FFFD(u8, 0, i, c);
    if(c > 0xffff) { c = U16_LEAD(c); }
    return CollationFCD::hasTccc(c);
}

UChar
FCDUTF8CollationIterator::handleGetTrailSurrogate() {
    if(state != IN_NORMALIZED) { return 0; }
    U_ASSERT(pos < normalized.length());
    UChar trail;
    if(U16_IS_TRAIL(trail = normalized[pos])) { ++pos; }
    return trail;
}

UBool
FCDUTF8CollationIterator::foundNULTerminator() {
    if(state == CHECK_FWD && length < 0) {
        length = --pos;
        return TRUE;
    } else {
        return FALSE;
    }
}

UChar32
FCDUTF8CollationIterator::nextCodePoint(UErrorCode &errorCode) {
    UChar32 c;
    for(;;) {
        if(state == CHECK_FWD) {
            if(pos == length || ((c = u8[pos]) == 0 && length < 0)) {
                return U_SENTINEL;
            }
            if(c < 0x80) {
                ++pos;
                return c;
            }
            U8_NEXT_OR_FFFD(u8, pos, length, c);
            if(CollationFCD::hasTccc(c <= 0xffff ? c : U16_LEAD(c)) &&
                    (CollationFCD::maybeTibetanCompositeVowel(c) ||
                        (pos != length && nextHasLccc()))) {
                
                
                pos -= U8_LENGTH(c);
                if(!nextSegment(errorCode)) {
                    return U_SENTINEL;
                }
                continue;
            }
            return c;
        } else if(state == IN_FCD_SEGMENT && pos != limit) {
            U8_NEXT_OR_FFFD(u8, pos, length, c);
            return c;
        } else if(state == IN_NORMALIZED && pos != normalized.length()) {
            c = normalized.char32At(pos);
            pos += U16_LENGTH(c);
            return c;
        } else {
            switchToForward();
        }
    }
}

UChar32
FCDUTF8CollationIterator::previousCodePoint(UErrorCode &errorCode) {
    UChar32 c;
    for(;;) {
        if(state == CHECK_BWD) {
            if(pos == 0) {
                return U_SENTINEL;
            }
            if((c = u8[pos - 1]) < 0x80) {
                --pos;
                return c;
            }
            U8_PREV_OR_FFFD(u8, 0, pos, c);
            if(CollationFCD::hasLccc(c <= 0xffff ? c : U16_LEAD(c)) &&
                    (CollationFCD::maybeTibetanCompositeVowel(c) ||
                        (pos != 0 && previousHasTccc()))) {
                
                
                pos += U8_LENGTH(c);
                if(!previousSegment(errorCode)) {
                    return U_SENTINEL;
                }
                continue;
            }
            return c;
        } else if(state == IN_FCD_SEGMENT && pos != start) {
            U8_PREV_OR_FFFD(u8, 0, pos, c);
            return c;
        } else if(state >= IN_NORMALIZED && pos != 0) {
            c = normalized.char32At(pos - 1);
            pos -= U16_LENGTH(c);
            return c;
        } else {
            switchToBackward();
        }
    }
}

void
FCDUTF8CollationIterator::forwardNumCodePoints(int32_t num, UErrorCode &errorCode) {
    
    
    while(num > 0 && FCDUTF8CollationIterator::nextCodePoint(errorCode) >= 0) {
        --num;
    }
}

void
FCDUTF8CollationIterator::backwardNumCodePoints(int32_t num, UErrorCode &errorCode) {
    
    
    while(num > 0 && FCDUTF8CollationIterator::previousCodePoint(errorCode) >= 0) {
        --num;
    }
}

void
FCDUTF8CollationIterator::switchToForward() {
    U_ASSERT(state == CHECK_BWD ||
             (state == IN_FCD_SEGMENT && pos == limit) ||
             (state == IN_NORMALIZED && pos == normalized.length()));
    if(state == CHECK_BWD) {
        
        start = pos;
        if(pos == limit) {
            state = CHECK_FWD;  
        } else {  
            state = IN_FCD_SEGMENT;  
        }
    } else {
        
        if(state == IN_FCD_SEGMENT) {
            
        } else {
            
            
            start = pos = limit;
        }
        state = CHECK_FWD;
    }
}

UBool
FCDUTF8CollationIterator::nextSegment(UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return FALSE; }
    U_ASSERT(state == CHECK_FWD && pos != length);
    
    int32_t segmentStart = pos;
    
    UnicodeString s;
    uint8_t prevCC = 0;
    for(;;) {
        
        int32_t cpStart = pos;
        UChar32 c;
        U8_NEXT_OR_FFFD(u8, pos, length, c);
        uint16_t fcd16 = nfcImpl.getFCD16(c);
        uint8_t leadCC = (uint8_t)(fcd16 >> 8);
        if(leadCC == 0 && cpStart != segmentStart) {
            
            pos = cpStart;
            break;
        }
        s.append(c);
        if(leadCC != 0 && (prevCC > leadCC || CollationFCD::isFCD16OfTibetanCompositeVowel(fcd16))) {
            
            while(pos != length) {
                cpStart = pos;
                U8_NEXT_OR_FFFD(u8, pos, length, c);
                if(nfcImpl.getFCD16(c) <= 0xff) {
                    pos = cpStart;
                    break;
                }
                s.append(c);
            }
            if(!normalize(s, errorCode)) { return FALSE; }
            start = segmentStart;
            limit = pos;
            state = IN_NORMALIZED;
            pos = 0;
            return TRUE;
        }
        prevCC = (uint8_t)fcd16;
        if(pos == length || prevCC == 0) {
            
            break;
        }
    }
    limit = pos;
    pos = segmentStart;
    U_ASSERT(pos != limit);
    state = IN_FCD_SEGMENT;
    return TRUE;
}

void
FCDUTF8CollationIterator::switchToBackward() {
    U_ASSERT(state == CHECK_FWD ||
             (state == IN_FCD_SEGMENT && pos == start) ||
             (state >= IN_NORMALIZED && pos == 0));
    if(state == CHECK_FWD) {
        
        limit = pos;
        if(pos == start) {
            state = CHECK_BWD;  
        } else {  
            state = IN_FCD_SEGMENT;  
        }
    } else {
        
        if(state == IN_FCD_SEGMENT) {
            
        } else {
            
            
            limit = pos = start;
        }
        state = CHECK_BWD;
    }
}

UBool
FCDUTF8CollationIterator::previousSegment(UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return FALSE; }
    U_ASSERT(state == CHECK_BWD && pos != 0);
    
    int32_t segmentLimit = pos;
    
    UnicodeString s;
    uint8_t nextCC = 0;
    for(;;) {
        
        int32_t cpLimit = pos;
        UChar32 c;
        U8_PREV_OR_FFFD(u8, 0, pos, c);
        uint16_t fcd16 = nfcImpl.getFCD16(c);
        uint8_t trailCC = (uint8_t)fcd16;
        if(trailCC == 0 && cpLimit != segmentLimit) {
            
            pos = cpLimit;
            break;
        }
        s.append(c);
        if(trailCC != 0 && ((nextCC != 0 && trailCC > nextCC) ||
                            CollationFCD::isFCD16OfTibetanCompositeVowel(fcd16))) {
            
            while(fcd16 > 0xff && pos != 0) {
                cpLimit = pos;
                U8_PREV_OR_FFFD(u8, 0, pos, c);
                fcd16 = nfcImpl.getFCD16(c);
                if(fcd16 == 0) {
                    pos = cpLimit;
                    break;
                }
                s.append(c);
            }
            s.reverse();
            if(!normalize(s, errorCode)) { return FALSE; }
            limit = segmentLimit;
            start = pos;
            state = IN_NORMALIZED;
            pos = normalized.length();
            return TRUE;
        }
        nextCC = (uint8_t)(fcd16 >> 8);
        if(pos == 0 || nextCC == 0) {
            
            break;
        }
    }
    start = pos;
    pos = segmentLimit;
    U_ASSERT(pos != start);
    state = IN_FCD_SEGMENT;
    return TRUE;
}

UBool
FCDUTF8CollationIterator::normalize(const UnicodeString &s, UErrorCode &errorCode) {
    
    U_ASSERT(U_SUCCESS(errorCode));
    nfcImpl.decompose(s, normalized, errorCode);
    return U_SUCCESS(errorCode);
}

U_NAMESPACE_END

#endif  
