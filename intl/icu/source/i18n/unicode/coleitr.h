





























#ifndef COLEITR_H
#define COLEITR_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/unistr.h"
#include "unicode/uobject.h"

struct UCollationElements;
struct UHashtable;

U_NAMESPACE_BEGIN

struct CollationData;

class CollationIterator;
class RuleBasedCollator;
class UCollationPCE;
class UVector32;
































































class U_I18N_API CollationElementIterator U_FINAL : public UObject {
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

#ifndef U_HIDE_INTERNAL_API
    
    static inline CollationElementIterator *fromUCollationElements(UCollationElements *uc) {
        return reinterpret_cast<CollationElementIterator *>(uc);
    }
    
    static inline const CollationElementIterator *fromUCollationElements(const UCollationElements *uc) {
        return reinterpret_cast<const CollationElementIterator *>(uc);
    }
    
    inline UCollationElements *toUCollationElements() {
        return reinterpret_cast<UCollationElements *>(this);
    }
    
    inline const UCollationElements *toUCollationElements() const {
        return reinterpret_cast<const UCollationElements *>(this);
    }
#endif  

private:
    friend class RuleBasedCollator;
    friend class UCollationPCE;

    








    CollationElementIterator(const UnicodeString& sourceText,
        const RuleBasedCollator* order, UErrorCode& status);
    
    
    
    
    
    
    
    

    








    CollationElementIterator(const CharacterIterator& sourceText,
        const RuleBasedCollator* order, UErrorCode& status);

    




    const CollationElementIterator&
        operator=(const CollationElementIterator& other);

    CollationElementIterator(); 

    
    inline int8_t normalizeDir() const { return dir_ == 1 ? 0 : dir_; }

    static UHashtable *computeMaxExpansions(const CollationData *data, UErrorCode &errorCode);

    static int32_t getMaxExpansion(const UHashtable *maxExpansions, int32_t order);

    

    CollationIterator *iter_;  
    const RuleBasedCollator *rbc_;  
    uint32_t otherHalf_;
    



    int8_t dir_;
    




    UVector32 *offsets_;

    UnicodeString string_;
};



inline int32_t CollationElementIterator::primaryOrder(int32_t order)
{
    return (order >> 16) & 0xffff;
}

inline int32_t CollationElementIterator::secondaryOrder(int32_t order)
{
    return (order >> 8) & 0xff;
}

inline int32_t CollationElementIterator::tertiaryOrder(int32_t order)
{
    return order & 0xff;
}

inline UBool CollationElementIterator::isIgnorable(int32_t order)
{
    return (order & 0xffff0000) == 0;
}

U_NAMESPACE_END

#endif

#endif
