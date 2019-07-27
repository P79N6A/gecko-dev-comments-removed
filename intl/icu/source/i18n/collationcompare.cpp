










#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "cmemory.h"
#include "collation.h"
#include "collationcompare.h"
#include "collationiterator.h"
#include "collationsettings.h"
#include "uassert.h"

U_NAMESPACE_BEGIN

UCollationResult
CollationCompare::compareUpToQuaternary(CollationIterator &left, CollationIterator &right,
                                        const CollationSettings &settings,
                                        UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return UCOL_EQUAL; }

    int32_t options = settings.options;
    uint32_t variableTop;
    if((options & CollationSettings::ALTERNATE_MASK) == 0) {
        variableTop = 0;
    } else {
        
        variableTop = settings.variableTop + 1;
    }
    UBool anyVariable = FALSE;

    
    U_ALIGN_CODE(16);
    for(;;) {
        
        uint32_t leftPrimary;
        do {
            int64_t ce = left.nextCE(errorCode);
            leftPrimary = (uint32_t)(ce >> 32);
            if(leftPrimary < variableTop && leftPrimary > Collation::MERGE_SEPARATOR_PRIMARY) {
                
                
                anyVariable = TRUE;
                do {
                    
                    left.setCurrentCE(ce & INT64_C(0xffffffff00000000));
                    for(;;) {
                        ce = left.nextCE(errorCode);
                        leftPrimary = (uint32_t)(ce >> 32);
                        if(leftPrimary == 0) {
                            left.setCurrentCE(0);
                        } else {
                            break;
                        }
                    }
                } while(leftPrimary < variableTop &&
                        leftPrimary > Collation::MERGE_SEPARATOR_PRIMARY);
            }
        } while(leftPrimary == 0);

        uint32_t rightPrimary;
        do {
            int64_t ce = right.nextCE(errorCode);
            rightPrimary = (uint32_t)(ce >> 32);
            if(rightPrimary < variableTop && rightPrimary > Collation::MERGE_SEPARATOR_PRIMARY) {
                
                
                anyVariable = TRUE;
                do {
                    
                    right.setCurrentCE(ce & INT64_C(0xffffffff00000000));
                    for(;;) {
                        ce = right.nextCE(errorCode);
                        rightPrimary = (uint32_t)(ce >> 32);
                        if(rightPrimary == 0) {
                            right.setCurrentCE(0);
                        } else {
                            break;
                        }
                    }
                } while(rightPrimary < variableTop &&
                        rightPrimary > Collation::MERGE_SEPARATOR_PRIMARY);
            }
        } while(rightPrimary == 0);

        if(leftPrimary != rightPrimary) {
            
            if(settings.hasReordering()) {
                leftPrimary = settings.reorder(leftPrimary);
                rightPrimary = settings.reorder(rightPrimary);
            }
            return (leftPrimary < rightPrimary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftPrimary == Collation::NO_CE_PRIMARY) { break; }
    }
    if(U_FAILURE(errorCode)) { return UCOL_EQUAL; }

    
    
    
    if(CollationSettings::getStrength(options) >= UCOL_SECONDARY) {
        if((options & CollationSettings::BACKWARD_SECONDARY) == 0) {
            int32_t leftIndex = 0;
            int32_t rightIndex = 0;
            for(;;) {
                uint32_t leftSecondary;
                do {
                    leftSecondary = ((uint32_t)left.getCE(leftIndex++)) >> 16;
                } while(leftSecondary == 0);

                uint32_t rightSecondary;
                do {
                    rightSecondary = ((uint32_t)right.getCE(rightIndex++)) >> 16;
                } while(rightSecondary == 0);

                if(leftSecondary != rightSecondary) {
                    return (leftSecondary < rightSecondary) ? UCOL_LESS : UCOL_GREATER;
                }
                if(leftSecondary == Collation::NO_CE_WEIGHT16) { break; }
            }
        } else {
            
            
            int32_t leftStart = 0;
            int32_t rightStart = 0;
            for(;;) {
                
                uint32_t p;
                int32_t leftLimit = leftStart;
                while((p = (uint32_t)(left.getCE(leftLimit) >> 32)) >
                            Collation::MERGE_SEPARATOR_PRIMARY ||
                        p == 0) {
                    ++leftLimit;
                }
                int32_t rightLimit = rightStart;
                while((p = (uint32_t)(right.getCE(rightLimit) >> 32)) >
                            Collation::MERGE_SEPARATOR_PRIMARY ||
                        p == 0) {
                    ++rightLimit;
                }

                
                int32_t leftIndex = leftLimit;
                int32_t rightIndex = rightLimit;
                for(;;) {
                    int32_t leftSecondary = 0;
                    while(leftSecondary == 0 && leftIndex > leftStart) {
                        leftSecondary = ((uint32_t)left.getCE(--leftIndex)) >> 16;
                    }

                    int32_t rightSecondary = 0;
                    while(rightSecondary == 0 && rightIndex > rightStart) {
                        rightSecondary = ((uint32_t)right.getCE(--rightIndex)) >> 16;
                    }

                    if(leftSecondary != rightSecondary) {
                        return (leftSecondary < rightSecondary) ? UCOL_LESS : UCOL_GREATER;
                    }
                    if(leftSecondary == 0) { break; }
                }

                
                
                
                U_ASSERT(left.getCE(leftLimit) == right.getCE(rightLimit));
                if(p == Collation::NO_CE_PRIMARY) { break; }
                
                leftStart = leftLimit + 1;
                rightStart = rightLimit + 1;
            }
        }
    }

    if((options & CollationSettings::CASE_LEVEL) != 0) {
        int32_t strength = CollationSettings::getStrength(options);
        int32_t leftIndex = 0;
        int32_t rightIndex = 0;
        for(;;) {
            uint32_t leftCase, leftLower32, rightCase;
            if(strength == UCOL_PRIMARY) {
                
                
                
                
                
                int64_t ce;
                do {
                    ce = left.getCE(leftIndex++);
                    leftCase = (uint32_t)ce;
                } while((uint32_t)(ce >> 32) == 0 || leftCase == 0);
                leftLower32 = leftCase;
                leftCase &= 0xc000;

                do {
                    ce = right.getCE(rightIndex++);
                    rightCase = (uint32_t)ce;
                } while((uint32_t)(ce >> 32) == 0 || rightCase == 0);
                rightCase &= 0xc000;
            } else {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                do {
                    leftCase = (uint32_t)left.getCE(leftIndex++);
                } while(leftCase <= 0xffff);
                leftLower32 = leftCase;
                leftCase &= 0xc000;

                do {
                    rightCase = (uint32_t)right.getCE(rightIndex++);
                } while(rightCase <= 0xffff);
                rightCase &= 0xc000;
            }

            
            
            
            if(leftCase != rightCase) {
                if((options & CollationSettings::UPPER_FIRST) == 0) {
                    return (leftCase < rightCase) ? UCOL_LESS : UCOL_GREATER;
                } else {
                    return (leftCase < rightCase) ? UCOL_GREATER : UCOL_LESS;
                }
            }
            if((leftLower32 >> 16) == Collation::NO_CE_WEIGHT16) { break; }
        }
    }
    if(CollationSettings::getStrength(options) <= UCOL_SECONDARY) { return UCOL_EQUAL; }

    uint32_t tertiaryMask = CollationSettings::getTertiaryMask(options);

    int32_t leftIndex = 0;
    int32_t rightIndex = 0;
    uint32_t anyQuaternaries = 0;
    for(;;) {
        uint32_t leftLower32, leftTertiary;
        do {
            leftLower32 = (uint32_t)left.getCE(leftIndex++);
            anyQuaternaries |= leftLower32;
            U_ASSERT((leftLower32 & Collation::ONLY_TERTIARY_MASK) != 0 ||
                     (leftLower32 & 0xc0c0) == 0);
            leftTertiary = leftLower32 & tertiaryMask;
        } while(leftTertiary == 0);

        uint32_t rightLower32, rightTertiary;
        do {
            rightLower32 = (uint32_t)right.getCE(rightIndex++);
            anyQuaternaries |= rightLower32;
            U_ASSERT((rightLower32 & Collation::ONLY_TERTIARY_MASK) != 0 ||
                     (rightLower32 & 0xc0c0) == 0);
            rightTertiary = rightLower32 & tertiaryMask;
        } while(rightTertiary == 0);

        if(leftTertiary != rightTertiary) {
            if(CollationSettings::sortsTertiaryUpperCaseFirst(options)) {
                
                
                
                
                
                if(leftTertiary > Collation::NO_CE_WEIGHT16) {
                    if(leftLower32 > 0xffff) {
                        leftTertiary ^= 0xc000;
                    } else {
                        leftTertiary += 0x4000;
                    }
                }
                if(rightTertiary > Collation::NO_CE_WEIGHT16) {
                    if(rightLower32 > 0xffff) {
                        rightTertiary ^= 0xc000;
                    } else {
                        rightTertiary += 0x4000;
                    }
                }
            }
            return (leftTertiary < rightTertiary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftTertiary == Collation::NO_CE_WEIGHT16) { break; }
    }
    if(CollationSettings::getStrength(options) <= UCOL_TERTIARY) { return UCOL_EQUAL; }

    if(!anyVariable && (anyQuaternaries & 0xc0) == 0) {
        
        
        return UCOL_EQUAL;
    }

    leftIndex = 0;
    rightIndex = 0;
    for(;;) {
        uint32_t leftQuaternary;
        do {
            int64_t ce = left.getCE(leftIndex++);
            leftQuaternary = (uint32_t)ce & 0xffff;
            if(leftQuaternary <= Collation::NO_CE_WEIGHT16) {
                
                leftQuaternary = (uint32_t)(ce >> 32);
            } else {
                
                
                leftQuaternary |= 0xffffff3f;
            }
        } while(leftQuaternary == 0);

        uint32_t rightQuaternary;
        do {
            int64_t ce = right.getCE(rightIndex++);
            rightQuaternary = (uint32_t)ce & 0xffff;
            if(rightQuaternary <= Collation::NO_CE_WEIGHT16) {
                
                rightQuaternary = (uint32_t)(ce >> 32);
            } else {
                
                
                rightQuaternary |= 0xffffff3f;
            }
        } while(rightQuaternary == 0);

        if(leftQuaternary != rightQuaternary) {
            
            if(settings.hasReordering()) {
                leftQuaternary = settings.reorder(leftQuaternary);
                rightQuaternary = settings.reorder(rightQuaternary);
            }
            return (leftQuaternary < rightQuaternary) ? UCOL_LESS : UCOL_GREATER;
        }
        if(leftQuaternary == Collation::NO_CE_PRIMARY) { break; }
    }
    return UCOL_EQUAL;
}

U_NAMESPACE_END

#endif  
