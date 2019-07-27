










#ifndef __COLLATIONROOTELEMENTS_H__
#define __COLLATIONROOTELEMENTS_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "collation.h"

U_NAMESPACE_BEGIN








class U_I18N_API CollationRootElements : public UMemory {
public:
    CollationRootElements(const uint32_t *rootElements, int32_t rootElementsLength)
            : elements(rootElements), length(rootElementsLength) {}

    


    static const uint32_t PRIMARY_SENTINEL = 0xffffff00;

    



    static const uint32_t SEC_TER_DELTA_FLAG = 0x80;
    


    static const uint8_t PRIMARY_STEP_MASK = 0x7f;

    enum {
        



        IX_FIRST_TERTIARY_INDEX,
        


        IX_FIRST_SECONDARY_INDEX,
        


        IX_FIRST_PRIMARY_INDEX,
        


        IX_COMMON_SEC_AND_TER_CE,
        






        IX_SEC_TER_BOUNDARIES,
        



        IX_COUNT
    };

    





    uint32_t getTertiaryBoundary() const {
        return (elements[IX_SEC_TER_BOUNDARIES] << 8) & 0xff00;
    }

    


    uint32_t getFirstTertiaryCE() const {
        return elements[elements[IX_FIRST_TERTIARY_INDEX]] & ~SEC_TER_DELTA_FLAG;
    }

    


    uint32_t getLastTertiaryCE() const {
        return elements[elements[IX_FIRST_SECONDARY_INDEX] - 1] & ~SEC_TER_DELTA_FLAG;
    }

    



    uint32_t getLastCommonSecondary() const {
        return (elements[IX_SEC_TER_BOUNDARIES] >> 16) & 0xff00;
    }

    





    uint32_t getSecondaryBoundary() const {
        return (elements[IX_SEC_TER_BOUNDARIES] >> 8) & 0xff00;
    }

    


    uint32_t getFirstSecondaryCE() const {
        return elements[elements[IX_FIRST_SECONDARY_INDEX]] & ~SEC_TER_DELTA_FLAG;
    }

    


    uint32_t getLastSecondaryCE() const {
        return elements[elements[IX_FIRST_PRIMARY_INDEX] - 1] & ~SEC_TER_DELTA_FLAG;
    }

    


    uint32_t getFirstPrimary() const {
        return elements[elements[IX_FIRST_PRIMARY_INDEX]];  
    }

    


    int64_t getFirstPrimaryCE() const {
        return Collation::makeCE(getFirstPrimary());
    }

    



    int64_t lastCEWithPrimaryBefore(uint32_t p) const;

    



    int64_t firstCEWithPrimaryAtLeast(uint32_t p) const;

    



    uint32_t getPrimaryBefore(uint32_t p, UBool isCompressible) const;

    
    uint32_t getSecondaryBefore(uint32_t p, uint32_t s) const;

    
    uint32_t getTertiaryBefore(uint32_t p, uint32_t s, uint32_t t) const;

    



    int32_t findPrimary(uint32_t p) const;

    



    uint32_t getPrimaryAfter(uint32_t p, int32_t index, UBool isCompressible) const;
    









    uint32_t getSecondaryAfter(int32_t index, uint32_t s) const;
    









    uint32_t getTertiaryAfter(int32_t index, uint32_t s, uint32_t t) const;

private:
    


    uint32_t getFirstSecTerForPrimary(int32_t index) const;

    




    int32_t findP(uint32_t p) const;

    static inline UBool isEndOfPrimaryRange(uint32_t q) {
        return (q & SEC_TER_DELTA_FLAG) == 0 && (q & PRIMARY_STEP_MASK) != 0;
    }

    





































    const uint32_t *elements;
    int32_t length;
};

U_NAMESPACE_END

#endif  
#endif  
