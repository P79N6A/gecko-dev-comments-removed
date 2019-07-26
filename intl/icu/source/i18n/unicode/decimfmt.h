






















#ifndef DECIMFMT_H
#define DECIMFMT_H

#include "unicode/utypes.h"





#if !UCONFIG_NO_FORMATTING

#include "unicode/dcfmtsym.h"
#include "unicode/numfmt.h"
#include "unicode/locid.h"
#include "unicode/fpositer.h"
#include "unicode/stringpiece.h"
#include "unicode/curramt.h"
#include "unicode/enumset.h"





#if UCONFIG_FORMAT_FASTPATHS_49
#define UNUM_DECIMALFORMAT_INTERNAL_SIZE 16
#endif

U_NAMESPACE_BEGIN

class DigitList;
class ChoiceFormat;
class CurrencyPluralInfo;
class Hashtable;
class UnicodeSet;
class FieldPositionHandler;


#if defined (_MSC_VER)
template class U_I18N_API    EnumSet<UNumberFormatAttribute,
            UNUM_MAX_NONBOOLEAN_ATTRIBUTE+1, 
            UNUM_LIMIT_BOOLEAN_ATTRIBUTE>;
#endif


















































































































































































































































































































































































































































































































































































































class U_I18N_API DecimalFormat: public NumberFormat {
public:
    



    enum ERoundingMode {
        kRoundCeiling,  
        kRoundFloor,    
        kRoundDown,     
        kRoundUp,       
        kRoundHalfEven, 

        kRoundHalfDown, 

        kRoundHalfUp,   

        



        kRoundUnnecessary
    };

    



    enum EPadPosition {
        kPadBeforePrefix,
        kPadAfterPrefix,
        kPadBeforeSuffix,
        kPadAfterSuffix
    };

    












    DecimalFormat(UErrorCode& status);

    













    DecimalFormat(const UnicodeString& pattern,
                  UErrorCode& status);

    

















    DecimalFormat(  const UnicodeString& pattern,
                    DecimalFormatSymbols* symbolsToAdopt,
                    UErrorCode& status);

#ifndef U_HIDE_INTERNAL_API
    











    DecimalFormat(  const UnicodeString& pattern,
                    DecimalFormatSymbols* symbolsToAdopt,
                    UNumberFormatStyle style,
                    UErrorCode& status);


    









    virtual DecimalFormat& setAttribute( UNumberFormatAttribute attr,
                                       int32_t newvalue,
                                       UErrorCode &status);

    








    virtual int32_t getAttribute( UNumberFormatAttribute attr,
                                  UErrorCode &status) const;

#if UCONFIG_HAVE_PARSEALLINPUT
    


    void setParseAllInput(UNumberFormatAttributeValue value);
#endif

#endif  

    


















    DecimalFormat(  const UnicodeString& pattern,
                    DecimalFormatSymbols* symbolsToAdopt,
                    UParseError& parseError,
                    UErrorCode& status);
    
















    DecimalFormat(  const UnicodeString& pattern,
                    const DecimalFormatSymbols& symbols,
                    UErrorCode& status);

    





    DecimalFormat(const DecimalFormat& source);

    





    DecimalFormat& operator=(const DecimalFormat& rhs);

    



    virtual ~DecimalFormat();

    






    virtual Format* clone(void) const;

    







    virtual UBool operator==(const Format& other) const;


    using NumberFormat::format;

    










    virtual UnicodeString& format(double number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;


    











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
                                  FieldPosition& pos) const;

    










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
                                  UErrorCode &status) const;

    












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


    











    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

    










    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

    









    UnicodeString& format(double number,
                          UnicodeString& appendTo) const;

    










    UnicodeString& format(int32_t number,
                          UnicodeString& appendTo) const;

    










    UnicodeString& format(int64_t number,
                          UnicodeString& appendTo) const;
   


















    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       ParsePosition& parsePosition) const;

    
    







    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       UErrorCode& status) const;


    


















    virtual CurrencyAmount* parseCurrency(const UnicodeString& text,
                                          ParsePosition& pos) const;

    






    virtual const DecimalFormatSymbols* getDecimalFormatSymbols(void) const;

    





    virtual void adoptDecimalFormatSymbols(DecimalFormatSymbols* symbolsToAdopt);

    





    virtual void setDecimalFormatSymbols(const DecimalFormatSymbols& symbols);


    





    virtual const CurrencyPluralInfo* getCurrencyPluralInfo(void) const;

    





    virtual void adoptCurrencyPluralInfo(CurrencyPluralInfo* toAdopt);

    





    virtual void setCurrencyPluralInfo(const CurrencyPluralInfo& info);


    







    UnicodeString& getPositivePrefix(UnicodeString& result) const;

    






    virtual void setPositivePrefix(const UnicodeString& newValue);

    







    UnicodeString& getNegativePrefix(UnicodeString& result) const;

    






    virtual void setNegativePrefix(const UnicodeString& newValue);

    







    UnicodeString& getPositiveSuffix(UnicodeString& result) const;

    






    virtual void setPositiveSuffix(const UnicodeString& newValue);

    







    UnicodeString& getNegativeSuffix(UnicodeString& result) const;

    






    virtual void setNegativeSuffix(const UnicodeString& newValue);

    









    int32_t getMultiplier(void) const;

    









    virtual void setMultiplier(int32_t newValue);

    








    virtual double getRoundingIncrement(void) const;

    









    virtual void setRoundingIncrement(double newValue);

    







    virtual ERoundingMode getRoundingMode(void) const;

    







    virtual void setRoundingMode(ERoundingMode roundingMode);

    










    virtual int32_t getFormatWidth(void) const;

    













    virtual void setFormatWidth(int32_t width);

    











    virtual UnicodeString getPadCharacterString() const;

    













    virtual void setPadCharacter(const UnicodeString &padChar);

    














    virtual EPadPosition getPadPosition(void) const;

    















    virtual void setPadPosition(EPadPosition padPos);

    









    virtual UBool isScientificNotation(void);

    














    virtual void setScientificNotation(UBool useScientific);

    









    virtual int8_t getMinimumExponentDigits(void) const;

    











    virtual void setMinimumExponentDigits(int8_t minExpDig);

    











    virtual UBool isExponentSignAlwaysShown(void);

    












    virtual void setExponentSignAlwaysShown(UBool expSignAlways);

    










    int32_t getGroupingSize(void) const;

    










    virtual void setGroupingSize(int32_t newValue);

    

















    int32_t getSecondaryGroupingSize(void) const;

    










    virtual void setSecondaryGroupingSize(int32_t newValue);

    







    UBool isDecimalSeparatorAlwaysShown(void) const;

    







    virtual void setDecimalSeparatorAlwaysShown(UBool newValue);

    









    virtual UnicodeString& toPattern(UnicodeString& result) const;

    









    virtual UnicodeString& toLocalizedPattern(UnicodeString& result) const;

    




























    virtual void applyPattern(const UnicodeString& pattern,
                             UParseError& parseError,
                             UErrorCode& status);
    







    virtual void applyPattern(const UnicodeString& pattern,
                             UErrorCode& status);

    





























    virtual void applyLocalizedPattern(const UnicodeString& pattern,
                                       UParseError& parseError,
                                       UErrorCode& status);

    








    virtual void applyLocalizedPattern(const UnicodeString& pattern,
                                       UErrorCode& status);


    








    virtual void setMaximumIntegerDigits(int32_t newValue);

    








    virtual void setMinimumIntegerDigits(int32_t newValue);

    








    virtual void setMaximumFractionDigits(int32_t newValue);

    








    virtual void setMinimumFractionDigits(int32_t newValue);

    






    int32_t getMinimumSignificantDigits() const;

    






    int32_t getMaximumSignificantDigits() const;

    








    void setMinimumSignificantDigits(int32_t min);

    









    void setMaximumSignificantDigits(int32_t max);

    





    UBool areSignificantDigitsUsed() const;

    






    void setSignificantDigitsUsed(UBool useSignificantDigits);

 public:
    











    virtual void setCurrency(const UChar* theCurrency, UErrorCode& ec);

    




    virtual void setCurrency(const UChar* theCurrency);

    




    static const char fgNumberPatterns[];

public:

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;

private:

    DecimalFormat(); 

    int32_t precision() const;

    



    void init(UErrorCode& status);

    


    void construct(UErrorCode&               status,
                   UParseError&             parseErr,
                   const UnicodeString*     pattern = 0,
                   DecimalFormatSymbols*    symbolsToAdopt = 0
                   );

    







    UnicodeString& toPattern(UnicodeString& result, UBool localized) const;

    









    void applyPattern(const UnicodeString& pattern,
                            UBool localized,
                            UParseError& parseError,
                            UErrorCode& status);

    


    void applyPatternInternally(const UnicodeString& pluralCount,
                                const UnicodeString& pattern,
                                UBool localized,
                                UParseError& parseError,
                                UErrorCode& status);

    


    void applyPatternWithoutExpandAffix(const UnicodeString& pattern,
                                        UBool localized,
                                        UParseError& parseError,
                                        UErrorCode& status);


    


    void expandAffixAdjustWidth(const UnicodeString* pluralCount);


    









    UnicodeString& subformat(UnicodeString& appendTo,
                             FieldPositionHandler& handler,
                             DigitList&     digits,
                             UBool          isInteger, 
                             UErrorCode &status) const;


    void parse(const UnicodeString& text,
               Formattable& result,
               ParsePosition& pos,
               UChar* currency) const;

    enum {
        fgStatusInfinite,
        fgStatusLength      
    } StatusFlags;

    UBool subparse(const UnicodeString& text,
                   const UnicodeString* negPrefix,
                   const UnicodeString* negSuffix,
                   const UnicodeString* posPrefix,
                   const UnicodeString* posSuffix,
                   UBool currencyParsing,
                   int8_t type,
                   ParsePosition& parsePosition,
                   DigitList& digits, UBool* status,
                   UChar* currency) const;

    
    
    
    
    
    UBool parseForCurrency(const UnicodeString& text,
                           ParsePosition& parsePosition,
                           DigitList& digits,
                           UBool* status,
                           UChar* currency) const;

    int32_t skipPadding(const UnicodeString& text, int32_t position) const;

    int32_t compareAffix(const UnicodeString& input,
                         int32_t pos,
                         UBool isNegative,
                         UBool isPrefix,
                         const UnicodeString* affixPat,
                         UBool currencyParsing,
                         int8_t type,
                         UChar* currency) const;

    static int32_t compareSimpleAffix(const UnicodeString& affix,
                                      const UnicodeString& input,
                                      int32_t pos,
                                      UBool lenient);

    static int32_t skipPatternWhiteSpace(const UnicodeString& text, int32_t pos);

    static int32_t skipUWhiteSpace(const UnicodeString& text, int32_t pos);

    int32_t compareComplexAffix(const UnicodeString& affixPat,
                                const UnicodeString& input,
                                int32_t pos,
                                int8_t type,
                                UChar* currency) const;

    static int32_t match(const UnicodeString& text, int32_t pos, UChar32 ch);

    static int32_t match(const UnicodeString& text, int32_t pos, const UnicodeString& str);

    static UBool matchSymbol(const UnicodeString &text, int32_t position, int32_t length, const UnicodeString &symbol,
                             UnicodeSet *sset, UChar32 schar);

    static UBool matchDecimal(UChar32 symbolChar,
                            UBool sawDecimal,  UChar32 sawDecimalChar,
                             const UnicodeSet *sset, UChar32 schar);

    static UBool matchGrouping(UChar32 groupingChar,
                            UBool sawGrouping, UChar32 sawGroupingChar,
                             const UnicodeSet *sset,
                             UChar32 decimalChar, const UnicodeSet *decimalSet,
                             UChar32 schar);

    




    inline const UnicodeString &getConstSymbol(DecimalFormatSymbols::ENumberFormatSymbol symbol) const;

    int32_t appendAffix(UnicodeString& buf,
                        double number,
                        FieldPositionHandler& handler,
                        UBool isNegative,
                        UBool isPrefix) const;

    




    void appendAffixPattern(UnicodeString& appendTo, const UnicodeString& affix,
                            UBool localized) const;

    void appendAffixPattern(UnicodeString& appendTo,
                            const UnicodeString* affixPattern,
                            const UnicodeString& expAffix, UBool localized) const;

    void expandAffix(const UnicodeString& pattern,
                     UnicodeString& affix,
                     double number,
                     FieldPositionHandler& handler,
                     UBool doFormat,
                     const UnicodeString* pluralCount) const;

    void expandAffixes(const UnicodeString* pluralCount);

    void addPadding(UnicodeString& appendTo,
                    FieldPositionHandler& handler,
                    int32_t prefixLen, int32_t suffixLen) const;

    UBool isGroupingPosition(int32_t pos) const;

    void setCurrencyForSymbols();

    
    
    
    
    virtual void setCurrencyInternally(const UChar* theCurrency, UErrorCode& ec);

    
    
    
    
    void setupCurrencyAffixPatterns(UErrorCode& status);

    
    
    
    
    void setupCurrencyAffixes(const UnicodeString& pattern,
                              UBool setupForCurrentPattern,
                              UBool setupForPluralPattern,
                              UErrorCode& status);

    
    Hashtable* initHashForAffixPattern(UErrorCode& status);
    Hashtable* initHashForAffix(UErrorCode& status);

    void deleteHashForAffixPattern();
    void deleteHashForAffix(Hashtable*& table);

    void copyHashForAffixPattern(const Hashtable* source,
                                 Hashtable* target, UErrorCode& status);
    void copyHashForAffix(const Hashtable* source,
                          Hashtable* target, UErrorCode& status);

    UnicodeString& _format(int64_t number,
                           UnicodeString& appendTo,
                           FieldPositionHandler& handler,
                           UErrorCode &status) const;
    UnicodeString& _format(double number,
                           UnicodeString& appendTo,
                           FieldPositionHandler& handler,
                           UErrorCode &status) const;
    UnicodeString& _format(const DigitList &number,
                           UnicodeString& appendTo,
                           FieldPositionHandler& handler,
                           UErrorCode &status) const;

    
    enum {
        fgCurrencySignCountZero,
        fgCurrencySignCountInSymbolFormat,
        fgCurrencySignCountInISOFormat,
        fgCurrencySignCountInPluralFormat
    } CurrencySignCount;

    



    UnicodeString           fPositivePrefix;
    UnicodeString           fPositiveSuffix;
    UnicodeString           fNegativePrefix;
    UnicodeString           fNegativeSuffix;
    UnicodeString*          fPosPrefixPattern;
    UnicodeString*          fPosSuffixPattern;
    UnicodeString*          fNegPrefixPattern;
    UnicodeString*          fNegSuffixPattern;

    




    ChoiceFormat*           fCurrencyChoice;

    DigitList *             fMultiplier;   
    int32_t                 fGroupingSize;
    int32_t                 fGroupingSize2;
    UBool                   fDecimalSeparatorAlwaysShown;
    DecimalFormatSymbols*   fSymbols;

    UBool                   fUseSignificantDigits;
    int32_t                 fMinSignificantDigits;
    int32_t                 fMaxSignificantDigits;

    UBool                   fUseExponentialNotation;
    int8_t                  fMinExponentDigits;
    UBool                   fExponentSignAlwaysShown;

    EnumSet<UNumberFormatAttribute,
            UNUM_MAX_NONBOOLEAN_ATTRIBUTE+1, 
            UNUM_LIMIT_BOOLEAN_ATTRIBUTE>  
                            fBoolFlags;

    DigitList*              fRoundingIncrement;  
    ERoundingMode           fRoundingMode;

    UChar32                 fPad;
    int32_t                 fFormatWidth;
    EPadPosition            fPadPosition;

    


    
    UnicodeString fFormatPattern;
    
    
    int fStyle;
    









    int fCurrencySignCount;


    





    
    

























    



    
























    
    
    
    
    
    
    
    
    
    
    
    
    
    Hashtable* fAffixPatternsForCurrency;

    
    
    
    
    
    
    
    Hashtable* fAffixesForCurrency;  
    Hashtable* fPluralAffixesForCurrency;  

    
    CurrencyPluralInfo* fCurrencyPluralInfo;

#if UCONFIG_HAVE_PARSEALLINPUT
    UNumberFormatAttributeValue fParseAllInput;
#endif


protected:

    







    virtual void getEffectiveCurrency(UChar* result, UErrorCode& ec) const;

  


    static const int32_t  kDoubleIntegerDigits;
  


    static const int32_t  kDoubleFractionDigits;

    









    static const int32_t  kMaxScientificIntegerDigits;

#if UCONFIG_FORMAT_FASTPATHS_49
 private:
    



    uint8_t fReserved[UNUM_DECIMALFORMAT_INTERNAL_SIZE];


    


    void handleChanged();
#endif
};

inline UnicodeString&
DecimalFormat::format(const Formattable& obj,
                      UnicodeString& appendTo,
                      UErrorCode& status) const {
    
    
    return NumberFormat::format(obj, appendTo, status);
}

inline UnicodeString&
DecimalFormat::format(double number,
                      UnicodeString& appendTo) const {
    FieldPosition pos(0);
    return format(number, appendTo, pos);
}

inline UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo) const {
    FieldPosition pos(0);
    return format((int64_t)number, appendTo, pos);
}

#ifndef U_HIDE_INTERNAL_API
inline const UnicodeString &
DecimalFormat::getConstSymbol(DecimalFormatSymbols::ENumberFormatSymbol symbol) const {
    return fSymbols->getConstSymbol(symbol);
}

#endif

U_NAMESPACE_END

#endif 

#endif 

