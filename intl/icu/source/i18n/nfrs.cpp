














#include "nfrs.h"

#if U_HAVE_RBNF

#include "unicode/uchar.h"
#include "nfrule.h"
#include "nfrlist.h"
#include "patternprops.h"

#ifdef RBNF_DEBUG
#include "cmemory.h"
#endif

U_NAMESPACE_BEGIN

#if 0





static int64_t
util_lcm(int64_t x, int64_t y)
{
    x.abs();
    y.abs();

    if (x == 0 || y == 0) {
        return 0;
    } else {
        do {
            if (x < y) {
                int64_t t = x; x = y; y = t;
            }
            x -= y * (x/y);
        } while (x != 0);

        return y;
    }
}

#else



static int64_t
util_lcm(int64_t x, int64_t y)
{
    
    
    int64_t x1 = x;
    int64_t y1 = y;

    int p2 = 0;
    while ((x1 & 1) == 0 && (y1 & 1) == 0) {
        ++p2;
        x1 >>= 1;
        y1 >>= 1;
    }

    int64_t t;
    if ((x1 & 1) == 1) {
        t = -y1;
    } else {
        t = x1;
    }

    while (t != 0) {
        while ((t & 1) == 0) {
            t = t >> 1;
        }
        if (t > 0) {
            x1 = t;
        } else {
            y1 = -t;
        }
        t = x1 - y1;
    }

    int64_t gcd = x1 << p2;

    
    return x / gcd * y;
}
#endif

static const UChar gPercent = 0x0025;
static const UChar gColon = 0x003a;
static const UChar gSemicolon = 0x003b;
static const UChar gLineFeed = 0x000a;

static const UChar gFourSpaces[] =
{
    0x20, 0x20, 0x20, 0x20, 0
}; 
static const UChar gPercentPercent[] =
{
    0x25, 0x25, 0
}; 

static const UChar gNoparse[] =
{
    0x40, 0x6E, 0x6F, 0x70, 0x61, 0x72, 0x73, 0x65, 0
}; 

NFRuleSet::NFRuleSet(UnicodeString* descriptions, int32_t index, UErrorCode& status)
  : name()
  , rules(0)
  , negativeNumberRule(NULL)
  , fIsFractionRuleSet(FALSE)
  , fIsPublic(FALSE)
  , fIsParseable(TRUE)
  , fRecursionCount(0)
{
    for (int i = 0; i < 3; ++i) {
        fractionRules[i] = NULL;
    }

    if (U_FAILURE(status)) {
        return;
    }

    UnicodeString& description = descriptions[index]; 

    if (description.length() == 0) {
        
        status = U_PARSE_ERROR;
        return;
    }

    
    
    
    
    if (description.charAt(0) == gPercent) {
        int32_t pos = description.indexOf(gColon);
        if (pos == -1) {
            
            status = U_PARSE_ERROR;
        } else {
            name.setTo(description, 0, pos);
            while (pos < description.length() && PatternProps::isWhiteSpace(description.charAt(++pos))) {
            }
            description.remove(0, pos);
        }
    } else {
        name.setTo(UNICODE_STRING_SIMPLE("%default"));
    }

    if (description.length() == 0) {
        
        status = U_PARSE_ERROR;
    }

    fIsPublic = name.indexOf(gPercentPercent, 2, 0) != 0;

    if ( name.endsWith(gNoparse,8) ) {
        fIsParseable = FALSE;
        name.truncate(name.length()-8); 
    }

    
    
}

void
NFRuleSet::parseRules(UnicodeString& description, const RuleBasedNumberFormat* owner, UErrorCode& status)
{
    
    
    
    

    if (U_FAILURE(status)) {
        return;
    }

    
    rules.deleteAll();

    
    

    UnicodeString currentDescription;
    int32_t oldP = 0;
    while (oldP < description.length()) {
        int32_t p = description.indexOf(gSemicolon, oldP);
        if (p == -1) {
            p = description.length();
        }
        currentDescription.setTo(description, oldP, p - oldP);
        NFRule::makeRules(currentDescription, this, rules.last(), owner, rules, status);
        oldP = p + 1;
    }

    
    
    
    
    int64_t defaultBaseValue = 0;

    
    
    
    uint32_t i = 0;
    while (i < rules.size()) {
        NFRule* rule = rules[i];

        switch (rule->getType()) {
            
            
            
            
            
        case NFRule::kNoBase:
            rule->setBaseValue(defaultBaseValue, status);
            if (!isFractionRuleSet()) {
                ++defaultBaseValue;
            }
            ++i;
            break;

            
            
        case NFRule::kNegativeNumberRule:
            if (negativeNumberRule) {
                delete negativeNumberRule;
            }
            negativeNumberRule = rules.remove(i);
            break;

            
            
        case NFRule::kImproperFractionRule:
            if (fractionRules[0]) {
                delete fractionRules[0];
            }
            fractionRules[0] = rules.remove(i);
            break;

            
            
        case NFRule::kProperFractionRule:
            if (fractionRules[1]) {
                delete fractionRules[1];
            }
            fractionRules[1] = rules.remove(i);
            break;

            
            
        case NFRule::kMasterRule:
            if (fractionRules[2]) {
                delete fractionRules[2];
            }
            fractionRules[2] = rules.remove(i);
            break;

            
            
            
        default:
            if (rule->getBaseValue() < defaultBaseValue) {
                
                status = U_PARSE_ERROR;
                return;
            }
            defaultBaseValue = rule->getBaseValue();
            if (!isFractionRuleSet()) {
                ++defaultBaseValue;
            }
            ++i;
            break;
        }
    }
}

NFRuleSet::~NFRuleSet()
{
    delete negativeNumberRule;
    delete fractionRules[0];
    delete fractionRules[1];
    delete fractionRules[2];
}

static UBool
util_equalRules(const NFRule* rule1, const NFRule* rule2)
{
    if (rule1) {
        if (rule2) {
            return *rule1 == *rule2;
        }
    } else if (!rule2) {
        return TRUE;
    }
    return FALSE;
}

UBool
NFRuleSet::operator==(const NFRuleSet& rhs) const
{
    if (rules.size() == rhs.rules.size() &&
        fIsFractionRuleSet == rhs.fIsFractionRuleSet &&
        name == rhs.name &&
        util_equalRules(negativeNumberRule, rhs.negativeNumberRule) &&
        util_equalRules(fractionRules[0], rhs.fractionRules[0]) &&
        util_equalRules(fractionRules[1], rhs.fractionRules[1]) &&
        util_equalRules(fractionRules[2], rhs.fractionRules[2])) {

        for (uint32_t i = 0; i < rules.size(); ++i) {
            if (*rules[i] != *rhs.rules[i]) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

#define RECURSION_LIMIT 50

void
NFRuleSet::format(int64_t number, UnicodeString& toAppendTo, int32_t pos) const
{
    NFRule *rule = findNormalRule(number);
    if (rule) { 
        NFRuleSet* ncThis = (NFRuleSet*)this;
        if (ncThis->fRecursionCount++ >= RECURSION_LIMIT) {
            
            ncThis->fRecursionCount = 0;
        } else {
            rule->doFormat(number, toAppendTo, pos);
            ncThis->fRecursionCount--;
        }
    }
}

void
NFRuleSet::format(double number, UnicodeString& toAppendTo, int32_t pos) const
{
    NFRule *rule = findDoubleRule(number);
    if (rule) { 
        NFRuleSet* ncThis = (NFRuleSet*)this;
        if (ncThis->fRecursionCount++ >= RECURSION_LIMIT) {
            
            ncThis->fRecursionCount = 0;
        } else {
            rule->doFormat(number, toAppendTo, pos);
            ncThis->fRecursionCount--;
        }
    }
}

NFRule*
NFRuleSet::findDoubleRule(double number) const
{
    
    if (isFractionRuleSet()) {
        return findFractionRuleSetRule(number);
    }

    
    
    
    if (number < 0) {
        if (negativeNumberRule) {
            return  negativeNumberRule;
        } else {
            number = -number;
        }
    }

    
    if (number != uprv_floor(number)) {
        
        
        if (number < 1 && fractionRules[1]) {
            return fractionRules[1];
        }
        
        else if (fractionRules[0]) {
            return fractionRules[0];
        }
    }

    
    if (fractionRules[2]) {
        return fractionRules[2];
    }

    
    
    int64_t r = util64_fromDouble(number + 0.5);
    return findNormalRule(r);
}

NFRule *
NFRuleSet::findNormalRule(int64_t number) const
{
    
    
    
    if (fIsFractionRuleSet) {
        return findFractionRuleSetRule((double)number);
    }

    
    
    if (number < 0) {
        if (negativeNumberRule) {
            return negativeNumberRule;
        } else {
            number = -number;
        }
    }

    
    
    
    
    
    
    
    

    
    

    
    
    
    int32_t hi = rules.size();
    if (hi > 0) {
        int32_t lo = 0;

        while (lo < hi) {
            int32_t mid = (lo + hi) / 2;
            if (rules[mid]->getBaseValue() == number) {
                return rules[mid];
            }
            else if (rules[mid]->getBaseValue() > number) {
                hi = mid;
            }
            else {
                lo = mid + 1;
            }
        }
        if (hi == 0) { 
            return NULL; 
        }

        NFRule *result = rules[hi - 1];

        
        
        
        
        
        if (result->shouldRollBack((double)number)) {
            if (hi == 1) { 
                return NULL;
            }
            result = rules[hi - 2];
        }
        return result;
    }
    
    return fractionRules[2];
}
















NFRule*
NFRuleSet::findFractionRuleSetRule(double number) const
{
    
    
    
    

    
    
    
    
    int64_t leastCommonMultiple = rules[0]->getBaseValue();
    int64_t numerator;
    {
        for (uint32_t i = 1; i < rules.size(); ++i) {
            leastCommonMultiple = util_lcm(leastCommonMultiple, rules[i]->getBaseValue());
        }
        numerator = util64_fromDouble(number * (double)leastCommonMultiple + 0.5);
    }
    
    int64_t tempDifference;
    int64_t difference = util64_fromDouble(uprv_maxMantissa());
    int32_t winner = 0;
    for (uint32_t i = 0; i < rules.size(); ++i) {
        
        
        
        
        
        
        tempDifference = numerator * rules[i]->getBaseValue() % leastCommonMultiple;


        
        
        
        if (leastCommonMultiple - tempDifference < tempDifference) {
            tempDifference = leastCommonMultiple - tempDifference;
        }

        
        
        
        
        if (tempDifference < difference) {
            difference = tempDifference;
            winner = i;
            if (difference == 0) {
                break;
            }
        }
    }

    
    
    
    
    
    
    if ((unsigned)(winner + 1) < rules.size() &&
        rules[winner + 1]->getBaseValue() == rules[winner]->getBaseValue()) {
        double n = ((double)rules[winner]->getBaseValue()) * number;
        if (n < 0.5 || n >= 2) {
            ++winner;
        }
    }

    
    return rules[winner];
}






















#ifdef RBNF_DEBUG
#include <stdio.h>

static void dumpUS(FILE* f, const UnicodeString& us) {
  int len = us.length();
  char* buf = (char *)uprv_malloc((len+1)*sizeof(char)); 
  if (buf != NULL) {
	  us.extract(0, len, buf);
	  buf[len] = 0;
	  fprintf(f, "%s", buf);
	  uprv_free(buf); 
  }
}
#endif

UBool
NFRuleSet::parse(const UnicodeString& text, ParsePosition& pos, double upperBound, Formattable& result) const
{
    
    
    

    result.setLong(0);

    
    if (text.length() == 0) {
        return 0;
    }

    ParsePosition highWaterMark;
    ParsePosition workingPos = pos;

#ifdef RBNF_DEBUG
    fprintf(stderr, "<nfrs> %x '", this);
    dumpUS(stderr, name);
    fprintf(stderr, "' text '");
    dumpUS(stderr, text);
    fprintf(stderr, "'\n");
    fprintf(stderr, "  parse negative: %d\n", this, negativeNumberRule != 0);
#endif

    
    if (negativeNumberRule) {
        Formattable tempResult;
#ifdef RBNF_DEBUG
        fprintf(stderr, "  <nfrs before negative> %x ub: %g\n", negativeNumberRule, upperBound);
#endif
        UBool success = negativeNumberRule->doParse(text, workingPos, 0, upperBound, tempResult);
#ifdef RBNF_DEBUG
        fprintf(stderr, "  <nfrs after negative> success: %d wpi: %d\n", success, workingPos.getIndex());
#endif
        if (success && workingPos.getIndex() > highWaterMark.getIndex()) {
            result = tempResult;
            highWaterMark = workingPos;
        }
        workingPos = pos;
    }
#ifdef RBNF_DEBUG
    fprintf(stderr, "<nfrs> continue fractional with text '");
    dumpUS(stderr, text);
    fprintf(stderr, "' hwm: %d\n", highWaterMark.getIndex());
#endif
    
    {
        for (int i = 0; i < 3; i++) {
            if (fractionRules[i]) {
                Formattable tempResult;
                UBool success = fractionRules[i]->doParse(text, workingPos, 0, upperBound, tempResult);
                if (success && (workingPos.getIndex() > highWaterMark.getIndex())) {
                    result = tempResult;
                    highWaterMark = workingPos;
                }
                workingPos = pos;
            }
        }
    }
#ifdef RBNF_DEBUG
    fprintf(stderr, "<nfrs> continue other with text '");
    dumpUS(stderr, text);
    fprintf(stderr, "' hwm: %d\n", highWaterMark.getIndex());
#endif

    
    
    
    
    
    
    
    
    
    {
        int64_t ub = util64_fromDouble(upperBound);
#ifdef RBNF_DEBUG
        {
            char ubstr[64];
            util64_toa(ub, ubstr, 64);
            char ubstrhex[64];
            util64_toa(ub, ubstrhex, 64, 16);
            fprintf(stderr, "ub: %g, i64: %s (%s)\n", upperBound, ubstr, ubstrhex);
        }
#endif
        for (int32_t i = rules.size(); --i >= 0 && highWaterMark.getIndex() < text.length();) {
            if ((!fIsFractionRuleSet) && (rules[i]->getBaseValue() >= ub)) {
                continue;
            }
            Formattable tempResult;
            UBool success = rules[i]->doParse(text, workingPos, fIsFractionRuleSet, upperBound, tempResult);
            if (success && workingPos.getIndex() > highWaterMark.getIndex()) {
                result = tempResult;
                highWaterMark = workingPos;
            }
            workingPos = pos;
        }
    }
#ifdef RBNF_DEBUG
    fprintf(stderr, "<nfrs> exit\n");
#endif
    
    
    
    pos = highWaterMark;

    return 1;
}

void
NFRuleSet::appendRules(UnicodeString& result) const
{
    
    result.append(name);
    result.append(gColon);
    result.append(gLineFeed);

    
    for (uint32_t i = 0; i < rules.size(); i++) {
        result.append(gFourSpaces, 4);
        rules[i]->_appendRuleText(result);
        result.append(gLineFeed);
    }

    
    if (negativeNumberRule) {
        result.append(gFourSpaces, 4);
        negativeNumberRule->_appendRuleText(result);
        result.append(gLineFeed);
    }

    {
        for (uint32_t i = 0; i < 3; ++i) {
            if (fractionRules[i]) {
                result.append(gFourSpaces, 4);
                fractionRules[i]->_appendRuleText(result);
                result.append(gLineFeed);
            }
        }
    }
}



int64_t util64_fromDouble(double d) {
    int64_t result = 0;
    if (!uprv_isNaN(d)) {
        double mant = uprv_maxMantissa();
        if (d < -mant) {
            d = -mant;
        } else if (d > mant) {
            d = mant;
        }
        UBool neg = d < 0; 
        if (neg) {
            d = -d;
        }
        result = (int64_t)uprv_floor(d);
        if (neg) {
            result = -result;
        }
    }
    return result;
}

int64_t util64_pow(int32_t r, uint32_t e)  { 
    if (r == 0) {
        return 0;
    } else if (e == 0) {
        return 1;
    } else {
        int64_t n = r;
        while (--e > 0) {
            n *= r;
        }
        return n;
    }
}

static const uint8_t asciiDigits[] = { 
    0x30u, 0x31u, 0x32u, 0x33u, 0x34u, 0x35u, 0x36u, 0x37u,
    0x38u, 0x39u, 0x61u, 0x62u, 0x63u, 0x64u, 0x65u, 0x66u,
    0x67u, 0x68u, 0x69u, 0x6au, 0x6bu, 0x6cu, 0x6du, 0x6eu,
    0x6fu, 0x70u, 0x71u, 0x72u, 0x73u, 0x74u, 0x75u, 0x76u,
    0x77u, 0x78u, 0x79u, 0x7au,  
};

static const UChar kUMinus = (UChar)0x002d;

#ifdef RBNF_DEBUG
static const char kMinus = '-';

static const uint8_t digitInfo[] = {
        0,     0,     0,     0,     0,     0,     0,     0,
        0,     0,     0,     0,     0,     0,     0,     0,
        0,     0,     0,     0,     0,     0,     0,     0,
        0,     0,     0,     0,     0,     0,     0,     0,
        0,     0,     0,     0,     0,     0,     0,     0,
        0,     0,     0,     0,     0,     0,     0,     0,
    0x80u, 0x81u, 0x82u, 0x83u, 0x84u, 0x85u, 0x86u, 0x87u,
    0x88u, 0x89u,     0,     0,     0,     0,     0,     0,
        0, 0x8au, 0x8bu, 0x8cu, 0x8du, 0x8eu, 0x8fu, 0x90u,
    0x91u, 0x92u, 0x93u, 0x94u, 0x95u, 0x96u, 0x97u, 0x98u,
    0x99u, 0x9au, 0x9bu, 0x9cu, 0x9du, 0x9eu, 0x9fu, 0xa0u,
    0xa1u, 0xa2u, 0xa3u,     0,     0,     0,     0,     0,
        0, 0x8au, 0x8bu, 0x8cu, 0x8du, 0x8eu, 0x8fu, 0x90u,
    0x91u, 0x92u, 0x93u, 0x94u, 0x95u, 0x96u, 0x97u, 0x98u,
    0x99u, 0x9au, 0x9bu, 0x9cu, 0x9du, 0x9eu, 0x9fu, 0xa0u,
    0xa1u, 0xa2u, 0xa3u,     0,     0,     0,     0,     0,
};

int64_t util64_atoi(const char* str, uint32_t radix)
{
    if (radix > 36) {
        radix = 36;
    } else if (radix < 2) {
        radix = 2;
    }
    int64_t lradix = radix;

    int neg = 0;
    if (*str == kMinus) {
        ++str;
        neg = 1;
    }
    int64_t result = 0;
    uint8_t b;
    while ((b = digitInfo[*str++]) && ((b &= 0x7f) < radix)) {
        result *= lradix;
        result += (int32_t)b;
    }
    if (neg) {
        result = -result;
    }
    return result;
}

int64_t util64_utoi(const UChar* str, uint32_t radix)
{
    if (radix > 36) {
        radix = 36;
    } else if (radix < 2) {
        radix = 2;
    }
    int64_t lradix = radix;

    int neg = 0;
    if (*str == kUMinus) {
        ++str;
        neg = 1;
    }
    int64_t result = 0;
    UChar c;
    uint8_t b;
    while (((c = *str++) < 0x0080) && (b = digitInfo[c]) && ((b &= 0x7f) < radix)) {
        result *= lradix;
        result += (int32_t)b;
    }
    if (neg) {
        result = -result;
    }
    return result;
}

uint32_t util64_toa(int64_t w, char* buf, uint32_t len, uint32_t radix, UBool raw)
{    
    if (radix > 36) {
        radix = 36;
    } else if (radix < 2) {
        radix = 2;
    }
    int64_t base = radix;

    char* p = buf;
    if (len && (w < 0) && (radix == 10) && !raw) {
        w = -w;
        *p++ = kMinus;
        --len;
    } else if (len && (w == 0)) {
        *p++ = (char)raw ? 0 : asciiDigits[0];
        --len;
    }

    while (len && w != 0) {
        int64_t n = w / base;
        int64_t m = n * base;
        int32_t d = (int32_t)(w-m);
        *p++ = raw ? (char)d : asciiDigits[d];
        w = n;
        --len;
    }
    if (len) {
        *p = 0; 
    }

    len = p - buf;
    if (*buf == kMinus) {
        ++buf;
    }
    while (--p > buf) {
        char c = *p;
        *p = *buf;
        *buf = c;
        ++buf;
    }

    return len;
}
#endif

uint32_t util64_tou(int64_t w, UChar* buf, uint32_t len, uint32_t radix, UBool raw)
{    
    if (radix > 36) {
        radix = 36;
    } else if (radix < 2) {
        radix = 2;
    }
    int64_t base = radix;

    UChar* p = buf;
    if (len && (w < 0) && (radix == 10) && !raw) {
        w = -w;
        *p++ = kUMinus;
        --len;
    } else if (len && (w == 0)) {
        *p++ = (UChar)raw ? 0 : asciiDigits[0];
        --len;
    }

    while (len && (w != 0)) {
        int64_t n = w / base;
        int64_t m = n * base;
        int32_t d = (int32_t)(w-m);
        *p++ = (UChar)(raw ? d : asciiDigits[d]);
        w = n;
        --len;
    }
    if (len) {
        *p = 0; 
    }

    len = (uint32_t)(p - buf);
    if (*buf == kUMinus) {
        ++buf;
    }
    while (--p > buf) {
        UChar c = *p;
        *p = *buf;
        *buf = c;
        ++buf;
    }

    return len;
}


U_NAMESPACE_END


#endif

