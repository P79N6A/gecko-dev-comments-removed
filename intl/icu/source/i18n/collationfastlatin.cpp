










#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "collationdata.h"
#include "collationfastlatin.h"
#include "collationsettings.h"
#include "putilimp.h"  
#include "uassert.h"

U_NAMESPACE_BEGIN

int32_t
CollationFastLatin::getOptions(const CollationData *data, const CollationSettings &settings,
                               uint16_t *primaries, int32_t capacity) {
    const uint16_t *table = data->fastLatinTable;
    if(table == NULL) { return -1; }
    U_ASSERT(capacity == LATIN_LIMIT);
    if(capacity != LATIN_LIMIT) { return -1; }

    uint32_t miniVarTop;
    if((settings.options & CollationSettings::ALTERNATE_MASK) == 0) {
        
        
        miniVarTop = MIN_LONG - 1;
    } else {
        int32_t headerLength = *table & 0xff;
        int32_t i = 1 + settings.getMaxVariable();
        if(i >= headerLength) {
            return -1;  
        }
        miniVarTop = table[i];
    }

    UBool digitsAreReordered = FALSE;
    if(settings.hasReordering()) {
        uint32_t prevStart = 0;
        uint32_t beforeDigitStart = 0;
        uint32_t digitStart = 0;
        uint32_t afterDigitStart = 0;
        for(int32_t group = UCOL_REORDER_CODE_FIRST;
                group < UCOL_REORDER_CODE_FIRST + CollationData::MAX_NUM_SPECIAL_REORDER_CODES;
                ++group) {
            uint32_t start = data->getFirstPrimaryForGroup(group);
            start = settings.reorder(start);
            if(group == UCOL_REORDER_CODE_DIGIT) {
                beforeDigitStart = prevStart;
                digitStart = start;
            } else if(start != 0) {
                if(start < prevStart) {
                    
                    return -1;
                }
                
                if(digitStart != 0 && afterDigitStart == 0 && prevStart == beforeDigitStart) {
                    afterDigitStart = start;
                }
                prevStart = start;
            }
        }
        uint32_t latinStart = data->getFirstPrimaryForGroup(USCRIPT_LATIN);
        latinStart = settings.reorder(latinStart);
        if(latinStart < prevStart) {
            return -1;
        }
        if(afterDigitStart == 0) {
            afterDigitStart = latinStart;
        }
        if(!(beforeDigitStart < digitStart && digitStart < afterDigitStart)) {
            digitsAreReordered = TRUE;
        }
    }

    table += (table[0] & 0xff);  
    for(UChar32 c = 0; c < LATIN_LIMIT; ++c) {
        uint32_t p = table[c];
        if(p >= MIN_SHORT) {
            p &= SHORT_PRIMARY_MASK;
        } else if(p > miniVarTop) {
            p &= LONG_PRIMARY_MASK;
        } else {
            p = 0;
        }
        primaries[c] = (uint16_t)p;
    }
    if(digitsAreReordered || (settings.options & CollationSettings::NUMERIC) != 0) {
        
        for(UChar32 c = 0x30; c <= 0x39; ++c) { primaries[c] = 0; }
    }

    
    return ((int32_t)miniVarTop << 16) | settings.options;
}

int32_t
CollationFastLatin::compareUTF16(const uint16_t *table, const uint16_t *primaries, int32_t options,
                                 const UChar *left, int32_t leftLength,
                                 const UChar *right, int32_t rightLength) {
    
    
    
    

    U_ASSERT((table[0] >> 8) == VERSION);
    table += (table[0] & 0xff);  
    uint32_t variableTop = (uint32_t)options >> 16;  
    options &= 0xffff;  

    
    U_ALIGN_CODE(16);
    int32_t leftIndex = 0, rightIndex = 0;
    




    uint32_t leftPair = 0, rightPair = 0;
    for(;;) {
        
        while(leftPair == 0) {
            if(leftIndex == leftLength) {
                leftPair = EOS;
                break;
            }
            UChar32 c = left[leftIndex++];
            if(c <= LATIN_MAX) {
                leftPair = primaries[c];
                if(leftPair != 0) { break; }
                if(c <= 0x39 && c >= 0x30 && (options & CollationSettings::NUMERIC) != 0) {
                    return BAIL_OUT_RESULT;
                }
                leftPair = table[c];
            } else if(PUNCT_START <= c && c < PUNCT_LIMIT) {
                leftPair = table[c - PUNCT_START + LATIN_LIMIT];
            } else {
                leftPair = lookup(table, c);
            }
            if(leftPair >= MIN_SHORT) {
                leftPair &= SHORT_PRIMARY_MASK;
                break;
            } else if(leftPair > variableTop) {
                leftPair &= LONG_PRIMARY_MASK;
                break;
            } else {
                leftPair = nextPair(table, c, leftPair, left, NULL, leftIndex, leftLength);
                if(leftPair == BAIL_OUT) { return BAIL_OUT_RESULT; }
                leftPair = getPrimaries(variableTop, leftPair);
            }
        }

        while(rightPair == 0) {
            if(rightIndex == rightLength) {
                rightPair = EOS;
                break;
            }
            UChar32 c = right[rightIndex++];
            if(c <= LATIN_MAX) {
                rightPair = primaries[c];
                if(rightPair != 0) { break; }
                if(c <= 0x39 && c >= 0x30 && (options & CollationSettings::NUMERIC) != 0) {
                    return BAIL_OUT_RESULT;
                }
                rightPair = table[c];
            } else if(PUNCT_START <= c && c < PUNCT_LIMIT) {
                rightPair = table[c - PUNCT_START + LATIN_LIMIT];
            } else {
                rightPair = lookup(table, c);
            }
            if(rightPair >= MIN_SHORT) {
                rightPair &= SHORT_PRIMARY_MASK;
                break;
            } else if(rightPair > variableTop) {
                rightPair &= LONG_PRIMARY_MASK;
                break;
            } else {
                rightPair = nextPair(table, c, rightPair, right, NULL, rightIndex, rightLength);
                if(rightPair == BAIL_OUT) { return BAIL_OUT_RESULT; }
                rightPair = getPrimaries(variableTop, rightPair);
            }
        }

        if(leftPair == rightPair) {
            if(leftPair == EOS) { break; }
            leftPair = rightPair = 0;
            continue;
        }
        uint32_t leftPrimary = leftPair & 0xffff;
        uint32_t rightPrimary = rightPair & 0xffff;
        if(leftPrimary != rightPrimary) {
            
            return (leftPrimary < rightPrimary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftPair == EOS) { break; }
        leftPair >>= 16;
        rightPair >>= 16;
    }
    
    
    

    
    
    if(CollationSettings::getStrength(options) >= UCOL_SECONDARY) {
        leftIndex = rightIndex = 0;
        leftPair = rightPair = 0;
        for(;;) {
            while(leftPair == 0) {
                if(leftIndex == leftLength) {
                    leftPair = EOS;
                    break;
                }
                UChar32 c = left[leftIndex++];
                if(c <= LATIN_MAX) {
                    leftPair = table[c];
                } else if(PUNCT_START <= c && c < PUNCT_LIMIT) {
                    leftPair = table[c - PUNCT_START + LATIN_LIMIT];
                } else {
                    leftPair = lookup(table, c);
                }
                if(leftPair >= MIN_SHORT) {
                    leftPair = getSecondariesFromOneShortCE(leftPair);
                    break;
                } else if(leftPair > variableTop) {
                    leftPair = COMMON_SEC_PLUS_OFFSET;
                    break;
                } else {
                    leftPair = nextPair(table, c, leftPair, left, NULL, leftIndex, leftLength);
                    leftPair = getSecondaries(variableTop, leftPair);
                }
            }

            while(rightPair == 0) {
                if(rightIndex == rightLength) {
                    rightPair = EOS;
                    break;
                }
                UChar32 c = right[rightIndex++];
                if(c <= LATIN_MAX) {
                    rightPair = table[c];
                } else if(PUNCT_START <= c && c < PUNCT_LIMIT) {
                    rightPair = table[c - PUNCT_START + LATIN_LIMIT];
                } else {
                    rightPair = lookup(table, c);
                }
                if(rightPair >= MIN_SHORT) {
                    rightPair = getSecondariesFromOneShortCE(rightPair);
                    break;
                } else if(rightPair > variableTop) {
                    rightPair = COMMON_SEC_PLUS_OFFSET;
                    break;
                } else {
                    rightPair = nextPair(table, c, rightPair, right, NULL, rightIndex, rightLength);
                    rightPair = getSecondaries(variableTop, rightPair);
                }
            }

            if(leftPair == rightPair) {
                if(leftPair == EOS) { break; }
                leftPair = rightPair = 0;
                continue;
            }
            uint32_t leftSecondary = leftPair & 0xffff;
            uint32_t rightSecondary = rightPair & 0xffff;
            if(leftSecondary != rightSecondary) {
                if((options & CollationSettings::BACKWARD_SECONDARY) != 0) {
                    
                    
                    return BAIL_OUT_RESULT;
                }
                return (leftSecondary < rightSecondary) ? UCOL_LESS : UCOL_GREATER;
            }
            if(leftPair == EOS) { break; }
            leftPair >>= 16;
            rightPair >>= 16;
        }
    }

    if((options & CollationSettings::CASE_LEVEL) != 0) {
        UBool strengthIsPrimary = CollationSettings::getStrength(options) == UCOL_PRIMARY;
        leftIndex = rightIndex = 0;
        leftPair = rightPair = 0;
        for(;;) {
            while(leftPair == 0) {
                if(leftIndex == leftLength) {
                    leftPair = EOS;
                    break;
                }
                UChar32 c = left[leftIndex++];
                leftPair = (c <= LATIN_MAX) ? table[c] : lookup(table, c);
                if(leftPair < MIN_LONG) {
                    leftPair = nextPair(table, c, leftPair, left, NULL, leftIndex, leftLength);
                }
                leftPair = getCases(variableTop, strengthIsPrimary, leftPair);
            }

            while(rightPair == 0) {
                if(rightIndex == rightLength) {
                    rightPair = EOS;
                    break;
                }
                UChar32 c = right[rightIndex++];
                rightPair = (c <= LATIN_MAX) ? table[c] : lookup(table, c);
                if(rightPair < MIN_LONG) {
                    rightPair = nextPair(table, c, rightPair, right, NULL, rightIndex, rightLength);
                }
                rightPair = getCases(variableTop, strengthIsPrimary, rightPair);
            }

            if(leftPair == rightPair) {
                if(leftPair == EOS) { break; }
                leftPair = rightPair = 0;
                continue;
            }
            uint32_t leftCase = leftPair & 0xffff;
            uint32_t rightCase = rightPair & 0xffff;
            if(leftCase != rightCase) {
                if((options & CollationSettings::UPPER_FIRST) == 0) {
                    return (leftCase < rightCase) ? UCOL_LESS : UCOL_GREATER;
                } else {
                    return (leftCase < rightCase) ? UCOL_GREATER : UCOL_LESS;
                }
            }
            if(leftPair == EOS) { break; }
            leftPair >>= 16;
            rightPair >>= 16;
        }
    }
    if(CollationSettings::getStrength(options) <= UCOL_SECONDARY) { return UCOL_EQUAL; }

    
    UBool withCaseBits = CollationSettings::isTertiaryWithCaseBits(options);

    leftIndex = rightIndex = 0;
    leftPair = rightPair = 0;
    for(;;) {
        while(leftPair == 0) {
            if(leftIndex == leftLength) {
                leftPair = EOS;
                break;
            }
            UChar32 c = left[leftIndex++];
            leftPair = (c <= LATIN_MAX) ? table[c] : lookup(table, c);
            if(leftPair < MIN_LONG) {
                leftPair = nextPair(table, c, leftPair, left, NULL, leftIndex, leftLength);
            }
            leftPair = getTertiaries(variableTop, withCaseBits, leftPair);
        }

        while(rightPair == 0) {
            if(rightIndex == rightLength) {
                rightPair = EOS;
                break;
            }
            UChar32 c = right[rightIndex++];
            rightPair = (c <= LATIN_MAX) ? table[c] : lookup(table, c);
            if(rightPair < MIN_LONG) {
                rightPair = nextPair(table, c, rightPair, right, NULL, rightIndex, rightLength);
            }
            rightPair = getTertiaries(variableTop, withCaseBits, rightPair);
        }

        if(leftPair == rightPair) {
            if(leftPair == EOS) { break; }
            leftPair = rightPair = 0;
            continue;
        }
        uint32_t leftTertiary = leftPair & 0xffff;
        uint32_t rightTertiary = rightPair & 0xffff;
        if(leftTertiary != rightTertiary) {
            if(CollationSettings::sortsTertiaryUpperCaseFirst(options)) {
                
                
                
                if(leftTertiary > MERGE_WEIGHT) {
                    leftTertiary ^= CASE_MASK;
                }
                if(rightTertiary > MERGE_WEIGHT) {
                    rightTertiary ^= CASE_MASK;
                }
            }
            return (leftTertiary < rightTertiary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftPair == EOS) { break; }
        leftPair >>= 16;
        rightPair >>= 16;
    }
    if(CollationSettings::getStrength(options) <= UCOL_TERTIARY) { return UCOL_EQUAL; }

    leftIndex = rightIndex = 0;
    leftPair = rightPair = 0;
    for(;;) {
        while(leftPair == 0) {
            if(leftIndex == leftLength) {
                leftPair = EOS;
                break;
            }
            UChar32 c = left[leftIndex++];
            leftPair = (c <= LATIN_MAX) ? table[c] : lookup(table, c);
            if(leftPair < MIN_LONG) {
                leftPair = nextPair(table, c, leftPair, left, NULL, leftIndex, leftLength);
            }
            leftPair = getQuaternaries(variableTop, leftPair);
        }

        while(rightPair == 0) {
            if(rightIndex == rightLength) {
                rightPair = EOS;
                break;
            }
            UChar32 c = right[rightIndex++];
            rightPair = (c <= LATIN_MAX) ? table[c] : lookup(table, c);
            if(rightPair < MIN_LONG) {
                rightPair = nextPair(table, c, rightPair, right, NULL, rightIndex, rightLength);
            }
            rightPair = getQuaternaries(variableTop, rightPair);
        }

        if(leftPair == rightPair) {
            if(leftPair == EOS) { break; }
            leftPair = rightPair = 0;
            continue;
        }
        uint32_t leftQuaternary = leftPair & 0xffff;
        uint32_t rightQuaternary = rightPair & 0xffff;
        if(leftQuaternary != rightQuaternary) {
            return (leftQuaternary < rightQuaternary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftPair == EOS) { break; }
        leftPair >>= 16;
        rightPair >>= 16;
    }
    return UCOL_EQUAL;
}

int32_t
CollationFastLatin::compareUTF8(const uint16_t *table, const uint16_t *primaries, int32_t options,
                                 const uint8_t *left, int32_t leftLength,
                                 const uint8_t *right, int32_t rightLength) {
    

    U_ASSERT((table[0] >> 8) == VERSION);
    table += (table[0] & 0xff);  
    uint32_t variableTop = (uint32_t)options >> 16;  
    options &= 0xffff;  

    
    U_ALIGN_CODE(16);
    int32_t leftIndex = 0, rightIndex = 0;
    




    uint32_t leftPair = 0, rightPair = 0;
    
    
    
    for(;;) {
        
        while(leftPair == 0) {
            if(leftIndex == leftLength) {
                leftPair = EOS;
                break;
            }
            UChar32 c = left[leftIndex++];
            uint8_t t;
            if(c <= 0x7f) {
                leftPair = primaries[c];
                if(leftPair != 0) { break; }
                if(c <= 0x39 && c >= 0x30 && (options & CollationSettings::NUMERIC) != 0) {
                    return BAIL_OUT_RESULT;
                }
                leftPair = table[c];
            } else if(c <= LATIN_MAX_UTF8_LEAD && 0xc2 <= c && leftIndex != leftLength &&
                    0x80 <= (t = left[leftIndex]) && t <= 0xbf) {
                ++leftIndex;
                c = ((c - 0xc2) << 6) + t;
                leftPair = primaries[c];
                if(leftPair != 0) { break; }
                leftPair = table[c];
            } else {
                leftPair = lookupUTF8(table, c, left, leftIndex, leftLength);
            }
            if(leftPair >= MIN_SHORT) {
                leftPair &= SHORT_PRIMARY_MASK;
                break;
            } else if(leftPair > variableTop) {
                leftPair &= LONG_PRIMARY_MASK;
                break;
            } else {
                leftPair = nextPair(table, c, leftPair, NULL, left, leftIndex, leftLength);
                if(leftPair == BAIL_OUT) { return BAIL_OUT_RESULT; }
                leftPair = getPrimaries(variableTop, leftPair);
            }
        }

        while(rightPair == 0) {
            if(rightIndex == rightLength) {
                rightPair = EOS;
                break;
            }
            UChar32 c = right[rightIndex++];
            uint8_t t;
            if(c <= 0x7f) {
                rightPair = primaries[c];
                if(rightPair != 0) { break; }
                if(c <= 0x39 && c >= 0x30 && (options & CollationSettings::NUMERIC) != 0) {
                    return BAIL_OUT_RESULT;
                }
                rightPair = table[c];
            } else if(c <= LATIN_MAX_UTF8_LEAD && 0xc2 <= c && rightIndex != rightLength &&
                    0x80 <= (t = right[rightIndex]) && t <= 0xbf) {
                ++rightIndex;
                c = ((c - 0xc2) << 6) + t;
                rightPair = primaries[c];
                if(rightPair != 0) { break; }
                rightPair = table[c];
            } else {
                rightPair = lookupUTF8(table, c, right, rightIndex, rightLength);
            }
            if(rightPair >= MIN_SHORT) {
                rightPair &= SHORT_PRIMARY_MASK;
                break;
            } else if(rightPair > variableTop) {
                rightPair &= LONG_PRIMARY_MASK;
                break;
            } else {
                rightPair = nextPair(table, c, rightPair, NULL, right, rightIndex, rightLength);
                if(rightPair == BAIL_OUT) { return BAIL_OUT_RESULT; }
                rightPair = getPrimaries(variableTop, rightPair);
            }
        }

        if(leftPair == rightPair) {
            if(leftPair == EOS) { break; }
            leftPair = rightPair = 0;
            continue;
        }
        uint32_t leftPrimary = leftPair & 0xffff;
        uint32_t rightPrimary = rightPair & 0xffff;
        if(leftPrimary != rightPrimary) {
            
            return (leftPrimary < rightPrimary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftPair == EOS) { break; }
        leftPair >>= 16;
        rightPair >>= 16;
    }
    
    
    

    
    
    if(CollationSettings::getStrength(options) >= UCOL_SECONDARY) {
        leftIndex = rightIndex = 0;
        leftPair = rightPair = 0;
        for(;;) {
            while(leftPair == 0) {
                if(leftIndex == leftLength) {
                    leftPair = EOS;
                    break;
                }
                UChar32 c = left[leftIndex++];
                if(c <= 0x7f) {
                    leftPair = table[c];
                } else if(c <= LATIN_MAX_UTF8_LEAD) {
                    leftPair = table[((c - 0xc2) << 6) + left[leftIndex++]];
                } else {
                    leftPair = lookupUTF8Unsafe(table, c, left, leftIndex);
                }
                if(leftPair >= MIN_SHORT) {
                    leftPair = getSecondariesFromOneShortCE(leftPair);
                    break;
                } else if(leftPair > variableTop) {
                    leftPair = COMMON_SEC_PLUS_OFFSET;
                    break;
                } else {
                    leftPair = nextPair(table, c, leftPair, NULL, left, leftIndex, leftLength);
                    leftPair = getSecondaries(variableTop, leftPair);
                }
            }

            while(rightPair == 0) {
                if(rightIndex == rightLength) {
                    rightPair = EOS;
                    break;
                }
                UChar32 c = right[rightIndex++];
                if(c <= 0x7f) {
                    rightPair = table[c];
                } else if(c <= LATIN_MAX_UTF8_LEAD) {
                    rightPair = table[((c - 0xc2) << 6) + right[rightIndex++]];
                } else {
                    rightPair = lookupUTF8Unsafe(table, c, right, rightIndex);
                }
                if(rightPair >= MIN_SHORT) {
                    rightPair = getSecondariesFromOneShortCE(rightPair);
                    break;
                } else if(rightPair > variableTop) {
                    rightPair = COMMON_SEC_PLUS_OFFSET;
                    break;
                } else {
                    rightPair = nextPair(table, c, rightPair, NULL, right, rightIndex, rightLength);
                    rightPair = getSecondaries(variableTop, rightPair);
                }
            }

            if(leftPair == rightPair) {
                if(leftPair == EOS) { break; }
                leftPair = rightPair = 0;
                continue;
            }
            uint32_t leftSecondary = leftPair & 0xffff;
            uint32_t rightSecondary = rightPair & 0xffff;
            if(leftSecondary != rightSecondary) {
                if((options & CollationSettings::BACKWARD_SECONDARY) != 0) {
                    
                    
                    return BAIL_OUT_RESULT;
                }
                return (leftSecondary < rightSecondary) ? UCOL_LESS : UCOL_GREATER;
            }
            if(leftPair == EOS) { break; }
            leftPair >>= 16;
            rightPair >>= 16;
        }
    }

    if((options & CollationSettings::CASE_LEVEL) != 0) {
        UBool strengthIsPrimary = CollationSettings::getStrength(options) == UCOL_PRIMARY;
        leftIndex = rightIndex = 0;
        leftPair = rightPair = 0;
        for(;;) {
            while(leftPair == 0) {
                if(leftIndex == leftLength) {
                    leftPair = EOS;
                    break;
                }
                UChar32 c = left[leftIndex++];
                leftPair = (c <= 0x7f) ? table[c] : lookupUTF8Unsafe(table, c, left, leftIndex);
                if(leftPair < MIN_LONG) {
                    leftPair = nextPair(table, c, leftPair, NULL, left, leftIndex, leftLength);
                }
                leftPair = getCases(variableTop, strengthIsPrimary, leftPair);
            }

            while(rightPair == 0) {
                if(rightIndex == rightLength) {
                    rightPair = EOS;
                    break;
                }
                UChar32 c = right[rightIndex++];
                rightPair = (c <= 0x7f) ? table[c] : lookupUTF8Unsafe(table, c, right, rightIndex);
                if(rightPair < MIN_LONG) {
                    rightPair = nextPair(table, c, rightPair, NULL, right, rightIndex, rightLength);
                }
                rightPair = getCases(variableTop, strengthIsPrimary, rightPair);
            }

            if(leftPair == rightPair) {
                if(leftPair == EOS) { break; }
                leftPair = rightPair = 0;
                continue;
            }
            uint32_t leftCase = leftPair & 0xffff;
            uint32_t rightCase = rightPair & 0xffff;
            if(leftCase != rightCase) {
                if((options & CollationSettings::UPPER_FIRST) == 0) {
                    return (leftCase < rightCase) ? UCOL_LESS : UCOL_GREATER;
                } else {
                    return (leftCase < rightCase) ? UCOL_GREATER : UCOL_LESS;
                }
            }
            if(leftPair == EOS) { break; }
            leftPair >>= 16;
            rightPair >>= 16;
        }
    }
    if(CollationSettings::getStrength(options) <= UCOL_SECONDARY) { return UCOL_EQUAL; }

    
    UBool withCaseBits = CollationSettings::isTertiaryWithCaseBits(options);

    leftIndex = rightIndex = 0;
    leftPair = rightPair = 0;
    for(;;) {
        while(leftPair == 0) {
            if(leftIndex == leftLength) {
                leftPair = EOS;
                break;
            }
            UChar32 c = left[leftIndex++];
            leftPair = (c <= 0x7f) ? table[c] : lookupUTF8Unsafe(table, c, left, leftIndex);
            if(leftPair < MIN_LONG) {
                leftPair = nextPair(table, c, leftPair, NULL, left, leftIndex, leftLength);
            }
            leftPair = getTertiaries(variableTop, withCaseBits, leftPair);
        }

        while(rightPair == 0) {
            if(rightIndex == rightLength) {
                rightPair = EOS;
                break;
            }
            UChar32 c = right[rightIndex++];
            rightPair = (c <= 0x7f) ? table[c] : lookupUTF8Unsafe(table, c, right, rightIndex);
            if(rightPair < MIN_LONG) {
                rightPair = nextPair(table, c, rightPair, NULL, right, rightIndex, rightLength);
            }
            rightPair = getTertiaries(variableTop, withCaseBits, rightPair);
        }

        if(leftPair == rightPair) {
            if(leftPair == EOS) { break; }
            leftPair = rightPair = 0;
            continue;
        }
        uint32_t leftTertiary = leftPair & 0xffff;
        uint32_t rightTertiary = rightPair & 0xffff;
        if(leftTertiary != rightTertiary) {
            if(CollationSettings::sortsTertiaryUpperCaseFirst(options)) {
                
                
                
                if(leftTertiary > MERGE_WEIGHT) {
                    leftTertiary ^= CASE_MASK;
                }
                if(rightTertiary > MERGE_WEIGHT) {
                    rightTertiary ^= CASE_MASK;
                }
            }
            return (leftTertiary < rightTertiary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftPair == EOS) { break; }
        leftPair >>= 16;
        rightPair >>= 16;
    }
    if(CollationSettings::getStrength(options) <= UCOL_TERTIARY) { return UCOL_EQUAL; }

    leftIndex = rightIndex = 0;
    leftPair = rightPair = 0;
    for(;;) {
        while(leftPair == 0) {
            if(leftIndex == leftLength) {
                leftPair = EOS;
                break;
            }
            UChar32 c = left[leftIndex++];
            leftPair = (c <= 0x7f) ? table[c] : lookupUTF8Unsafe(table, c, left, leftIndex);
            if(leftPair < MIN_LONG) {
                leftPair = nextPair(table, c, leftPair, NULL, left, leftIndex, leftLength);
            }
            leftPair = getQuaternaries(variableTop, leftPair);
        }

        while(rightPair == 0) {
            if(rightIndex == rightLength) {
                rightPair = EOS;
                break;
            }
            UChar32 c = right[rightIndex++];
            rightPair = (c <= 0x7f) ? table[c] : lookupUTF8Unsafe(table, c, right, rightIndex);
            if(rightPair < MIN_LONG) {
                rightPair = nextPair(table, c, rightPair, NULL, right, rightIndex, rightLength);
            }
            rightPair = getQuaternaries(variableTop, rightPair);
        }

        if(leftPair == rightPair) {
            if(leftPair == EOS) { break; }
            leftPair = rightPair = 0;
            continue;
        }
        uint32_t leftQuaternary = leftPair & 0xffff;
        uint32_t rightQuaternary = rightPair & 0xffff;
        if(leftQuaternary != rightQuaternary) {
            return (leftQuaternary < rightQuaternary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftPair == EOS) { break; }
        leftPair >>= 16;
        rightPair >>= 16;
    }
    return UCOL_EQUAL;
}

uint32_t
CollationFastLatin::lookup(const uint16_t *table, UChar32 c) {
    U_ASSERT(c > LATIN_MAX);
    if(PUNCT_START <= c && c < PUNCT_LIMIT) {
        return table[c - PUNCT_START + LATIN_LIMIT];
    } else if(c == 0xfffe) {
        return MERGE_WEIGHT;
    } else if(c == 0xffff) {
        return MAX_SHORT | COMMON_SEC | LOWER_CASE | COMMON_TER;
    } else {
        return BAIL_OUT;
    }
}

uint32_t
CollationFastLatin::lookupUTF8(const uint16_t *table, UChar32 c,
                               const uint8_t *s8, int32_t &sIndex, int32_t sLength) {
    
    U_ASSERT(c > 0x7f);
    int32_t i2 = sIndex + 1;
    if(i2 < sLength || sLength < 0) {
        uint8_t t1 = s8[sIndex];
        uint8_t t2 = s8[i2];
        sIndex += 2;
        if(c == 0xe2 && t1 == 0x80 && 0x80 <= t2 && t2 <= 0xbf) {
            return table[(LATIN_LIMIT - 0x80) + t2];  
        } else if(c == 0xef && t1 == 0xbf) {
            if(t2 == 0xbe) {
                return MERGE_WEIGHT;  
            } else if(t2 == 0xbf) {
                return MAX_SHORT | COMMON_SEC | LOWER_CASE | COMMON_TER;  
            }
        }
    }
    return BAIL_OUT;
}

uint32_t
CollationFastLatin::lookupUTF8Unsafe(const uint16_t *table, UChar32 c,
                                     const uint8_t *s8, int32_t &sIndex) {
    
    
    U_ASSERT(c > 0x7f);
    if(c <= LATIN_MAX_UTF8_LEAD) {
        return table[((c - 0xc2) << 6) + s8[sIndex++]];  
    }
    uint8_t t2 = s8[sIndex + 1];
    sIndex += 2;
    if(c == 0xe2) {
        return table[(LATIN_LIMIT - 0x80) + t2];  
    } else if(t2 == 0xbe) {
        return MERGE_WEIGHT;  
    } else {
        return MAX_SHORT | COMMON_SEC | LOWER_CASE | COMMON_TER;  
    }
}

uint32_t
CollationFastLatin::nextPair(const uint16_t *table, UChar32 c, uint32_t ce,
                             const UChar *s16, const uint8_t *s8, int32_t &sIndex, int32_t &sLength) {
    if(ce >= MIN_LONG || ce < CONTRACTION) {
        return ce;  
    } else if(ce >= EXPANSION) {
        int32_t index = NUM_FAST_CHARS + (ce & INDEX_MASK);
        return ((uint32_t)table[index + 1] << 16) | table[index];
    } else  {
        if(c == 0 && sLength < 0) {
            sLength = sIndex - 1;
            return EOS;
        }
        
        
        int32_t index = NUM_FAST_CHARS + (ce & INDEX_MASK);
        if(sIndex != sLength) {
            
            int32_t c2;
            int32_t nextIndex = sIndex;
            if(s16 != NULL) {
                c2 = s16[nextIndex++];
                if(c2 > LATIN_MAX) {
                    if(PUNCT_START <= c2 && c2 < PUNCT_LIMIT) {
                        c2 = c2 - PUNCT_START + LATIN_LIMIT;  
                    } else if(c2 == 0xfffe || c2 == 0xffff) {
                        c2 = -1;  
                    } else {
                        return BAIL_OUT;
                    }
                }
            } else {
                c2 = s8[nextIndex++];
                if(c2 > 0x7f) {
                    uint8_t t;
                    if(c2 <= 0xc5 && 0xc2 <= c2 && nextIndex != sLength &&
                            0x80 <= (t = s8[nextIndex]) && t <= 0xbf) {
                        c2 = ((c2 - 0xc2) << 6) + t;  
                        ++nextIndex;
                    } else {
                        int32_t i2 = nextIndex + 1;
                        if(i2 < sLength || sLength < 0) {
                            if(c2 == 0xe2 && s8[nextIndex] == 0x80 &&
                                    0x80 <= (t = s8[i2]) && t <= 0xbf) {
                                c2 = (LATIN_LIMIT - 0x80) + t;  
                            } else if(c2 == 0xef && s8[nextIndex] == 0xbf &&
                                    ((t = s8[i2]) == 0xbe || t == 0xbf)) {
                                c2 = -1;  
                            } else {
                                return BAIL_OUT;
                            }
                        } else {
                            return BAIL_OUT;
                        }
                        nextIndex += 2;
                    }
                }
            }
            if(c2 == 0 && sLength < 0) {
                sLength = sIndex;
                c2 = -1;
            }
            
            
            int32_t i = index;
            int32_t head = table[i];  
            int32_t x;
            do {
                i += head >> CONTR_LENGTH_SHIFT;
                head = table[i];
                x = head & CONTR_CHAR_MASK;
            } while(x < c2);
            if(x == c2) {
                index = i;
                sIndex = nextIndex;
            }
        }
        
        int32_t length = table[index] >> CONTR_LENGTH_SHIFT;
        if(length == 1) {
            return BAIL_OUT;
        }
        ce = table[index + 1];
        if(length == 2) {
            return ce;
        } else {
            return ((uint32_t)table[index + 2] << 16) | ce;
        }
    }
}

uint32_t
CollationFastLatin::getSecondaries(uint32_t variableTop, uint32_t pair) {
    if(pair <= 0xffff) {
        
        if(pair >= MIN_SHORT) {
            pair = getSecondariesFromOneShortCE(pair);
        } else if(pair > variableTop) {
            pair = COMMON_SEC_PLUS_OFFSET;
        } else if(pair >= MIN_LONG) {
            pair = 0;  
        }
        
    } else {
        uint32_t ce = pair & 0xffff;
        if(ce >= MIN_SHORT) {
            pair = (pair & TWO_SECONDARIES_MASK) + TWO_SEC_OFFSETS;
        } else if(ce > variableTop) {
            pair = TWO_COMMON_SEC_PLUS_OFFSET;
        } else {
            U_ASSERT(ce >= MIN_LONG);
            pair = 0;  
        }
    }
    return pair;
}

uint32_t
CollationFastLatin::getCases(uint32_t variableTop, UBool strengthIsPrimary, uint32_t pair) {
    
    
    
    
    if(pair <= 0xffff) {
        
        if(pair >= MIN_SHORT) {
            
            
            uint32_t ce = pair;
            pair &= CASE_MASK;  
            if(!strengthIsPrimary && (ce & SECONDARY_MASK) >= MIN_SEC_HIGH) {
                pair |= LOWER_CASE << 16;  
            }
        } else if(pair > variableTop) {
            pair = LOWER_CASE;
        } else if(pair >= MIN_LONG) {
            pair = 0;  
        }
        
    } else {
        
        uint32_t ce = pair & 0xffff;
        if(ce >= MIN_SHORT) {
            if(strengthIsPrimary && (pair & (SHORT_PRIMARY_MASK << 16)) == 0) {
                pair &= CASE_MASK;
            } else {
                pair &= TWO_CASES_MASK;
            }
        } else if(ce > variableTop) {
            pair = TWO_LOWER_CASES;
        } else {
            U_ASSERT(ce >= MIN_LONG);
            pair = 0;  
        }
    }
    return pair;
}

uint32_t
CollationFastLatin::getTertiaries(uint32_t variableTop, UBool withCaseBits, uint32_t pair) {
    if(pair <= 0xffff) {
        
        if(pair >= MIN_SHORT) {
            
            
            uint32_t ce = pair;
            if(withCaseBits) {
                pair = (pair & CASE_AND_TERTIARY_MASK) + TER_OFFSET;
                if((ce & SECONDARY_MASK) >= MIN_SEC_HIGH) {
                    pair |= (LOWER_CASE | COMMON_TER_PLUS_OFFSET) << 16;
                }
            } else {
                pair = (pair & TERTIARY_MASK) + TER_OFFSET;
                if((ce & SECONDARY_MASK) >= MIN_SEC_HIGH) {
                    pair |= COMMON_TER_PLUS_OFFSET << 16;
                }
            }
        } else if(pair > variableTop) {
            pair = (pair & TERTIARY_MASK) + TER_OFFSET;
            if(withCaseBits) {
                pair |= LOWER_CASE;
            }
        } else if(pair >= MIN_LONG) {
            pair = 0;  
        }
        
    } else {
        
        uint32_t ce = pair & 0xffff;
        if(ce >= MIN_SHORT) {
            if(withCaseBits) {
                pair &= TWO_CASES_MASK | TWO_TERTIARIES_MASK;
            } else {
                pair &= TWO_TERTIARIES_MASK;
            }
            pair += TWO_TER_OFFSETS;
        } else if(ce > variableTop) {
            pair = (pair & TWO_TERTIARIES_MASK) + TWO_TER_OFFSETS;
            if(withCaseBits) {
                pair |= TWO_LOWER_CASES;
            }
        } else {
            U_ASSERT(ce >= MIN_LONG);
            pair = 0;  
        }
    }
    return pair;
}

uint32_t
CollationFastLatin::getQuaternaries(uint32_t variableTop, uint32_t pair) {
    
    
    if(pair <= 0xffff) {
        
        if(pair >= MIN_SHORT) {
            
            
            if((pair & SECONDARY_MASK) >= MIN_SEC_HIGH) {
                pair = TWO_SHORT_PRIMARIES_MASK;
            } else {
                pair = SHORT_PRIMARY_MASK;
            }
        } else if(pair > variableTop) {
            pair = SHORT_PRIMARY_MASK;
        } else if(pair >= MIN_LONG) {
            pair &= LONG_PRIMARY_MASK;  
        }
        
    } else {
        
        uint32_t ce = pair & 0xffff;
        if(ce > variableTop) {
            pair = TWO_SHORT_PRIMARIES_MASK;
        } else {
            U_ASSERT(ce >= MIN_LONG);
            pair &= TWO_LONG_PRIMARIES_MASK;  
        }
    }
    return pair;
}

U_NAMESPACE_END

#endif  
