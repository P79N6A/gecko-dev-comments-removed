






























#ifndef COLEITR_H
#define COLEITR_H

#include "unicode/utypes.h"

 
#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "unicode/tblcoll.h"
#include "unicode/ucoleitr.h"






typedef struct UCollationElements UCollationElements;

U_NAMESPACE_BEGIN




































































class U_I18N_API CollationElementIterator : public UObject {
public: 

    

    enum {
        



        NULLORDER = (int32_t)0xffffffff
    };

    

    





    CollationElementIterator(const CollationElementIterator& other);

    



    virtual ~CollationElementIterator();

    

    






    UBool operator==(const CollationElementIterator& other) const;

    






    UBool operator!=(const CollationElementIterator& other) const;

    



    void reset(void);

    






    int32_t next(UErrorCode& status);

    






    int32_t previous(UErrorCode& status);

    





    static inline int32_t primaryOrder(int32_t order);

    





    static inline int32_t secondaryOrder(int32_t order);

    





    static inline int32_t tertiaryOrder(int32_t order);

    








    int32_t getMaxExpansion(int32_t order) const;

    





    int32_t strengthOrder(int32_t order) const;

    





    void setText(const UnicodeString& str, UErrorCode& status);

    





    void setText(CharacterIterator& str, UErrorCode& status);

    





    static inline UBool isIgnorable(int32_t order);

    




    int32_t getOffset(void) const;

    






    void setOffset(int32_t newOffset, UErrorCode& status);

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

protected:
  
    
    


    friend class RuleBasedCollator;

    









    CollationElementIterator(const UnicodeString& sourceText,
        const RuleBasedCollator* order, UErrorCode& status);

    









    CollationElementIterator(const CharacterIterator& sourceText,
        const RuleBasedCollator* order, UErrorCode& status);

    

    





    const CollationElementIterator&
        operator=(const CollationElementIterator& other);

private:
    CollationElementIterator(); 

    

    


    UCollationElements *m_data_;

    


    UBool isDataOwned_;

};








inline int32_t CollationElementIterator::primaryOrder(int32_t order)
{
    order &= RuleBasedCollator::PRIMARYORDERMASK;
    return (order >> RuleBasedCollator::PRIMARYORDERSHIFT);
}






inline int32_t CollationElementIterator::secondaryOrder(int32_t order)
{
    order = order & RuleBasedCollator::SECONDARYORDERMASK;
    return (order >> RuleBasedCollator::SECONDARYORDERSHIFT);
}






inline int32_t CollationElementIterator::tertiaryOrder(int32_t order)
{
    return (order &= RuleBasedCollator::TERTIARYORDERMASK);
}

inline int32_t CollationElementIterator::getMaxExpansion(int32_t order) const
{
    return ucol_getMaxExpansion(m_data_, (uint32_t)order);
}

inline UBool CollationElementIterator::isIgnorable(int32_t order)
{
    return (primaryOrder(order) == RuleBasedCollator::PRIMIGNORABLE);
}

U_NAMESPACE_END

#endif

#endif
