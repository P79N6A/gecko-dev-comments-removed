




















#include "utypeinfo.h"  

#include "unicode/utypes.h"

#ifndef U_I18N_IMPLEMENTATION
#error U_I18N_IMPLEMENTATION not set - must be set for all ICU source files in i18n/ - see http://userguide.icu-project.org/howtouseicu
#endif







#if UCONFIG_NO_COLLATION && UCONFIG_NO_FORMATTING && UCONFIG_NO_TRANSLITERATION
U_CAPI int32_t U_EXPORT2
uprv_icuin_lib_dummy(int32_t i) {
    return -i;
}
#endif



#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/ures.h"
#include "cstring.h"
#include "locbased.h"





U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(FieldPosition)

FieldPosition::~FieldPosition() {}

FieldPosition *
FieldPosition::clone() const {
    return new FieldPosition(*this);
}




Format::Format()
    : UObject()
{
    *validLocale = *actualLocale = 0;
}



Format::~Format()
{
}




Format::Format(const Format &that)
    : UObject(that)
{
    *this = that;
}




Format&
Format::operator=(const Format& that)
{
    if (this != &that) {
        uprv_strcpy(validLocale, that.validLocale);
        uprv_strcpy(actualLocale, that.actualLocale);
    }
    return *this;
}





UnicodeString&
Format::format(const Formattable& obj,
               UnicodeString& toAppendTo,
               UErrorCode& status) const
{
    if (U_FAILURE(status)) return toAppendTo;

    FieldPosition pos(FieldPosition::DONT_CARE);

    return format(obj, toAppendTo, pos, status);
}





UnicodeString&
Format::format(const Formattable& ,
               UnicodeString& toAppendTo,
               FieldPositionIterator* ,
               UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
      status = U_UNSUPPORTED_ERROR;
    }
    return toAppendTo;
}





void
Format::parseObject(const UnicodeString& source,
                    Formattable& result,
                    UErrorCode& status) const
{
    if (U_FAILURE(status)) return;

    ParsePosition parsePosition(0);
    parseObject(source, result, parsePosition);
    if (parsePosition.getIndex() == 0) {
        status = U_INVALID_FORMAT_ERROR;
    }
}



UBool
Format::operator==(const Format& that) const
{
    
    return typeid(*this) == typeid(that);
}










void Format::syntaxError(const UnicodeString& pattern,
                         int32_t pos,
                         UParseError& parseError) {
    parseError.offset = pos;
    parseError.line=0;  

    
    int32_t start = (pos < U_PARSE_CONTEXT_LEN)? 0 : (pos - (U_PARSE_CONTEXT_LEN-1
                                                             ));
    int32_t stop  = pos;
    pattern.extract(start,stop-start,parseError.preContext,0);
    
    parseError.preContext[stop-start] = 0;

    
    start = pos+1;
    stop  = ((pos+U_PARSE_CONTEXT_LEN)<=pattern.length()) ? (pos+(U_PARSE_CONTEXT_LEN-1)) :
        pattern.length();
    pattern.extract(start,stop-start,parseError.postContext,0);
    
    parseError.postContext[stop-start]= 0;
}

Locale
Format::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocale(type, status);
}

const char *
Format::getLocaleID(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocaleID(type, status);
}

void
Format::setLocaleIDs(const char* valid, const char* actual) {
    U_LOCALE_BASED(locBased, *this);
    locBased.setLocaleIDs(valid, actual);
}

U_NAMESPACE_END

#endif 


