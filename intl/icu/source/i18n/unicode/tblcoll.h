


























































#ifndef TBLCOLL_H
#define TBLCOLL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/locid.h"
#include "unicode/uiter.h"
#include "unicode/ucol.h"

U_NAMESPACE_BEGIN

struct CollationCacheEntry;
struct CollationData;
struct CollationSettings;
struct CollationTailoring;



class StringSearch;



class CollationElementIterator;
class CollationKey;
class SortKeyByteSink;
class UnicodeSet;
class UnicodeString;
class UVector64;




















class U_I18N_API RuleBasedCollator : public Collator {
public:
    







    RuleBasedCollator(const UnicodeString& rules, UErrorCode& status);

    








    RuleBasedCollator(const UnicodeString& rules,
                       ECollationStrength collationStrength,
                       UErrorCode& status);

    








    RuleBasedCollator(const UnicodeString& rules,
                    UColAttributeValue decompositionMode,
                    UErrorCode& status);

    









    RuleBasedCollator(const UnicodeString& rules,
                    ECollationStrength collationStrength,
                    UColAttributeValue decompositionMode,
                    UErrorCode& status);

#ifndef U_HIDE_INTERNAL_API 
    



    RuleBasedCollator(const UnicodeString &rules,
                      UParseError &parseError, UnicodeString &reason,
                      UErrorCode &errorCode);
#endif  

    




    RuleBasedCollator(const RuleBasedCollator& other);


    
















    RuleBasedCollator(const uint8_t *bin, int32_t length, 
                    const RuleBasedCollator *base, 
                    UErrorCode &status);

    



    virtual ~RuleBasedCollator();

    




    RuleBasedCollator& operator=(const RuleBasedCollator& other);

    





    virtual UBool operator==(const Collator& other) const;

    




    virtual Collator* clone(void) const;

    









    virtual CollationElementIterator* createCollationElementIterator(
                                           const UnicodeString& source) const;

    








    virtual CollationElementIterator* createCollationElementIterator(
                                         const CharacterIterator& source) const;

    
    using Collator::compare;

    











    virtual UCollationResult compare(const UnicodeString& source,
                                     const UnicodeString& target,
                                     UErrorCode &status) const;

    












    virtual UCollationResult compare(const UnicodeString& source,
                                     const UnicodeString& target,
                                     int32_t length,
                                     UErrorCode &status) const;

    















    virtual UCollationResult compare(const UChar* source, int32_t sourceLength,
                                     const UChar* target, int32_t targetLength,
                                     UErrorCode &status) const;

    










    virtual UCollationResult compare(UCharIterator &sIter,
                                     UCharIterator &tIter,
                                     UErrorCode &status) const;

    












    virtual UCollationResult compareUTF8(const StringPiece &source,
                                         const StringPiece &target,
                                         UErrorCode &status) const;

    













    virtual CollationKey& getCollationKey(const UnicodeString& source,
                                          CollationKey& key,
                                          UErrorCode& status) const;

    














    virtual CollationKey& getCollationKey(const UChar *source,
                                          int32_t sourceLength,
                                          CollationKey& key,
                                          UErrorCode& status) const;

    




    virtual int32_t hashCode() const;

    









    virtual Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const;

    




    const UnicodeString& getRules() const;

    




    virtual void getVersion(UVersionInfo info) const;

#ifndef U_HIDE_DEPRECATED_API 
    















    int32_t getMaxExpansion(int32_t order) const;
#endif  

    









    virtual UClassID getDynamicClassID(void) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);

#ifndef U_HIDE_DEPRECATED_API 
    









    uint8_t *cloneRuleData(int32_t &length, UErrorCode &status) const;
#endif  

    









    int32_t cloneBinary(uint8_t *buffer, int32_t capacity, UErrorCode &status) const;

    










    void getRules(UColRuleOption delta, UnicodeString &buffer) const;

    






    virtual void setAttribute(UColAttribute attr, UColAttributeValue value,
                              UErrorCode &status);

    






    virtual UColAttributeValue getAttribute(UColAttribute attr,
                                            UErrorCode &status) const;

    















    virtual Collator &setMaxVariable(UColReorderCode group, UErrorCode &errorCode);

    





    virtual UColReorderCode getMaxVariable() const;

    















    virtual uint32_t setVariableTop(const UChar *varTop, int32_t len, UErrorCode &status);

    














    virtual uint32_t setVariableTop(const UnicodeString &varTop, UErrorCode &status);

    










    virtual void setVariableTop(uint32_t varTop, UErrorCode &status);

    






    virtual uint32_t getVariableTop(UErrorCode &status) const;

    








    virtual UnicodeSet *getTailoredSet(UErrorCode &status) const;

    













    virtual int32_t getSortKey(const UnicodeString& source, uint8_t *result,
                               int32_t resultLength) const;

    















    virtual int32_t getSortKey(const UChar *source, int32_t sourceLength,
                               uint8_t *result, int32_t resultLength) const;

    












     virtual int32_t getReorderCodes(int32_t *dest,
                                     int32_t destCapacity,
                                     UErrorCode& status) const;

    










     virtual void setReorderCodes(const int32_t* reorderCodes,
                                  int32_t reorderCodesLength,
                                  UErrorCode& status) ;

    



    virtual UCollationResult internalCompareUTF8(
            const char *left, int32_t leftLength,
            const char *right, int32_t rightLength,
            UErrorCode &errorCode) const;

    






















    virtual int32_t internalGetShortDefinitionString(const char *locale,
                                                     char *buffer,
                                                     int32_t capacity,
                                                     UErrorCode &status) const;

    



    virtual int32_t internalNextSortKeyPart(
            UCharIterator *iter, uint32_t state[2],
            uint8_t *dest, int32_t count, UErrorCode &errorCode) const;

    



    RuleBasedCollator();

#ifndef U_HIDE_INTERNAL_API
    





    const char *internalGetLocaleID(ULocDataLocaleType type, UErrorCode &errorCode) const;

    











    void internalGetContractionsAndExpansions(
            UnicodeSet *contractions, UnicodeSet *expansions,
            UBool addPrefixes, UErrorCode &errorCode) const;

    




    void internalAddContractions(UChar32 c, UnicodeSet &set, UErrorCode &errorCode) const;

    



    void internalBuildTailoring(
            const UnicodeString &rules,
            int32_t strength,
            UColAttributeValue decompositionMode,
            UParseError *outParseError, UnicodeString *outReason,
            UErrorCode &errorCode);

    
    static inline RuleBasedCollator *rbcFromUCollator(UCollator *uc) {
        return dynamic_cast<RuleBasedCollator *>(fromUCollator(uc));
    }
    
    static inline const RuleBasedCollator *rbcFromUCollator(const UCollator *uc) {
        return dynamic_cast<const RuleBasedCollator *>(fromUCollator(uc));
    }

    



    void internalGetCEs(const UnicodeString &str, UVector64 &ces, UErrorCode &errorCode) const;
#endif  

protected:
   






    virtual void setLocales(const Locale& requestedLocale, const Locale& validLocale, const Locale& actualLocale);

private:
    friend class CollationElementIterator;
    friend class Collator;

    RuleBasedCollator(const CollationCacheEntry *entry);

    




    enum Attributes {
        ATTR_VARIABLE_TOP = UCOL_ATTRIBUTE_COUNT,
        ATTR_LIMIT
    };

    void adoptTailoring(CollationTailoring *t, UErrorCode &errorCode);

    
    UCollationResult doCompare(const UChar *left, int32_t leftLength,
                               const UChar *right, int32_t rightLength,
                               UErrorCode &errorCode) const;
    UCollationResult doCompare(const uint8_t *left, int32_t leftLength,
                               const uint8_t *right, int32_t rightLength,
                               UErrorCode &errorCode) const;

    void writeSortKey(const UChar *s, int32_t length,
                      SortKeyByteSink &sink, UErrorCode &errorCode) const;

    void writeIdenticalLevel(const UChar *s, const UChar *limit,
                             SortKeyByteSink &sink, UErrorCode &errorCode) const;

    const CollationSettings &getDefaultSettings() const;

    void setAttributeDefault(int32_t attribute) {
        explicitlySetAttributes &= ~((uint32_t)1 << attribute);
    }
    void setAttributeExplicitly(int32_t attribute) {
        explicitlySetAttributes |= (uint32_t)1 << attribute;
    }
    UBool attributeHasBeenSetExplicitly(int32_t attribute) const {
        
        return (UBool)((explicitlySetAttributes & ((uint32_t)1 << attribute)) != 0);
    }

    






    UBool isUnsafe(UChar32 c) const;

    static void computeMaxExpansions(const CollationTailoring *t, UErrorCode &errorCode);
    UBool initMaxExpansions(UErrorCode &errorCode) const;

    void setFastLatinOptions(CollationSettings &ownedSettings) const;

    const CollationData *data;
    const CollationSettings *settings;  
    const CollationTailoring *tailoring;  
    const CollationCacheEntry *cacheEntry;  
    Locale validLocale;
    uint32_t explicitlySetAttributes;

    UBool actualLocaleIsSameAsValid;
};

U_NAMESPACE_END

#endif  
#endif  
