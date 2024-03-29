














#ifndef NFRS_H
#define NFRS_H

#include "unicode/uobject.h"
#include "unicode/rbnf.h"

#if U_HAVE_RBNF

#include "unicode/utypes.h"
#include "unicode/umisc.h"

#include "nfrlist.h"

U_NAMESPACE_BEGIN

class NFRuleSet : public UMemory {
public:
    NFRuleSet(UnicodeString* descriptions, int32_t index, UErrorCode& status);
    void parseRules(UnicodeString& rules, const RuleBasedNumberFormat* owner, UErrorCode& status);
    void makeIntoFractionRuleSet() { fIsFractionRuleSet = TRUE; }

    ~NFRuleSet();

    UBool operator==(const NFRuleSet& rhs) const;
    UBool operator!=(const NFRuleSet& rhs) const { return !operator==(rhs); }

    UBool isPublic() const { return fIsPublic; }

    UBool isParseable() const { return fIsParseable; }

    UBool isFractionRuleSet() const { return fIsFractionRuleSet; }

    void  getName(UnicodeString& result) const { result.setTo(name); }
    UBool isNamed(const UnicodeString& _name) const { return this->name == _name; }

    void  format(int64_t number, UnicodeString& toAppendTo, int32_t pos, UErrorCode& status) const;
    void  format(double number, UnicodeString& toAppendTo, int32_t pos, UErrorCode& status) const;

    UBool parse(const UnicodeString& text, ParsePosition& pos, double upperBound, Formattable& result) const;

    void appendRules(UnicodeString& result) const; 

private:
    NFRule * findNormalRule(int64_t number) const;
    NFRule * findDoubleRule(double number) const;
    NFRule * findFractionRuleSetRule(double number) const;

private:
    UnicodeString name;
    NFRuleList rules;
    NFRule *negativeNumberRule;
    NFRule *fractionRules[3];
    UBool fIsFractionRuleSet;
    UBool fIsPublic;
    UBool fIsParseable;
    int32_t fRecursionCount;

    NFRuleSet(const NFRuleSet &other); 
    NFRuleSet &operator=(const NFRuleSet &other); 
};



int64_t util64_fromDouble(double d);


int64_t util64_pow(int32_t radix, uint32_t exponent);


uint32_t util64_tou(int64_t n, UChar* buffer, uint32_t buflen, uint32_t radix = 10, UBool raw = FALSE);

#ifdef RBNF_DEBUG
int64_t util64_utoi(const UChar* str, uint32_t radix = 10);
uint32_t util64_toa(int64_t n, char* buffer, uint32_t buflen, uint32_t radix = 10, UBool raw = FALSE);
int64_t util64_atoi(const char* str, uint32_t radix);
#endif


U_NAMESPACE_END


#endif


#endif

