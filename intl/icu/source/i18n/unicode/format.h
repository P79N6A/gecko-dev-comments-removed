



















#ifndef FORMAT_H
#define FORMAT_H


#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/fmtable.h"
#include "unicode/fieldpos.h"
#include "unicode/fpositer.h"
#include "unicode/parsepos.h"
#include "unicode/parseerr.h" 
#include "unicode/locid.h"

U_NAMESPACE_BEGIN



















































class U_I18N_API Format : public UObject {
public:

    


    virtual ~Format();

    







    virtual UBool operator==(const Format& other) const = 0;

    






    UBool operator!=(const Format& other) const { return !operator==(other); }

    





    virtual Format* clone() const = 0;

    









    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

    















    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const = 0;
    















    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;

    






































    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& parse_pos) const = 0;

    











    void parseObject(const UnicodeString& source,
                     Formattable& result,
                     UErrorCode& status) const;

    





    Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const;

#ifndef U_HIDE_INTERNAL_API
    





    const char* getLocaleID(ULocDataLocaleType type, UErrorCode &status) const;
#endif  

 protected:
    
    void setLocaleIDs(const char* valid, const char* actual);

protected:
    



    Format();

    


    Format(const Format&); 

    


    Format& operator=(const Format&); 

       
    







    static void syntaxError(const UnicodeString& pattern,
                            int32_t pos,
                            UParseError& parseError);

 private:
    char actualLocale[ULOC_FULLNAME_CAPACITY];
    char validLocale[ULOC_FULLNAME_CAPACITY];
};

U_NAMESPACE_END

#endif 

#endif 

