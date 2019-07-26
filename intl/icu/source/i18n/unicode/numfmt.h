



















#ifndef NUMFMT_H
#define NUMFMT_H


#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/format.h"
#include "unicode/unum.h" 
#include "unicode/locid.h"
#include "unicode/stringpiece.h"
#include "unicode/curramt.h"

class NumberFormatTest;

U_NAMESPACE_BEGIN

#if !UCONFIG_NO_SERVICE
class NumberFormatFactory;
class StringEnumeration;
#endif





















































































































class U_I18N_API NumberFormat : public Format {
public:
    














    enum EAlignmentFields {
        
        kIntegerField = UNUM_INTEGER_FIELD,
        
        kFractionField = UNUM_FRACTION_FIELD,
        
        kDecimalSeparatorField = UNUM_DECIMAL_SEPARATOR_FIELD,
        
        kExponentSymbolField = UNUM_EXPONENT_SYMBOL_FIELD,
        
        kExponentSignField = UNUM_EXPONENT_SIGN_FIELD,
        
        kExponentField = UNUM_EXPONENT_FIELD,
        
        kGroupingSeparatorField = UNUM_GROUPING_SEPARATOR_FIELD,
        
        kCurrencyField = UNUM_CURRENCY_FIELD,
        
        kPercentField = UNUM_PERCENT_FIELD,
        
        kPermillField = UNUM_PERMILL_FIELD,
        
        kSignField = UNUM_SIGN_FIELD,

    



        
        INTEGER_FIELD        = UNUM_INTEGER_FIELD,
        
        FRACTION_FIELD       = UNUM_FRACTION_FIELD
    };

    



    virtual ~NumberFormat();

    





    virtual UBool operator==(const Format& other) const;


    using Format::format;

    














    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

    















    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;

    



























    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& parse_pos) const;

    









    UnicodeString& format(  double number,
                            UnicodeString& appendTo) const;

    









    UnicodeString& format(  int32_t number,
                            UnicodeString& appendTo) const;

    









    UnicodeString& format(  int64_t number,
                            UnicodeString& appendTo) const;

    











    virtual UnicodeString& format(double number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const = 0;
    













    virtual UnicodeString& format(double number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode &status) const;
    













    virtual UnicodeString& format(double number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;
    











    virtual UnicodeString& format(int32_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const = 0;

    












    virtual UnicodeString& format(int32_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode &status) const;

    













    virtual UnicodeString& format(int32_t number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;
    












    virtual UnicodeString& format(int64_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;

    












    virtual UnicodeString& format(int64_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;
    













    virtual UnicodeString& format(int64_t number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;

    















    virtual UnicodeString& format(const StringPiece &number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;
public:
    
















    virtual UnicodeString& format(const DigitList &number,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;

    
















    virtual UnicodeString& format(const DigitList &number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

public:

    









    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

   


















    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       ParsePosition& parsePosition) const = 0;

    












    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       UErrorCode& status) const;


    


















    virtual CurrencyAmount* parseCurrency(const UnicodeString& text,
                                          ParsePosition& pos) const;

    










    UBool isParseIntegerOnly(void) const;

    






    virtual void setParseIntegerOnly(UBool value);

    






    virtual void setLenient(UBool enable);

    







    virtual UBool isLenient(void) const;

    







    static NumberFormat* U_EXPORT2 createInstance(UErrorCode&);

    







    static NumberFormat* U_EXPORT2 createInstance(const Locale& inLocale,
                                        UErrorCode&);

    







    static NumberFormat* U_EXPORT2 createInstance(const Locale& desiredLocale,
                                                  UNumberFormatStyle style,
                                                  UErrorCode& errorCode);

    



    static NumberFormat* U_EXPORT2 createCurrencyInstance(UErrorCode&);

    




    static NumberFormat* U_EXPORT2 createCurrencyInstance(const Locale& inLocale,
                                                UErrorCode&);

    



    static NumberFormat* U_EXPORT2 createPercentInstance(UErrorCode&);

    




    static NumberFormat* U_EXPORT2 createPercentInstance(const Locale& inLocale,
                                               UErrorCode&);

    



    static NumberFormat* U_EXPORT2 createScientificInstance(UErrorCode&);

    




    static NumberFormat* U_EXPORT2 createScientificInstance(const Locale& inLocale,
                                                UErrorCode&);

    




    static const Locale* U_EXPORT2 getAvailableLocales(int32_t& count);

#if !UCONFIG_NO_SERVICE
    






    static URegistryKey U_EXPORT2 registerFactory(NumberFormatFactory* toAdopt, UErrorCode& status);

    








    static UBool U_EXPORT2 unregister(URegistryKey key, UErrorCode& status);

    





    static StringEnumeration* U_EXPORT2 getAvailableLocales(void);
#endif 

    








    UBool isGroupingUsed(void) const;

    





    virtual void setGroupingUsed(UBool newValue);

    







    int32_t getMaximumIntegerDigits(void) const;

    











    virtual void setMaximumIntegerDigits(int32_t newValue);

    







    int32_t getMinimumIntegerDigits(void) const;

    









    virtual void setMinimumIntegerDigits(int32_t newValue);

    







    int32_t getMaximumFractionDigits(void) const;

    









    virtual void setMaximumFractionDigits(int32_t newValue);

    







    int32_t getMinimumFractionDigits(void) const;

    









    virtual void setMinimumFractionDigits(int32_t newValue);

    











    virtual void setCurrency(const UChar* theCurrency, UErrorCode& ec);

    






    const UChar* getCurrency() const;

public:

    







    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const = 0;

protected:

    



    NumberFormat();

    



    NumberFormat(const NumberFormat&);

    



    NumberFormat& operator=(const NumberFormat&);

    







    virtual void getEffectiveCurrency(UChar* result, UErrorCode& ec) const;

private:

    static UBool isStyleSupported(UNumberFormatStyle style);

    






    static NumberFormat* makeInstance(const Locale& desiredLocale,
                                      UNumberFormatStyle style,
                                      UErrorCode& errorCode);

    UBool      fGroupingUsed;
    int32_t     fMaxIntegerDigits;
    int32_t     fMinIntegerDigits;
    int32_t     fMaxFractionDigits;
    int32_t     fMinFractionDigits;
    UBool      fParseIntegerOnly;
    UBool      fLenient; 

    
    UChar      fCurrency[4];

    friend class ICUNumberFormatFactory; 
    friend class ICUNumberFormatService;
    friend class ::NumberFormatTest;  
};

#if !UCONFIG_NO_SERVICE








class U_I18N_API NumberFormatFactory : public UObject {
public:

    



    virtual ~NumberFormatFactory();

    





    virtual UBool visible(void) const = 0;

    




    virtual const UnicodeString * getSupportedIDs(int32_t &count, UErrorCode& status) const = 0;

    






    virtual NumberFormat* createFormat(const Locale& loc, UNumberFormatStyle formatType) = 0;
};





class U_I18N_API SimpleNumberFormatFactory : public NumberFormatFactory {
protected:
    



    const UBool _visible;

    



    UnicodeString _id;

public:
    


    SimpleNumberFormatFactory(const Locale& locale, UBool visible = TRUE);

    


    virtual ~SimpleNumberFormatFactory();

    


    virtual UBool visible(void) const;

    


    virtual const UnicodeString * getSupportedIDs(int32_t &count, UErrorCode& status) const;
};
#endif 



inline UBool
NumberFormat::isParseIntegerOnly() const
{
    return fParseIntegerOnly;
}

inline UBool
NumberFormat::isLenient() const
{
    return fLenient;
}

inline UnicodeString&
NumberFormat::format(const Formattable& obj,
                     UnicodeString& appendTo,
                     UErrorCode& status) const {
    return Format::format(obj, appendTo, status);
}

U_NAMESPACE_END

#endif

#endif

