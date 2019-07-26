









#ifndef LOCBASED_H
#define LOCBASED_H

#include "unicode/locid.h"
#include "unicode/uobject.h"






#define U_LOCALE_BASED(varname, objname) \
  LocaleBased varname((objname).validLocale, (objname).actualLocale);

U_NAMESPACE_BEGIN








class U_COMMON_API LocaleBased : public UMemory {

 public:

    



    inline LocaleBased(char* validAlias, char* actualAlias);

    



    inline LocaleBased(const char* validAlias, const char* actualAlias);

    







    Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const;

    







    const char* getLocaleID(ULocDataLocaleType type, UErrorCode& status) const;

    





    void setLocaleIDs(const char* valid, const char* actual);

 private:

    char* valid;
    
    char* actual;
};

inline LocaleBased::LocaleBased(char* validAlias, char* actualAlias) :
    valid(validAlias), actual(actualAlias) {
}

inline LocaleBased::LocaleBased(const char* validAlias,
                                const char* actualAlias) :
    
    valid((char*)validAlias), actual((char*)actualAlias) {
}

U_NAMESPACE_END

#endif
