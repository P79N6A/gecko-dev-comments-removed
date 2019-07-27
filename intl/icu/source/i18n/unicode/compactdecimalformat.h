









#ifndef __COMPACT_DECIMAL_FORMAT_H__
#define __COMPACT_DECIMAL_FORMAT_H__

#include "unicode/utypes.h"





#if !UCONFIG_NO_FORMATTING

#include "unicode/decimfmt.h"

struct UHashtable;

U_NAMESPACE_BEGIN

class PluralRules;























class U_I18N_API CompactDecimalFormat : public DecimalFormat {
public:

     






     static CompactDecimalFormat* U_EXPORT2 createInstance(
          const Locale& inLocale, UNumberCompactStyle style, UErrorCode& status);

    





    CompactDecimalFormat(const CompactDecimalFormat& source);

    



    virtual ~CompactDecimalFormat();

    





    CompactDecimalFormat& operator=(const CompactDecimalFormat& rhs);

    






    virtual Format* clone() const;

    







    virtual UBool operator==(const Format& other) const;


    using DecimalFormat::format;

    










    virtual UnicodeString& format(double number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;

    













    virtual UnicodeString& format(double number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;

    










    virtual UnicodeString& format(int64_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;

    













    virtual UnicodeString& format(int64_t number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;

    















    virtual UnicodeString& format(const StringPiece &number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;

    














    virtual UnicodeString& format(const DigitList &number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;

    














    virtual UnicodeString& format(const DigitList &number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

   








    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       ParsePosition& parsePosition) const;

    








    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       UErrorCode& status) const;

    



















    virtual CurrencyAmount* parseCurrency(const UnicodeString& text,
                                          ParsePosition& pos) const;

    










    static UClassID U_EXPORT2 getStaticClassID();

    










    virtual UClassID getDynamicClassID() const;

private:

    const UHashtable* _unitsByVariant;
    const double* _divisors;
    PluralRules* _pluralRules;

    
    CompactDecimalFormat(const DecimalFormat &, const UHashtable* unitsByVariant, const double* divisors, PluralRules* pluralRules);

    UBool eqHelper(const CompactDecimalFormat& that) const;
};

U_NAMESPACE_END

#endif 

#endif 

