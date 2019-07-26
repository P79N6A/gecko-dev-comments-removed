

























































#ifndef TBLCOLL_H
#define TBLCOLL_H

#include "unicode/utypes.h"

 
#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/ucol.h"
#include "unicode/sortkey.h"
#include "unicode/normlzr.h"

U_NAMESPACE_BEGIN




class StringSearch;



class CollationElementIterator;





























class U_I18N_API RuleBasedCollator : public Collator
{
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

    











    virtual CollationKey& getCollationKey(const UnicodeString& source,
                                          CollationKey& key,
                                          UErrorCode& status) const;

    












    virtual CollationKey& getCollationKey(const UChar *source,
                                          int32_t sourceLength,
                                          CollationKey& key,
                                          UErrorCode& status) const;

    




    virtual int32_t hashCode(void) const;

    









    virtual Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const;

    




    const UnicodeString& getRules(void) const;

    




    virtual void getVersion(UVersionInfo info) const;

    









    int32_t getMaxExpansion(int32_t order) const;

    









    virtual UClassID getDynamicClassID(void) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    







    uint8_t *cloneRuleData(int32_t &length, UErrorCode &status);


    









    int32_t cloneBinary(uint8_t *buffer, int32_t capacity, UErrorCode &status);

    










    void getRules(UColRuleOption delta, UnicodeString &buffer);

    






    virtual void setAttribute(UColAttribute attr, UColAttributeValue value,
                              UErrorCode &status);

    






    virtual UColAttributeValue getAttribute(UColAttribute attr,
                                            UErrorCode &status) const;

    









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

    















    static int32_t U_EXPORT2 getEquivalentReorderCodes(int32_t reorderCode,
                                int32_t* dest,
                                int32_t destCapacity,
                                UErrorCode& status);

private:

    

    enum {
        
        CHARINDEX = 0x70000000,
        
        EXPANDCHARINDEX = 0x7E000000,
        
        CONTRACTCHARINDEX = 0x7F000000,
        
        UNMAPPED = 0xFFFFFFFF,
        
        PRIMARYORDERINCREMENT = 0x00010000,
        
        SECONDARYORDERINCREMENT = 0x00000100,
        
        TERTIARYORDERINCREMENT = 0x00000001,
        
        PRIMARYORDERMASK = 0xffff0000,
        
        SECONDARYORDERMASK = 0x0000ff00,
        
        TERTIARYORDERMASK = 0x000000ff,
        
        IGNORABLEMASK = 0x0000ffff,
        
        PRIMARYDIFFERENCEONLY = 0xffff0000,
        
        SECONDARYDIFFERENCEONLY = 0xffffff00,
        
        PRIMARYORDERSHIFT = 16,
        
        SECONDARYORDERSHIFT = 8,
        
        COLELEMENTSTART = 0x02020202,
        
        PRIMARYLOWZEROMASK = 0x00FF0000,
        
        RESETSECONDARYTERTIARY = 0x00000202,
        
        RESETTERTIARY = 0x00000002,

        PRIMIGNORABLE = 0x0202
    };

    

    UBool dataIsOwned;

    UBool isWriteThroughAlias;

    



    UCollator *ucollator;

    


    UnicodeString urulestring;

    

    


    friend class CollationElementIterator;

    



    friend class Collator;

    


    friend class StringSearch;

    

    


    RuleBasedCollator();

    









    RuleBasedCollator(const Locale& desiredLocale, UErrorCode& status);

    







    void
    construct(const UnicodeString& rules,
              UColAttributeValue collationStrength,
              UColAttributeValue decompositionMode,
              UErrorCode& status);

    

    




    void setUCollator(const Locale& locale, UErrorCode& status);

    




    void setUCollator(const char* locale, UErrorCode& status);

    





    void setUCollator(UCollator *collator);

public:
#ifndef U_HIDE_INTERNAL_API
    




    const UCollator * getUCollator();
#endif  

protected:
   






    virtual void setLocales(const Locale& requestedLocale, const Locale& validLocale, const Locale& actualLocale);

private:
    
    void checkOwned(void);

    
    void setRuleStringFromCollator();

public:
    






















    virtual int32_t internalGetShortDefinitionString(const char *locale,
                                                     char *buffer,
                                                     int32_t capacity,
                                                     UErrorCode &status) const;
};



inline void RuleBasedCollator::setUCollator(const Locale &locale,
                                               UErrorCode &status)
{
    setUCollator(locale.getName(), status);
}


inline void RuleBasedCollator::setUCollator(UCollator     *collator)
{

    if (ucollator && dataIsOwned) {
        ucol_close(ucollator);
    }
    ucollator   = collator;
    dataIsOwned = FALSE;
    isWriteThroughAlias = TRUE;
    setRuleStringFromCollator();
}

#ifndef U_HIDE_INTERNAL_API
inline const UCollator * RuleBasedCollator::getUCollator()
{
    return ucollator;
}
#endif

U_NAMESPACE_END

#endif

#endif
