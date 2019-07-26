



















#ifndef SORTKEY_H
#define SORTKEY_H

#include "unicode/utypes.h"





 
#if !UCONFIG_NO_COLLATION
#ifndef U_HIDE_DEPRECATED_API

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/coll.h"

U_NAMESPACE_BEGIN


class RuleBasedCollator;























































class U_I18N_API CollationKey : public UObject {
public:
    






    CollationKey();


    





    CollationKey(const  uint8_t*    values,
                int32_t     count);

    




    CollationKey(const CollationKey& other);

    



    virtual ~CollationKey();

    




    const   CollationKey&   operator=(const CollationKey& other);

    





    UBool                   operator==(const CollationKey& source) const;

    





    UBool                   operator!=(const CollationKey& source) const;


    





    UBool                   isBogus(void) const;

    








    const    uint8_t*       getByteArray(int32_t& count) const;

#ifdef U_USE_COLLATION_KEY_DEPRECATES
    






    uint8_t*                toByteArray(int32_t& count) const;
#endif

    








    Collator::EComparisonResult compareTo(const CollationKey& target) const;

    









    UCollationResult compareTo(const CollationKey& target, UErrorCode &status) const;

    



















    int32_t                 hashCode(void) const;

    



    virtual UClassID getDynamicClassID() const;

    



    static UClassID U_EXPORT2 getStaticClassID();

private:
    




    uint8_t *reallocate(int32_t newCapacity, int32_t length);
    


    void setLength(int32_t newLength);

    uint8_t *getBytes() {
        return (fFlagAndLength >= 0) ? fUnion.fStackBuffer : fUnion.fFields.fBytes;
    }
    const uint8_t *getBytes() const {
        return (fFlagAndLength >= 0) ? fUnion.fStackBuffer : fUnion.fFields.fBytes;
    }
    int32_t getCapacity() const {
        return (fFlagAndLength >= 0) ? (int32_t)sizeof(fUnion) : fUnion.fFields.fCapacity;
    }
    int32_t getLength() const { return fFlagAndLength & 0x7fffffff; }

    



    CollationKey&           setToBogus(void);
    



    CollationKey&           reset(void);

    


    friend  class           RuleBasedCollator;
    friend  class           CollationKeyByteSink;

    
    
    
    

    
    




    int32_t fFlagAndLength;
    



    mutable int32_t fHashCode;
    



    union StackBufferOrFields {
        
        uint8_t fStackBuffer[32];
        struct {
            uint8_t *fBytes;
            int32_t fCapacity;
        } fFields;
    } fUnion;
};

inline UBool
CollationKey::operator!=(const CollationKey& other) const
{
    return !(*this == other);
}

inline UBool
CollationKey::isBogus() const
{
    return fHashCode == 2;  
}

inline const uint8_t*
CollationKey::getByteArray(int32_t &count) const
{
    count = getLength();
    return getBytes();
}

U_NAMESPACE_END

#endif  
#endif 

#endif
