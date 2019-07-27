










#ifndef __COLLATIONSETTINGS_H__
#define __COLLATIONSETTINGS_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "collation.h"
#include "sharedobject.h"
#include "umutex.h"

U_NAMESPACE_BEGIN

struct CollationData;





struct U_I18N_API CollationSettings : public SharedObject {
    


    static const int32_t CHECK_FCD = 1;
    






    static const int32_t NUMERIC = 2;
    


    static const int32_t SHIFTED = 4;
    



    static const int32_t ALTERNATE_MASK = 0xc;
    


    static const int32_t MAX_VARIABLE_SHIFT = 4;
    
    static const int32_t MAX_VARIABLE_MASK = 0x70;
    
    


    static const int32_t UPPER_FIRST = 0x100;
    







    static const int32_t CASE_FIRST = 0x200;
    



    static const int32_t CASE_FIRST_AND_UPPER_MASK = CASE_FIRST | UPPER_FIRST;
    


    static const int32_t CASE_LEVEL = 0x400;
    


    static const int32_t BACKWARD_SECONDARY = 0x800;
    



    static const int32_t STRENGTH_SHIFT = 12;
    
    static const int32_t STRENGTH_MASK = 0xf000;

    
    enum MaxVariable {
        MAX_VAR_SPACE,
        MAX_VAR_PUNCT,
        MAX_VAR_SYMBOL,
        MAX_VAR_CURRENCY
    };

    CollationSettings()
            : options((UCOL_DEFAULT_STRENGTH << STRENGTH_SHIFT) |
                      (MAX_VAR_PUNCT << MAX_VARIABLE_SHIFT)),
              variableTop(0),
              reorderTable(NULL),
              minHighNoReorder(0),
              reorderRanges(NULL), reorderRangesLength(0),
              reorderCodes(NULL), reorderCodesLength(0), reorderCodesCapacity(0),
              fastLatinOptions(-1) {}

    CollationSettings(const CollationSettings &other);
    virtual ~CollationSettings();

    UBool operator==(const CollationSettings &other) const;

    inline UBool operator!=(const CollationSettings &other) const {
        return !operator==(other);
    }

    int32_t hashCode() const;

    void resetReordering();
    void aliasReordering(const CollationData &data, const int32_t *codes, int32_t length,
                         const uint32_t *ranges, int32_t rangesLength,
                         const uint8_t *table, UErrorCode &errorCode);
    void setReordering(const CollationData &data, const int32_t *codes, int32_t codesLength,
                       UErrorCode &errorCode);
    void copyReorderingFrom(const CollationSettings &other, UErrorCode &errorCode);

    inline UBool hasReordering() const { return reorderTable != NULL; }
    static UBool reorderTableHasSplitBytes(const uint8_t table[256]);
    inline uint32_t reorder(uint32_t p) const {
        uint8_t b = reorderTable[p >> 24];
        if(b != 0 || p <= Collation::NO_CE_PRIMARY) {
            return ((uint32_t)b << 24) | (p & 0xffffff);
        } else {
            return reorderEx(p);
        }
    }

    void setStrength(int32_t value, int32_t defaultOptions, UErrorCode &errorCode);

    static int32_t getStrength(int32_t options) {
        return options >> STRENGTH_SHIFT;
    }

    int32_t getStrength() const {
        return getStrength(options);
    }

    
    void setFlag(int32_t bit, UColAttributeValue value,
                 int32_t defaultOptions, UErrorCode &errorCode);

    UColAttributeValue getFlag(int32_t bit) const {
        return ((options & bit) != 0) ? UCOL_ON : UCOL_OFF;
    }

    void setCaseFirst(UColAttributeValue value, int32_t defaultOptions, UErrorCode &errorCode);

    UColAttributeValue getCaseFirst() const {
        int32_t option = options & CASE_FIRST_AND_UPPER_MASK;
        return (option == 0) ? UCOL_OFF :
                (option == CASE_FIRST) ? UCOL_LOWER_FIRST : UCOL_UPPER_FIRST;
    }

    void setAlternateHandling(UColAttributeValue value,
                              int32_t defaultOptions, UErrorCode &errorCode);

    UColAttributeValue getAlternateHandling() const {
        return ((options & ALTERNATE_MASK) == 0) ? UCOL_NON_IGNORABLE : UCOL_SHIFTED;
    }

    void setMaxVariable(int32_t value, int32_t defaultOptions, UErrorCode &errorCode);

    MaxVariable getMaxVariable() const {
        return (MaxVariable)((options & MAX_VARIABLE_MASK) >> MAX_VARIABLE_SHIFT);
    }

    


    static inline UBool isTertiaryWithCaseBits(int32_t options) {
        return (options & (CASE_LEVEL | CASE_FIRST)) == CASE_FIRST;
    }
    static uint32_t getTertiaryMask(int32_t options) {
        
        return isTertiaryWithCaseBits(options) ?
                Collation::CASE_AND_TERTIARY_MASK : Collation::ONLY_TERTIARY_MASK;
    }

    static UBool sortsTertiaryUpperCaseFirst(int32_t options) {
        
        
        return (options & (CASE_LEVEL | CASE_FIRST_AND_UPPER_MASK)) == CASE_FIRST_AND_UPPER_MASK;
    }

    inline UBool dontCheckFCD() const {
        return (options & CHECK_FCD) == 0;
    }

    inline UBool hasBackwardSecondary() const {
        return (options & BACKWARD_SECONDARY) != 0;
    }

    inline UBool isNumeric() const {
        return (options & NUMERIC) != 0;
    }

    
    int32_t options;
    
    uint32_t variableTop;
    





    const uint8_t *reorderTable;
    
    uint32_t minHighNoReorder;
    


















    const uint32_t *reorderRanges;
    int32_t reorderRangesLength;
    
    const int32_t *reorderCodes;
    
    int32_t reorderCodesLength;
    





    int32_t reorderCodesCapacity;

    
    int32_t fastLatinOptions;
    uint16_t fastLatinPrimaries[0x180];

private:
    void setReorderArrays(const int32_t *codes, int32_t codesLength,
                          const uint32_t *ranges, int32_t rangesLength,
                          const uint8_t *table, UErrorCode &errorCode);
    uint32_t reorderEx(uint32_t p) const;
};

U_NAMESPACE_END

#endif  
#endif  
