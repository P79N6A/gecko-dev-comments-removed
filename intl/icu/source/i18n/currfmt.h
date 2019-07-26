









#ifndef CURRENCYFORMAT_H
#define CURRENCYFORMAT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/measfmt.h"

U_NAMESPACE_BEGIN

class NumberFormat;













class CurrencyFormat : public MeasureFormat {

 public:

    


    CurrencyFormat(const Locale& locale, UErrorCode& ec);

    


    CurrencyFormat(const CurrencyFormat& other);

    


    virtual ~CurrencyFormat();

    


    virtual UBool operator==(const Format& other) const;

    


    virtual Format* clone() const;


    using MeasureFormat::format;

    


    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& ec) const;

    


    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& pos) const;

    


    virtual UClassID getDynamicClassID() const;

    


    static UClassID U_EXPORT2 getStaticClassID();

 private:

    NumberFormat* fmt;
};

U_NAMESPACE_END

#endif 
#endif 
