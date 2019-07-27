























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
class DecimalFormatStaticSets;
class FixedDecimal;


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

#if UCONFIG_HAVE_PARSEALLINPUT
    


    void setParseAllInput(UNumberFormatAttributeValue value);
#endif

#endif  


    









    virtual DecimalFormat& setAttribute( UNumberFormatAttribute attr,
                                       int32_t newvalue,
                                       UErrorCode &status);

    








    virtual int32_t getAttribute( UNumberFormatAttribute attr,
                                  UErrorCode &status) const;

    
    





    virtual void setGroupingUsed(UBool newValue);

    






    virtual void setParseIntegerOnly(UBool value);

    








    virtual void setContext(UDisplayContext value, UErrorCode& status);

    


















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

   using NumberFormat::parse;

   


















    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       ParsePosition& parsePosition) const;

    


















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

    









    virtual UBool isScientificNotation(void) const;

    














    virtual void setScientificNotation(UBool useScientific);

    









    virtual int8_t getMinimumExponentDigits(void) const;

    











    virtual void setMinimumExponentDigits(int8_t minExpDig);

    











    virtual UBool isExponentSignAlwaysShown(void) const;

    












    virtual void setExponentSignAlwaysShown(UBool expSignAlways);

    










    int32_t getGroupingSize(void) const;

    










    virtual void setGroupingSize(int32_t newValue);

    

















    int32_t getSecondaryGroupingSize(void) const;

    










    virtual void setSecondaryGroupingSize(int32_t newValue);

    







    UBool isDecimalSeparatorAlwaysShown(void) const;

    







    virtual void setDecimalSeparatorAlwaysShown(UBool newValue);

#ifndef U_HIDE_DRAFT_API
    





    UBool isDecimalPatternMatchRequired(void) const;
#endif  

    








    virtual void setDecimalPatternMatchRequired(UBool newValue);


    









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

#ifndef U_HIDE_DRAFT_API
    






    void setCurrencyUsage(UCurrencyUsage newUsage, UErrorCode* ec);

    



    UCurrencyUsage getCurrencyUsage() const;
#endif  


#ifndef U_HIDE_DEPRECATED_API
    




    static const char fgNumberPatterns[];
#endif  

#ifndef U_HIDE_INTERNAL_API
    





     FixedDecimal getFixedDecimal(double number, UErrorCode &status) const;

    





     FixedDecimal getFixedDecimal(const Formattable &number, UErrorCode &status) const;

    





     FixedDecimal getFixedDecimal(DigitList &number, UErrorCode &status) const;
#endif  

public:

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;

private:

    DecimalFormat(); 

    int32_t precision() const;

    



    void init();

    


    void construct(UErrorCode&              status,
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
                   UBool complexCurrencyParsing,
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
                         UBool complexCurrencyParsing,
                         int8_t type,
                         UChar* currency) const;

    static UnicodeString& trimMarksFromAffix(const UnicodeString& affix, UnicodeString& trimmedAffix);

    UBool equalWithSignCompatibility(UChar32 lhs, UChar32 rhs) const;

    int32_t compareSimpleAffix(const UnicodeString& affix,
                                      const UnicodeString& input,
                                      int32_t pos,
                                      UBool lenient) const;

    static int32_t skipPatternWhiteSpace(const UnicodeString& text, int32_t pos);

    static int32_t skipUWhiteSpace(const UnicodeString& text, int32_t pos);

    static int32_t skipUWhiteSpaceAndMarks(const UnicodeString& text, int32_t pos);

    static int32_t skipBidiMarks(const UnicodeString& text, int32_t pos);

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
	
    
    double getCurrencyRounding(const UChar* currency,
                               UErrorCode* ec) const;
	
    
    int getCurrencyFractionDigits(const UChar* currency,
                                  UErrorCode* ec) const;

    
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
    int32_t                 fScale;
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

    
    const DecimalFormatStaticSets *fStaticSets;
	
    
    UCurrencyUsage fCurrencyUsage;

protected:

#ifndef U_HIDE_INTERNAL_API
    



    DigitList& _round(const DigitList& number, DigitList& adjustedNum, UBool& isNegative, UErrorCode& status) const;
#endif  

    







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

inline const UnicodeString &
DecimalFormat::getConstSymbol(DecimalFormatSymbols::ENumberFormatSymbol symbol) const {
    return fSymbols->getConstSymbol(symbol);
}

U_NAMESPACE_END

#endif 

#endif 

