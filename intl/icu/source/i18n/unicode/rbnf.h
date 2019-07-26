






#ifndef RBNF_H
#define RBNF_H

#include "unicode/utypes.h"













#if UCONFIG_NO_FORMATTING
#define U_HAVE_RBNF 0
#else
#define U_HAVE_RBNF 1

#include "unicode/coll.h"
#include "unicode/dcfmtsym.h"
#include "unicode/fmtable.h"
#include "unicode/locid.h"
#include "unicode/numfmt.h"
#include "unicode/unistr.h"
#include "unicode/strenum.h"

U_NAMESPACE_BEGIN

class NFRuleSet;
class LocalizationInfo;






enum URBNFRuleSetTag {
    URBNF_SPELLOUT,
    URBNF_ORDINAL,
    URBNF_DURATION,
    URBNF_NUMBERING_SYSTEM,
    URBNF_COUNT
};

#if UCONFIG_NO_COLLATION
class Collator;
#endif




























































































































































































































































































































































































































































class U_I18N_API RuleBasedNumberFormat : public NumberFormat {
public:

  
  
  

    









    RuleBasedNumberFormat(const UnicodeString& rules, UParseError& perror, UErrorCode& status);

    






















    RuleBasedNumberFormat(const UnicodeString& rules, const UnicodeString& localizations,
                        UParseError& perror, UErrorCode& status);

  














  RuleBasedNumberFormat(const UnicodeString& rules, const Locale& locale,
                        UParseError& perror, UErrorCode& status);

    

























    RuleBasedNumberFormat(const UnicodeString& rules, const UnicodeString& localizations,
                        const Locale& locale, UParseError& perror, UErrorCode& status);

  














  RuleBasedNumberFormat(URBNFRuleSetTag tag, const Locale& locale, UErrorCode& status);

  
  
  

  




  RuleBasedNumberFormat(const RuleBasedNumberFormat& rhs);

  




  RuleBasedNumberFormat& operator=(const RuleBasedNumberFormat& rhs);

  



  virtual ~RuleBasedNumberFormat();

  





  virtual Format* clone(void) const;

  






  virtual UBool operator==(const Format& other) const;





  




  virtual UnicodeString getRules() const;

  




  virtual int32_t getNumberOfRuleSetNames() const;

  






  virtual UnicodeString getRuleSetName(int32_t index) const;

  




  virtual int32_t getNumberOfRuleSetDisplayNameLocales(void) const;

  







  virtual Locale getRuleSetDisplayNameLocale(int32_t index, UErrorCode& status) const;

    












  virtual UnicodeString getRuleSetDisplayName(int32_t index,
                          const Locale& locale = Locale::getDefault());

    







  virtual UnicodeString getRuleSetDisplayName(const UnicodeString& ruleSetName,
                          const Locale& locale = Locale::getDefault());


  using NumberFormat::format;

  







  virtual UnicodeString& format(int32_t number,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos) const;

  







  virtual UnicodeString& format(int64_t number,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos) const;
  







  virtual UnicodeString& format(double number,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos) const;

  










  virtual UnicodeString& format(int32_t number,
                                const UnicodeString& ruleSetName,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos,
                                UErrorCode& status) const;
  










  virtual UnicodeString& format(int64_t number,
                                const UnicodeString& ruleSetName,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos,
                                UErrorCode& status) const;
  










  virtual UnicodeString& format(double number,
                                const UnicodeString& ruleSetName,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos,
                                UErrorCode& status) const;

  








  virtual UnicodeString& format(const Formattable& obj,
                                UnicodeString& toAppendTo,
                                FieldPosition& pos,
                                UErrorCode& status) const;
  







  UnicodeString& format(const Formattable& obj,
                        UnicodeString& result,
                        UErrorCode& status) const;

  






   UnicodeString& format(double number,
                         UnicodeString& output) const;

  






   UnicodeString& format(int32_t number,
                         UnicodeString& output) const;

  













  virtual void parse(const UnicodeString& text,
                     Formattable& result,
                     ParsePosition& parsePosition) const;


  






  virtual inline void parse(const UnicodeString& text,
                      Formattable& result,
                      UErrorCode& status) const;

#if !UCONFIG_NO_COLLATION

  
































  virtual void setLenient(UBool enabled);

  






  virtual inline UBool isLenient(void) const;

#endif

  







  virtual void setDefaultRuleSet(const UnicodeString& ruleSetName, UErrorCode& status);

  





  virtual UnicodeString getDefaultRuleSetName() const;

public:
    




    static UClassID U_EXPORT2 getStaticClassID(void);

    




    virtual UClassID getDynamicClassID(void) const;

    







    virtual void adoptDecimalFormatSymbols(DecimalFormatSymbols* symbolsToAdopt);

    








    virtual void setDecimalFormatSymbols(const DecimalFormatSymbols& symbols);

private:
    RuleBasedNumberFormat(); 

    
    
    RuleBasedNumberFormat(const UnicodeString& description, LocalizationInfo* localizations,
              const Locale& locale, UParseError& perror, UErrorCode& status);

    void init(const UnicodeString& rules, LocalizationInfo* localizations, UParseError& perror, UErrorCode& status);
    void dispose();
    void stripWhitespace(UnicodeString& src);
    void initDefaultRuleSet();
    void format(double number, NFRuleSet& ruleSet);
    NFRuleSet* findRuleSet(const UnicodeString& name, UErrorCode& status) const;

    
    friend class NFSubstitution;
    friend class NFRule;
    friend class FractionalPartSubstitution;

    inline NFRuleSet * getDefaultRuleSet() const;
    Collator * getCollator() const;
    DecimalFormatSymbols * getDecimalFormatSymbols() const;

private:
    NFRuleSet **ruleSets;
    UnicodeString* ruleSetDescriptions;
    int32_t numRuleSets;
    NFRuleSet *defaultRuleSet;
    Locale locale;
    Collator* collator;
    DecimalFormatSymbols* decimalFormatSymbols;
    UBool lenient;
    UnicodeString* lenientParseRules;
    LocalizationInfo* localizations;
};



inline UnicodeString&
RuleBasedNumberFormat::format(const Formattable& obj,
                              UnicodeString& result,
                              UErrorCode& status) const
{
    
    
    
    
    
    return NumberFormat::format(obj, result, status);
}

inline UnicodeString&
RuleBasedNumberFormat::format(double number, UnicodeString& output) const {
    FieldPosition pos(0);
    return format(number, output, pos);
}

inline UnicodeString&
RuleBasedNumberFormat::format(int32_t number, UnicodeString& output) const {
    FieldPosition pos(0);
    return format(number, output, pos);
}

inline void
RuleBasedNumberFormat::parse(const UnicodeString& text, Formattable& result, UErrorCode& status) const
{
    NumberFormat::parse(text, result, status);
}

#if !UCONFIG_NO_COLLATION

inline UBool
RuleBasedNumberFormat::isLenient(void) const {
    return lenient;
}

#endif

inline NFRuleSet*
RuleBasedNumberFormat::getDefaultRuleSet() const {
    return defaultRuleSet;
}

U_NAMESPACE_END


#endif


#endif
