






#ifndef LOCDSPNM_H
#define LOCDSPNM_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/locid.h"
#include "unicode/uscript.h"
#include "unicode/uldnames.h"
#include "unicode/udisplaycontext.h"

U_NAMESPACE_BEGIN







class U_I18N_API LocaleDisplayNames : public UObject {
public:
    



    virtual ~LocaleDisplayNames();

    







    static LocaleDisplayNames* U_EXPORT2 createInstance(const Locale& locale);

    









    static LocaleDisplayNames* U_EXPORT2 createInstance(const Locale& locale,
                            UDialectHandling dialectHandling);

#ifndef U_HIDE_INTERNAL_API
    










    static LocaleDisplayNames* U_EXPORT2 createInstance(const Locale& locale,
                            UDisplayContext *contexts, int32_t length);
#endif  

    
    





    virtual const Locale& getLocale() const = 0;

    




    virtual UDialectHandling getDialectHandling() const = 0;

    





    virtual UDisplayContext getContext(UDisplayContextType type) const = 0;

    
    






    virtual UnicodeString& localeDisplayName(const Locale& locale,
                         UnicodeString& result) const = 0;

    






    virtual UnicodeString& localeDisplayName(const char* localeId,
                         UnicodeString& result) const = 0;

    
    






    virtual UnicodeString& languageDisplayName(const char* lang,
                           UnicodeString& result) const = 0;

    






    virtual UnicodeString& scriptDisplayName(const char* script,
                         UnicodeString& result) const = 0;

    






    virtual UnicodeString& scriptDisplayName(UScriptCode scriptCode,
                         UnicodeString& result) const = 0;

    






    virtual UnicodeString& regionDisplayName(const char* region,
                         UnicodeString& result) const = 0;

    






    virtual UnicodeString& variantDisplayName(const char* variant,
                          UnicodeString& result) const = 0;

    






    virtual UnicodeString& keyDisplayName(const char* key,
                      UnicodeString& result) const = 0;

    







    virtual UnicodeString& keyValueDisplayName(const char* key, const char* value,
                           UnicodeString& result) const = 0;

private:
    
    virtual UClassID getDynamicClassID() const;
};

inline LocaleDisplayNames* LocaleDisplayNames::createInstance(const Locale& locale) {
  return LocaleDisplayNames::createInstance(locale, ULDN_STANDARD_NAMES);
}

U_NAMESPACE_END

#endif

#endif
