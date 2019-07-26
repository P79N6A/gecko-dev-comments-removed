














#include "nfrule.h"

#if U_HAVE_RBNF

#include "unicode/rbnf.h"
#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "unicode/uchar.h"
#include "nfrs.h"
#include "nfrlist.h"
#include "nfsubs.h"
#include "patternprops.h"

U_NAMESPACE_BEGIN

NFRule::NFRule(const RuleBasedNumberFormat* _rbnf)
  : baseValue((int32_t)0)
  , radix(0)
  , exponent(0)
  , ruleText()
  , sub1(NULL)
  , sub2(NULL)
  , formatter(_rbnf)
{
}

NFRule::~NFRule()
{
  delete sub1;
  delete sub2;
}

static const UChar gLeftBracket = 0x005b;
static const UChar gRightBracket = 0x005d;
static const UChar gColon = 0x003a;
static const UChar gZero = 0x0030;
static const UChar gNine = 0x0039;
static const UChar gSpace = 0x0020;
static const UChar gSlash = 0x002f;
static const UChar gGreaterThan = 0x003e;
static const UChar gLessThan = 0x003c;
static const UChar gComma = 0x002c;
static const UChar gDot = 0x002e;
static const UChar gTick = 0x0027;

static const UChar gSemicolon = 0x003b;

static const UChar gMinusX[] =                  {0x2D, 0x78, 0};    
static const UChar gXDotX[] =                   {0x78, 0x2E, 0x78, 0}; 
static const UChar gXDotZero[] =                {0x78, 0x2E, 0x30, 0}; 
static const UChar gZeroDotX[] =                {0x30, 0x2E, 0x78, 0}; 

static const UChar gLessLess[] =                {0x3C, 0x3C, 0};    
static const UChar gLessPercent[] =             {0x3C, 0x25, 0};    
static const UChar gLessHash[] =                {0x3C, 0x23, 0};    
static const UChar gLessZero[] =                {0x3C, 0x30, 0};    
static const UChar gGreaterGreater[] =          {0x3E, 0x3E, 0};    
static const UChar gGreaterPercent[] =          {0x3E, 0x25, 0};    
static const UChar gGreaterHash[] =             {0x3E, 0x23, 0};    
static const UChar gGreaterZero[] =             {0x3E, 0x30, 0};    
static const UChar gEqualPercent[] =            {0x3D, 0x25, 0};    
static const UChar gEqualHash[] =               {0x3D, 0x23, 0};    
static const UChar gEqualZero[] =               {0x3D, 0x30, 0};    
static const UChar gGreaterGreaterGreater[] =   {0x3E, 0x3E, 0x3E, 0}; 

static const UChar * const tokenStrings[] = {
    gLessLess, gLessPercent, gLessHash, gLessZero,
    gGreaterGreater, gGreaterPercent,gGreaterHash, gGreaterZero,
    gEqualPercent, gEqualHash, gEqualZero, NULL
};

void
NFRule::makeRules(UnicodeString& description,
                  const NFRuleSet *ruleSet,
                  const NFRule *predecessor,
                  const RuleBasedNumberFormat *rbnf,
                  NFRuleList& rules,
                  UErrorCode& status)
{
    
    
    
    
    NFRule* rule1 = new NFRule(rbnf);
    
    if (rule1 == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    rule1->parseRuleDescriptor(description, status);

    
    
    int32_t brack1 = description.indexOf(gLeftBracket);
    int32_t brack2 = description.indexOf(gRightBracket);

    
    
    
    
    if (brack1 == -1 || brack2 == -1 || brack1 > brack2
        || rule1->getType() == kProperFractionRule
        || rule1->getType() == kNegativeNumberRule) {
        rule1->ruleText = description;
        rule1->extractSubstitutions(ruleSet, predecessor, rbnf, status);
        rules.add(rule1);
    } else {
        
        
        NFRule* rule2 = NULL;
        UnicodeString sbuf;

        
        
        
        if ((rule1->baseValue > 0
            && (rule1->baseValue % util64_pow(rule1->radix, rule1->exponent)) == 0)
            || rule1->getType() == kImproperFractionRule
            || rule1->getType() == kMasterRule) {

            
            
            
            
            
            rule2 = new NFRule(rbnf);
            
            if (rule2 == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            if (rule1->baseValue >= 0) {
                rule2->baseValue = rule1->baseValue;
                if (!ruleSet->isFractionRuleSet()) {
                    ++rule1->baseValue;
                }
            }

            
            
            
            else if (rule1->getType() == kImproperFractionRule) {
                rule2->setType(kProperFractionRule);
            }

            
            
            
            else if (rule1->getType() == kMasterRule) {
                rule2->baseValue = rule1->baseValue;
                rule1->setType(kImproperFractionRule);
            }

            
            
            rule2->radix = rule1->radix;
            rule2->exponent = rule1->exponent;

            
            
            sbuf.append(description, 0, brack1);
            if (brack2 + 1 < description.length()) {
                sbuf.append(description, brack2 + 1, description.length() - brack2 - 1);
            }
            rule2->ruleText.setTo(sbuf);
            rule2->extractSubstitutions(ruleSet, predecessor, rbnf, status);
        }

        
        
        
        sbuf.setTo(description, 0, brack1);
        sbuf.append(description, brack1 + 1, brack2 - brack1 - 1);
        if (brack2 + 1 < description.length()) {
            sbuf.append(description, brack2 + 1, description.length() - brack2 - 1);
        }
        rule1->ruleText.setTo(sbuf);
        rule1->extractSubstitutions(ruleSet, predecessor, rbnf, status);

        
        
        
        
        
        if (rule2 != NULL) {
            rules.add(rule2);
        }
        rules.add(rule1);
    }
}













void
NFRule::parseRuleDescriptor(UnicodeString& description, UErrorCode& status)
{
    
    
    
    int32_t p = description.indexOf(gColon);
    if (p == -1) {
        setBaseValue((int32_t)0, status);
    } else {
        
        
        
        UnicodeString descriptor;
        descriptor.setTo(description, 0, p);

        ++p;
        while (p < description.length() && PatternProps::isWhiteSpace(description.charAt(p))) {
            ++p;
        }
        description.removeBetween(0, p);

        
        
        
        if (0 == descriptor.compare(gMinusX, 2)) {
            setType(kNegativeNumberRule);
        }
        else if (0 == descriptor.compare(gXDotX, 3)) {
            setType(kImproperFractionRule);
        }
        else if (0 == descriptor.compare(gZeroDotX, 3)) {
            setType(kProperFractionRule);
        }
        else if (0 == descriptor.compare(gXDotZero, 3)) {
            setType(kMasterRule);
        }

        
        
        
        
        else if (descriptor.charAt(0) >= gZero && descriptor.charAt(0) <= gNine) {
            int64_t val = 0;
            p = 0;
            UChar c = gSpace;

            
            
            
            
            int64_t ll_10 = 10;
            while (p < descriptor.length()) {
                c = descriptor.charAt(p);
                if (c >= gZero && c <= gNine) {
                    val = val * ll_10 + (int32_t)(c - gZero);
                }
                else if (c == gSlash || c == gGreaterThan) {
                    break;
                }
                else if (PatternProps::isWhiteSpace(c) || c == gComma || c == gDot) {
                }
                else {
                    
                    status = U_PARSE_ERROR;
                    return;
                }
                ++p;
            }

            
            setBaseValue(val, status);

            
            
            
            
            if (c == gSlash) {
                val = 0;
                ++p;
                int64_t ll_10 = 10;
                while (p < descriptor.length()) {
                    c = descriptor.charAt(p);
                    if (c >= gZero && c <= gNine) {
                        val = val * ll_10 + (int32_t)(c - gZero);
                    }
                    else if (c == gGreaterThan) {
                        break;
                    }
                    else if (PatternProps::isWhiteSpace(c) || c == gComma || c == gDot) {
                    }
                    else {
                        
                        status = U_PARSE_ERROR;
                        return;
                    }
                    ++p;
                }

                
                
                radix = (int32_t)val;
                if (radix == 0) {
                    
                    status = U_PARSE_ERROR;
                }

                exponent = expectedExponent();
            }

            
            
            
            
            
            if (c == gGreaterThan) {
                while (p < descriptor.length()) {
                    c = descriptor.charAt(p);
                    if (c == gGreaterThan && exponent > 0) {
                        --exponent;
                    } else {
                        
                        status = U_PARSE_ERROR;
                        return;
                    }
                    ++p;
                }
            }
        }
    }

    
    
    
    if (description.length() > 0 && description.charAt(0) == gTick) {
        description.removeBetween(0, 1);
    }

    
    
    
}









void
NFRule::extractSubstitutions(const NFRuleSet* ruleSet,
                             const NFRule* predecessor,
                             const RuleBasedNumberFormat* rbnf,
                             UErrorCode& status)
{
    if (U_SUCCESS(status)) {
        sub1 = extractSubstitution(ruleSet, predecessor, rbnf, status);
        sub2 = extractSubstitution(ruleSet, predecessor, rbnf, status);
    }
}













NFSubstitution *
NFRule::extractSubstitution(const NFRuleSet* ruleSet,
                            const NFRule* predecessor,
                            const RuleBasedNumberFormat* rbnf,
                            UErrorCode& status)
{
    NFSubstitution* result = NULL;

    
    
    int32_t subStart = indexOfAny(tokenStrings);
    int32_t subEnd = subStart;

    
    
    if (subStart == -1) {
        return NFSubstitution::makeSubstitution(ruleText.length(), this, predecessor,
            ruleSet, rbnf, UnicodeString(), status);
    }

    
    
    if (ruleText.indexOf(gGreaterGreaterGreater, 3, 0) == subStart) {
        subEnd = subStart + 2;

        
        
    } else {
        UChar c = ruleText.charAt(subStart);
        subEnd = ruleText.indexOf(c, subStart + 1);
        
        if (c == gLessThan && subEnd != -1 && subEnd < ruleText.length() - 1 && ruleText.charAt(subEnd+1) == c) {
            
            
            
            
            ++subEnd;
        }
   }

    
    
    
    if (subEnd == -1) {
        return NFSubstitution::makeSubstitution(ruleText.length(), this, predecessor,
            ruleSet, rbnf, UnicodeString(), status);
    }

    
    
    
    UnicodeString subToken;
    subToken.setTo(ruleText, subStart, subEnd + 1 - subStart);
    result = NFSubstitution::makeSubstitution(subStart, this, predecessor, ruleSet,
        rbnf, subToken, status);

    
    ruleText.removeBetween(subStart, subEnd+1);

    return result;
}








void
NFRule::setBaseValue(int64_t newBaseValue, UErrorCode& status)
{
    
    baseValue = newBaseValue;

    
    
    
    
    
    if (baseValue >= 1) {
        radix = 10;
        exponent = expectedExponent();

        
        
        
        
        if (sub1 != NULL) {
            sub1->setDivisor(radix, exponent, status);
        }
        if (sub2 != NULL) {
            sub2->setDivisor(radix, exponent, status);
        }

        
        
    } else {
        radix = 10;
        exponent = 0;
    }
}






int16_t
NFRule::expectedExponent() const
{
    
    
    
    if (radix == 0 || baseValue < 1) {
        return 0;
    }

    
    
    
    int16_t tempResult = (int16_t)(uprv_log((double)baseValue) / uprv_log((double)radix));
    int64_t temp = util64_pow(radix, tempResult + 1);
    if (temp <= baseValue) {
        tempResult += 1;
    }
    return tempResult;
}










int32_t
NFRule::indexOfAny(const UChar* const strings[]) const
{
    int result = -1;
    for (int i = 0; strings[i]; i++) {
        int32_t pos = ruleText.indexOf(*strings[i]);
        if (pos != -1 && (result == -1 || pos < result)) {
            result = pos;
        }
    }
    return result;
}










UBool
NFRule::operator==(const NFRule& rhs) const
{
    return baseValue == rhs.baseValue
        && radix == rhs.radix
        && exponent == rhs.exponent
        && ruleText == rhs.ruleText
        && *sub1 == *rhs.sub1
        && *sub2 == *rhs.sub2;
}







static void util_append64(UnicodeString& result, int64_t n)
{
    UChar buffer[256];
    int32_t len = util64_tou(n, buffer, sizeof(buffer));
    UnicodeString temp(buffer, len);
    result.append(temp);
}

void
NFRule::_appendRuleText(UnicodeString& result) const
{
    switch (getType()) {
    case kNegativeNumberRule: result.append(gMinusX, 2); break;
    case kImproperFractionRule: result.append(gXDotX, 3); break;
    case kProperFractionRule: result.append(gZeroDotX, 3); break;
    case kMasterRule: result.append(gXDotZero, 3); break;
    default:
        
        
        
        
        
        
        util_append64(result, baseValue);
        if (radix != 10) {
            result.append(gSlash);
            util_append64(result, radix);
        }
        int numCarets = expectedExponent() - exponent;
        for (int i = 0; i < numCarets; i++) {
            result.append(gGreaterThan);
        }
        break;
    }
    result.append(gColon);
    result.append(gSpace);

    
    
    
    if (ruleText.charAt(0) == gSpace && sub1->getPos() != 0) {
        result.append(gTick);
    }

    
    
    UnicodeString ruleTextCopy;
    ruleTextCopy.setTo(ruleText);

    UnicodeString temp;
    sub2->toString(temp);
    ruleTextCopy.insert(sub2->getPos(), temp);
    sub1->toString(temp);
    ruleTextCopy.insert(sub1->getPos(), temp);

    result.append(ruleTextCopy);

    
    
    result.append(gSemicolon);
}














void
NFRule::doFormat(int64_t number, UnicodeString& toInsertInto, int32_t pos) const
{
    
    
    
    
    
    toInsertInto.insert(pos, ruleText);
    sub2->doSubstitution(number, toInsertInto, pos);
    sub1->doSubstitution(number, toInsertInto, pos);
}










void
NFRule::doFormat(double number, UnicodeString& toInsertInto, int32_t pos) const
{
    
    
    
    
    
    
    toInsertInto.insert(pos, ruleText);
    sub2->doSubstitution(number, toInsertInto, pos);
    sub1->doSubstitution(number, toInsertInto, pos);
}









UBool
NFRule::shouldRollBack(double number) const
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if ((sub1->isModulusSubstitution()) || (sub2->isModulusSubstitution())) {
        int64_t re = util64_pow(radix, exponent);
        return uprv_fmod(number, (double)re) == 0 && (baseValue % re) != 0;
    }
    return FALSE;
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
NFRule::doParse(const UnicodeString& text,
                ParsePosition& parsePosition,
                UBool isFractionRule,
                double upperBound,
                Formattable& resVal) const
{
    
    
    ParsePosition pp;
    UnicodeString workText(text);

    
    
    
    
    UnicodeString prefix;
    prefix.setTo(ruleText, 0, sub1->getPos());

#ifdef RBNF_DEBUG
    fprintf(stderr, "doParse %x ", this);
    {
        UnicodeString rt;
        _appendRuleText(rt);
        dumpUS(stderr, rt);
    }

    fprintf(stderr, " text: '", this);
    dumpUS(stderr, text);
    fprintf(stderr, "' prefix: '");
    dumpUS(stderr, prefix);
#endif
    stripPrefix(workText, prefix, pp);
    int32_t prefixLength = text.length() - workText.length();

#ifdef RBNF_DEBUG
    fprintf(stderr, "' pl: %d ppi: %d s1p: %d\n", prefixLength, pp.getIndex(), sub1->getPos());
#endif

    if (pp.getIndex() == 0 && sub1->getPos() != 0) {
        
        
        parsePosition.setErrorIndex(pp.getErrorIndex());
        resVal.setLong(0);
        return TRUE;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int highWaterMark = 0;
    double result = 0;
    int start = 0;
    double tempBaseValue = (double)(baseValue <= 0 ? 0 : baseValue);

    UnicodeString temp;
    do {
        
        
        
        
        pp.setIndex(0);

        temp.setTo(ruleText, sub1->getPos(), sub2->getPos() - sub1->getPos());
        double partialResult = matchToDelimiter(workText, start, tempBaseValue,
            temp, pp, sub1,
            upperBound);

        
        
        
        
        if (pp.getIndex() != 0 || sub1->isNullSubstitution()) {
            start = pp.getIndex();

            UnicodeString workText2;
            workText2.setTo(workText, pp.getIndex(), workText.length() - pp.getIndex());
            ParsePosition pp2;

            
            
            
            
            temp.setTo(ruleText, sub2->getPos(), ruleText.length() - sub2->getPos());
            partialResult = matchToDelimiter(workText2, 0, partialResult,
                temp, pp2, sub2,
                upperBound);

            
            
            
            if (pp2.getIndex() != 0 || sub2->isNullSubstitution()) {
                if (prefixLength + pp.getIndex() + pp2.getIndex() > highWaterMark) {
                    highWaterMark = prefixLength + pp.getIndex() + pp2.getIndex();
                    result = partialResult;
                }
            }
            
            
            else {
                int32_t temp = pp2.getErrorIndex() + sub1->getPos() + pp.getIndex();
                if (temp> parsePosition.getErrorIndex()) {
                    parsePosition.setErrorIndex(temp);
                }
            }
        }
        
        
        else {
            int32_t temp = sub1->getPos() + pp.getErrorIndex();
            if (temp > parsePosition.getErrorIndex()) {
                parsePosition.setErrorIndex(temp);
            }
        }
        
        
        
    } while (sub1->getPos() != sub2->getPos()
        && pp.getIndex() > 0
        && pp.getIndex() < workText.length()
        && pp.getIndex() != start);

    
    
    
    
    parsePosition.setIndex(highWaterMark);
    
    
    if (highWaterMark > 0) {
        parsePosition.setErrorIndex(0);
    }

    
    
    
    
    
    
    if (isFractionRule &&
        highWaterMark > 0 &&
        sub1->isNullSubstitution()) {
        result = 1 / result;
    }

    resVal.setDouble(result);
    return TRUE; 
}

















void
NFRule::stripPrefix(UnicodeString& text, const UnicodeString& prefix, ParsePosition& pp) const
{
    
    if (prefix.length() != 0) {
    	UErrorCode status = U_ZERO_ERROR;
        
        
        
        
        int32_t pfl = prefixLength(text, prefix, status);
        if (U_FAILURE(status)) { 
        	return;
        }
        if (pfl != 0) {
            
            
            pp.setIndex(pp.getIndex() + pfl);
            text.remove(0, pfl);
        }
    }
}






























double
NFRule::matchToDelimiter(const UnicodeString& text,
                         int32_t startPos,
                         double _baseValue,
                         const UnicodeString& delimiter,
                         ParsePosition& pp,
                         const NFSubstitution* sub,
                         double upperBound) const
{
	UErrorCode status = U_ZERO_ERROR;
    
    
    
    
    if (!allIgnorable(delimiter, status)) {
    	if (U_FAILURE(status)) { 
    		return 0;
    	}
        ParsePosition tempPP;
        Formattable result;

        
        
        
        
        int32_t dLen;
        int32_t dPos = findText(text, delimiter, startPos, &dLen);

        
        
        while (dPos >= 0) {
            UnicodeString subText;
            subText.setTo(text, 0, dPos);
            if (subText.length() > 0) {
                UBool success = sub->doParse(subText, tempPP, _baseValue, upperBound,
#if UCONFIG_NO_COLLATION
                    FALSE,
#else
                    formatter->isLenient(),
#endif
                    result);

                
                
                
                
                
                
                if (success && tempPP.getIndex() == dPos) {
                    pp.setIndex(dPos + dLen);
                    return result.getDouble();
                }
                
                
                else {
                    if (tempPP.getErrorIndex() > 0) {
                        pp.setErrorIndex(tempPP.getErrorIndex());
                    } else {
                        pp.setErrorIndex(tempPP.getIndex());
                    }
                }
            }

            
            
            
            tempPP.setIndex(0);
            dPos = findText(text, delimiter, dPos + dLen, &dLen);
        }
        
        
        pp.setIndex(0);
        return 0;

        
        
        
        
    } else {
        ParsePosition tempPP;
        Formattable result;

        
        UBool success = sub->doParse(text, tempPP, _baseValue, upperBound,
#if UCONFIG_NO_COLLATION
            FALSE,
#else
            formatter->isLenient(),
#endif
            result);
        if (success && (tempPP.getIndex() != 0 || sub->isNullSubstitution())) {
            
            
            
            
            pp.setIndex(tempPP.getIndex());
            return result.getDouble();
        }
        
        
        else {
            pp.setErrorIndex(tempPP.getErrorIndex());
        }

        
        
        return 0;
    }
}















int32_t
NFRule::prefixLength(const UnicodeString& str, const UnicodeString& prefix, UErrorCode& status) const
{
    
    
    if (prefix.length() == 0) {
        return 0;
    }

#if !UCONFIG_NO_COLLATION
    
    if (formatter->isLenient()) {
        
        
        
        
        
        
        
        RuleBasedCollator* collator = (RuleBasedCollator*)formatter->getCollator();
        CollationElementIterator* strIter = collator->createCollationElementIterator(str);
        CollationElementIterator* prefixIter = collator->createCollationElementIterator(prefix);
        
        if (collator == NULL || strIter == NULL || prefixIter == NULL) {
        	delete collator;
        	delete strIter;
        	delete prefixIter;
        	status = U_MEMORY_ALLOCATION_ERROR;
        	return 0;
        }

        UErrorCode err = U_ZERO_ERROR;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        
        int32_t oStr = strIter->next(err);
        int32_t oPrefix = prefixIter->next(err);

        while (oPrefix != CollationElementIterator::NULLORDER) {
            
            while (CollationElementIterator::primaryOrder(oStr) == 0
                && oStr != CollationElementIterator::NULLORDER) {
                oStr = strIter->next(err);
            }

            
            while (CollationElementIterator::primaryOrder(oPrefix) == 0
                && oPrefix != CollationElementIterator::NULLORDER) {
                oPrefix = prefixIter->next(err);
            }

            
            
            

            
            
            if (oPrefix == CollationElementIterator::NULLORDER) {
                break;
            }

            
            
            if (oStr == CollationElementIterator::NULLORDER) {
                delete prefixIter;
                delete strIter;
                return 0;
            }

            
            
            
            if (CollationElementIterator::primaryOrder(oStr)
                != CollationElementIterator::primaryOrder(oPrefix)) {
                delete prefixIter;
                delete strIter;
                return 0;

                
                
                
            } else {
                oStr = strIter->next(err);
                oPrefix = prefixIter->next(err);
            }
        }

        int32_t result = strIter->getOffset();
        if (oStr != CollationElementIterator::NULLORDER) {
            --result; 
        }

#ifdef RBNF_DEBUG
        fprintf(stderr, "prefix length: %d\n", result);
#endif
        delete prefixIter;
        delete strIter;

        return result;
#if 0
        
        
        
        
        

        
        
        
        
        
        
        
        
        
        collator->setStrength(Collator::PRIMARY);
        if (str.length() >= prefix.length()) {
            UnicodeString temp;
            temp.setTo(str, 0, prefix.length());
            if (collator->equals(temp, prefix)) {
#ifdef RBNF_DEBUG
                fprintf(stderr, "returning: %d\n", prefix.length());
#endif
                return prefix.length();
            }
        }

        
        
        
        
        int32_t p = 1;
        while (p <= str.length()) {
            UnicodeString temp;
            temp.setTo(str, 0, p);
            if (collator->equals(temp, prefix)) {
                return p;
            } else {
                ++p;
            }
        }

        
        return 0;
        
#endif

        
        
  } else
#endif
  {
      if (str.startsWith(prefix)) {
          return prefix.length();
      } else {
          return 0;
      }
  }
}
















int32_t
NFRule::findText(const UnicodeString& str,
                 const UnicodeString& key,
                 int32_t startingAt,
                 int32_t* length) const
{
#if !UCONFIG_NO_COLLATION
    
    
    if (!formatter->isLenient()) {
        *length = key.length();
        return str.indexOf(key, startingAt);

        
        
    } else
#endif
    {
        
        

        
        
        
        
        
        int32_t p = startingAt;
        int32_t keyLen = 0;

        
        
        
        
        
        
        
        UnicodeString temp;
        UErrorCode status = U_ZERO_ERROR;
        while (p < str.length() && keyLen == 0) {
            temp.setTo(str, p, str.length() - p);
            keyLen = prefixLength(temp, key, status);
            if (U_FAILURE(status)) {
            	break;
            }
            if (keyLen != 0) {
                *length = keyLen;
                return p;
            }
            ++p;
        }
        
        
        
        *length = 0;
        return -1;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
    }
}









UBool
NFRule::allIgnorable(const UnicodeString& str, UErrorCode& status) const
{
    
    if (str.length() == 0) {
        return TRUE;
    }

#if !UCONFIG_NO_COLLATION
    
    
    
    if (formatter->isLenient()) {
        RuleBasedCollator* collator = (RuleBasedCollator*)(formatter->getCollator());
        CollationElementIterator* iter = collator->createCollationElementIterator(str);
        
        
        if (collator == NULL || iter == NULL) {
        	delete collator;
        	delete iter;
        	status = U_MEMORY_ALLOCATION_ERROR;
        	return FALSE;
        }

        UErrorCode err = U_ZERO_ERROR;
        int32_t o = iter->next(err);
        while (o != CollationElementIterator::NULLORDER
            && CollationElementIterator::primaryOrder(o) == 0) {
            o = iter->next(err);
        }

        delete iter;
        return o == CollationElementIterator::NULLORDER;
    }
#endif

    
    
    return FALSE;
}

U_NAMESPACE_END


#endif


