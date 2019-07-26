










#ifndef __WINNMFMT
#define __WINNMFMT

#include "unicode/utypes.h"

#if U_PLATFORM_USES_ONLY_WIN32_API

#include "unicode/format.h"
#include "unicode/datefmt.h"
#include "unicode/calendar.h"
#include "unicode/ustring.h"
#include "unicode/locid.h"

#if !UCONFIG_NO_FORMATTING






U_NAMESPACE_BEGIN

union FormatInfo;

class Win32NumberFormat : public NumberFormat
{
public:
    Win32NumberFormat(const Locale &locale, UBool currency, UErrorCode &status);

    Win32NumberFormat(const Win32NumberFormat &other);

    virtual ~Win32NumberFormat();

    virtual Format *clone(void) const;

    Win32NumberFormat &operator=(const Win32NumberFormat &other);

    










    virtual UnicodeString& format(double number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;
    










    virtual UnicodeString& format(int32_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;

    









    virtual UnicodeString& format(int64_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;






    virtual void parse(const UnicodeString& text, Formattable& result, ParsePosition& parsePosition) const;

    








    virtual void setMaximumFractionDigits(int32_t newValue);

    








    virtual void setMinimumFractionDigits(int32_t newValue);

    









    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

    









    virtual UClassID getDynamicClassID(void) const;

private:
    UnicodeString &format(int32_t numDigits, UnicodeString &appendTo, wchar_t *format, ...) const;

    UBool fCurrency;
    int32_t fLCID;
    FormatInfo *fFormatInfo;
    UBool fFractionDigitsSet;

};

U_NAMESPACE_END

#endif 

#endif 

#endif 
