














#ifndef NFSUBS_H
#define NFSUBS_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "nfrule.h"

#if U_HAVE_RBNF

#include "unicode/utypes.h"
#include "unicode/decimfmt.h"
#include "nfrs.h"
#include <float.h>

U_NAMESPACE_BEGIN

class NFSubstitution : public UObject {
    int32_t pos;
    const NFRuleSet* ruleSet;
    const DecimalFormat* numberFormat;
    
protected:
    NFSubstitution(int32_t pos,
        const NFRuleSet* ruleSet,
        const RuleBasedNumberFormat* rbnf,
        const UnicodeString& description,
        UErrorCode& status);
    
    



    const NFRuleSet* getRuleSet() const { return ruleSet; }

    



    const DecimalFormat* getNumberFormat() const { return numberFormat; }
    
public:
    static NFSubstitution* makeSubstitution(int32_t pos, 
        const NFRule* rule, 
        const NFRule* predecessor,
        const NFRuleSet* ruleSet, 
        const RuleBasedNumberFormat* rbnf, 
        const UnicodeString& description,
        UErrorCode& status);
    
    


    virtual ~NFSubstitution();
    
    





    virtual UBool operator==(const NFSubstitution& rhs) const;

    





    UBool operator!=(const NFSubstitution& rhs) const { return !operator==(rhs); }
    
    






    virtual void setDivisor(int32_t radix, int32_t exponent, UErrorCode& status);
    
    



    virtual void toString(UnicodeString& result) const;
    
    
    
    
    
    









    virtual void doSubstitution(int64_t number, UnicodeString& toInsertInto, int32_t pos) const;

    









    virtual void doSubstitution(double number, UnicodeString& toInsertInto, int32_t pos) const;
    
protected:
    








    virtual int64_t transformNumber(int64_t number) const = 0;

    








    virtual double transformNumber(double number) const = 0;
    
public:
    
    
    
    
    




























    virtual UBool doParse(const UnicodeString& text, 
        ParsePosition& parsePosition, 
        double baseValue,
        double upperBound, 
        UBool lenientParse,
        Formattable& result) const;
    
    











    virtual double composeRuleValue(double newRuleValue, double oldRuleValue) const = 0;
    
    








    virtual double calcUpperBound(double oldUpperBound) const = 0;
    
    
    
    
    
    



    int32_t getPos() const { return pos; }
    
    




    virtual UChar tokenChar() const = 0;
    
    





    virtual UBool isNullSubstitution() const;
    
    





    virtual UBool isModulusSubstitution() const;
    
private:
    NFSubstitution(const NFSubstitution &other); 
    NFSubstitution &operator=(const NFSubstitution &other); 

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

U_NAMESPACE_END


#endif


#endif
