



























#ifndef LOCID_H
#define LOCID_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/putil.h"
#include "unicode/uloc.h"
#include "unicode/strenum.h"






U_NAMESPACE_BEGIN









































































































































class U_COMMON_API Locale : public UObject {
public:
    
    static const Locale &U_EXPORT2 getRoot(void);
    
    static const Locale &U_EXPORT2 getEnglish(void);
    
    static const Locale &U_EXPORT2 getFrench(void);
    
    static const Locale &U_EXPORT2 getGerman(void);
    
    static const Locale &U_EXPORT2 getItalian(void);
    
    static const Locale &U_EXPORT2 getJapanese(void);
    
    static const Locale &U_EXPORT2 getKorean(void);
    
    static const Locale &U_EXPORT2 getChinese(void);
    
    static const Locale &U_EXPORT2 getSimplifiedChinese(void);
    
    static const Locale &U_EXPORT2 getTraditionalChinese(void);

    
    static const Locale &U_EXPORT2 getFrance(void);
    
    static const Locale &U_EXPORT2 getGermany(void);
    
    static const Locale &U_EXPORT2 getItaly(void);
    
    static const Locale &U_EXPORT2 getJapan(void);
    
    static const Locale &U_EXPORT2 getKorea(void);
    
    static const Locale &U_EXPORT2 getChina(void);
    
    static const Locale &U_EXPORT2 getPRC(void);
    
    static const Locale &U_EXPORT2 getTaiwan(void);
    
    static const Locale &U_EXPORT2 getUK(void);
    
    static const Locale &U_EXPORT2 getUS(void);
    
    static const Locale &U_EXPORT2 getCanada(void);
    
    static const Locale &U_EXPORT2 getCanadaFrench(void);


    






    Locale();

    























    Locale( const   char * language,
            const   char * country  = 0,
            const   char * variant  = 0,
            const   char * keywordsAndValues = 0);

    





    Locale(const    Locale& other);


    



    virtual ~Locale() ;

    






    Locale& operator=(const Locale& other);

    






    UBool   operator==(const    Locale&     other) const;

    







    UBool   operator!=(const    Locale&     other) const;

    










    Locale *clone() const;

#ifndef U_HIDE_SYSTEM_API
    














    static const Locale& U_EXPORT2 getDefault(void);

    











    static void U_EXPORT2 setDefault(const Locale& newLocale,
                                     UErrorCode&   success);
#endif  

    








    static Locale U_EXPORT2 createFromName(const char *name);

    







    static Locale U_EXPORT2 createCanonical(const char* name);

    




    inline const char *  getLanguage( ) const;

    






    inline const char *  getScript( ) const;

    




    inline const char *  getCountry( ) const;

    




    inline const char *  getVariant( ) const;

    







    inline const char * getName() const;

    






    const char * getBaseName() const;


    







    StringEnumeration * createKeywords(UErrorCode &status) const;

    










    int32_t getKeywordValue(const char* keywordName, char *buffer, int32_t bufferCapacity, UErrorCode &status) const;

#ifndef U_HIDE_DRAFT_API
    










    void setKeywordValue(const char* keywordName, const char* keywordValue, UErrorCode &status);
#endif  

    





    const char * getISO3Language() const;

    




    const char * getISO3Country() const;

    






    uint32_t        getLCID(void) const;

    








    UnicodeString&  getDisplayLanguage(UnicodeString&   dispLang) const;

    












    UnicodeString&  getDisplayLanguage( const   Locale&         displayLocale,
                                                UnicodeString&  dispLang) const;

    








    UnicodeString&  getDisplayScript(          UnicodeString& dispScript) const;

    













    UnicodeString&  getDisplayScript(  const   Locale&         displayLocale,
                                               UnicodeString&  dispScript) const;

    








    UnicodeString&  getDisplayCountry(          UnicodeString& dispCountry) const;

    













    UnicodeString&  getDisplayCountry(  const   Locale&         displayLocale,
                                                UnicodeString&  dispCountry) const;

    






    UnicodeString&  getDisplayVariant(      UnicodeString& dispVar) const;

    







    UnicodeString&  getDisplayVariant(  const   Locale&         displayLocale,
                                                UnicodeString&  dispVar) const;

    










    UnicodeString&  getDisplayName(         UnicodeString&  name) const;

    











    UnicodeString&  getDisplayName( const   Locale&         displayLocale,
                                            UnicodeString&  name) const;

    



    int32_t         hashCode(void) const;

    







    void setToBogus();

    




    UBool isBogus(void) const;

    







    static const Locale* U_EXPORT2 getAvailableLocales(int32_t& count);

    







    static const char* const* U_EXPORT2 getISOCountries();

    







    static const char* const* U_EXPORT2 getISOLanguages();

    




    static UClassID U_EXPORT2 getStaticClassID();

    




    virtual UClassID getDynamicClassID() const;

protected: 
#ifndef U_HIDE_INTERNAL_API
    



    void setFromPOSIXID(const char *posixID);
#endif  

private:
    






    Locale& init(const char* cLocaleID, UBool canonicalize);

    




    enum ELocaleType {
        eBOGUS
    };
    Locale(ELocaleType);

    


    static Locale *getLocaleCache(void);

    char language[ULOC_LANG_CAPACITY];
    char script[ULOC_SCRIPT_CAPACITY];
    char country[ULOC_COUNTRY_CAPACITY];
    int32_t variantBegin;
    char* fullName;
    char fullNameBuffer[ULOC_FULLNAME_CAPACITY];
    
    char* baseName;
    char baseNameBuffer[ULOC_FULLNAME_CAPACITY];

    UBool fIsBogus;

    static const Locale &getLocale(int locid);

    



    friend Locale *locale_set_default_internal(const char *, UErrorCode& status);
};

inline UBool
Locale::operator!=(const    Locale&     other) const
{
    return !operator==(other);
}

inline const char *
Locale::getCountry() const
{
    return country;
}

inline const char *
Locale::getLanguage() const
{
    return language;
}

inline const char *
Locale::getScript() const
{
    return script;
}

inline const char *
Locale::getVariant() const
{
    getBaseName(); 
    return &baseName[variantBegin];
}

inline const char *
Locale::getName() const
{
    return fullName;
}

inline UBool
Locale::isBogus(void) const {
    return fIsBogus;
}

U_NAMESPACE_END

#endif
