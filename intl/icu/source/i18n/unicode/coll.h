










 




































#ifndef COLL_H
#define COLL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "unicode/ucol.h"
#include "unicode/normlzr.h"
#include "unicode/locid.h"
#include "unicode/uniset.h"
#include "unicode/umisc.h"
#include "unicode/uiter.h"
#include "unicode/stringpiece.h"

U_NAMESPACE_BEGIN

class StringEnumeration;

#if !UCONFIG_NO_SERVICE



class CollatorFactory;
#endif




class CollationKey;

































































































class U_I18N_API Collator : public UObject {
public:

    

    
























    enum ECollationStrength
    {
        PRIMARY    = UCOL_PRIMARY,  
        SECONDARY  = UCOL_SECONDARY,  
        TERTIARY   = UCOL_TERTIARY,  
        QUATERNARY = UCOL_QUATERNARY,  
        IDENTICAL  = UCOL_IDENTICAL  
    };

    









    enum EComparisonResult
    {
        LESS = UCOL_LESS,  
        EQUAL = UCOL_EQUAL,  
        GREATER = UCOL_GREATER  
    };

    

    



    virtual ~Collator();

    

    

















    virtual UBool operator==(const Collator& other) const;

    






    virtual UBool operator!=(const Collator& other) const;

    




    virtual Collator* clone(void) const = 0;

    


















    static Collator* U_EXPORT2 createInstance(UErrorCode&  err);

    






















    static Collator* U_EXPORT2 createInstance(const Locale& loc, UErrorCode& err);

#ifdef U_USE_COLLATION_OBSOLETE_2_6
    






















    static Collator *createInstance(const Locale &loc, UVersionInfo version, UErrorCode &err);
#endif

    










    virtual EComparisonResult compare(const UnicodeString& source,
                                      const UnicodeString& target) const;

    











    virtual UCollationResult compare(const UnicodeString& source,
                                      const UnicodeString& target,
                                      UErrorCode &status) const = 0;

    











    virtual EComparisonResult compare(const UnicodeString& source,
                                      const UnicodeString& target,
                                      int32_t length) const;

    












    virtual UCollationResult compare(const UnicodeString& source,
                                      const UnicodeString& target,
                                      int32_t length,
                                      UErrorCode &status) const = 0;

    
































    virtual EComparisonResult compare(const UChar* source, int32_t sourceLength,
                                      const UChar* target, int32_t targetLength)
                                      const;

    















    virtual UCollationResult compare(const UChar* source, int32_t sourceLength,
                                      const UChar* target, int32_t targetLength,
                                      UErrorCode &status) const = 0;

    










    virtual UCollationResult compare(UCharIterator &sIter,
                                     UCharIterator &tIter,
                                     UErrorCode &status) const;

    












    virtual UCollationResult compareUTF8(const StringPiece &source,
                                         const StringPiece &target,
                                         UErrorCode &status) const;

    














    virtual CollationKey& getCollationKey(const UnicodeString&  source,
                                          CollationKey& key,
                                          UErrorCode& status) const = 0;

    















    virtual CollationKey& getCollationKey(const UChar*source,
                                          int32_t sourceLength,
                                          CollationKey& key,
                                          UErrorCode& status) const = 0;
    



    virtual int32_t hashCode(void) const = 0;

    











    virtual Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const = 0;

    








    UBool greater(const UnicodeString& source, const UnicodeString& target)
                  const;

    








    UBool greaterOrEqual(const UnicodeString& source,
                         const UnicodeString& target) const;

    








    UBool equals(const UnicodeString& source, const UnicodeString& target) const;

    









    virtual ECollationStrength getStrength(void) const;

    

















    virtual void setStrength(ECollationStrength newStrength);

    














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

    








    static UnicodeString& U_EXPORT2 getDisplayName(const Locale& objectLocale,
                                         const Locale& displayLocale,
                                         UnicodeString& name);

    







    static UnicodeString& U_EXPORT2 getDisplayName(const Locale& objectLocale,
                                         UnicodeString& name);

    










    static const Locale* U_EXPORT2 getAvailableLocales(int32_t& count);

    







    static StringEnumeration* U_EXPORT2 getAvailableLocales(void);

    








    static StringEnumeration* U_EXPORT2 getKeywords(UErrorCode& status);

    










    static StringEnumeration* U_EXPORT2 getKeywordValues(const char *keyword, UErrorCode& status);

    















    static StringEnumeration* U_EXPORT2 getKeywordValuesForLocale(const char* keyword, const Locale& locale,
                                                                    UBool commonlyUsed, UErrorCode& status);

    


























    static Locale U_EXPORT2 getFunctionalEquivalent(const char* keyword, const Locale& locale,
                                          UBool& isAvailable, UErrorCode& status);

#if !UCONFIG_NO_SERVICE
    







    static URegistryKey U_EXPORT2 registerInstance(Collator* toAdopt, const Locale& locale, UErrorCode& status);

    






    static URegistryKey U_EXPORT2 registerFactory(CollatorFactory* toAdopt, UErrorCode& status);

    









    static UBool U_EXPORT2 unregister(URegistryKey key, UErrorCode& status);
#endif 

    




    virtual void getVersion(UVersionInfo info) const = 0;

    









    virtual UClassID getDynamicClassID(void) const = 0;

    







    virtual void setAttribute(UColAttribute attr, UColAttributeValue value,
                              UErrorCode &status) = 0;

    







    virtual UColAttributeValue getAttribute(UColAttribute attr,
                                            UErrorCode &status) const = 0;

    









    virtual uint32_t setVariableTop(const UChar *varTop, int32_t len, UErrorCode &status) = 0;

    








    virtual uint32_t setVariableTop(const UnicodeString &varTop, UErrorCode &status) = 0;

    






    virtual void setVariableTop(uint32_t varTop, UErrorCode &status) = 0;

    





    virtual uint32_t getVariableTop(UErrorCode &status) const = 0;

    








    virtual UnicodeSet *getTailoredSet(UErrorCode &status) const;

    






    virtual Collator* safeClone(void) const;

    











    virtual int32_t getSortKey(const UnicodeString& source,
                              uint8_t* result,
                              int32_t resultLength) const = 0;

    














    virtual int32_t getSortKey(const UChar*source, int32_t sourceLength,
                               uint8_t*result, int32_t resultLength) const = 0;

    




































    static int32_t U_EXPORT2 getBound(const uint8_t       *source,
            int32_t             sourceLength,
            UColBoundMode       boundType,
            uint32_t            noOfLevels,
            uint8_t             *result,
            int32_t             resultLength,
            UErrorCode          &status);


protected:

    

    






    Collator();

#ifndef U_HIDE_DEPRECATED_API
    










    Collator(UCollationStrength collationStrength,
             UNormalizationMode decompositionMode);
#endif  

    




    Collator(const Collator& other);

    


   






    virtual void setLocales(const Locale& requestedLocale, const Locale& validLocale, const Locale& actualLocale);

public:
#if !UCONFIG_NO_SERVICE
#ifndef U_HIDE_INTERNAL_API
    



    static UCollator* createUCollator(const char* loc, UErrorCode* status);
#endif  
#endif

    






















    virtual int32_t internalGetShortDefinitionString(const char *locale,
                                                     char *buffer,
                                                     int32_t capacity,
                                                     UErrorCode &status) const;
private:
    



    Collator& operator=(const Collator& other);

    friend class CFactory;
    friend class SimpleCFactory;
    friend class ICUCollatorFactory;
    friend class ICUCollatorService;
    static Collator* makeInstance(const Locale& desiredLocale,
                                  UErrorCode& status);

    

    




    

};

#if !UCONFIG_NO_SERVICE
















class U_I18N_API CollatorFactory : public UObject {
public:

    



    virtual ~CollatorFactory();

    






    virtual UBool visible(void) const;

    






    virtual Collator* createCollator(const Locale& loc) = 0;

    









    virtual  UnicodeString& getDisplayName(const Locale& objectLocale,
                                           const Locale& displayLocale,
                                           UnicodeString& result);

    








    virtual const UnicodeString * getSupportedIDs(int32_t &count, UErrorCode& status) = 0;
};
#endif 



U_NAMESPACE_END

#endif

#endif
