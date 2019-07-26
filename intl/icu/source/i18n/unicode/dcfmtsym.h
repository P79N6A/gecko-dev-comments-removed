























#ifndef DCFMTSYM_H
#define DCFMTSYM_H

#include "unicode/utypes.h"
#include "unicode/uchar.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"
#include "unicode/locid.h"
#include "unicode/unum.h"







U_NAMESPACE_BEGIN








































class U_I18N_API DecimalFormatSymbols : public UObject {
public:
    



    enum ENumberFormatSymbol {
        
        kDecimalSeparatorSymbol,
        
        kGroupingSeparatorSymbol,
        
        kPatternSeparatorSymbol,
        
        kPercentSymbol,
        
        kZeroDigitSymbol,
        
        kDigitSymbol,
        
        kMinusSignSymbol,
        
        kPlusSignSymbol,
        
        kCurrencySymbol,
        
        kIntlCurrencySymbol,
        
        kMonetarySeparatorSymbol,
        
        kExponentialSymbol,
        
        kPerMillSymbol,
        
        kPadEscapeSymbol,
        
        kInfinitySymbol,
        
        kNaNSymbol,
        

        kSignificantDigitSymbol,
        


        kMonetaryGroupingSeparatorSymbol,
        


        kOneDigitSymbol,
        


        kTwoDigitSymbol,
        


        kThreeDigitSymbol,
        


        kFourDigitSymbol,
        


        kFiveDigitSymbol,
        


        kSixDigitSymbol,
        


        kSevenDigitSymbol,
        


        kEightDigitSymbol,
        


        kNineDigitSymbol,
        
        kFormatSymbolCount
    };

    







    DecimalFormatSymbols(const Locale& locale, UErrorCode& status);

    









    DecimalFormatSymbols( UErrorCode& status);

    



    DecimalFormatSymbols(const DecimalFormatSymbols&);

    



    DecimalFormatSymbols& operator=(const DecimalFormatSymbols&);

    



    virtual ~DecimalFormatSymbols();

    






    UBool operator==(const DecimalFormatSymbols& other) const;

    






    UBool operator!=(const DecimalFormatSymbols& other) const { return !operator==(other); }

    








    inline UnicodeString getSymbol(ENumberFormatSymbol symbol) const;

    











    void setSymbol(ENumberFormatSymbol symbol, const UnicodeString &value, const UBool propogateDigits);

    



    inline Locale getLocale() const;

    




    Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const;

    















     const UnicodeString& getPatternForCurrencySpacing(UCurrencySpacing type,
                                                 UBool beforeCurrency,
                                                 UErrorCode& status) const;
     









     void setPatternForCurrencySpacing(UCurrencySpacing type,
                                       UBool beforeCurrency,
                                       const UnicodeString& pattern);

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

private:
    DecimalFormatSymbols(); 

    









    void initialize(const Locale& locale, UErrorCode& success, UBool useLastResortData = FALSE);

    


    void initialize();

    void setCurrencyForSymbols();

public:
#ifndef U_HIDE_INTERNAL_API
    










    inline const UnicodeString &getConstSymbol(ENumberFormatSymbol symbol) const;

    



    inline const UChar* getCurrencyPattern(void) const;
#endif  

private:
    














    UnicodeString fSymbols[kFormatSymbolCount];

    



    UnicodeString fNoSymbol;

    Locale locale;

    char actualLocale[ULOC_FULLNAME_CAPACITY];
    char validLocale[ULOC_FULLNAME_CAPACITY];
    const UChar* currPattern;

    UnicodeString currencySpcBeforeSym[UNUM_CURRENCY_SPACING_COUNT];
    UnicodeString currencySpcAfterSym[UNUM_CURRENCY_SPACING_COUNT];
};



inline UnicodeString
DecimalFormatSymbols::getSymbol(ENumberFormatSymbol symbol) const {
    const UnicodeString *strPtr;
    if(symbol < kFormatSymbolCount) {
        strPtr = &fSymbols[symbol];
    } else {
        strPtr = &fNoSymbol;
    }
    return *strPtr;
}

#ifndef U_HIDE_INTERNAL_API
inline const UnicodeString &
DecimalFormatSymbols::getConstSymbol(ENumberFormatSymbol symbol) const {
    const UnicodeString *strPtr;
    if(symbol < kFormatSymbolCount) {
        strPtr = &fSymbols[symbol];
    } else {
        strPtr = &fNoSymbol;
    }
    return *strPtr;
}
#endif




inline void
DecimalFormatSymbols::setSymbol(ENumberFormatSymbol symbol, const UnicodeString &value, const UBool propogateDigits = TRUE) {
    if(symbol<kFormatSymbolCount) {
        fSymbols[symbol]=value;
    }

    
    
    if ( propogateDigits && symbol == kZeroDigitSymbol && value.countChar32() == 1 ) {
        UChar32 sym = value.char32At(0);
        if ( u_charDigitValue(sym) == 0 ) {
            for ( int8_t i = 1 ; i<= 9 ; i++ ) {
                sym++;
                fSymbols[(int)kOneDigitSymbol+i-1] = UnicodeString(sym);
            }
        }
    }
}



inline Locale
DecimalFormatSymbols::getLocale() const {
    return locale;
}

#ifndef U_HIDE_INTERNAL_API
inline const UChar*
DecimalFormatSymbols::getCurrencyPattern() const {
    return currPattern;
}
#endif

U_NAMESPACE_END

#endif

#endif

