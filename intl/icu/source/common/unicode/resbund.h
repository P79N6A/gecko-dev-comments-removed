












































#ifndef RESBUND_H
#define RESBUND_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/ures.h"
#include "unicode/unistr.h"
#include "unicode/locid.h"





 
U_NAMESPACE_BEGIN

















class U_COMMON_API ResourceBundle : public UObject {
public:
    

























    ResourceBundle(const UnicodeString&    packageName,
                   const Locale&           locale,
                   UErrorCode&              err);

    










    ResourceBundle(const UnicodeString&    packageName,
                   UErrorCode&              err);

    





    ResourceBundle(UErrorCode &err);

    













    ResourceBundle(const char* packageName,
                   const Locale& locale,
                   UErrorCode& err);

    





    ResourceBundle(const ResourceBundle &original);

    








    ResourceBundle(UResourceBundle *res,
                   UErrorCode &status);

    





    ResourceBundle&
      operator=(const ResourceBundle& other);

    


    virtual ~ResourceBundle();

    










    ResourceBundle *clone() const;

    









    int32_t
      getSize(void) const;

    









    UnicodeString
      getString(UErrorCode& status) const;

    











    const uint8_t*
      getBinary(int32_t& len, UErrorCode& status) const;


    










    const int32_t*
      getIntVector(int32_t& len, UErrorCode& status) const;

    










    uint32_t
      getUInt(UErrorCode& status) const;

    










    int32_t
      getInt(UErrorCode& status) const;

    





    UBool
      hasNext(void) const;

    




    void
      resetIterator(void);

    






    const char*
      getKey(void) const;

    






    const char*
      getName(void) const;


    





    UResType
      getType(void) const;

    






    ResourceBundle
      getNext(UErrorCode& status);

    







    UnicodeString
      getNextString(UErrorCode& status);

    








    UnicodeString
      getNextString(const char ** key,
                    UErrorCode& status);

    







    ResourceBundle
      get(int32_t index,
          UErrorCode& status) const;

    







    UnicodeString
      getStringEx(int32_t index,
                  UErrorCode& status) const;

    








    ResourceBundle
      get(const char* key,
          UErrorCode& status) const;

    








    UnicodeString
      getStringEx(const char* key,
                  UErrorCode& status) const;

#ifndef U_HIDE_DEPRECATED_API
    








    const char*
      getVersionNumber(void) const;
#endif  

    






    void
      getVersion(UVersionInfo versionInfo) const;

#ifndef U_HIDE_DEPRECATED_API
    





    const Locale&
      getLocale(void) const;
#endif  

    









    const Locale
      getLocale(ULocDataLocaleType type, UErrorCode &status) const;
#ifndef U_HIDE_INTERNAL_API
    



    ResourceBundle
        getWithFallback(const char* key, UErrorCode& status);
#endif  
    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

private:
    ResourceBundle(); 

    UResourceBundle *fResource;
    void constructForLocale(const UnicodeString& path, const Locale& locale, UErrorCode& error);
    Locale *fLocale;

};

U_NAMESPACE_END
#endif
