














#include <stdio.h>
#include "utypeinfo.h"  

#include "nfsubs.h"
#include "digitlst.h"

#if U_HAVE_RBNF

static const UChar gLessThan = 0x003c;
static const UChar gEquals = 0x003d;
static const UChar gGreaterThan = 0x003e;
static const UChar gPercent = 0x0025;
static const UChar gPound = 0x0023;
static const UChar gZero = 0x0030;
static const UChar gSpace = 0x0020;

static const UChar gEqualsEquals[] =
{
    0x3D, 0x3D, 0
}; 
static const UChar gGreaterGreaterGreaterThan[] =
{
    0x3E, 0x3E, 0x3E, 0
}; 
static const UChar gGreaterGreaterThan[] =
{
    0x3E, 0x3E, 0
}; 

U_NAMESPACE_BEGIN

class SameValueSubstitution : public NFSubstitution {
public:
    SameValueSubstitution(int32_t pos,
        const NFRuleSet* ruleset,
        const RuleBasedNumberFormat* formatter,
        const UnicodeString& description,
        UErrorCode& status);
    virtual ~SameValueSubstitution();

    virtual int64_t transformNumber(int64_t number) const { return number; }
    virtual double transformNumber(double number) const { return number; }
    virtual double composeRuleValue(double newRuleValue, double ) const { return newRuleValue; }
    virtual double calcUpperBound(double oldUpperBound) const { return oldUpperBound; }
    virtual UChar tokenChar() const { return (UChar)0x003d; } 

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

SameValueSubstitution::~SameValueSubstitution() {}

class MultiplierSubstitution : public NFSubstitution {
    double divisor;
    int64_t ldivisor;

public:
    MultiplierSubstitution(int32_t _pos,
        double _divisor,
        const NFRuleSet* _ruleSet,
        const RuleBasedNumberFormat* formatter,
        const UnicodeString& description,
        UErrorCode& status)
        : NFSubstitution(_pos, _ruleSet, formatter, description, status), divisor(_divisor)
    {
        ldivisor = util64_fromDouble(divisor);
        if (divisor == 0) {
            status = U_PARSE_ERROR;
        }
    }
    virtual ~MultiplierSubstitution();

    virtual void setDivisor(int32_t radix, int32_t exponent, UErrorCode& status) { 
        divisor = uprv_pow(radix, exponent);
        ldivisor = util64_fromDouble(divisor);

        if(divisor == 0) {
            status = U_PARSE_ERROR;
        }
    }

    virtual UBool operator==(const NFSubstitution& rhs) const;

    virtual int64_t transformNumber(int64_t number) const {
        return number / ldivisor;
    }

    virtual double transformNumber(double number) const {
        if (getRuleSet()) {
            return uprv_floor(number / divisor);
        } else {
            return number/divisor;
        }
    }

    virtual double composeRuleValue(double newRuleValue, double ) const {
        return newRuleValue * divisor;
    }

    virtual double calcUpperBound(double ) const { return divisor; }

    virtual UChar tokenChar() const { return (UChar)0x003c; } 

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

MultiplierSubstitution::~MultiplierSubstitution() {}

class ModulusSubstitution : public NFSubstitution {
    double divisor;
    int64_t  ldivisor;
    const NFRule* ruleToUse;
public:
    ModulusSubstitution(int32_t pos,
        double _divisor,
        const NFRule* rulePredecessor,
        const NFRuleSet* ruleSet,
        const RuleBasedNumberFormat* formatter,
        const UnicodeString& description,
        UErrorCode& status);
    virtual ~ModulusSubstitution();

    virtual void setDivisor(int32_t radix, int32_t exponent, UErrorCode& status) { 
        divisor = uprv_pow(radix, exponent);
        ldivisor = util64_fromDouble(divisor);

        if (divisor == 0) {
            status = U_PARSE_ERROR;
        }
    }

    virtual UBool operator==(const NFSubstitution& rhs) const;

    virtual void doSubstitution(int64_t number, UnicodeString& toInsertInto, int32_t pos) const;
    virtual void doSubstitution(double number, UnicodeString& toInsertInto, int32_t pos) const;

    virtual int64_t transformNumber(int64_t number) const { return number % ldivisor; }
    virtual double transformNumber(double number) const { return uprv_fmod(number, divisor); }

    virtual UBool doParse(const UnicodeString& text, 
        ParsePosition& parsePosition,
        double baseValue,
        double upperBound,
        UBool lenientParse,
        Formattable& result) const;

    virtual double composeRuleValue(double newRuleValue, double oldRuleValue) const {
        return oldRuleValue - uprv_fmod(oldRuleValue, divisor) + newRuleValue;
    }

    virtual double calcUpperBound(double ) const { return divisor; }

    virtual UBool isModulusSubstitution() const { return TRUE; }

    virtual UChar tokenChar() const { return (UChar)0x003e; } 

	virtual void toString(UnicodeString& result) const;

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

ModulusSubstitution::~ModulusSubstitution() {}

class IntegralPartSubstitution : public NFSubstitution {
public:
    IntegralPartSubstitution(int32_t _pos,
        const NFRuleSet* _ruleSet,
        const RuleBasedNumberFormat* formatter,
        const UnicodeString& description,
        UErrorCode& status)
        : NFSubstitution(_pos, _ruleSet, formatter, description, status) {}
    virtual ~IntegralPartSubstitution();

    virtual int64_t transformNumber(int64_t number) const { return number; }
    virtual double transformNumber(double number) const { return uprv_floor(number); }
    virtual double composeRuleValue(double newRuleValue, double oldRuleValue) const { return newRuleValue + oldRuleValue; }
    virtual double calcUpperBound(double ) const { return DBL_MAX; }
    virtual UChar tokenChar() const { return (UChar)0x003c; } 

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

IntegralPartSubstitution::~IntegralPartSubstitution() {}

class FractionalPartSubstitution : public NFSubstitution {
    UBool byDigits;
    UBool useSpaces;
    enum { kMaxDecimalDigits = 8 };
public:
    FractionalPartSubstitution(int32_t pos,
        const NFRuleSet* ruleSet,
        const RuleBasedNumberFormat* formatter,
        const UnicodeString& description,
        UErrorCode& status);
    virtual ~FractionalPartSubstitution();

    virtual UBool operator==(const NFSubstitution& rhs) const;

    virtual void doSubstitution(double number, UnicodeString& toInsertInto, int32_t pos) const;
    virtual void doSubstitution(int64_t , UnicodeString& , int32_t ) const {}
    virtual int64_t transformNumber(int64_t ) const { return 0; }
    virtual double transformNumber(double number) const { return number - uprv_floor(number); }

    virtual UBool doParse(const UnicodeString& text,
        ParsePosition& parsePosition,
        double baseValue,
        double upperBound,
        UBool lenientParse,
        Formattable& result) const;

    virtual double composeRuleValue(double newRuleValue, double oldRuleValue) const { return newRuleValue + oldRuleValue; }
    virtual double calcUpperBound(double ) const { return 0.0; }
    virtual UChar tokenChar() const { return (UChar)0x003e; } 

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

FractionalPartSubstitution::~FractionalPartSubstitution() {}

class AbsoluteValueSubstitution : public NFSubstitution {
public:
    AbsoluteValueSubstitution(int32_t _pos,
        const NFRuleSet* _ruleSet,
        const RuleBasedNumberFormat* formatter,
        const UnicodeString& description,
        UErrorCode& status)
        : NFSubstitution(_pos, _ruleSet, formatter, description, status) {}
    virtual ~AbsoluteValueSubstitution();

    virtual int64_t transformNumber(int64_t number) const { return number >= 0 ? number : -number; }
    virtual double transformNumber(double number) const { return uprv_fabs(number); }
    virtual double composeRuleValue(double newRuleValue, double ) const { return -newRuleValue; }
    virtual double calcUpperBound(double ) const { return DBL_MAX; }
    virtual UChar tokenChar() const { return (UChar)0x003e; } 

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

AbsoluteValueSubstitution::~AbsoluteValueSubstitution() {}

class NumeratorSubstitution : public NFSubstitution {
    double denominator;
    int64_t ldenominator;
    UBool withZeros;
public:
    static inline UnicodeString fixdesc(const UnicodeString& desc) {
        if (desc.endsWith(LTLT, 2)) {
            UnicodeString result(desc, 0, desc.length()-1);
            return result;
        }
        return desc;
    }
    NumeratorSubstitution(int32_t _pos,
        double _denominator,
        const NFRuleSet* _ruleSet,
        const RuleBasedNumberFormat* formatter,
        const UnicodeString& description,
        UErrorCode& status)
        : NFSubstitution(_pos, _ruleSet, formatter, fixdesc(description), status), denominator(_denominator) 
    {
        ldenominator = util64_fromDouble(denominator);
        withZeros = description.endsWith(LTLT, 2);
    }
    virtual ~NumeratorSubstitution();

    virtual UBool operator==(const NFSubstitution& rhs) const;

    virtual int64_t transformNumber(int64_t number) const { return number * ldenominator; }
    virtual double transformNumber(double number) const { return uprv_round(number * denominator); }

    virtual void doSubstitution(int64_t , UnicodeString& , int32_t ) const {}
    virtual void doSubstitution(double number, UnicodeString& toInsertInto, int32_t pos) const;
    virtual UBool doParse(const UnicodeString& text, 
        ParsePosition& parsePosition,
        double baseValue,
        double upperBound,
        UBool ,
        Formattable& result) const;

    virtual double composeRuleValue(double newRuleValue, double oldRuleValue) const { return newRuleValue / oldRuleValue; }
    virtual double calcUpperBound(double ) const { return denominator; }
    virtual UChar tokenChar() const { return (UChar)0x003c; } 
private:
    static const UChar LTLT[2];

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

NumeratorSubstitution::~NumeratorSubstitution() {}

class NullSubstitution : public NFSubstitution {
public:
    NullSubstitution(int32_t _pos,
        const NFRuleSet* _ruleSet,
        const RuleBasedNumberFormat* formatter,
        const UnicodeString& description,
        UErrorCode& status)
        : NFSubstitution(_pos, _ruleSet, formatter, description, status) {}
    virtual ~NullSubstitution();

    virtual void toString(UnicodeString& ) const {}
    virtual void doSubstitution(double , UnicodeString& , int32_t ) const {}
    virtual void doSubstitution(int64_t , UnicodeString& , int32_t ) const {}
    virtual int64_t transformNumber(int64_t ) const { return 0; }
    virtual double transformNumber(double ) const { return 0; }
    virtual UBool doParse(const UnicodeString& ,
        ParsePosition& , 
        double baseValue,
        double ,
        UBool ,
        Formattable& result) const
    { result.setDouble(baseValue); return TRUE; }
    virtual double composeRuleValue(double , double ) const { return 0.0; } 
    virtual double calcUpperBound(double ) const { return 0; } 
    virtual UBool isNullSubstitution() const { return TRUE; }
    virtual UChar tokenChar() const { return (UChar)0x0020; } 

public:
    static UClassID getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

NullSubstitution::~NullSubstitution() {}

NFSubstitution*
NFSubstitution::makeSubstitution(int32_t pos,
                                 const NFRule* rule,
                                 const NFRule* predecessor,
                                 const NFRuleSet* ruleSet,
                                 const RuleBasedNumberFormat* formatter,
                                 const UnicodeString& description,
                                 UErrorCode& status)
{
    
    if (description.length() == 0) {
        return new NullSubstitution(pos, ruleSet, formatter, description, status);
    }

    switch (description.charAt(0)) {
        
    case gLessThan:
        
        
        if (rule->getBaseValue() == NFRule::kNegativeNumberRule) {
            
            status = U_PARSE_ERROR;
            return NULL;
        }

        
        
        else if (rule->getBaseValue() == NFRule::kImproperFractionRule
            || rule->getBaseValue() == NFRule::kProperFractionRule
            || rule->getBaseValue() == NFRule::kMasterRule) {
            return new IntegralPartSubstitution(pos, ruleSet, formatter, description, status);
        }

        
        
        else if (ruleSet->isFractionRuleSet()) {
            return new NumeratorSubstitution(pos, (double)rule->getBaseValue(),
                formatter->getDefaultRuleSet(), formatter, description, status);
        }

        
        else {
            return new MultiplierSubstitution(pos, rule->getDivisor(), ruleSet,
                formatter, description, status);
        }

        
    case gGreaterThan:
        
        
        if (rule->getBaseValue() == NFRule::kNegativeNumberRule) {
            return new AbsoluteValueSubstitution(pos, ruleSet, formatter, description, status);
        }

        
        
        else if (rule->getBaseValue() == NFRule::kImproperFractionRule
            || rule->getBaseValue() == NFRule::kProperFractionRule
            || rule->getBaseValue() == NFRule::kMasterRule) {
            return new FractionalPartSubstitution(pos, ruleSet, formatter, description, status);
        }

        
        
        else if (ruleSet->isFractionRuleSet()) {
            
            status = U_PARSE_ERROR;
            return NULL;
        }

        
        else {
            return new ModulusSubstitution(pos, rule->getDivisor(), predecessor,
                ruleSet, formatter, description, status);
        }

        
        
    case gEquals:
        return new SameValueSubstitution(pos, ruleSet, formatter, description, status);

        
    default:
        
        status = U_PARSE_ERROR;
    }
    return NULL;
}

NFSubstitution::NFSubstitution(int32_t _pos,
                               const NFRuleSet* _ruleSet,
                               const RuleBasedNumberFormat* formatter,
                               const UnicodeString& description,
                               UErrorCode& status)
                               : pos(_pos), ruleSet(NULL), numberFormat(NULL)
{
    
    
    
    
    UnicodeString workingDescription(description);
    if (description.length() >= 2
        && description.charAt(0) == description.charAt(description.length() - 1))
    {
        workingDescription.remove(description.length() - 1, 1);
        workingDescription.remove(0, 1);
    }
    else if (description.length() != 0) {
        
        status = U_PARSE_ERROR;
        return;
    }

    
    
    
    if (workingDescription.length() == 0) {
        this->ruleSet = _ruleSet;
    }
    
    
    
    else if (workingDescription.charAt(0) == gPercent) {
        this->ruleSet = formatter->findRuleSet(workingDescription, status);
    }
    
    
    
    
    else if (workingDescription.charAt(0) == gPound || workingDescription.charAt(0) ==gZero) {
        DecimalFormatSymbols* sym = formatter->getDecimalFormatSymbols();
        if (!sym) {
            status = U_MISSING_RESOURCE_ERROR;
            return;
        }
        this->numberFormat = new DecimalFormat(workingDescription, *sym, status);
        
        if (this->numberFormat == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        if (U_FAILURE(status)) {
            delete (DecimalFormat*)this->numberFormat;
            this->numberFormat = NULL;
            return;
        }
        
    }
    
    
    
    
    
    else if (workingDescription.charAt(0) == gGreaterThan) {
        
        
        this->ruleSet = _ruleSet;
        this->numberFormat = NULL;
    }
    
    else {
        
        status = U_PARSE_ERROR;
    }
}

NFSubstitution::~NFSubstitution()
{
  
  delete (NumberFormat*)numberFormat; numberFormat = NULL;
}








void
NFSubstitution::setDivisor(int32_t , int32_t , UErrorCode& ) {
  
}






UOBJECT_DEFINE_RTTI_IMPLEMENTATION(NFSubstitution)






UBool
NFSubstitution::operator==(const NFSubstitution& rhs) const
{
  
  
  
  return typeid(*this) == typeid(rhs)
  && pos == rhs.pos
  && (ruleSet == NULL) == (rhs.ruleSet == NULL)
  
  && (numberFormat == NULL
      ? (rhs.numberFormat == NULL)
      : (*numberFormat == *rhs.numberFormat));
}







void
NFSubstitution::toString(UnicodeString& text) const
{
  
  
  
  
  text.remove();
  text.append(tokenChar());

  UnicodeString temp;
  if (ruleSet != NULL) {
    ruleSet->getName(temp);
  } else if (numberFormat != NULL) {
    numberFormat->toPattern(temp);
  }
  text.append(temp);
  text.append(tokenChar());
}















void
NFSubstitution::doSubstitution(int64_t number, UnicodeString& toInsertInto, int32_t _pos) const
{
    if (ruleSet != NULL) {
        
        
        
        ruleSet->format(transformNumber(number), toInsertInto, _pos + this->pos);
    } else if (numberFormat != NULL) {
        
        
        
        
        double numberToFormat = transformNumber((double)number);
        if (numberFormat->getMaximumFractionDigits() == 0) {
            numberToFormat = uprv_floor(numberToFormat);
        }

        UnicodeString temp;
        numberFormat->format(numberToFormat, temp);
        toInsertInto.insert(_pos + this->pos, temp);
    }
}











void
NFSubstitution::doSubstitution(double number, UnicodeString& toInsertInto, int32_t _pos) const {
    
    
    double numberToFormat = transformNumber(number);

    
    
    if (numberToFormat == uprv_floor(numberToFormat) && ruleSet != NULL) {
        ruleSet->format(util64_fromDouble(numberToFormat), toInsertInto, _pos + this->pos);

        
        
        
    } else {
        if (ruleSet != NULL) {
            ruleSet->format(numberToFormat, toInsertInto, _pos + this->pos);
        } else if (numberFormat != NULL) {
            UnicodeString temp;
            numberFormat->format(numberToFormat, temp);
            toInsertInto.insert(_pos + this->pos, temp);
        }
    }
}


    
    
    

#ifdef RBNF_DEBUG
#include <stdio.h>
#endif






























UBool
NFSubstitution::doParse(const UnicodeString& text,
                        ParsePosition& parsePosition,
                        double baseValue,
                        double upperBound,
                        UBool lenientParse,
                        Formattable& result) const
{
#ifdef RBNF_DEBUG
    fprintf(stderr, "<nfsubs> %x bv: %g ub: %g\n", this, baseValue, upperBound);
#endif
    
    
    
    
    
    
    upperBound = calcUpperBound(upperBound);

    
    
    
    
    
    
    if (ruleSet != NULL) {
        ruleSet->parse(text, parsePosition, upperBound, result);
        if (lenientParse && !ruleSet->isFractionRuleSet() && parsePosition.getIndex() == 0) {
            UErrorCode status = U_ZERO_ERROR;
            NumberFormat* fmt = NumberFormat::createInstance(status);
            if (U_SUCCESS(status)) {
                fmt->parse(text, result, parsePosition);
            }
            delete fmt;
        }

        
    } else if (numberFormat != NULL) {
        numberFormat->parse(text, result, parsePosition);
    }

    
    
    
    
    if (parsePosition.getIndex() != 0) {
        UErrorCode status = U_ZERO_ERROR;
        double tempResult = result.getDouble(status);

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        tempResult = composeRuleValue(tempResult, baseValue);
        result.setDouble(tempResult);
        return TRUE;
        
    } else {
        result.setLong(0);
        return FALSE;
    }
}

UBool
NFSubstitution::isNullSubstitution() const {
    return FALSE;
}

    





UBool
NFSubstitution::isModulusSubstitution() const {
    return FALSE;
}









SameValueSubstitution::SameValueSubstitution(int32_t _pos,
                        const NFRuleSet* _ruleSet,
                        const RuleBasedNumberFormat* formatter,
                        const UnicodeString& description,
                        UErrorCode& status)
: NFSubstitution(_pos, _ruleSet, formatter, description, status)
{
    if (0 == description.compare(gEqualsEquals, 2)) {
        
        status = U_PARSE_ERROR;
    }
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SameValueSubstitution)





UOBJECT_DEFINE_RTTI_IMPLEMENTATION(MultiplierSubstitution)

UBool MultiplierSubstitution::operator==(const NFSubstitution& rhs) const
{
    return NFSubstitution::operator==(rhs) &&
        divisor == ((const MultiplierSubstitution*)&rhs)->divisor;
}











ModulusSubstitution::ModulusSubstitution(int32_t _pos,
                                         double _divisor,
                                         const NFRule* predecessor,
                                         const NFRuleSet* _ruleSet,
                                         const RuleBasedNumberFormat* formatter,
                                         const UnicodeString& description,
                                         UErrorCode& status)
 : NFSubstitution(_pos, _ruleSet, formatter, description, status)
 , divisor(_divisor)
 , ruleToUse(NULL)
{
  ldivisor = util64_fromDouble(_divisor);

  
  
  

  if (ldivisor == 0) {
      status = U_PARSE_ERROR;
  }

  if (0 == description.compare(gGreaterGreaterGreaterThan, 3)) {
    
    
    
    
    
    ruleToUse = predecessor;
  }
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ModulusSubstitution)

UBool ModulusSubstitution::operator==(const NFSubstitution& rhs) const
{
  return NFSubstitution::operator==(rhs) &&
  divisor == ((const ModulusSubstitution*)&rhs)->divisor &&
  ruleToUse == ((const ModulusSubstitution*)&rhs)->ruleToUse;
}














void
ModulusSubstitution::doSubstitution(int64_t number, UnicodeString& toInsertInto, int32_t _pos) const
{
    
    
    
    if (ruleToUse == NULL) {
        NFSubstitution::doSubstitution(number, toInsertInto, _pos);

        
        
    } else {
        int64_t numberToFormat = transformNumber(number);
        ruleToUse->doFormat(numberToFormat, toInsertInto, _pos + getPos());
    }
}









void
ModulusSubstitution::doSubstitution(double number, UnicodeString& toInsertInto, int32_t _pos) const
{
    
    
    
    if (ruleToUse == NULL) {
        NFSubstitution::doSubstitution(number, toInsertInto, _pos);

        
        
    } else {
        double numberToFormat = transformNumber(number);

        ruleToUse->doFormat(numberToFormat, toInsertInto, _pos + getPos());
    }
}














UBool
ModulusSubstitution::doParse(const UnicodeString& text,
                             ParsePosition& parsePosition,
                             double baseValue,
                             double upperBound,
                             UBool lenientParse,
                             Formattable& result) const
{
    
    
    if (ruleToUse == NULL) {
        return NFSubstitution::doParse(text, parsePosition, baseValue, upperBound, lenientParse, result);

        
        
        
    } else {
        ruleToUse->doParse(text, parsePosition, FALSE, upperBound, result);

        if (parsePosition.getIndex() != 0) {
            UErrorCode status = U_ZERO_ERROR;
            double tempResult = result.getDouble(status);
            tempResult = composeRuleValue(tempResult, baseValue);
            result.setDouble(tempResult);
        }

        return TRUE;
    }
}






void
ModulusSubstitution::toString(UnicodeString& text) const
{
  
  
  
  

  if ( ruleToUse != NULL ) { 
      text.remove();
      text.append(tokenChar());
      text.append(tokenChar());
      text.append(tokenChar());
  } else { 
	  NFSubstitution::toString(text);
  }
}




UOBJECT_DEFINE_RTTI_IMPLEMENTATION(IntegralPartSubstitution)







    




FractionalPartSubstitution::FractionalPartSubstitution(int32_t _pos,
                             const NFRuleSet* _ruleSet,
                             const RuleBasedNumberFormat* formatter,
                             const UnicodeString& description,
                             UErrorCode& status)
 : NFSubstitution(_pos, _ruleSet, formatter, description, status)
 , byDigits(FALSE)
 , useSpaces(TRUE)

{
    
    if (0 == description.compare(gGreaterGreaterThan, 2) ||
        0 == description.compare(gGreaterGreaterGreaterThan, 3) ||
        _ruleSet == getRuleSet()) {
        byDigits = TRUE;
        if (0 == description.compare(gGreaterGreaterGreaterThan, 3)) {
            useSpaces = FALSE;
        }
    } else {
        
        ((NFRuleSet*)getRuleSet())->makeIntoFractionRuleSet();
    }
}















void
FractionalPartSubstitution::doSubstitution(double number, UnicodeString& toInsertInto, int32_t _pos) const
{
  
  
  if (!byDigits) {
    NFSubstitution::doSubstitution(number, toInsertInto, _pos);

    
    
    
    
    
    
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    DigitList dl;
    dl.set(number);
    dl.roundFixedPoint(20);     
    dl.reduce();                
    
    UBool pad = FALSE;
    for (int32_t didx = dl.getCount()-1; didx>=dl.getDecimalAt(); didx--) {
      
      
      
      if (pad && useSpaces) {
        toInsertInto.insert(_pos + getPos(), gSpace);
      } else {
        pad = TRUE;
      }
      int64_t digit = didx>=0 ? dl.getDigit(didx) - '0' : 0;
      getRuleSet()->format(digit, toInsertInto, _pos + getPos());
    }

    if (!pad) {
      
      
      getRuleSet()->format((int64_t)0, toInsertInto, _pos + getPos());
    }
  }
}






















UBool
FractionalPartSubstitution::doParse(const UnicodeString& text,
                ParsePosition& parsePosition,
                double baseValue,
                double ,
                UBool lenientParse,
                Formattable& resVal) const
{
    
    
    if (!byDigits) {
        return NFSubstitution::doParse(text, parsePosition, baseValue, 0, lenientParse, resVal);

        
        
        
        
    } else {
        UnicodeString workText(text);
        ParsePosition workPos(1);
        double result = 0;
        int32_t digit;


        DigitList dl;
        NumberFormat* fmt = NULL;
        while (workText.length() > 0 && workPos.getIndex() != 0) {
            workPos.setIndex(0);
            Formattable temp;
            getRuleSet()->parse(workText, workPos, 10, temp);
            UErrorCode status = U_ZERO_ERROR;
            digit = temp.getLong(status);




            if (lenientParse && workPos.getIndex() == 0) {
                if (!fmt) {
                    status = U_ZERO_ERROR;
                    fmt = NumberFormat::createInstance(status);
                    if (U_FAILURE(status)) {
                        delete fmt;
                        fmt = NULL;
                    }
                }
                if (fmt) {
                    fmt->parse(workText, temp, workPos);
                    digit = temp.getLong(status);
                }
            }

            if (workPos.getIndex() != 0) {
                dl.append((char)('0' + digit));


                parsePosition.setIndex(parsePosition.getIndex() + workPos.getIndex());
                workText.removeBetween(0, workPos.getIndex());
                while (workText.length() > 0 && workText.charAt(0) == gSpace) {
                    workText.removeBetween(0, 1);
                    parsePosition.setIndex(parsePosition.getIndex() + 1);
                }
            }
        }
        delete fmt;

        result = dl.getCount() == 0 ? 0 : dl.getDouble();
        result = composeRuleValue(result, baseValue);
        resVal.setDouble(result);
        return TRUE;
    }
}

UBool
FractionalPartSubstitution::operator==(const NFSubstitution& rhs) const
{
  return NFSubstitution::operator==(rhs) &&
  ((const FractionalPartSubstitution*)&rhs)->byDigits == byDigits;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(FractionalPartSubstitution)






UOBJECT_DEFINE_RTTI_IMPLEMENTATION(AbsoluteValueSubstitution)





void
NumeratorSubstitution::doSubstitution(double number, UnicodeString& toInsertInto, int32_t apos) const {
    
    

    double numberToFormat = transformNumber(number);
    int64_t longNF = util64_fromDouble(numberToFormat);

    const NFRuleSet* aruleSet = getRuleSet();
    if (withZeros && aruleSet != NULL) {
        
        int64_t nf =longNF;
        int32_t len = toInsertInto.length();
        while ((nf *= 10) < denominator) {
            toInsertInto.insert(apos + getPos(), gSpace);
            aruleSet->format((int64_t)0, toInsertInto, apos + getPos());
        }
        apos += toInsertInto.length() - len;
    }

    
    
    if (numberToFormat == longNF && aruleSet != NULL) {
        aruleSet->format(longNF, toInsertInto, apos + getPos());

        
        
        
    } else {
        if (aruleSet != NULL) {
            aruleSet->format(numberToFormat, toInsertInto, apos + getPos());
        } else {
            UErrorCode status = U_ZERO_ERROR;
            UnicodeString temp;
            getNumberFormat()->format(numberToFormat, temp, status);
            toInsertInto.insert(apos + getPos(), temp);
        }
    }
}

UBool 
NumeratorSubstitution::doParse(const UnicodeString& text, 
                               ParsePosition& parsePosition,
                               double baseValue,
                               double upperBound,
                               UBool ,
                               Formattable& result) const
{
    
    
    

    
    
    UErrorCode status = U_ZERO_ERROR;
    int32_t zeroCount = 0;
    UnicodeString workText(text);

    if (withZeros) {
        ParsePosition workPos(1);
        Formattable temp;

        while (workText.length() > 0 && workPos.getIndex() != 0) {
            workPos.setIndex(0);
            getRuleSet()->parse(workText, workPos, 1, temp); 
            if (workPos.getIndex() == 0) {
                
                
                break;
            }

            ++zeroCount;
            parsePosition.setIndex(parsePosition.getIndex() + workPos.getIndex());
            workText.remove(0, workPos.getIndex());
            while (workText.length() > 0 && workText.charAt(0) == gSpace) {
                workText.remove(0, 1);
                parsePosition.setIndex(parsePosition.getIndex() + 1);
            }
        }

        workText = text;
        workText.remove(0, (int32_t)parsePosition.getIndex());
        parsePosition.setIndex(0);
    }

    
    NFSubstitution::doParse(workText, parsePosition, withZeros ? 1 : baseValue, upperBound, FALSE, result);

    if (withZeros) {
        
        

        
        int64_t n = result.getLong(status); 
        int64_t d = 1;
        int32_t pow = 0;
        while (d <= n) {
            d *= 10;
            ++pow;
        }
        
        while (zeroCount > 0) {
            d *= 10;
            --zeroCount;
        }
        
        result.setDouble((double)n/(double)d);
    }

    return TRUE;
}

UBool
NumeratorSubstitution::operator==(const NFSubstitution& rhs) const
{
    return NFSubstitution::operator==(rhs) &&
        denominator == ((const NumeratorSubstitution*)&rhs)->denominator;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(NumeratorSubstitution)

const UChar NumeratorSubstitution::LTLT[] = { 0x003c, 0x003c };
        




UOBJECT_DEFINE_RTTI_IMPLEMENTATION(NullSubstitution)

U_NAMESPACE_END


#endif

