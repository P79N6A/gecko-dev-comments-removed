






#include "uassert.h"
#include "decimalformatpattern.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dcfmtsym.h"
#include "unicode/format.h"
#include "unicode/utf16.h"

#ifdef FMT_DEBUG
#define debug(x) printf("%s:%d: %s\n", __FILE__,__LINE__, x);
#else
#define debug(x)
#endif

#define kPatternZeroDigit            ((UChar)0x0030) /*'0'*/
#define kPatternSignificantDigit     ((UChar)0x0040) /*'@'*/
#define kPatternGroupingSeparator    ((UChar)0x002C) /*','*/
#define kPatternDecimalSeparator     ((UChar)0x002E) /*'.'*/
#define kPatternPerMill              ((UChar)0x2030)
#define kPatternPercent              ((UChar)0x0025) /*'%'*/
#define kPatternDigit                ((UChar)0x0023) /*'#'*/
#define kPatternSeparator            ((UChar)0x003B) /*';'*/
#define kPatternExponent             ((UChar)0x0045) /*'E'*/
#define kPatternPlus                 ((UChar)0x002B) /*'+'*/
#define kPatternMinus                ((UChar)0x002D) /*'-'*/
#define kPatternPadEscape            ((UChar)0x002A) /*'*'*/
#define kQuote                       ((UChar)0x0027) /*'\''*/

#define kCurrencySign                ((UChar)0x00A4)
#define kDefaultPad                  ((UChar)0x0020) /* */

U_NAMESPACE_BEGIN


static int32_t kDoubleIntegerDigits  = 309;
static int32_t kDoubleFractionDigits = 340;



static int32_t gDefaultMaxIntegerDigits = 2000000000;


static void syntaxError(const UnicodeString& pattern,
                         int32_t pos,
                         UParseError& parseError) {
    parseError.offset = pos;
    parseError.line=0;  

    
    int32_t start = (pos < U_PARSE_CONTEXT_LEN)? 0 : (pos - (U_PARSE_CONTEXT_LEN-1
                                                             ));
    int32_t stop  = pos;
    pattern.extract(start,stop-start,parseError.preContext,0);
    
    parseError.preContext[stop-start] = 0;

    
    start = pos+1;
    stop  = ((pos+U_PARSE_CONTEXT_LEN)<=pattern.length()) ? (pos+(U_PARSE_CONTEXT_LEN-1)) :
        pattern.length();
    pattern.extract(start,stop-start,parseError.postContext,0);
    
    parseError.postContext[stop-start]= 0;
}

DecimalFormatPattern::DecimalFormatPattern()
        : fMinimumIntegerDigits(1),
          fMaximumIntegerDigits(gDefaultMaxIntegerDigits),
          fMinimumFractionDigits(0),
          fMaximumFractionDigits(3),
          fUseSignificantDigits(FALSE),
          fMinimumSignificantDigits(1),
          fMaximumSignificantDigits(6),
          fUseExponentialNotation(FALSE),
          fMinExponentDigits(0),
          fExponentSignAlwaysShown(FALSE),
          fCurrencySignCount(fgCurrencySignCountZero),
          fGroupingUsed(TRUE),
          fGroupingSize(0),
          fGroupingSize2(0),
          fMultiplier(1),
          fDecimalSeparatorAlwaysShown(FALSE),
          fFormatWidth(0),
          fRoundingIncrementUsed(FALSE),
          fRoundingIncrement(),
          fPad(kPatternPadEscape),
          fNegPatternsBogus(TRUE),
          fPosPatternsBogus(TRUE),
          fNegPrefixPattern(),
          fNegSuffixPattern(),
          fPosPrefixPattern(),
          fPosSuffixPattern(),
          fPadPosition(DecimalFormatPattern::kPadBeforePrefix) {
}


DecimalFormatPatternParser::DecimalFormatPatternParser() :
    fZeroDigit(kPatternZeroDigit),
    fSigDigit(kPatternSignificantDigit),
    fGroupingSeparator((UChar)kPatternGroupingSeparator),
    fDecimalSeparator((UChar)kPatternDecimalSeparator),
    fPercent((UChar)kPatternPercent),
    fPerMill((UChar)kPatternPerMill),
    fDigit((UChar)kPatternDigit),
    fSeparator((UChar)kPatternSeparator),
    fExponent((UChar)kPatternExponent),
    fPlus((UChar)kPatternPlus),
    fMinus((UChar)kPatternMinus),
    fPadEscape((UChar)kPatternPadEscape) {
}

void DecimalFormatPatternParser::useSymbols(
        const DecimalFormatSymbols& symbols) {
    fZeroDigit = symbols.getConstSymbol(
            DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
    fSigDigit = symbols.getConstSymbol(
            DecimalFormatSymbols::kSignificantDigitSymbol).char32At(0);
    fGroupingSeparator = symbols.getConstSymbol(
            DecimalFormatSymbols::kGroupingSeparatorSymbol);
    fDecimalSeparator = symbols.getConstSymbol(
            DecimalFormatSymbols::kDecimalSeparatorSymbol);
    fPercent = symbols.getConstSymbol(
            DecimalFormatSymbols::kPercentSymbol);
    fPerMill = symbols.getConstSymbol(
            DecimalFormatSymbols::kPerMillSymbol);
    fDigit = symbols.getConstSymbol(
            DecimalFormatSymbols::kDigitSymbol);
    fSeparator = symbols.getConstSymbol(
            DecimalFormatSymbols::kPatternSeparatorSymbol);
    fExponent = symbols.getConstSymbol(
            DecimalFormatSymbols::kExponentialSymbol);
    fPlus = symbols.getConstSymbol(
            DecimalFormatSymbols::kPlusSignSymbol);
    fMinus = symbols.getConstSymbol(
            DecimalFormatSymbols::kMinusSignSymbol);
    fPadEscape = symbols.getConstSymbol(
            DecimalFormatSymbols::kPadEscapeSymbol);
}

void
DecimalFormatPatternParser::applyPatternWithoutExpandAffix(
        const UnicodeString& pattern,
        DecimalFormatPattern& out,
        UParseError& parseError,
        UErrorCode& status) {
    if (U_FAILURE(status))
    {
        return;
    }
    out = DecimalFormatPattern();

    
    parseError.offset = -1;
    parseError.preContext[0] = parseError.postContext[0] = (UChar)0;

    
    UChar nineDigit = (UChar)(fZeroDigit + 9);
    int32_t digitLen = fDigit.length();
    int32_t groupSepLen = fGroupingSeparator.length();
    int32_t decimalSepLen = fDecimalSeparator.length();

    int32_t pos = 0;
    int32_t patLen = pattern.length();
    
    
    for (int32_t part=0; part<2 && pos<patLen; ++part) {
        
        
        
        
        
        int32_t subpart = 1, sub0Start = 0, sub0Limit = 0, sub2Limit = 0;

        
        
        
        
        
        
        UnicodeString prefix;
        UnicodeString suffix;
        int32_t decimalPos = -1;
        int32_t multiplier = 1;
        int32_t digitLeftCount = 0, zeroDigitCount = 0, digitRightCount = 0, sigDigitCount = 0;
        int8_t groupingCount = -1;
        int8_t groupingCount2 = -1;
        int32_t padPos = -1;
        UChar32 padChar = 0;
        int32_t roundingPos = -1;
        DigitList roundingInc;
        int8_t expDigits = -1;
        UBool expSignAlways = FALSE;

        
        UnicodeString* affix = &prefix;

        int32_t start = pos;
        UBool isPartDone = FALSE;
        UChar32 ch;

        for (; !isPartDone && pos < patLen; ) {
            
            ch = pattern.char32At(pos);
            switch (subpart) {
            case 0: 
                
                
                
                
                
                
                
                
                
                if (pattern.compare(pos, digitLen, fDigit) == 0) {
                    if (zeroDigitCount > 0 || sigDigitCount > 0) {
                        ++digitRightCount;
                    } else {
                        ++digitLeftCount;
                    }
                    if (groupingCount >= 0 && decimalPos < 0) {
                        ++groupingCount;
                    }
                    pos += digitLen;
                } else if ((ch >= fZeroDigit && ch <= nineDigit) ||
                           ch == fSigDigit) {
                    if (digitRightCount > 0) {
                        
                        debug("Unexpected '0'")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    if (ch == fSigDigit) {
                        ++sigDigitCount;
                    } else {
                        if (ch != fZeroDigit && roundingPos < 0) {
                            roundingPos = digitLeftCount + zeroDigitCount;
                        }
                        if (roundingPos >= 0) {
                            roundingInc.append((char)(ch - fZeroDigit + '0'));
                        }
                        ++zeroDigitCount;
                    }
                    if (groupingCount >= 0 && decimalPos < 0) {
                        ++groupingCount;
                    }
                    pos += U16_LENGTH(ch);
                } else if (pattern.compare(pos, groupSepLen, fGroupingSeparator) == 0) {
                    if (decimalPos >= 0) {
                        
                        debug("Grouping separator after decimal")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    groupingCount2 = groupingCount;
                    groupingCount = 0;
                    pos += groupSepLen;
                } else if (pattern.compare(pos, decimalSepLen, fDecimalSeparator) == 0) {
                    if (decimalPos >= 0) {
                        
                        debug("Multiple decimal separators")
                        status = U_MULTIPLE_DECIMAL_SEPARATORS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    
                    
                    
                    decimalPos = digitLeftCount + zeroDigitCount + digitRightCount;
                    pos += decimalSepLen;
                } else {
                    if (pattern.compare(pos, fExponent.length(), fExponent) == 0) {
                        if (expDigits >= 0) {
                            
                            debug("Multiple exponential symbols")
                            status = U_MULTIPLE_EXPONENTIAL_SYMBOLS;
                            syntaxError(pattern,pos,parseError);
                            return;
                        }
                        if (groupingCount >= 0) {
                            
                            debug("Grouping separator in exponential pattern")
                            status = U_MALFORMED_EXPONENTIAL_PATTERN;
                            syntaxError(pattern,pos,parseError);
                            return;
                        }
                        pos += fExponent.length();
                        
                        if (pos < patLen
                            && pattern.compare(pos, fPlus.length(), fPlus) == 0) {
                            expSignAlways = TRUE;
                            pos += fPlus.length();
                        }
                        
                        
                        expDigits = 0;
                        while (pos < patLen &&
                               pattern.char32At(pos) == fZeroDigit) {
                            ++expDigits;
                            pos += U16_LENGTH(fZeroDigit);
                        }

                        
                        
                        
                        if (((digitLeftCount + zeroDigitCount) < 1 &&
                             (sigDigitCount + digitRightCount) < 1) ||
                            (sigDigitCount > 0 && digitLeftCount > 0) ||
                            expDigits < 1) {
                            
                            debug("Malformed exponential pattern")
                            status = U_MALFORMED_EXPONENTIAL_PATTERN;
                            syntaxError(pattern,pos,parseError);
                            return;
                        }
                    }
                    
                    subpart = 2; 
                    affix = &suffix;
                    sub0Limit = pos;
                    continue;
                }
                break;
            case 1: 
            case 2: 
                
                
                

                
                
                
                if (!pattern.compare(pos, digitLen, fDigit) ||
                    !pattern.compare(pos, groupSepLen, fGroupingSeparator) ||
                    !pattern.compare(pos, decimalSepLen, fDecimalSeparator) ||
                    (ch >= fZeroDigit && ch <= nineDigit) ||
                    ch == fSigDigit) {
                    if (subpart == 1) { 
                        subpart = 0; 
                        sub0Start = pos; 
                        continue;
                    } else {
                        status = U_UNQUOTED_SPECIAL;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                } else if (ch == kCurrencySign) {
                    affix->append(kQuote); 
                    
                    
                    U_ASSERT(U16_LENGTH(kCurrencySign) == 1);
                    if ((pos+1) < pattern.length() && pattern[pos+1] == kCurrencySign) {
                        affix->append(kCurrencySign);
                        ++pos; 
                        if ((pos+1) < pattern.length() &&
                            pattern[pos+1] == kCurrencySign) {
                            affix->append(kCurrencySign);
                            ++pos; 
                            out.fCurrencySignCount = fgCurrencySignCountInPluralFormat;
                        } else {
                            out.fCurrencySignCount = fgCurrencySignCountInISOFormat;
                        }
                    } else {
                        out.fCurrencySignCount = fgCurrencySignCountInSymbolFormat;
                    }
                    
                } else if (ch == kQuote) {
                    
                    
                    
                    U_ASSERT(U16_LENGTH(kQuote) == 1);
                    ++pos;
                    if (pos < pattern.length() && pattern[pos] == kQuote) {
                        affix->append(kQuote); 
                        
                    } else {
                        subpart += 2; 
                        continue;
                    }
                } else if (pattern.compare(pos, fSeparator.length(), fSeparator) == 0) {
                    
                    
                    if (subpart == 1 || part == 1) {
                        
                        debug("Unexpected separator")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    sub2Limit = pos;
                    isPartDone = TRUE; 
                    pos += fSeparator.length();
                    break;
                } else if (pattern.compare(pos, fPercent.length(), fPercent) == 0) {
                    
                    if (multiplier != 1) {
                        
                        debug("Too many percent characters")
                        status = U_MULTIPLE_PERCENT_SYMBOLS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    affix->append(kQuote); 
                    affix->append(kPatternPercent); 
                    multiplier = 100;
                    pos += fPercent.length();
                    break;
                } else if (pattern.compare(pos, fPerMill.length(), fPerMill) == 0) {
                    
                    if (multiplier != 1) {
                        
                        debug("Too many perMill characters")
                        status = U_MULTIPLE_PERMILL_SYMBOLS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    affix->append(kQuote); 
                    affix->append(kPatternPerMill); 
                    multiplier = 1000;
                    pos += fPerMill.length();
                    break;
                } else if (pattern.compare(pos, fPadEscape.length(), fPadEscape) == 0) {
                    if (padPos >= 0 ||               
                        (pos+1) == pattern.length()) { 
                        debug("Multiple pad specifiers")
                        status = U_MULTIPLE_PAD_SPECIFIERS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    padPos = pos;
                    pos += fPadEscape.length();
                    padChar = pattern.char32At(pos);
                    pos += U16_LENGTH(padChar);
                    break;
                } else if (pattern.compare(pos, fMinus.length(), fMinus) == 0) {
                    affix->append(kQuote); 
                    affix->append(kPatternMinus);
                    pos += fMinus.length();
                    break;
                } else if (pattern.compare(pos, fPlus.length(), fPlus) == 0) {
                    affix->append(kQuote); 
                    affix->append(kPatternPlus);
                    pos += fPlus.length();
                    break;
                }
                
                
                
                affix->append(ch);
                pos += U16_LENGTH(ch);
                break;
            case 3: 
            case 4: 
                
                
                
                if (ch == kQuote) {
                    ++pos;
                    if (pos < pattern.length() && pattern[pos] == kQuote) {
                        affix->append(kQuote); 
                        
                    } else {
                        subpart -= 2; 
                        continue;
                    }
                }
                affix->append(ch);
                pos += U16_LENGTH(ch);
                break;
            }
        }

        if (sub0Limit == 0) {
            sub0Limit = pattern.length();
        }

        if (sub2Limit == 0) {
            sub2Limit = pattern.length();
        }

        












        if (zeroDigitCount == 0 && sigDigitCount == 0 &&
            digitLeftCount > 0 && decimalPos >= 0) {
            
            int n = decimalPos;
            if (n == 0)
                ++n; 
            digitRightCount = digitLeftCount - n;
            digitLeftCount = n - 1;
            zeroDigitCount = 1;
        }

        
        if ((decimalPos < 0 && digitRightCount > 0 && sigDigitCount == 0) ||
            (decimalPos >= 0 &&
             (sigDigitCount > 0 ||
              decimalPos < digitLeftCount ||
              decimalPos > (digitLeftCount + zeroDigitCount))) ||
            groupingCount == 0 || groupingCount2 == 0 ||
            (sigDigitCount > 0 && zeroDigitCount > 0) ||
            subpart > 2)
        { 
            debug("Syntax error")
            status = U_PATTERN_SYNTAX_ERROR;
            syntaxError(pattern,pos,parseError);
            return;
        }

        
        if (padPos >= 0) {
            if (padPos == start) {
                padPos = DecimalFormatPattern::kPadBeforePrefix;
            } else if (padPos+2 == sub0Start) {
                padPos = DecimalFormatPattern::kPadAfterPrefix;
            } else if (padPos == sub0Limit) {
                padPos = DecimalFormatPattern::kPadBeforeSuffix;
            } else if (padPos+2 == sub2Limit) {
                padPos = DecimalFormatPattern::kPadAfterSuffix;
            } else {
                
                debug("Illegal pad position")
                status = U_ILLEGAL_PAD_POSITION;
                syntaxError(pattern,pos,parseError);
                return;
            }
        }

        if (part == 0) {
            out.fPosPatternsBogus = FALSE;
            out.fPosPrefixPattern = prefix;
            out.fPosSuffixPattern = suffix;
            out.fNegPatternsBogus = TRUE;
            out.fNegPrefixPattern.remove();
            out.fNegSuffixPattern.remove();

            out.fUseExponentialNotation = (expDigits >= 0);
            if (out.fUseExponentialNotation) {
              out.fMinExponentDigits = expDigits;
            }
            out.fExponentSignAlwaysShown = expSignAlways;
            int32_t digitTotalCount = digitLeftCount + zeroDigitCount + digitRightCount;
            
            
            
            
            int32_t effectiveDecimalPos = decimalPos >= 0 ? decimalPos : digitTotalCount;
            UBool isSigDig = (sigDigitCount > 0);
            out.fUseSignificantDigits = isSigDig;
            if (isSigDig) {
                out.fMinimumSignificantDigits = sigDigitCount;
                out.fMaximumSignificantDigits = sigDigitCount + digitRightCount;
            } else {
                int32_t minInt = effectiveDecimalPos - digitLeftCount;
                out.fMinimumIntegerDigits = minInt;
                out.fMaximumIntegerDigits = out.fUseExponentialNotation
                    ? digitLeftCount + out.fMinimumIntegerDigits
                    : gDefaultMaxIntegerDigits;
                out.fMaximumFractionDigits = decimalPos >= 0
                    ? (digitTotalCount - decimalPos) : 0;
                out.fMinimumFractionDigits = decimalPos >= 0
                    ? (digitLeftCount + zeroDigitCount - decimalPos) : 0;
            }
            out.fGroupingUsed = groupingCount > 0;
            out.fGroupingSize = (groupingCount > 0) ? groupingCount : 0;
            out.fGroupingSize2 = (groupingCount2 > 0 && groupingCount2 != groupingCount)
                ? groupingCount2 : 0;
            out.fMultiplier = multiplier;
            out.fDecimalSeparatorAlwaysShown = decimalPos == 0
                    || decimalPos == digitTotalCount;
            if (padPos >= 0) {
                out.fPadPosition = (DecimalFormatPattern::EPadPosition) padPos;
                
                

                
                
                out.fFormatWidth = sub0Limit - sub0Start;
                out.fPad = padChar;
            } else {
                out.fFormatWidth = 0;
            }
            if (roundingPos >= 0) {
                out.fRoundingIncrementUsed = TRUE;
                roundingInc.setDecimalAt(effectiveDecimalPos - roundingPos);
                out.fRoundingIncrement = roundingInc;
            } else {
                out.fRoundingIncrementUsed = FALSE;
            }
        } else {
            out.fNegPatternsBogus = FALSE;
            out.fNegPrefixPattern = prefix;
            out.fNegSuffixPattern = suffix;
        }
    }

    if (pattern.length() == 0) {
        out.fNegPatternsBogus = TRUE;
        out.fNegPrefixPattern.remove();
        out.fNegSuffixPattern.remove();
        out.fPosPatternsBogus = FALSE;
        out.fPosPrefixPattern.remove();
        out.fPosSuffixPattern.remove();

        out.fMinimumIntegerDigits = 0;
        out.fMaximumIntegerDigits = kDoubleIntegerDigits;
        out.fMinimumFractionDigits = 0;
        out.fMaximumFractionDigits = kDoubleFractionDigits;

        out.fUseExponentialNotation = FALSE;
        out.fCurrencySignCount = fgCurrencySignCountZero;
        out.fGroupingUsed = FALSE;
        out.fGroupingSize = 0;
        out.fGroupingSize2 = 0;
        out.fMultiplier = 1;
        out.fDecimalSeparatorAlwaysShown = FALSE;
        out.fFormatWidth = 0;
        out.fRoundingIncrementUsed = FALSE;
    }

    
    
    
    if (out.fNegPatternsBogus ||
        (out.fNegPrefixPattern == out.fPosPrefixPattern
         && out.fNegSuffixPattern == out.fPosSuffixPattern)) {
        out.fNegPatternsBogus = FALSE;
        out.fNegSuffixPattern = out.fPosSuffixPattern;
        out.fNegPrefixPattern.remove();
        out.fNegPrefixPattern.append(kQuote).append(kPatternMinus)
            .append(out.fPosPrefixPattern);
    }
}

U_NAMESPACE_END

#endif 
