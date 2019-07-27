










#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucharstrie.h"
#include "unicode/ustringtrie.h"
#include "charstr.h"
#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "collationfcd.h"
#include "collationiterator.h"
#include "normalizer2impl.h"
#include "uassert.h"
#include "uvectr32.h"

U_NAMESPACE_BEGIN

CollationIterator::CEBuffer::~CEBuffer() {}

UBool
CollationIterator::CEBuffer::ensureAppendCapacity(int32_t appCap, UErrorCode &errorCode) {
    int32_t capacity = buffer.getCapacity();
    if((length + appCap) <= capacity) { return TRUE; }
    if(U_FAILURE(errorCode)) { return FALSE; }
    do {
        if(capacity < 1000) {
            capacity *= 4;
        } else {
            capacity *= 2;
        }
    } while(capacity < (length + appCap));
    int64_t *p = buffer.resize(capacity, length);
    if(p == NULL) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    return TRUE;
}



class SkippedState : public UMemory {
public:
    
    SkippedState() : pos(0), skipLengthAtMatch(0) {}
    void clear() {
        oldBuffer.remove();
        pos = 0;
        
    }

    UBool isEmpty() const { return oldBuffer.isEmpty(); }

    UBool hasNext() const { return pos < oldBuffer.length(); }

    
    UChar32 next() {
        UChar32 c = oldBuffer.char32At(pos);
        pos += U16_LENGTH(c);
        return c;
    }

    
    void incBeyond() {
        U_ASSERT(!hasNext());
        ++pos;
    }

    
    
    
    int32_t backwardNumCodePoints(int32_t n) {
        int32_t length = oldBuffer.length();
        int32_t beyond = pos - length;
        if(beyond > 0) {
            if(beyond >= n) {
                
                pos -= n;
                return n;
            } else {
                
                pos = oldBuffer.moveIndex32(length, beyond - n);
                return beyond;
            }
        } else {
            
            pos = oldBuffer.moveIndex32(pos, -n);
            return 0;
        }
    }

    void setFirstSkipped(UChar32 c) {
        skipLengthAtMatch = 0;
        newBuffer.setTo(c);
    }

    void skip(UChar32 c) {
        newBuffer.append(c);
    }

    void recordMatch() { skipLengthAtMatch = newBuffer.length(); }

    
    void replaceMatch() {
        
        oldBuffer.replace(0, pos, newBuffer, 0, skipLengthAtMatch);
        pos = 0;
    }

    void saveTrieState(const UCharsTrie &trie) { trie.saveState(state); }
    void resetToTrieState(UCharsTrie &trie) const { trie.resetToState(state); }

private:
    
    
    UnicodeString oldBuffer;
    
    
    UnicodeString newBuffer;
    
    
    int32_t pos;
    
    
    int32_t skipLengthAtMatch;
    
    
    UCharsTrie::State state;
};

CollationIterator::CollationIterator(const CollationIterator &other)
        : UObject(other),
          trie(other.trie),
          data(other.data),
          cesIndex(other.cesIndex),
          skipped(NULL),
          numCpFwd(other.numCpFwd),
          isNumeric(other.isNumeric) {
    UErrorCode errorCode = U_ZERO_ERROR;
    int32_t length = other.ceBuffer.length;
    if(length > 0 && ceBuffer.ensureAppendCapacity(length, errorCode)) {
        for(int32_t i = 0; i < length; ++i) {
            ceBuffer.set(i, other.ceBuffer.get(i));
        }
        ceBuffer.length = length;
    } else {
        cesIndex = 0;
    }
}

CollationIterator::~CollationIterator() {
    delete skipped;
}

UBool
CollationIterator::operator==(const CollationIterator &other) const {
    
    
    
    
    
    if(!(typeid(*this) == typeid(other) &&
            ceBuffer.length == other.ceBuffer.length &&
            cesIndex == other.cesIndex &&
            numCpFwd == other.numCpFwd &&
            isNumeric == other.isNumeric)) {
        return FALSE;
    }
    for(int32_t i = 0; i < ceBuffer.length; ++i) {
        if(ceBuffer.get(i) != other.ceBuffer.get(i)) { return FALSE; }
    }
    return TRUE;
}

void
CollationIterator::reset() {
    cesIndex = ceBuffer.length = 0;
    if(skipped != NULL) { skipped->clear(); }
}

int32_t
CollationIterator::fetchCEs(UErrorCode &errorCode) {
    while(U_SUCCESS(errorCode) && nextCE(errorCode) != Collation::NO_CE) {
        
        cesIndex = ceBuffer.length;
    }
    return ceBuffer.length;
}

uint32_t
CollationIterator::handleNextCE32(UChar32 &c, UErrorCode &errorCode) {
    c = nextCodePoint(errorCode);
    return (c < 0) ? Collation::FALLBACK_CE32 : data->getCE32(c);
}

UChar
CollationIterator::handleGetTrailSurrogate() {
    return 0;
}

UBool
CollationIterator::foundNULTerminator() {
    return FALSE;
}

UBool
CollationIterator::forbidSurrogateCodePoints() const {
    return FALSE;
}

uint32_t
CollationIterator::getDataCE32(UChar32 c) const {
    return data->getCE32(c);
}

uint32_t
CollationIterator::getCE32FromBuilderData(uint32_t , UErrorCode &errorCode) {
    if(U_SUCCESS(errorCode)) { errorCode = U_INTERNAL_PROGRAM_ERROR; }
    return 0;
}

int64_t
CollationIterator::nextCEFromCE32(const CollationData *d, UChar32 c, uint32_t ce32,
                                  UErrorCode &errorCode) {
    --ceBuffer.length;  
    appendCEsFromCE32(d, c, ce32, TRUE, errorCode);
    if(U_SUCCESS(errorCode)) {
        return ceBuffer.get(cesIndex++);
    } else {
        return Collation::NO_CE_PRIMARY;
    }
}

void
CollationIterator::appendCEsFromCE32(const CollationData *d, UChar32 c, uint32_t ce32,
                                     UBool forward, UErrorCode &errorCode) {
    while(Collation::isSpecialCE32(ce32)) {
        switch(Collation::tagFromCE32(ce32)) {
        case Collation::FALLBACK_TAG:
        case Collation::RESERVED_TAG_3:
            if(U_SUCCESS(errorCode)) { errorCode = U_INTERNAL_PROGRAM_ERROR; }
            return;
        case Collation::LONG_PRIMARY_TAG:
            ceBuffer.append(Collation::ceFromLongPrimaryCE32(ce32), errorCode);
            return;
        case Collation::LONG_SECONDARY_TAG:
            ceBuffer.append(Collation::ceFromLongSecondaryCE32(ce32), errorCode);
            return;
        case Collation::LATIN_EXPANSION_TAG:
            if(ceBuffer.ensureAppendCapacity(2, errorCode)) {
                ceBuffer.set(ceBuffer.length, Collation::latinCE0FromCE32(ce32));
                ceBuffer.set(ceBuffer.length + 1, Collation::latinCE1FromCE32(ce32));
                ceBuffer.length += 2;
            }
            return;
        case Collation::EXPANSION32_TAG: {
            const uint32_t *ce32s = d->ce32s + Collation::indexFromCE32(ce32);
            int32_t length = Collation::lengthFromCE32(ce32);
            if(ceBuffer.ensureAppendCapacity(length, errorCode)) {
                do {
                    ceBuffer.appendUnsafe(Collation::ceFromCE32(*ce32s++));
                } while(--length > 0);
            }
            return;
        }
        case Collation::EXPANSION_TAG: {
            const int64_t *ces = d->ces + Collation::indexFromCE32(ce32);
            int32_t length = Collation::lengthFromCE32(ce32);
            if(ceBuffer.ensureAppendCapacity(length, errorCode)) {
                do {
                    ceBuffer.appendUnsafe(*ces++);
                } while(--length > 0);
            }
            return;
        }
        case Collation::BUILDER_DATA_TAG:
            ce32 = getCE32FromBuilderData(ce32, errorCode);
            if(U_FAILURE(errorCode)) { return; }
            if(ce32 == Collation::FALLBACK_CE32) {
                d = data->base;
                ce32 = d->getCE32(c);
            }
            break;
        case Collation::PREFIX_TAG:
            if(forward) { backwardNumCodePoints(1, errorCode); }
            ce32 = getCE32FromPrefix(d, ce32, errorCode);
            if(forward) { forwardNumCodePoints(1, errorCode); }
            break;
        case Collation::CONTRACTION_TAG: {
            const UChar *p = d->contexts + Collation::indexFromCE32(ce32);
            uint32_t defaultCE32 = CollationData::readCE32(p);  
            if(!forward) {
                
                
                ce32 = defaultCE32;
                break;
            }
            UChar32 nextCp;
            if(skipped == NULL && numCpFwd < 0) {
                
                
                nextCp = nextCodePoint(errorCode);
                if(nextCp < 0) {
                    
                    ce32 = defaultCE32;
                    break;
                } else if((ce32 & Collation::CONTRACT_NEXT_CCC) != 0 &&
                        !CollationFCD::mayHaveLccc(nextCp)) {
                    
                    
                    backwardNumCodePoints(1, errorCode);
                    ce32 = defaultCE32;
                    break;
                }
            } else {
                nextCp = nextSkippedCodePoint(errorCode);
                if(nextCp < 0) {
                    
                    ce32 = defaultCE32;
                    break;
                } else if((ce32 & Collation::CONTRACT_NEXT_CCC) != 0 &&
                        !CollationFCD::mayHaveLccc(nextCp)) {
                    
                    
                    backwardNumSkipped(1, errorCode);
                    ce32 = defaultCE32;
                    break;
                }
            }
            ce32 = nextCE32FromContraction(d, ce32, p + 2, defaultCE32, nextCp, errorCode);
            if(ce32 == Collation::NO_CE32) {
                
                
                return;
            }
            break;
        }
        case Collation::DIGIT_TAG:
            if(isNumeric) {
                appendNumericCEs(ce32, forward, errorCode);
                return;
            } else {
                
                ce32 = d->ce32s[Collation::indexFromCE32(ce32)];
                break;
            }
        case Collation::U0000_TAG:
            U_ASSERT(c == 0);
            if(forward && foundNULTerminator()) {
                
                ceBuffer.append(Collation::NO_CE, errorCode);
                return;
            } else {
                
                ce32 = d->ce32s[0];
                break;
            }
        case Collation::HANGUL_TAG: {
            const uint32_t *jamoCE32s = d->jamoCE32s;
            c -= Hangul::HANGUL_BASE;
            UChar32 t = c % Hangul::JAMO_T_COUNT;
            c /= Hangul::JAMO_T_COUNT;
            UChar32 v = c % Hangul::JAMO_V_COUNT;
            c /= Hangul::JAMO_V_COUNT;
            if((ce32 & Collation::HANGUL_NO_SPECIAL_JAMO) != 0) {
                
                
                if(ceBuffer.ensureAppendCapacity(t == 0 ? 2 : 3, errorCode)) {
                    ceBuffer.set(ceBuffer.length, Collation::ceFromCE32(jamoCE32s[c]));
                    ceBuffer.set(ceBuffer.length + 1, Collation::ceFromCE32(jamoCE32s[19 + v]));
                    ceBuffer.length += 2;
                    if(t != 0) {
                        ceBuffer.appendUnsafe(Collation::ceFromCE32(jamoCE32s[39 + t]));
                    }
                }
                return;
            } else {
                
                
                appendCEsFromCE32(d, U_SENTINEL, jamoCE32s[c], forward, errorCode);
                appendCEsFromCE32(d, U_SENTINEL, jamoCE32s[19 + v], forward, errorCode);
                if(t == 0) { return; }
                
                
                
                
                ce32 = jamoCE32s[39 + t];
                c = U_SENTINEL;
                break;
            }
        }
        case Collation::LEAD_SURROGATE_TAG: {
            U_ASSERT(forward);  
            U_ASSERT(U16_IS_LEAD(c));
            UChar trail;
            if(U16_IS_TRAIL(trail = handleGetTrailSurrogate())) {
                c = U16_GET_SUPPLEMENTARY(c, trail);
                ce32 &= Collation::LEAD_TYPE_MASK;
                if(ce32 == Collation::LEAD_ALL_UNASSIGNED) {
                    ce32 = Collation::UNASSIGNED_CE32;  
                } else if(ce32 == Collation::LEAD_ALL_FALLBACK ||
                        (ce32 = d->getCE32FromSupplementary(c)) == Collation::FALLBACK_CE32) {
                    
                    d = d->base;
                    ce32 = d->getCE32FromSupplementary(c);
                }
            } else {
                
                ce32 = Collation::UNASSIGNED_CE32;
            }
            break;
        }
        case Collation::OFFSET_TAG:
            U_ASSERT(c >= 0);
            ceBuffer.append(d->getCEFromOffsetCE32(c, ce32), errorCode);
            return;
        case Collation::IMPLICIT_TAG:
            U_ASSERT(c >= 0);
            if(U_IS_SURROGATE(c) && forbidSurrogateCodePoints()) {
                ce32 = Collation::FFFD_CE32;
                break;
            } else {
                ceBuffer.append(Collation::unassignedCEFromCodePoint(c), errorCode);
                return;
            }
        }
    }
    ceBuffer.append(Collation::ceFromSimpleCE32(ce32), errorCode);
}

uint32_t
CollationIterator::getCE32FromPrefix(const CollationData *d, uint32_t ce32,
                                     UErrorCode &errorCode) {
    const UChar *p = d->contexts + Collation::indexFromCE32(ce32);
    ce32 = CollationData::readCE32(p);  
    p += 2;
    
    int32_t lookBehind = 0;
    UCharsTrie prefixes(p);
    for(;;) {
        UChar32 c = previousCodePoint(errorCode);
        if(c < 0) { break; }
        ++lookBehind;
        UStringTrieResult match = prefixes.nextForCodePoint(c);
        if(USTRINGTRIE_HAS_VALUE(match)) {
            ce32 = (uint32_t)prefixes.getValue();
        }
        if(!USTRINGTRIE_HAS_NEXT(match)) { break; }
    }
    forwardNumCodePoints(lookBehind, errorCode);
    return ce32;
}

UChar32
CollationIterator::nextSkippedCodePoint(UErrorCode &errorCode) {
    if(skipped != NULL && skipped->hasNext()) { return skipped->next(); }
    if(numCpFwd == 0) { return U_SENTINEL; }
    UChar32 c = nextCodePoint(errorCode);
    if(skipped != NULL && !skipped->isEmpty() && c >= 0) { skipped->incBeyond(); }
    if(numCpFwd > 0 && c >= 0) { --numCpFwd; }
    return c;
}

void
CollationIterator::backwardNumSkipped(int32_t n, UErrorCode &errorCode) {
    if(skipped != NULL && !skipped->isEmpty()) {
        n = skipped->backwardNumCodePoints(n);
    }
    backwardNumCodePoints(n, errorCode);
    if(numCpFwd >= 0) { numCpFwd += n; }
}

uint32_t
CollationIterator::nextCE32FromContraction(const CollationData *d, uint32_t contractionCE32,
                                           const UChar *p, uint32_t ce32, UChar32 c,
                                           UErrorCode &errorCode) {
    

    
    
    int32_t lookAhead = 1;
    
    int32_t sinceMatch = 1;
    
    
    
    UCharsTrie suffixes(p);
    if(skipped != NULL && !skipped->isEmpty()) { skipped->saveTrieState(suffixes); }
    UStringTrieResult match = suffixes.firstForCodePoint(c);
    for(;;) {
        UChar32 nextCp;
        if(USTRINGTRIE_HAS_VALUE(match)) {
            ce32 = (uint32_t)suffixes.getValue();
            if(!USTRINGTRIE_HAS_NEXT(match) || (c = nextSkippedCodePoint(errorCode)) < 0) {
                return ce32;
            }
            if(skipped != NULL && !skipped->isEmpty()) { skipped->saveTrieState(suffixes); }
            sinceMatch = 1;
        } else if(match == USTRINGTRIE_NO_MATCH || (nextCp = nextSkippedCodePoint(errorCode)) < 0) {
            
            
            if((contractionCE32 & Collation::CONTRACT_TRAILING_CCC) != 0 &&
                    
                    
                    ((contractionCE32 & Collation::CONTRACT_SINGLE_CP_NO_MATCH) == 0 ||
                        sinceMatch < lookAhead)) {
                
                
                
                
                if(sinceMatch > 1) {
                    
                    
                    backwardNumSkipped(sinceMatch, errorCode);
                    c = nextSkippedCodePoint(errorCode);
                    lookAhead -= sinceMatch - 1;
                    sinceMatch = 1;
                }
                if(d->getFCD16(c) > 0xff) {
                    return nextCE32FromDiscontiguousContraction(
                        d, suffixes, ce32, lookAhead, c, errorCode);
                }
            }
            break;
        } else {
            
            
            
            
            c = nextCp;
            ++sinceMatch;
        }
        ++lookAhead;
        match = suffixes.nextForCodePoint(c);
    }
    backwardNumSkipped(sinceMatch, errorCode);
    return ce32;
}

uint32_t
CollationIterator::nextCE32FromDiscontiguousContraction(
        const CollationData *d, UCharsTrie &suffixes, uint32_t ce32,
        int32_t lookAhead, UChar32 c,
        UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return 0; }

    
    
    
    
    
    

    
    
    
    
    
    
    
    

    
    uint16_t fcd16 = d->getFCD16(c);
    U_ASSERT(fcd16 > 0xff);  
    UChar32 nextCp = nextSkippedCodePoint(errorCode);
    if(nextCp < 0) {
        
        backwardNumSkipped(1, errorCode);
        return ce32;
    }
    ++lookAhead;
    uint8_t prevCC = (uint8_t)fcd16;
    fcd16 = d->getFCD16(nextCp);
    if(fcd16 <= 0xff) {
        
        backwardNumSkipped(2, errorCode);
        return ce32;
    }

    
    
    
    if(skipped == NULL || skipped->isEmpty()) {
        if(skipped == NULL) {
            skipped = new SkippedState();
            if(skipped == NULL) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }
        }
        suffixes.reset();
        if(lookAhead > 2) {
            
            backwardNumCodePoints(lookAhead, errorCode);
            suffixes.firstForCodePoint(nextCodePoint(errorCode));
            for(int32_t i = 3; i < lookAhead; ++i) {
                suffixes.nextForCodePoint(nextCodePoint(errorCode));
            }
            
            forwardNumCodePoints(2, errorCode);
        }
        skipped->saveTrieState(suffixes);
    } else {
        
        skipped->resetToTrieState(suffixes);
    }

    skipped->setFirstSkipped(c);
    
    int32_t sinceMatch = 2;
    c = nextCp;
    for(;;) {
        UStringTrieResult match;
        
        if(prevCC < (fcd16 >> 8) && USTRINGTRIE_HAS_VALUE(match = suffixes.nextForCodePoint(c))) {
            
            
            ce32 = (uint32_t)suffixes.getValue();
            sinceMatch = 0;
            skipped->recordMatch();
            if(!USTRINGTRIE_HAS_NEXT(match)) { break; }
            skipped->saveTrieState(suffixes);
        } else {
            
            skipped->skip(c);
            skipped->resetToTrieState(suffixes);
            prevCC = (uint8_t)fcd16;
        }
        if((c = nextSkippedCodePoint(errorCode)) < 0) { break; }
        ++sinceMatch;
        fcd16 = d->getFCD16(c);
        if(fcd16 <= 0xff) {
            
            break;
        }
    }
    backwardNumSkipped(sinceMatch, errorCode);
    UBool isTopDiscontiguous = skipped->isEmpty();
    skipped->replaceMatch();
    if(isTopDiscontiguous && !skipped->isEmpty()) {
        
        
        
        
        c = U_SENTINEL;
        for(;;) {
            appendCEsFromCE32(d, c, ce32, TRUE, errorCode);
            
            
            if(!skipped->hasNext()) { break; }
            c = skipped->next();
            ce32 = getDataCE32(c);
            if(ce32 == Collation::FALLBACK_CE32) {
                d = data->base;
                ce32 = d->getCE32(c);
            } else {
                d = data;
            }
            
            
            
        }
        skipped->clear();
        ce32 = Collation::NO_CE32;  
    }
    return ce32;
}

void
CollationIterator::appendNumericCEs(uint32_t ce32, UBool forward, UErrorCode &errorCode) {
    
    CharString digits;
    if(forward) {
        for(;;) {
            char digit = Collation::digitFromCE32(ce32);
            digits.append(digit, errorCode);
            if(numCpFwd == 0) { break; }
            UChar32 c = nextCodePoint(errorCode);
            if(c < 0) { break; }
            ce32 = data->getCE32(c);
            if(ce32 == Collation::FALLBACK_CE32) {
                ce32 = data->base->getCE32(c);
            }
            if(!Collation::hasCE32Tag(ce32, Collation::DIGIT_TAG)) {
                backwardNumCodePoints(1, errorCode);
                break;
            }
            if(numCpFwd > 0) { --numCpFwd; }
        }
    } else {
        for(;;) {
            char digit = Collation::digitFromCE32(ce32);
            digits.append(digit, errorCode);
            UChar32 c = previousCodePoint(errorCode);
            if(c < 0) { break; }
            ce32 = data->getCE32(c);
            if(ce32 == Collation::FALLBACK_CE32) {
                ce32 = data->base->getCE32(c);
            }
            if(!Collation::hasCE32Tag(ce32, Collation::DIGIT_TAG)) {
                forwardNumCodePoints(1, errorCode);
                break;
            }
        }
        
        char *p = digits.data();
        char *q = p + digits.length() - 1;
        while(p < q) {
            char digit = *p;
            *p++ = *q;
            *q-- = digit;
        }
    }
    if(U_FAILURE(errorCode)) { return; }
    int32_t pos = 0;
    do {
        
        while(pos < (digits.length() - 1) && digits[pos] == 0) { ++pos; }
        
        int32_t segmentLength = digits.length() - pos;
        if(segmentLength > 254) { segmentLength = 254; }
        appendNumericSegmentCEs(digits.data() + pos, segmentLength, errorCode);
        pos += segmentLength;
    } while(U_SUCCESS(errorCode) && pos < digits.length());
}

void
CollationIterator::appendNumericSegmentCEs(const char *digits, int32_t length, UErrorCode &errorCode) {
    U_ASSERT(1 <= length && length <= 254);
    U_ASSERT(length == 1 || digits[0] != 0);
    uint32_t numericPrimary = data->numericPrimary;
    
    if(length <= 7) {
        
        int32_t value = digits[0];
        for(int32_t i = 1; i < length; ++i) {
            value = value * 10 + digits[i];
        }
        
        
        
        
        
        int32_t firstByte = 2;
        int32_t numBytes = 74;
        if(value < numBytes) {
            
            uint32_t primary = numericPrimary | ((firstByte + value) << 16);
            ceBuffer.append(Collation::makeCE(primary), errorCode);
            return;
        }
        value -= numBytes;
        firstByte += numBytes;
        numBytes = 40;
        if(value < numBytes * 254) {
            
            uint32_t primary = numericPrimary |
                ((firstByte + value / 254) << 16) | ((2 + value % 254) << 8);
            ceBuffer.append(Collation::makeCE(primary), errorCode);
            return;
        }
        value -= numBytes * 254;
        firstByte += numBytes;
        numBytes = 16;
        if(value < numBytes * 254 * 254) {
            
            uint32_t primary = numericPrimary | (2 + value % 254);
            value /= 254;
            primary |= (2 + value % 254) << 8;
            value /= 254;
            primary |= (firstByte + value % 254) << 16;
            ceBuffer.append(Collation::makeCE(primary), errorCode);
            return;
        }
        
    }
    U_ASSERT(length >= 7);

    
    
    
    

    
    int32_t numPairs = (length + 1) / 2;
    uint32_t primary = numericPrimary | ((132 - 4 + numPairs) << 16);
    
    while(digits[length - 1] == 0 && digits[length - 2] == 0) {
        length -= 2;
    }
    
    uint32_t pair;
    int32_t pos;
    if(length & 1) {
        
        pair = digits[0];
        pos = 1;
    } else {
        pair = digits[0] * 10 + digits[1];
        pos = 2;
    }
    pair = 11 + 2 * pair;
    
    int32_t shift = 8;
    while(pos < length) {
        if(shift == 0) {
            
            
            primary |= pair;
            ceBuffer.append(Collation::makeCE(primary), errorCode);
            primary = numericPrimary;
            shift = 16;
        } else {
            primary |= pair << shift;
            shift -= 8;
        }
        pair = 11 + 2 * (digits[pos] * 10 + digits[pos + 1]);
        pos += 2;
    }
    primary |= (pair - 1) << shift;
    ceBuffer.append(Collation::makeCE(primary), errorCode);
}

int64_t
CollationIterator::previousCE(UVector32 &offsets, UErrorCode &errorCode) {
    if(ceBuffer.length > 0) {
        
        return ceBuffer.get(--ceBuffer.length);
    }
    offsets.removeAllElements();
    int32_t limitOffset = getOffset();
    UChar32 c = previousCodePoint(errorCode);
    if(c < 0) { return Collation::NO_CE; }
    if(data->isUnsafeBackward(c, isNumeric)) {
        return previousCEUnsafe(c, offsets, errorCode);
    }
    
    
    uint32_t ce32 = data->getCE32(c);
    const CollationData *d;
    if(ce32 == Collation::FALLBACK_CE32) {
        d = data->base;
        ce32 = d->getCE32(c);
    } else {
        d = data;
    }
    if(Collation::isSimpleOrLongCE32(ce32)) {
        return Collation::ceFromCE32(ce32);
    }
    appendCEsFromCE32(d, c, ce32, FALSE, errorCode);
    if(U_SUCCESS(errorCode)) {
        if(ceBuffer.length > 1) {
            offsets.addElement(getOffset(), errorCode);
            
            
            while(offsets.size() <= ceBuffer.length) {
                offsets.addElement(limitOffset, errorCode);
            };
        }
        return ceBuffer.get(--ceBuffer.length);
    } else {
        return Collation::NO_CE_PRIMARY;
    }
}

int64_t
CollationIterator::previousCEUnsafe(UChar32 c, UVector32 &offsets, UErrorCode &errorCode) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t numBackward = 1;
    while((c = previousCodePoint(errorCode)) >= 0) {
        ++numBackward;
        if(!data->isUnsafeBackward(c, isNumeric)) {
            break;
        }
    }
    
    
    
    numCpFwd = numBackward;
    
    cesIndex = 0;
    U_ASSERT(ceBuffer.length == 0);
    
    int32_t offset = getOffset();
    while(numCpFwd > 0) {
        
        
        --numCpFwd;
        
        (void)nextCE(errorCode);
        U_ASSERT(U_FAILURE(errorCode) || ceBuffer.get(ceBuffer.length - 1) != Collation::NO_CE);
        
        cesIndex = ceBuffer.length;
        
        
        
        U_ASSERT(offsets.size() < ceBuffer.length);
        offsets.addElement(offset, errorCode);
        
        
        offset = getOffset();
        while(offsets.size() < ceBuffer.length) {
            offsets.addElement(offset, errorCode);
        };
    }
    U_ASSERT(offsets.size() == ceBuffer.length);
    
    offsets.addElement(offset, errorCode);
    
    
    numCpFwd = -1;
    backwardNumCodePoints(numBackward, errorCode);
    
    cesIndex = 0;  
    if(U_SUCCESS(errorCode)) {
        return ceBuffer.get(--ceBuffer.length);
    } else {
        return Collation::NO_CE_PRIMARY;
    }
}

U_NAMESPACE_END

#endif  
