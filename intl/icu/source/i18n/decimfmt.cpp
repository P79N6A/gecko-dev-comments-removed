






































#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "fphdlimp.h"
#include "unicode/decimfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/ucurr.h"
#include "unicode/ustring.h"
#include "unicode/dcfmtsym.h"
#include "unicode/ures.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/curramt.h"
#include "unicode/currpinf.h"
#include "unicode/plurrule.h"
#include "unicode/utf16.h"
#include "unicode/numsys.h"
#include "unicode/localpointer.h"
#include "uresimp.h"
#include "ucurrimp.h"
#include "charstr.h"
#include "cmemory.h"
#include "patternprops.h"
#include "digitlst.h"
#include "cstring.h"
#include "umutex.h"
#include "uassert.h"
#include "putilimp.h"
#include <math.h>
#include "hash.h"
#include "decfmtst.h"
#include "dcfmtimp.h"

U_NAMESPACE_BEGIN




#if UCONFIG_FORMAT_FASTPATHS_49
inline DecimalFormatInternal& internalData(uint8_t *reserved) {
  return *reinterpret_cast<DecimalFormatInternal*>(reserved);
}
inline const DecimalFormatInternal& internalData(const uint8_t *reserved) {
  return *reinterpret_cast<const DecimalFormatInternal*>(reserved);
}
#else
#endif







struct AffixPatternsForCurrency : public UMemory {
	
	UnicodeString negPrefixPatternForCurrency;
	
	UnicodeString negSuffixPatternForCurrency;
	
	UnicodeString posPrefixPatternForCurrency;
	
	UnicodeString posSuffixPatternForCurrency;
	int8_t patternType;

	AffixPatternsForCurrency(const UnicodeString& negPrefix,
							 const UnicodeString& negSuffix,
							 const UnicodeString& posPrefix,
							 const UnicodeString& posSuffix,
							 int8_t type) {
		negPrefixPatternForCurrency = negPrefix;
		negSuffixPatternForCurrency = negSuffix;
		posPrefixPatternForCurrency = posPrefix;
		posSuffixPatternForCurrency = posSuffix;
		patternType = type;
	}
};





struct AffixesForCurrency : public UMemory {
	
	UnicodeString negPrefixForCurrency;
	
	UnicodeString negSuffixForCurrency;
	
	UnicodeString posPrefixForCurrency;
	
	UnicodeString posSuffixForCurrency;

	int32_t formatWidth;

	AffixesForCurrency(const UnicodeString& negPrefix,
					   const UnicodeString& negSuffix,
					   const UnicodeString& posPrefix,
					   const UnicodeString& posSuffix) {
		negPrefixForCurrency = negPrefix;
		negSuffixForCurrency = negSuffix;
		posPrefixForCurrency = posPrefix;
		posSuffixForCurrency = posSuffix;
	}
};

U_CDECL_BEGIN




static UBool U_CALLCONV decimfmtAffixValueComparator(UHashTok val1, UHashTok val2);




static UBool U_CALLCONV decimfmtAffixPatternValueComparator(UHashTok val1, UHashTok val2);


static UBool
U_CALLCONV decimfmtAffixValueComparator(UHashTok val1, UHashTok val2) {
    const AffixesForCurrency* affix_1 =
        (AffixesForCurrency*)val1.pointer;
    const AffixesForCurrency* affix_2 =
        (AffixesForCurrency*)val2.pointer;
    return affix_1->negPrefixForCurrency == affix_2->negPrefixForCurrency &&
           affix_1->negSuffixForCurrency == affix_2->negSuffixForCurrency &&
           affix_1->posPrefixForCurrency == affix_2->posPrefixForCurrency &&
           affix_1->posSuffixForCurrency == affix_2->posSuffixForCurrency;
}


static UBool
U_CALLCONV decimfmtAffixPatternValueComparator(UHashTok val1, UHashTok val2) {
    const AffixPatternsForCurrency* affix_1 =
        (AffixPatternsForCurrency*)val1.pointer;
    const AffixPatternsForCurrency* affix_2 =
        (AffixPatternsForCurrency*)val2.pointer;
    return affix_1->negPrefixPatternForCurrency ==
           affix_2->negPrefixPatternForCurrency &&
           affix_1->negSuffixPatternForCurrency ==
           affix_2->negSuffixPatternForCurrency &&
           affix_1->posPrefixPatternForCurrency ==
           affix_2->posPrefixPatternForCurrency &&
           affix_1->posSuffixPatternForCurrency ==
           affix_2->posSuffixPatternForCurrency &&
           affix_1->patternType == affix_2->patternType;
}

U_CDECL_END

#ifdef FMT_DEBUG
#include <stdio.h>
static void _debugout(const char *f, int l, const UnicodeString& s) {
    char buf[2000];
    s.extract((int32_t) 0, s.length(), buf);
    printf("%s:%d: %s\n", f,l, buf);
}
#define debugout(x) _debugout(__FILE__,__LINE__,x)
#define debug(x) printf("%s:%d: %s\n", __FILE__,__LINE__, x);
static const UnicodeString dbg_null("<NULL>","");
#define DEREFSTR(x)   ((x!=NULL)?(*x):(dbg_null))
#else
#define debugout(x)
#define debug(x)
#endif







UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DecimalFormat)


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

const int32_t DecimalFormat::kDoubleIntegerDigits  = 309;
const int32_t DecimalFormat::kDoubleFractionDigits = 340;

const int32_t DecimalFormat::kMaxScientificIntegerDigits = 8;





const char DecimalFormat::fgNumberPatterns[]="NumberPatterns"; 
static const char fgNumberElements[]="NumberElements";
static const char fgLatn[]="latn";
static const char fgPatterns[]="patterns";
static const char fgDecimalFormat[]="decimalFormat";
static const char fgCurrencyFormat[]="currencyFormat";
static const UChar fgTripleCurrencySign[] = {0xA4, 0xA4, 0xA4, 0};

inline int32_t _min(int32_t a, int32_t b) { return (a<b) ? a : b; }
inline int32_t _max(int32_t a, int32_t b) { return (a<b) ? b : a; }




DecimalFormat::DecimalFormat(UErrorCode& status) {
    init(status);
    UParseError parseError;
    construct(status, parseError);
}





DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             UErrorCode& status) {
    init(status);
    UParseError parseError;
    construct(status, parseError, &pattern);
}






DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             DecimalFormatSymbols* symbolsToAdopt,
                             UErrorCode& status) {
    init(status);
    UParseError parseError;
    if (symbolsToAdopt == NULL)
        status = U_ILLEGAL_ARGUMENT_ERROR;
    construct(status, parseError, &pattern, symbolsToAdopt);
}

DecimalFormat::DecimalFormat(  const UnicodeString& pattern,
                    DecimalFormatSymbols* symbolsToAdopt,
                    UParseError& parseErr,
                    UErrorCode& status) {
    init(status);
    if (symbolsToAdopt == NULL)
        status = U_ILLEGAL_ARGUMENT_ERROR;
    construct(status,parseErr, &pattern, symbolsToAdopt);
}






DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             const DecimalFormatSymbols& symbols,
                             UErrorCode& status) {
    init(status);
    UParseError parseError;
    construct(status, parseError, &pattern, new DecimalFormatSymbols(symbols));
}






DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             DecimalFormatSymbols* symbolsToAdopt,
                             UNumberFormatStyle style,
                             UErrorCode& status) {
    init(status);
    fStyle = style;
    UParseError parseError;
    construct(status, parseError, &pattern, symbolsToAdopt);
}





void
DecimalFormat::init(UErrorCode &status) {
    fPosPrefixPattern = 0;
    fPosSuffixPattern = 0;
    fNegPrefixPattern = 0;
    fNegSuffixPattern = 0;
    fCurrencyChoice = 0;
    fMultiplier = NULL;
    fGroupingSize = 0;
    fGroupingSize2 = 0;
    fDecimalSeparatorAlwaysShown = FALSE;
    fSymbols = NULL;
    fUseSignificantDigits = FALSE;
    fMinSignificantDigits = 1;
    fMaxSignificantDigits = 6;
    fUseExponentialNotation = FALSE;
    fMinExponentDigits = 0;
    fExponentSignAlwaysShown = FALSE;
    fBoolFlags.clear();
    fRoundingIncrement = 0;
    fRoundingMode = kRoundHalfEven;
    fPad = 0;
    fFormatWidth = 0;
    fPadPosition = kPadBeforePrefix;
    fStyle = UNUM_DECIMAL;
    fCurrencySignCount = 0;
    fAffixPatternsForCurrency = NULL;
    fAffixesForCurrency = NULL;
    fPluralAffixesForCurrency = NULL;
    fCurrencyPluralInfo = NULL;
#if UCONFIG_HAVE_PARSEALLINPUT
    fParseAllInput = UNUM_MAYBE;
#endif

#if UCONFIG_FORMAT_FASTPATHS_49
    DecimalFormatInternal &data = internalData(fReserved);
    data.fFastFormatStatus=kFastpathUNKNOWN; 
    data.fFastParseStatus=kFastpathUNKNOWN; 
#endif
    
    DecimalFormatStaticSets::initSets(&status);
}






void
DecimalFormat::construct(UErrorCode&             status,
                         UParseError&           parseErr,
                         const UnicodeString*   pattern,
                         DecimalFormatSymbols*  symbolsToAdopt)
{
    fSymbols = symbolsToAdopt; 
    fRoundingIncrement = NULL;
    fRoundingMode = kRoundHalfEven;
    fPad = kPatternPadEscape;
    fPadPosition = kPadBeforePrefix;
    if (U_FAILURE(status))
        return;

    fPosPrefixPattern = fPosSuffixPattern = NULL;
    fNegPrefixPattern = fNegSuffixPattern = NULL;
    setMultiplier(1);
    fGroupingSize = 3;
    fGroupingSize2 = 0;
    fDecimalSeparatorAlwaysShown = FALSE;
    fUseExponentialNotation = FALSE;
    fMinExponentDigits = 0;

    if (fSymbols == NULL)
    {
        fSymbols = new DecimalFormatSymbols(Locale::getDefault(), status);
        
        if (fSymbols == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }
    UErrorCode nsStatus = U_ZERO_ERROR;
    NumberingSystem *ns = NumberingSystem::createInstance(nsStatus);
    if (U_FAILURE(nsStatus)) {
        status = nsStatus;
        return;
    }

    UnicodeString str;
    
    
    if (pattern == NULL)
    {
        int32_t len = 0;
        UResourceBundle *top = ures_open(NULL, Locale::getDefault().getName(), &status);

        UResourceBundle *resource = ures_getByKeyWithFallback(top, fgNumberElements, NULL, &status);
        resource = ures_getByKeyWithFallback(resource, ns->getName(), resource, &status);
        resource = ures_getByKeyWithFallback(resource, fgPatterns, resource, &status);
        const UChar *resStr = ures_getStringByKeyWithFallback(resource, fgDecimalFormat, &len, &status);
        if ( status == U_MISSING_RESOURCE_ERROR && uprv_strcmp(fgLatn,ns->getName())) {
            status = U_ZERO_ERROR;
            resource = ures_getByKeyWithFallback(top, fgNumberElements, resource, &status);
            resource = ures_getByKeyWithFallback(resource, fgLatn, resource, &status);
            resource = ures_getByKeyWithFallback(resource, fgPatterns, resource, &status);
            resStr = ures_getStringByKeyWithFallback(resource, fgDecimalFormat, &len, &status);
        }
        str.setTo(TRUE, resStr, len);
        pattern = &str;
        ures_close(resource);
        ures_close(top);
    }

    delete ns;

    if (U_FAILURE(status))
    {
        return;
    }

    if (pattern->indexOf((UChar)kCurrencySign) >= 0) {
        
        
        setCurrencyForSymbols();
    } else {
        setCurrencyInternally(NULL, status);
    }

    const UnicodeString* patternUsed;
    UnicodeString currencyPluralPatternForOther;
    
    if (fStyle == UNUM_CURRENCY_PLURAL) {
        fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
        if (U_FAILURE(status)) {
            return;
        }

        
        
        
        
        
        
        
        fCurrencyPluralInfo->getCurrencyPluralPattern(UNICODE_STRING("other", 5), currencyPluralPatternForOther);
        patternUsed = &currencyPluralPatternForOther;
        
        setCurrencyForSymbols();

    } else {
        patternUsed = pattern;
    }

    if (patternUsed->indexOf(kCurrencySign) != -1) {
        
        
        if (fCurrencyPluralInfo == NULL) {
           fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
           if (U_FAILURE(status)) {
               return;
           }
        }
        
        setupCurrencyAffixPatterns(status);
        
        if (patternUsed->indexOf(fgTripleCurrencySign, 3, 0) != -1) {
            setupCurrencyAffixes(*patternUsed, TRUE, TRUE, status);
        }
    }

    applyPatternWithoutExpandAffix(*patternUsed,FALSE, parseErr, status);

    
    if (fCurrencySignCount != fgCurrencySignCountInPluralFormat) {
        expandAffixAdjustWidth(NULL);
    }

    
    
    if (fCurrencySignCount > fgCurrencySignCountZero) {
        setCurrencyInternally(getCurrency(), status);
    }
#if UCONFIG_FORMAT_FASTPATHS_49
    DecimalFormatInternal &data = internalData(fReserved);
    data.fFastFormatStatus = kFastpathNO; 
    data.fFastParseStatus = kFastpathNO; 
    handleChanged();
#endif
}


void
DecimalFormat::setupCurrencyAffixPatterns(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    UParseError parseErr;
    fAffixPatternsForCurrency = initHashForAffixPattern(status);
    if (U_FAILURE(status)) {
        return;
    }

    NumberingSystem *ns = NumberingSystem::createInstance(fSymbols->getLocale(),status);
    if (U_FAILURE(status)) {
        return;
    }

    
    
    
    UnicodeString currencyPattern;
    UErrorCode error = U_ZERO_ERROR;   
    
    UResourceBundle *resource = ures_open(NULL, fSymbols->getLocale().getName(), &error);
    UResourceBundle *numElements = ures_getByKeyWithFallback(resource, fgNumberElements, NULL, &error);
    resource = ures_getByKeyWithFallback(numElements, ns->getName(), resource, &error);
    resource = ures_getByKeyWithFallback(resource, fgPatterns, resource, &error);
    int32_t patLen = 0;
    const UChar *patResStr = ures_getStringByKeyWithFallback(resource, fgCurrencyFormat,  &patLen, &error);
    if ( error == U_MISSING_RESOURCE_ERROR && uprv_strcmp(ns->getName(),fgLatn)) {
        error = U_ZERO_ERROR;
        resource = ures_getByKeyWithFallback(numElements, fgLatn, resource, &error);
        resource = ures_getByKeyWithFallback(resource, fgPatterns, resource, &error);
        patResStr = ures_getStringByKeyWithFallback(resource, fgCurrencyFormat,  &patLen, &error);
    }
    ures_close(numElements);
    ures_close(resource);
    delete ns;

    if (U_SUCCESS(error)) {
        applyPatternWithoutExpandAffix(UnicodeString(patResStr, patLen), false,
                                       parseErr, status);
        AffixPatternsForCurrency* affixPtn = new AffixPatternsForCurrency(
                                                    *fNegPrefixPattern,
                                                    *fNegSuffixPattern,
                                                    *fPosPrefixPattern,
                                                    *fPosSuffixPattern,
                                                    UCURR_SYMBOL_NAME);
        fAffixPatternsForCurrency->put(UNICODE_STRING("default", 7), affixPtn, status);
    }

    
    Hashtable* pluralPtn = fCurrencyPluralInfo->fPluralCountToCurrencyUnitPattern;
    const UHashElement* element = NULL;
    int32_t pos = -1;
    Hashtable pluralPatternSet;
    while ((element = pluralPtn->nextElement(pos)) != NULL) {
        const UHashTok valueTok = element->value;
        const UnicodeString* value = (UnicodeString*)valueTok.pointer;
        const UHashTok keyTok = element->key;
        const UnicodeString* key = (UnicodeString*)keyTok.pointer;
        if (pluralPatternSet.geti(*value) != 1) {
            pluralPatternSet.puti(*value, 1, status);
            applyPatternWithoutExpandAffix(*value, false, parseErr, status);
            AffixPatternsForCurrency* affixPtn = new AffixPatternsForCurrency(
                                                    *fNegPrefixPattern,
                                                    *fNegSuffixPattern,
                                                    *fPosPrefixPattern,
                                                    *fPosSuffixPattern,
                                                    UCURR_LONG_NAME);
            fAffixPatternsForCurrency->put(*key, affixPtn, status);
        }
    }
}


void
DecimalFormat::setupCurrencyAffixes(const UnicodeString& pattern,
                                    UBool setupForCurrentPattern,
                                    UBool setupForPluralPattern,
                                    UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    UParseError parseErr;
    if (setupForCurrentPattern) {
        if (fAffixesForCurrency) {
            deleteHashForAffix(fAffixesForCurrency);
        }
        fAffixesForCurrency = initHashForAffix(status);
        if (U_SUCCESS(status)) {
            applyPatternWithoutExpandAffix(pattern, false, parseErr, status);
            const PluralRules* pluralRules = fCurrencyPluralInfo->getPluralRules();
            StringEnumeration* keywords = pluralRules->getKeywords(status);
            if (U_SUCCESS(status)) {
                const UnicodeString* pluralCount;
                while ((pluralCount = keywords->snext(status)) != NULL) {
                    if ( U_SUCCESS(status) ) {
                        expandAffixAdjustWidth(pluralCount);
                        AffixesForCurrency* affix = new AffixesForCurrency(
                            fNegativePrefix, fNegativeSuffix, fPositivePrefix, fPositiveSuffix);
                        fAffixesForCurrency->put(*pluralCount, affix, status);
                    }
                }
            }
            delete keywords;
        }
    }

    if (U_FAILURE(status)) {
        return;
    }

    if (setupForPluralPattern) {
        if (fPluralAffixesForCurrency) {
            deleteHashForAffix(fPluralAffixesForCurrency);
        }
        fPluralAffixesForCurrency = initHashForAffix(status);
        if (U_SUCCESS(status)) {
            const PluralRules* pluralRules = fCurrencyPluralInfo->getPluralRules();
            StringEnumeration* keywords = pluralRules->getKeywords(status);
            if (U_SUCCESS(status)) {
                const UnicodeString* pluralCount;
                while ((pluralCount = keywords->snext(status)) != NULL) {
                    if ( U_SUCCESS(status) ) {
                        UnicodeString ptn;
                        fCurrencyPluralInfo->getCurrencyPluralPattern(*pluralCount, ptn);
                        applyPatternInternally(*pluralCount, ptn, false, parseErr, status);
                        AffixesForCurrency* affix = new AffixesForCurrency(
                            fNegativePrefix, fNegativeSuffix, fPositivePrefix, fPositiveSuffix);
                        fPluralAffixesForCurrency->put(*pluralCount, affix, status);
                    }
                }
            }
            delete keywords;
        }
    }
}




DecimalFormat::~DecimalFormat()
{
    delete fPosPrefixPattern;
    delete fPosSuffixPattern;
    delete fNegPrefixPattern;
    delete fNegSuffixPattern;
    delete fCurrencyChoice;
    delete fMultiplier;
    delete fSymbols;
    delete fRoundingIncrement;
    deleteHashForAffixPattern();
    deleteHashForAffix(fAffixesForCurrency);
    deleteHashForAffix(fPluralAffixesForCurrency);
    delete fCurrencyPluralInfo;
}




DecimalFormat::DecimalFormat(const DecimalFormat &source) :
    NumberFormat(source) {
    UErrorCode status = U_ZERO_ERROR;
    init(status); 
    *this = source;
}




template <class T>
static void _copy_ptr(T** pdest, const T* source) {
    if (source == NULL) {
        delete *pdest;
        *pdest = NULL;
    } else if (*pdest == NULL) {
        *pdest = new T(*source);
    } else {
        **pdest = *source;
    }
}

template <class T>
static void _clone_ptr(T** pdest, const T* source) {
    delete *pdest;
    if (source == NULL) {
        *pdest = NULL;
    } else {
        *pdest = static_cast<T*>(source->clone());
    }
}

DecimalFormat&
DecimalFormat::operator=(const DecimalFormat& rhs)
{
    if(this != &rhs) {
        NumberFormat::operator=(rhs);
        fPositivePrefix = rhs.fPositivePrefix;
        fPositiveSuffix = rhs.fPositiveSuffix;
        fNegativePrefix = rhs.fNegativePrefix;
        fNegativeSuffix = rhs.fNegativeSuffix;
        _copy_ptr(&fPosPrefixPattern, rhs.fPosPrefixPattern);
        _copy_ptr(&fPosSuffixPattern, rhs.fPosSuffixPattern);
        _copy_ptr(&fNegPrefixPattern, rhs.fNegPrefixPattern);
        _copy_ptr(&fNegSuffixPattern, rhs.fNegSuffixPattern);
        _clone_ptr(&fCurrencyChoice, rhs.fCurrencyChoice);
        setRoundingIncrement(rhs.getRoundingIncrement());
        fRoundingMode = rhs.fRoundingMode;
        setMultiplier(rhs.getMultiplier());
        fGroupingSize = rhs.fGroupingSize;
        fGroupingSize2 = rhs.fGroupingSize2;
        fDecimalSeparatorAlwaysShown = rhs.fDecimalSeparatorAlwaysShown;
        _copy_ptr(&fSymbols, rhs.fSymbols);
        fUseExponentialNotation = rhs.fUseExponentialNotation;
        fExponentSignAlwaysShown = rhs.fExponentSignAlwaysShown;
        fBoolFlags = rhs.fBoolFlags;
        
        fCurrencySignCount = rhs.fCurrencySignCount;
        
        fMinExponentDigits = rhs.fMinExponentDigits;

        
        fFormatWidth = rhs.fFormatWidth;
        fPad = rhs.fPad;
        fPadPosition = rhs.fPadPosition;
        
        fMinSignificantDigits = rhs.fMinSignificantDigits;
        fMaxSignificantDigits = rhs.fMaxSignificantDigits;
        fUseSignificantDigits = rhs.fUseSignificantDigits;
        fFormatPattern = rhs.fFormatPattern;
        fStyle = rhs.fStyle;
        fCurrencySignCount = rhs.fCurrencySignCount;
        _clone_ptr(&fCurrencyPluralInfo, rhs.fCurrencyPluralInfo);
        deleteHashForAffixPattern();
        if (rhs.fAffixPatternsForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            fAffixPatternsForCurrency = initHashForAffixPattern(status);
            copyHashForAffixPattern(rhs.fAffixPatternsForCurrency,
                                    fAffixPatternsForCurrency, status);
        }
        deleteHashForAffix(fAffixesForCurrency);
        if (rhs.fAffixesForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            fAffixesForCurrency = initHashForAffixPattern(status);
            copyHashForAffix(rhs.fAffixesForCurrency, fAffixesForCurrency, status);
        }
        deleteHashForAffix(fPluralAffixesForCurrency);
        if (rhs.fPluralAffixesForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            fPluralAffixesForCurrency = initHashForAffixPattern(status);
            copyHashForAffix(rhs.fPluralAffixesForCurrency, fPluralAffixesForCurrency, status);
        }
    }
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
    return *this;
}



UBool
DecimalFormat::operator==(const Format& that) const
{
    if (this == &that)
        return TRUE;

    
    const DecimalFormat* other = (DecimalFormat*)&that;

#ifdef FMT_DEBUG
    
    
    UBool first = TRUE;
    if (!NumberFormat::operator==(that)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("NumberFormat::!=");
    } else {
    if (!((fPosPrefixPattern == other->fPosPrefixPattern && 
              fPositivePrefix == other->fPositivePrefix)
           || (fPosPrefixPattern != 0 && other->fPosPrefixPattern != 0 &&
               *fPosPrefixPattern  == *other->fPosPrefixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Pos Prefix !=");
    }
    if (!((fPosSuffixPattern == other->fPosSuffixPattern && 
           fPositiveSuffix == other->fPositiveSuffix)
          || (fPosSuffixPattern != 0 && other->fPosSuffixPattern != 0 &&
              *fPosSuffixPattern  == *other->fPosSuffixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Pos Suffix !=");
    }
    if (!((fNegPrefixPattern == other->fNegPrefixPattern && 
           fNegativePrefix == other->fNegativePrefix)
          || (fNegPrefixPattern != 0 && other->fNegPrefixPattern != 0 &&
              *fNegPrefixPattern  == *other->fNegPrefixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Neg Prefix ");
        if (fNegPrefixPattern == NULL) {
            debug("NULL(");
            debugout(fNegativePrefix);
            debug(")");
        } else {
            debugout(*fNegPrefixPattern);
        }
        debug(" != ");
        if (other->fNegPrefixPattern == NULL) {
            debug("NULL(");
            debugout(other->fNegativePrefix);
            debug(")");
        } else {
            debugout(*other->fNegPrefixPattern);
        }
    }
    if (!((fNegSuffixPattern == other->fNegSuffixPattern && 
           fNegativeSuffix == other->fNegativeSuffix)
          || (fNegSuffixPattern != 0 && other->fNegSuffixPattern != 0 &&
              *fNegSuffixPattern  == *other->fNegSuffixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Neg Suffix ");
        if (fNegSuffixPattern == NULL) {
            debug("NULL(");
            debugout(fNegativeSuffix);
            debug(")");
        } else {
            debugout(*fNegSuffixPattern);
        }
        debug(" != ");
        if (other->fNegSuffixPattern == NULL) {
            debug("NULL(");
            debugout(other->fNegativeSuffix);
            debug(")");
        } else {
            debugout(*other->fNegSuffixPattern);
        }
    }
    if (!((fRoundingIncrement == other->fRoundingIncrement) 
          || (fRoundingIncrement != NULL &&
              other->fRoundingIncrement != NULL &&
              *fRoundingIncrement == *other->fRoundingIncrement))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Rounding Increment !=");
              }
    if (getMultiplier() != other->getMultiplier()) {
        if (first) { printf("[ "); first = FALSE; }
        printf("Multiplier %ld != %ld", getMultiplier(), other->getMultiplier());
    }
    if (fGroupingSize != other->fGroupingSize) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Grouping Size %ld != %ld", fGroupingSize, other->fGroupingSize);
    }
    if (fGroupingSize2 != other->fGroupingSize2) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Secondary Grouping Size %ld != %ld", fGroupingSize2, other->fGroupingSize2);
    }
    if (fDecimalSeparatorAlwaysShown != other->fDecimalSeparatorAlwaysShown) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Dec Sep Always %d != %d", fDecimalSeparatorAlwaysShown, other->fDecimalSeparatorAlwaysShown);
    }
    if (fUseExponentialNotation != other->fUseExponentialNotation) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Use Exp !=");
    }
    if (!(!fUseExponentialNotation ||
          fMinExponentDigits != other->fMinExponentDigits)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Exp Digits !=");
    }
    if (*fSymbols != *(other->fSymbols)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Symbols !=");
    }
    
    if (fUseSignificantDigits != other->fUseSignificantDigits) {
        debug("fUseSignificantDigits !=");
    }
    if (fUseSignificantDigits &&
        fMinSignificantDigits != other->fMinSignificantDigits) {
        debug("fMinSignificantDigits !=");
    }
    if (fUseSignificantDigits &&
        fMaxSignificantDigits != other->fMaxSignificantDigits) {
        debug("fMaxSignificantDigits !=");
    }

    if (!first) { printf(" ]"); }
    if (fCurrencySignCount != other->fCurrencySignCount) {
        debug("fCurrencySignCount !=");
    }
    if (fCurrencyPluralInfo == other->fCurrencyPluralInfo) {
        debug("fCurrencyPluralInfo == ");
        if (fCurrencyPluralInfo == NULL) {
            debug("fCurrencyPluralInfo == NULL");
        }
    }
    if (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo != NULL &&
         *fCurrencyPluralInfo != *(other->fCurrencyPluralInfo)) {
        debug("fCurrencyPluralInfo !=");
    }
    if (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo == NULL ||
        fCurrencyPluralInfo == NULL && other->fCurrencyPluralInfo != NULL) {
        debug("fCurrencyPluralInfo one NULL, the other not");
    }
    if (fCurrencyPluralInfo == NULL && other->fCurrencyPluralInfo == NULL) {
        debug("fCurrencyPluralInfo == ");
    }
    }
#endif

    return (NumberFormat::operator==(that) &&
            ((fCurrencySignCount == fgCurrencySignCountInPluralFormat) ?
            (fAffixPatternsForCurrency->equals(*other->fAffixPatternsForCurrency)) :
            (((fPosPrefixPattern == other->fPosPrefixPattern && 
              fPositivePrefix == other->fPositivePrefix)
             || (fPosPrefixPattern != 0 && other->fPosPrefixPattern != 0 &&
                 *fPosPrefixPattern  == *other->fPosPrefixPattern)) &&
            ((fPosSuffixPattern == other->fPosSuffixPattern && 
              fPositiveSuffix == other->fPositiveSuffix)
             || (fPosSuffixPattern != 0 && other->fPosSuffixPattern != 0 &&
                 *fPosSuffixPattern  == *other->fPosSuffixPattern)) &&
            ((fNegPrefixPattern == other->fNegPrefixPattern && 
              fNegativePrefix == other->fNegativePrefix)
             || (fNegPrefixPattern != 0 && other->fNegPrefixPattern != 0 &&
                 *fNegPrefixPattern  == *other->fNegPrefixPattern)) &&
            ((fNegSuffixPattern == other->fNegSuffixPattern && 
              fNegativeSuffix == other->fNegativeSuffix)
             || (fNegSuffixPattern != 0 && other->fNegSuffixPattern != 0 &&
                 *fNegSuffixPattern  == *other->fNegSuffixPattern)))) &&
            ((fRoundingIncrement == other->fRoundingIncrement) 
             || (fRoundingIncrement != NULL &&
                 other->fRoundingIncrement != NULL &&
                 *fRoundingIncrement == *other->fRoundingIncrement)) &&
        getMultiplier() == other->getMultiplier() &&
        fGroupingSize == other->fGroupingSize &&
        fGroupingSize2 == other->fGroupingSize2 &&
        fDecimalSeparatorAlwaysShown == other->fDecimalSeparatorAlwaysShown &&
        fUseExponentialNotation == other->fUseExponentialNotation &&
        (!fUseExponentialNotation ||
         fMinExponentDigits == other->fMinExponentDigits) &&
        *fSymbols == *(other->fSymbols) &&
        fUseSignificantDigits == other->fUseSignificantDigits &&
        (!fUseSignificantDigits ||
         (fMinSignificantDigits == other->fMinSignificantDigits &&
          fMaxSignificantDigits == other->fMaxSignificantDigits)) &&
        fCurrencySignCount == other->fCurrencySignCount &&
        ((fCurrencyPluralInfo == other->fCurrencyPluralInfo &&
          fCurrencyPluralInfo == NULL) ||
         (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo != NULL &&
         *fCurrencyPluralInfo == *(other->fCurrencyPluralInfo))));
}



Format*
DecimalFormat::clone() const
{
    return new DecimalFormat(*this);
}



UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition) const
{
    return format((int64_t)number, appendTo, fieldPosition);
}

UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition,
                      UErrorCode& status) const
{
    return format((int64_t)number, appendTo, fieldPosition, status);
}

UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo,
                      FieldPositionIterator* posIter,
                      UErrorCode& status) const
{
    return format((int64_t)number, appendTo, posIter, status);
}


#if UCONFIG_FORMAT_FASTPATHS_49
void DecimalFormat::handleChanged() {
  DecimalFormatInternal &data = internalData(fReserved);

  if(data.fFastFormatStatus == kFastpathUNKNOWN || data.fFastParseStatus == kFastpathUNKNOWN) {
    return; 
  }

  data.fFastParseStatus = data.fFastFormatStatus = kFastpathNO;

#if UCONFIG_HAVE_PARSEALLINPUT
  if(fParseAllInput == UNUM_NO) {
    debug("No Parse fastpath: fParseAllInput==UNUM_NO");
  } else 
#endif
  if (fFormatWidth!=0) {
      debug("No Parse fastpath: fFormatWidth");
  } else if(fPositivePrefix.length()>0) {
    debug("No Parse fastpath: positive prefix");
  } else if(fPositiveSuffix.length()>0) {
    debug("No Parse fastpath: positive suffix");
  } else if(fNegativePrefix.length()>1 
            || ((fNegativePrefix.length()==1) && (fNegativePrefix.charAt(0)!=0x002D))) {
    debug("No Parse fastpath: negative prefix that isn't '-'");
  } else if(fNegativeSuffix.length()>0) {
    debug("No Parse fastpath: negative suffix");
  } else {
    data.fFastParseStatus = kFastpathYES;
    debug("parse fastpath: YES");
  }
  
  if (fGroupingSize!=0 && isGroupingUsed()) {
    debug("No format fastpath: fGroupingSize!=0 and grouping is used");
#ifdef FMT_DEBUG
    printf("groupingsize=%d\n", fGroupingSize);
#endif
  } else if(fGroupingSize2!=0 && isGroupingUsed()) {
    debug("No format fastpath: fGroupingSize2!=0");
  } else if(fUseExponentialNotation) {
    debug("No format fastpath: fUseExponentialNotation");
  } else if(fFormatWidth!=0) {
    debug("No format fastpath: fFormatWidth!=0");
  } else if(fMinSignificantDigits!=1) {
    debug("No format fastpath: fMinSignificantDigits!=1");
  } else if(fMultiplier!=NULL) {
    debug("No format fastpath: fMultiplier!=NULL");
  } else if(0x0030 != getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0)) {
    debug("No format fastpath: 0x0030 != getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0)");
  } else if(fDecimalSeparatorAlwaysShown) {
    debug("No format fastpath: fDecimalSeparatorAlwaysShown");
  } else if(getMinimumFractionDigits()>0) {
    debug("No format fastpath: fMinFractionDigits>0");
  } else if(fCurrencySignCount > fgCurrencySignCountZero) {
    debug("No format fastpath: fCurrencySignCount > fgCurrencySignCountZero");
  } else if(fRoundingIncrement!=0) {
    debug("No format fastpath: fRoundingIncrement!=0");
  } else {
    data.fFastFormatStatus = kFastpathYES;
    debug("format:kFastpathYES!");
  }


}
#endif


UnicodeString&
DecimalFormat::format(int64_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition) const
{
    UErrorCode status = U_ZERO_ERROR; 
    FieldPositionOnlyHandler handler(fieldPosition);
    return _format(number, appendTo, handler, status);
}

UnicodeString&
DecimalFormat::format(int64_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition,
                      UErrorCode& status) const
{
    FieldPositionOnlyHandler handler(fieldPosition);
    return _format(number, appendTo, handler, status);
}

UnicodeString&
DecimalFormat::format(int64_t number,
                      UnicodeString& appendTo,
                      FieldPositionIterator* posIter,
                      UErrorCode& status) const
{
    FieldPositionIteratorHandler handler(posIter, status);
    return _format(number, appendTo, handler, status);
}

UnicodeString&
DecimalFormat::_format(int64_t number,
                       UnicodeString& appendTo,
                       FieldPositionHandler& handler,
                       UErrorCode &status) const
{
    
    if (U_FAILURE(status)) {
        return appendTo;
    }

#if UCONFIG_FORMAT_FASTPATHS_49
  
  
  

  const DecimalFormatInternal &data = internalData(fReserved);

#ifdef FMT_DEBUG
  data.dump();
  printf("fastpath? [%d]\n", number);
#endif
    
  if( data.fFastFormatStatus==kFastpathYES) {

#define kZero 0x0030
    const int32_t MAX_IDX = MAX_DIGITS+2;
    UChar outputStr[MAX_IDX];
    int32_t destIdx = MAX_IDX;
    outputStr[--destIdx] = 0;  

    int64_t  n = number;
    if (number < 1) {
      
      
      outputStr[--destIdx] = (-(n % 10) + kZero);
      n /= -10;
    }
    
    while (n > 0) {
      outputStr[--destIdx] = (n % 10) + kZero;
      n /= 10;
    }
    

        
    U_ASSERT(destIdx >= 0);
    int32_t length = MAX_IDX - destIdx -1;
     appendAffix(appendTo, number, handler, number<0, TRUE);
    int32_t maxIntDig = getMaximumIntegerDigits();
    int32_t destlength = length<=maxIntDig?length:maxIntDig; 

    if(length>maxIntDig && fBoolFlags.contains(UNUM_FORMAT_FAIL_IF_MORE_THAN_MAX_DIGITS)) {
      status = U_ILLEGAL_ARGUMENT_ERROR;
    }

    int32_t prependZero = getMinimumIntegerDigits() - destlength;

#ifdef FMT_DEBUG
    printf("prependZero=%d, length=%d, minintdig=%d maxintdig=%d destlength=%d skip=%d\n", prependZero, length, getMinimumIntegerDigits(), maxIntDig, destlength, length-destlength);
#endif    
    int32_t intBegin = appendTo.length();

    while((prependZero--)>0) {
      appendTo.append((UChar)0x0030); 
    }

    appendTo.append(outputStr+destIdx+
                    (length-destlength), 
                    destlength);
    handler.addAttribute(kIntegerField, intBegin, appendTo.length());

     appendAffix(appendTo, number, handler, number<0, FALSE);

    
    
#ifdef FMT_DEBUG
        printf("Writing [%s] length [%d] max %d for [%d]\n", outputStr+destIdx, length, MAX_IDX, number);
#endif

#undef kZero

    return appendTo;
  } 
#endif

  
    DigitList digits;
    digits.set(number);
    return _format(digits, appendTo, handler, status);
}



UnicodeString&
DecimalFormat::format(  double number,
                        UnicodeString& appendTo,
                        FieldPosition& fieldPosition) const
{
    UErrorCode status = U_ZERO_ERROR; 
    FieldPositionOnlyHandler handler(fieldPosition);
    return _format(number, appendTo, handler, status);
}

UnicodeString&
DecimalFormat::format(  double number,
                        UnicodeString& appendTo,
                        FieldPosition& fieldPosition,
                        UErrorCode& status) const
{
    FieldPositionOnlyHandler handler(fieldPosition);
    return _format(number, appendTo, handler, status);
}

UnicodeString&
DecimalFormat::format(  double number,
                        UnicodeString& appendTo,
                        FieldPositionIterator* posIter,
                        UErrorCode& status) const
{
  FieldPositionIteratorHandler handler(posIter, status);
  return _format(number, appendTo, handler, status);
}

UnicodeString&
DecimalFormat::_format( double number,
                        UnicodeString& appendTo,
                        FieldPositionHandler& handler,
                        UErrorCode &status) const
{
    if (U_FAILURE(status)) {
        return appendTo;
    }
    
    
    
    if (uprv_isNaN(number))
    {
        int begin = appendTo.length();
        appendTo += getConstSymbol(DecimalFormatSymbols::kNaNSymbol);

        handler.addAttribute(kIntegerField, begin, appendTo.length());

        addPadding(appendTo, handler, 0, 0);
        return appendTo;
    }

    DigitList digits;
    digits.set(number);
    _format(digits, appendTo, handler, status);
    
    return appendTo;
}




UnicodeString&
DecimalFormat::format(const StringPiece &number,
                      UnicodeString &toAppendTo,
                      FieldPositionIterator *posIter,
                      UErrorCode &status) const
{
#if UCONFIG_FORMAT_FASTPATHS_49
  
  int32_t len    = number.length();

  if(len>0&&len<10) { 
    const char *data = number.data();
    int64_t num = 0;
    UBool neg = FALSE;
    UBool ok = TRUE;
    
    int32_t start  = 0;

    if(data[start]=='+') {
      start++;
    } else if(data[start]=='-') {
      neg=TRUE;
      start++;
    }

    int32_t place = 1; 
    for(int32_t i=len-1;i>=start;i--) {
      if(data[i]>='0'&&data[i]<='9') {
        num+=place*(int64_t)(data[i]-'0');
      } else {
        ok=FALSE;
        break;
      }
      place *= 10;
    }

    if(ok) {
      if(neg) {
        num = -num;
      }
      
      return format(num, toAppendTo, posIter, status);
    }
    
  }
#endif

    DigitList   dnum;
    dnum.set(number, status);
    if (U_FAILURE(status)) {
        return toAppendTo;
    }
    FieldPositionIteratorHandler handler(posIter, status);
    _format(dnum, toAppendTo, handler, status);
    return toAppendTo;
}


UnicodeString&
DecimalFormat::format(const DigitList &number,
                      UnicodeString &appendTo,
                      FieldPositionIterator *posIter,
                      UErrorCode &status) const {
    FieldPositionIteratorHandler handler(posIter, status);
    _format(number, appendTo, handler, status);
    return appendTo;
}



UnicodeString&
DecimalFormat::format(const DigitList &number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode &status) const {
    FieldPositionOnlyHandler handler(pos);
    _format(number, appendTo, handler, status);
    return appendTo;
}



UnicodeString&
DecimalFormat::_format(const DigitList &number,
                        UnicodeString& appendTo,
                        FieldPositionHandler& handler,
                        UErrorCode &status) const
{
    
    
    if (number.isNaN())
    {
        int begin = appendTo.length();
        appendTo += getConstSymbol(DecimalFormatSymbols::kNaNSymbol);

        handler.addAttribute(kIntegerField, begin, appendTo.length());

        addPadding(appendTo, handler, 0, 0);
        return appendTo;
    }

    
    
    
    

    DigitList adjustedNum(number);  
    adjustedNum.setRoundingMode(fRoundingMode);
    if (fMultiplier != NULL) {
        adjustedNum.mult(*fMultiplier, status);
    }

    




    UBool isNegative = !adjustedNum.isPositive();

    
    
    adjustedNum.fContext.status &= ~DEC_Inexact;
    if (fRoundingIncrement != NULL) {
        adjustedNum.div(*fRoundingIncrement, status);
        adjustedNum.toIntegralValue();
        adjustedNum.mult(*fRoundingIncrement, status);
        adjustedNum.trim();
    }
    if (fRoundingMode == kRoundUnnecessary && (adjustedNum.fContext.status & DEC_Inexact)) {
        status = U_FORMAT_INEXACT_ERROR;
        return appendTo;
    }
        

    
    if (adjustedNum.isInfinite()) {
        int32_t prefixLen = appendAffix(appendTo, adjustedNum.getDouble(), handler, isNegative, TRUE);

        int begin = appendTo.length();
        appendTo += getConstSymbol(DecimalFormatSymbols::kInfinitySymbol);

        handler.addAttribute(kIntegerField, begin, appendTo.length());

        int32_t suffixLen = appendAffix(appendTo, adjustedNum.getDouble(), handler, isNegative, FALSE);

        addPadding(appendTo, handler, prefixLen, suffixLen);
        return appendTo;
    }

    if (fUseExponentialNotation || areSignificantDigitsUsed()) {
        int32_t sigDigits = precision();
        if (sigDigits > 0) {
            adjustedNum.round(sigDigits);
        }
    } else {
        
        int32_t numFractionDigits = precision();
        adjustedNum.roundFixedPoint(numFractionDigits);
    }
    if (fRoundingMode == kRoundUnnecessary && (adjustedNum.fContext.status & DEC_Inexact)) {
        status = U_FORMAT_INEXACT_ERROR;
        return appendTo;
    }
 
    return subformat(appendTo, handler, adjustedNum, FALSE, status);
}


UnicodeString&
DecimalFormat::format(  const Formattable& obj,
                        UnicodeString& appendTo,
                        FieldPosition& fieldPosition,
                        UErrorCode& status) const
{
    return NumberFormat::format(obj, appendTo, fieldPosition, status);
}











UBool DecimalFormat::isGroupingPosition(int32_t pos) const {
    UBool result = FALSE;
    if (isGroupingUsed() && (pos > 0) && (fGroupingSize > 0)) {
        if ((fGroupingSize2 > 0) && (pos > fGroupingSize)) {
            result = ((pos - fGroupingSize) % fGroupingSize2) == 0;
        } else {
            result = pos % fGroupingSize == 0;
        }
    }
    return result;
}







UnicodeString&
DecimalFormat::subformat(UnicodeString& appendTo,
                         FieldPositionHandler& handler,
                         DigitList&     digits,
                         UBool          isInteger,
                         UErrorCode& status) const
{
    
    
    

    UChar32 localizedDigits[10];
    localizedDigits[0] = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
    localizedDigits[1] = getConstSymbol(DecimalFormatSymbols::kOneDigitSymbol).char32At(0);
    localizedDigits[2] = getConstSymbol(DecimalFormatSymbols::kTwoDigitSymbol).char32At(0);
    localizedDigits[3] = getConstSymbol(DecimalFormatSymbols::kThreeDigitSymbol).char32At(0);
    localizedDigits[4] = getConstSymbol(DecimalFormatSymbols::kFourDigitSymbol).char32At(0);
    localizedDigits[5] = getConstSymbol(DecimalFormatSymbols::kFiveDigitSymbol).char32At(0);
    localizedDigits[6] = getConstSymbol(DecimalFormatSymbols::kSixDigitSymbol).char32At(0);
    localizedDigits[7] = getConstSymbol(DecimalFormatSymbols::kSevenDigitSymbol).char32At(0);
    localizedDigits[8] = getConstSymbol(DecimalFormatSymbols::kEightDigitSymbol).char32At(0);
    localizedDigits[9] = getConstSymbol(DecimalFormatSymbols::kNineDigitSymbol).char32At(0);

    const UnicodeString *grouping ;
    if(fCurrencySignCount > fgCurrencySignCountZero) {
        grouping = &getConstSymbol(DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol);
    }else{
        grouping = &getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol);
    }
    const UnicodeString *decimal;
    if(fCurrencySignCount > fgCurrencySignCountZero) {
        decimal = &getConstSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol);
    } else {
        decimal = &getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
    }
    UBool useSigDig = areSignificantDigitsUsed();
    int32_t maxIntDig = getMaximumIntegerDigits();
    int32_t minIntDig = getMinimumIntegerDigits();

    
    double doubleValue = digits.getDouble();
    int32_t prefixLen = appendAffix(appendTo, doubleValue, handler, !digits.isPositive(), TRUE);

    if (fUseExponentialNotation)
    {
        int currentLength = appendTo.length();
        int intBegin = currentLength;
        int intEnd = -1;
        int fracBegin = -1;

        int32_t minFracDig = 0;
        if (useSigDig) {
            maxIntDig = minIntDig = 1;
            minFracDig = getMinimumSignificantDigits() - 1;
        } else {
            minFracDig = getMinimumFractionDigits();
            if (maxIntDig > kMaxScientificIntegerDigits) {
                maxIntDig = 1;
                if (maxIntDig < minIntDig) {
                    maxIntDig = minIntDig;
                }
            }
            if (maxIntDig > minIntDig) {
                minIntDig = 1;
            }
        }

        
        
        

        
        
        
        
        
        
        
        digits.reduce();   
        int32_t exponent = digits.getDecimalAt();
        if (maxIntDig > 1 && maxIntDig != minIntDig) {
            
            exponent = (exponent > 0) ? (exponent - 1) / maxIntDig
                                      : (exponent / maxIntDig) - 1;
            exponent *= maxIntDig;
        } else {
            
            
            exponent -= (minIntDig > 0 || minFracDig > 0)
                        ? minIntDig : 1;
        }

        
        
        
        
        int32_t minimumDigits =  minIntDig + minFracDig;
        
        
        int32_t integerDigits = digits.isZero() ? minIntDig :
            digits.getDecimalAt() - exponent;
        int32_t totalDigits = digits.getCount();
        if (minimumDigits > totalDigits)
            totalDigits = minimumDigits;
        if (integerDigits > totalDigits)
            totalDigits = integerDigits;

        
        int32_t i;
        for (i=0; i<totalDigits; ++i)
        {
            if (i == integerDigits)
            {
                intEnd = appendTo.length();
                handler.addAttribute(kIntegerField, intBegin, intEnd);

                appendTo += *decimal;

                fracBegin = appendTo.length();
                handler.addAttribute(kDecimalSeparatorField, fracBegin - 1, fracBegin);
            }
            
            UChar32 c = (UChar32)((i < digits.getCount()) ?
                          localizedDigits[digits.getDigitValue(i)] :
                          localizedDigits[0]);
            appendTo += c;
        }

        currentLength = appendTo.length();

        if (intEnd < 0) {
            handler.addAttribute(kIntegerField, intBegin, currentLength);
        }
        if (fracBegin > 0) {
            handler.addAttribute(kFractionField, fracBegin, currentLength);
        }

        
        
        
        
        appendTo += getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);

        handler.addAttribute(kExponentSymbolField, currentLength, appendTo.length());
        currentLength = appendTo.length();

        
        
        
        if (digits.isZero())
            exponent = 0;

        if (exponent < 0) {
            appendTo += getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
            handler.addAttribute(kExponentSignField, currentLength, appendTo.length());
        } else if (fExponentSignAlwaysShown) {
            appendTo += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
            handler.addAttribute(kExponentSignField, currentLength, appendTo.length());
        }

        currentLength = appendTo.length();

        DigitList expDigits;
        expDigits.set(exponent);
        {
            int expDig = fMinExponentDigits;
            if (fUseExponentialNotation && expDig < 1) {
                expDig = 1;
            }
            for (i=expDigits.getDecimalAt(); i<expDig; ++i)
                appendTo += (localizedDigits[0]);
        }
        for (i=0; i<expDigits.getDecimalAt(); ++i)
        {
            UChar32 c = (UChar32)((i < expDigits.getCount()) ?
                          localizedDigits[expDigits.getDigitValue(i)] : 
                          localizedDigits[0]);
            appendTo += c;
        }

        handler.addAttribute(kExponentField, currentLength, appendTo.length());
    }
    else  
    {
        int currentLength = appendTo.length();
        int intBegin = currentLength;

        int32_t sigCount = 0;
        int32_t minSigDig = getMinimumSignificantDigits();
        int32_t maxSigDig = getMaximumSignificantDigits();
        if (!useSigDig) {
            minSigDig = 0;
            maxSigDig = INT32_MAX;
        }

        
        
        
        
        int32_t count = useSigDig ?
            _max(1, digits.getDecimalAt()) : minIntDig;
        if (digits.getDecimalAt() > 0 && count < digits.getDecimalAt()) {
            count = digits.getDecimalAt();
        }

        
        
        
        

        int32_t digitIndex = 0; 
        if (count > maxIntDig && maxIntDig >= 0) {
            count = maxIntDig;
            digitIndex = digits.getDecimalAt() - count;
            if(fBoolFlags.contains(UNUM_FORMAT_FAIL_IF_MORE_THAN_MAX_DIGITS)) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
            }
        }

        int32_t sizeBeforeIntegerPart = appendTo.length();

        int32_t i;
        for (i=count-1; i>=0; --i)
        {
            if (i < digits.getDecimalAt() && digitIndex < digits.getCount() &&
                sigCount < maxSigDig) {
                
                appendTo += (UChar32)localizedDigits[digits.getDigitValue(digitIndex++)];
                ++sigCount;
            }
            else
            {
                
                appendTo += localizedDigits[0];
                if (sigCount > 0) {
                    ++sigCount;
                }
            }

            
            if (isGroupingPosition(i)) {
                currentLength = appendTo.length();
                appendTo.append(*grouping);
                handler.addAttribute(kGroupingSeparatorField, currentLength, appendTo.length());
            }
        }

        
        
        
        
        

        
        
        UBool fractionPresent = (!isInteger && digitIndex < digits.getCount()) ||
            (useSigDig ? (sigCount < minSigDig) : (getMinimumFractionDigits() > 0));

        
        
        
        if (!fractionPresent && appendTo.length() == sizeBeforeIntegerPart)
            appendTo += localizedDigits[0];

        currentLength = appendTo.length();
        handler.addAttribute(kIntegerField, intBegin, currentLength);

        
        if (fDecimalSeparatorAlwaysShown || fractionPresent) {
            appendTo += *decimal;
            handler.addAttribute(kDecimalSeparatorField, currentLength, appendTo.length());
            currentLength = appendTo.length();
        }

        int fracBegin = currentLength;

        count = useSigDig ? INT32_MAX : getMaximumFractionDigits();
        if (useSigDig && (sigCount == maxSigDig ||
                          (sigCount >= minSigDig && digitIndex == digits.getCount()))) {
            count = 0;
        }

        for (i=0; i < count; ++i) {
            
            
            
            
            
            
            
            if (!useSigDig && i >= getMinimumFractionDigits() &&
                (isInteger || digitIndex >= digits.getCount())) {
                break;
            }

            
            
            
            
            if (-1-i > (digits.getDecimalAt()-1)) {
                appendTo += localizedDigits[0];
                continue;
            }

            
            
            if (!isInteger && digitIndex < digits.getCount()) {
                appendTo += (UChar32)localizedDigits[digits.getDigitValue(digitIndex++)];
            } else {
                appendTo += localizedDigits[0];
            }

            
            
            
            ++sigCount;
            if (useSigDig &&
                (sigCount == maxSigDig ||
                 (digitIndex == digits.getCount() && sigCount >= minSigDig))) {
                break;
            }
        }

        handler.addAttribute(kFractionField, fracBegin, appendTo.length());
    }

    int32_t suffixLen = appendAffix(appendTo, doubleValue, handler, !digits.isPositive(), FALSE);

    addPadding(appendTo, handler, prefixLen, suffixLen);
    return appendTo;
}





void DecimalFormat::addPadding(UnicodeString& appendTo,
                               FieldPositionHandler& handler,
                               int32_t prefixLen,
                               int32_t suffixLen) const
{
    if (fFormatWidth > 0) {
        int32_t len = fFormatWidth - appendTo.length();
        if (len > 0) {
            UnicodeString padding;
            for (int32_t i=0; i<len; ++i) {
                padding += fPad;
            }
            switch (fPadPosition) {
            case kPadAfterPrefix:
                appendTo.insert(prefixLen, padding);
                break;
            case kPadBeforePrefix:
                appendTo.insert(0, padding);
                break;
            case kPadBeforeSuffix:
                appendTo.insert(appendTo.length() - suffixLen, padding);
                break;
            case kPadAfterSuffix:
                appendTo += padding;
                break;
            }
            if (fPadPosition == kPadBeforePrefix || fPadPosition == kPadAfterPrefix) {
                handler.shiftLast(len);
            }
        }
    }
}



void
DecimalFormat::parse(const UnicodeString& text,
                     Formattable& result,
                     UErrorCode& status) const
{
    NumberFormat::parse(text, result, status);
}

void
DecimalFormat::parse(const UnicodeString& text,
                     Formattable& result,
                     ParsePosition& parsePosition) const {
    parse(text, result, parsePosition, NULL);
}

CurrencyAmount* DecimalFormat::parseCurrency(const UnicodeString& text,
                                             ParsePosition& pos) const {
    Formattable parseResult;
    int32_t start = pos.getIndex();
    UChar curbuf[4];
    parse(text, parseResult, pos, curbuf);
    if (pos.getIndex() != start) {
        UErrorCode ec = U_ZERO_ERROR;
        LocalPointer<CurrencyAmount> currAmt(new CurrencyAmount(parseResult, curbuf, ec));
        if (U_FAILURE(ec)) {
            pos.setIndex(start); 
        } else {
            return currAmt.orphan();
        }
    }
    return NULL;
}















void DecimalFormat::parse(const UnicodeString& text,
                          Formattable& result,
                          ParsePosition& parsePosition,
                          UChar* currency) const {
    int32_t startIdx, backup;
    int32_t i = startIdx = backup = parsePosition.getIndex();

    
    
    result.setLong(0);

    

    
    if (fFormatWidth > 0 && (fPadPosition == kPadBeforePrefix ||
                             fPadPosition == kPadAfterPrefix)) {
        i = skipPadding(text, i);
    }

    if (isLenient()) {
        
        i = backup = skipUWhiteSpace(text, i);
    }

    
    const UnicodeString *nan = &getConstSymbol(DecimalFormatSymbols::kNaNSymbol);
    int32_t nanLen = (text.compare(i, nan->length(), *nan)
                      ? 0 : nan->length());
    if (nanLen) {
        i += nanLen;
        if (fFormatWidth > 0 && (fPadPosition == kPadBeforeSuffix ||
                                 fPadPosition == kPadAfterSuffix)) {
            i = skipPadding(text, i);
        }
        parsePosition.setIndex(i);
        result.setDouble(uprv_getNaN());
        return;
    }

    
    i = backup;
    parsePosition.setIndex(i);

    
    UBool status[fgStatusLength];

    DigitList *digits = result.getInternalDigitList(); 
    if (digits == NULL) {
        return;    
    }

    if (fCurrencySignCount > fgCurrencySignCountZero) {
        if (!parseForCurrency(text, parsePosition, *digits,
                              status, currency)) {
          return;
        }
    } else {
        if (!subparse(text,
                      fNegPrefixPattern, fNegSuffixPattern,
                      fPosPrefixPattern, fPosSuffixPattern,
                      FALSE, UCURR_SYMBOL_NAME,
                      parsePosition, *digits, status, currency)) {
            debug("!subparse(...) - rewind");
            parsePosition.setIndex(startIdx);
            return;
        }
    }

    
    if (status[fgStatusInfinite]) {
        double inf = uprv_getInfinity();
        result.setDouble(digits->isPositive() ? inf : -inf);
        
    }

    else {

        if (fMultiplier != NULL) {
            UErrorCode ec = U_ZERO_ERROR;
            digits->div(*fMultiplier, ec);
        }

        
        
        
        if (digits->isZero() && !digits->isPositive() && isParseIntegerOnly()) {
            digits->setPositive(TRUE);
        }
        result.adoptDigitList(digits);
    }
}



UBool
DecimalFormat::parseForCurrency(const UnicodeString& text,
                                ParsePosition& parsePosition,
                                DigitList& digits,
                                UBool* status,
                                UChar* currency) const {
    int origPos = parsePosition.getIndex();
    int maxPosIndex = origPos;
    int maxErrorPos = -1;
    
    
    
    
    UBool tmpStatus[fgStatusLength];
    ParsePosition tmpPos(origPos);
    DigitList tmpDigitList;
    UBool found;
    if (fStyle == UNUM_CURRENCY_PLURAL) {
        found = subparse(text,
                         fNegPrefixPattern, fNegSuffixPattern,
                         fPosPrefixPattern, fPosSuffixPattern,
                         TRUE, UCURR_LONG_NAME,
                         tmpPos, tmpDigitList, tmpStatus, currency);
    } else {
        found = subparse(text,
                         fNegPrefixPattern, fNegSuffixPattern,
                         fPosPrefixPattern, fPosSuffixPattern,
                         TRUE, UCURR_SYMBOL_NAME,
                         tmpPos, tmpDigitList, tmpStatus, currency);
    }
    if (found) {
        if (tmpPos.getIndex() > maxPosIndex) {
            maxPosIndex = tmpPos.getIndex();
            for (int32_t i = 0; i < fgStatusLength; ++i) {
                status[i] = tmpStatus[i];
            }
            digits = tmpDigitList;
        }
    } else {
        maxErrorPos = tmpPos.getErrorIndex();
    }
    
    
    int32_t pos = -1;
    const UHashElement* element = NULL;
    while ( (element = fAffixPatternsForCurrency->nextElement(pos)) != NULL ) {
        const UHashTok valueTok = element->value;
        const AffixPatternsForCurrency* affixPtn = (AffixPatternsForCurrency*)valueTok.pointer;
        UBool tmpStatus[fgStatusLength];
        ParsePosition tmpPos(origPos);
        DigitList tmpDigitList;
        UBool result = subparse(text,
                                &affixPtn->negPrefixPatternForCurrency,
                                &affixPtn->negSuffixPatternForCurrency,
                                &affixPtn->posPrefixPatternForCurrency,
                                &affixPtn->posSuffixPatternForCurrency,
                                TRUE, affixPtn->patternType,
                                tmpPos, tmpDigitList, tmpStatus, currency);
        if (result) {
            found = true;
            if (tmpPos.getIndex() > maxPosIndex) {
                maxPosIndex = tmpPos.getIndex();
                for (int32_t i = 0; i < fgStatusLength; ++i) {
                    status[i] = tmpStatus[i];
                }
                digits = tmpDigitList;
            }
        } else {
            maxErrorPos = (tmpPos.getErrorIndex() > maxErrorPos) ?
                          tmpPos.getErrorIndex() : maxErrorPos;
        }
    }
    
    
    
    
    
    
    
    
    UBool tmpStatus_2[fgStatusLength];
    ParsePosition tmpPos_2(origPos);
    DigitList tmpDigitList_2;
    
    
    
    UBool result = subparse(text,
                            &fNegativePrefix, &fNegativeSuffix,
                            &fPositivePrefix, &fPositiveSuffix,
                            FALSE, UCURR_SYMBOL_NAME,
                            tmpPos_2, tmpDigitList_2, tmpStatus_2,
                            currency);
    if (result) {
        if (tmpPos_2.getIndex() > maxPosIndex) {
            maxPosIndex = tmpPos_2.getIndex();
            for (int32_t i = 0; i < fgStatusLength; ++i) {
                status[i] = tmpStatus_2[i];
            }
            digits = tmpDigitList_2;
        }
        found = true;
    } else {
            maxErrorPos = (tmpPos_2.getErrorIndex() > maxErrorPos) ?
                          tmpPos_2.getErrorIndex() : maxErrorPos;
    }

    if (!found) {
        
        parsePosition.setErrorIndex(maxErrorPos);
    } else {
        parsePosition.setIndex(maxPosIndex);
        parsePosition.setErrorIndex(-1);
    }
    return found;
}






















UBool DecimalFormat::subparse(const UnicodeString& text,
                              const UnicodeString* negPrefix,
                              const UnicodeString* negSuffix,
                              const UnicodeString* posPrefix,
                              const UnicodeString* posSuffix,
                              UBool currencyParsing,
                              int8_t type,
                              ParsePosition& parsePosition,
                              DigitList& digits, UBool* status,
                              UChar* currency) const
{
    
    
    
    UErrorCode err = U_ZERO_ERROR;
    CharString parsedNum;
    digits.setToZero();

    int32_t position = parsePosition.getIndex();
    int32_t oldStart = position;
    int32_t textLength = text.length(); 
    UBool strictParse = !isLenient();
    UChar32 zero = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
    const UnicodeString *groupingString = &getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol);
    UChar32 groupingChar = groupingString->char32At(0);
    int32_t groupingStringLength = groupingString->length();
    int32_t groupingCharLength   = U16_LENGTH(groupingChar);
    UBool   groupingUsed = isGroupingUsed();
#ifdef FMT_DEBUG
    UChar dbgbuf[300];
    UnicodeString s(dbgbuf,0,300);;
    s.append((UnicodeString)"PARSE \"").append(text.tempSubString(position)).append((UnicodeString)"\" " );
#define DBGAPPD(x) if(x) { s.append(UnicodeString(#x "="));  if(x->isEmpty()) { s.append(UnicodeString("<empty>")); } else { s.append(*x); } s.append(UnicodeString(" ")); } else { s.append(UnicodeString(#x "=NULL ")); }
    DBGAPPD(negPrefix);
    DBGAPPD(negSuffix);
    DBGAPPD(posPrefix);
    DBGAPPD(posSuffix);
    debugout(s);
    printf("currencyParsing=%d, fFormatWidth=%d, isParseIntegerOnly=%c text.length=%d negPrefLen=%d\n", currencyParsing, fFormatWidth, (isParseIntegerOnly())?'Y':'N', text.length(),  negPrefix!=NULL?negPrefix->length():-1);
#endif

    UBool fastParseOk = false; 
    UBool fastParseHadDecimal = FALSE; 
    const DecimalFormatInternal &data = internalData(fReserved);
    if((data.fFastParseStatus==kFastpathYES) &&
       !currencyParsing &&
       
       text.length()>0 &&
       text.length()<32 &&
       (posPrefix==NULL||posPrefix->isEmpty()) &&
       (posSuffix==NULL||posSuffix->isEmpty()) &&
       
       
       TRUE) {  
      int j=position;
      int l=text.length();
      int digitCount=0;
      UChar32 ch = text.char32At(j);
      const UnicodeString *decimalString = &getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
      UChar32 decimalChar = 0;
      UBool intOnly = FALSE;
      UChar32 lookForGroup = (groupingUsed&&intOnly&&strictParse)?groupingChar:0;

      int32_t decimalCount = decimalString->countChar32(0,3);
      if(isParseIntegerOnly()) {
        decimalChar = 0; 
        intOnly = TRUE; 
      } else if(decimalCount==1) {
        decimalChar = decimalString->char32At(0); 
      } else if(decimalCount==0) {
        decimalChar=0; 
      } else {
        j=l+1;
      }

#ifdef FMT_DEBUG
      printf("Preparing to do fastpath parse: decimalChar=U+%04X, groupingChar=U+%04X, first ch=U+%04X intOnly=%c strictParse=%c\n",
        decimalChar, groupingChar, ch,
        (intOnly)?'y':'n',
        (strictParse)?'y':'n');
#endif
      if(ch==0x002D) { 
        j=l+1;
        
        




      } else {
        parsedNum.append('+',err);
      }
      while(j<l) {
        int32_t digit = ch - zero;
        if(digit >=0 && digit <= 9) {
          parsedNum.append((char)(digit + '0'), err);
          if((digitCount>0) || digit!=0 || j==(l-1)) {
            digitCount++;
          }
        } else if(ch == 0) { 
          digitCount=-1;
          break;
        } else if(ch == decimalChar) {
          parsedNum.append((char)('.'), err);
          decimalChar=0; 
          fastParseHadDecimal=TRUE;
        } else if(ch == lookForGroup) {
          
        } else if(intOnly && (lookForGroup!=0) && !u_isdigit(ch)) {
          
        } else {
          digitCount=-1; 
          break;
        }
        j+=U16_LENGTH(ch);
        ch = text.char32At(j); 
      }
      if(
         ((j==l)||intOnly) 
         && (digitCount>0)) { 
#ifdef FMT_DEBUG
        printf("PP -> %d, good = [%s]  digitcount=%d, fGroupingSize=%d fGroupingSize2=%d!\n", j, parsedNum.data(), digitCount, fGroupingSize, fGroupingSize2);
#endif
        fastParseOk=true; 

#ifdef SKIP_OPT
        debug("SKIP_OPT");
        
        fastParseOk=false;
        parsedNum.clear();
#else
        parsePosition.setIndex(position=j);
        status[fgStatusInfinite]=false;
#endif
      } else {
        
#ifdef FMT_DEBUG
        printf("Fall through: j=%d, l=%d, digitCount=%d\n", j, l, digitCount);
#endif
        parsedNum.clear();
      }
    } else {
#ifdef FMT_DEBUG
      printf("Could not fastpath parse. ");
      printf("fFormatWidth=%d ", fFormatWidth);
      printf("text.length()=%d ", text.length());
      printf("posPrefix=%p posSuffix=%p ", posPrefix, posSuffix);

      printf("\n");
#endif
    }

  if(!fastParseOk 
#if UCONFIG_HAVE_PARSEALLINPUT
     && fParseAllInput!=UNUM_YES
#endif
     ) 
  {
    
    if (fFormatWidth > 0 && fPadPosition == kPadBeforePrefix) {
        position = skipPadding(text, position);
    }

    
    int32_t posMatch = compareAffix(text, position, FALSE, TRUE, posPrefix, currencyParsing, type, currency);
    int32_t negMatch = compareAffix(text, position, TRUE,  TRUE, negPrefix, currencyParsing, type, currency);
    if (posMatch >= 0 && negMatch >= 0) {
        if (posMatch > negMatch) {
            negMatch = -1;
        } else if (negMatch > posMatch) {
            posMatch = -1;
        }
    }
    if (posMatch >= 0) {
        position += posMatch;
        parsedNum.append('+', err);
    } else if (negMatch >= 0) {
        position += negMatch;
        parsedNum.append('-', err);
    } else if (strictParse){
        parsePosition.setErrorIndex(position);
        return FALSE;
    } else {
        
        parsedNum.append('+', err);
    }

    
    if (fFormatWidth > 0 && fPadPosition == kPadAfterPrefix) {
        position = skipPadding(text, position);
    }

    if (! strictParse) {
        position = skipUWhiteSpace(text, position);
    }

    
    const UnicodeString *inf = &getConstSymbol(DecimalFormatSymbols::kInfinitySymbol);
    int32_t infLen = (text.compare(position, inf->length(), *inf)
        ? 0 : inf->length());
    position += infLen; 
    status[fgStatusInfinite] = infLen != 0;

    if (infLen != 0) {
        parsedNum.append("Infinity", err);
    } else {
        
        
        
        
        
        


        UBool strictFail = FALSE; 
        int32_t lastGroup = -1; 
        int32_t digitStart = position;
        int32_t gs2 = fGroupingSize2 == 0 ? fGroupingSize : fGroupingSize2;

        const UnicodeString *decimalString;
        if (fCurrencySignCount > fgCurrencySignCountZero) {
            decimalString = &getConstSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol);
        } else {
            decimalString = &getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
        }
        UChar32 decimalChar = decimalString->char32At(0);
        int32_t decimalStringLength = decimalString->length();
        int32_t decimalCharLength   = U16_LENGTH(decimalChar);

        UBool sawDecimal = FALSE;
        UChar32 sawDecimalChar = 0xFFFF;
        UBool sawGrouping = FALSE;
        UChar32 sawGroupingChar = 0xFFFF;
        UBool sawDigit = FALSE;
        int32_t backup = -1;
        int32_t digit;

        
        const UnicodeSet *decimalSet = NULL;
        const UnicodeSet *groupingSet = NULL;

        if (decimalCharLength == decimalStringLength) {
            decimalSet = DecimalFormatStaticSets::getSimilarDecimals(decimalChar, strictParse);
        }

        if (groupingCharLength == groupingStringLength) {
            if (strictParse) {
                groupingSet = DecimalFormatStaticSets::gStaticSets->fStrictDefaultGroupingSeparators;
            } else {
                groupingSet = DecimalFormatStaticSets::gStaticSets->fDefaultGroupingSeparators;
            }
        }

        
        
        

        
        
        int32_t digitCount = 0;
        int32_t integerDigitCount = 0;

        for (; position < textLength; )
        {
            UChar32 ch = text.char32At(position);

            










            digit = ch - zero;
            if (digit < 0 || digit > 9)
            {
                digit = u_charDigitValue(ch);
            }
            
            
            
            if ( (digit < 0 || digit > 9) && u_charDigitValue(zero) != 0) {
                digit = 0;
                if ( getConstSymbol((DecimalFormatSymbols::ENumberFormatSymbol)(DecimalFormatSymbols::kZeroDigitSymbol)).char32At(0) == ch ) {
                    break;
                }
                for (digit = 1 ; digit < 10 ; digit++ ) {
                    if ( getConstSymbol((DecimalFormatSymbols::ENumberFormatSymbol)(DecimalFormatSymbols::kOneDigitSymbol+digit-1)).char32At(0) == ch ) {
                        break;
                    }
                }
            }

            if (digit >= 0 && digit <= 9)
            {
                if (strictParse && backup != -1) {
                    
                    
                    
                    
                    
                    if ((lastGroup != -1 && backup - lastGroup - 1 != gs2) ||
                        (lastGroup == -1 && position - digitStart - 1 > gs2)) {
                        strictFail = TRUE;
                        break;
                    }
                    
                    lastGroup = backup;
                }
                
                
                backup = -1;
                sawDigit = TRUE;
                
                
                parsedNum.append((char)(digit + '0'), err);

                
                if (digit > 0 || digitCount > 0 || sawDecimal) {
                    digitCount += 1;
                    
                    
                    if (! sawDecimal) {
                        integerDigitCount += 1;
                    }
                }
                    
                position += U16_LENGTH(ch);
            }
            else if (groupingStringLength > 0 && 
                matchGrouping(groupingChar, sawGrouping, sawGroupingChar, groupingSet, 
                            decimalChar, decimalSet,
                            ch) && groupingUsed)
            {
                if (sawDecimal) {
                    break;
                }

                if (strictParse) {
                    if ((!sawDigit || backup != -1)) {
                        
                        strictFail = TRUE;
                        break;
                    }
                }

                
                
                
                backup = position;
                position += groupingStringLength;
                sawGrouping=TRUE;
                
                sawGroupingChar=ch;
            }
            else if (matchDecimal(decimalChar,sawDecimal,sawDecimalChar, decimalSet, ch))
            {
                if (strictParse) {
                    if (backup != -1 ||
                        (lastGroup != -1 && position - lastGroup != fGroupingSize + 1)) {
                        strictFail = TRUE;
                        break;
                    }
                }

                
                
                if (isParseIntegerOnly() || sawDecimal) {
                    break;
                }

                parsedNum.append('.', err);
                position += decimalStringLength;
                sawDecimal = TRUE;
                
                sawDecimalChar=ch;
                
            }
            else {

                if(!fBoolFlags.contains(UNUM_PARSE_NO_EXPONENT) || 
                    fUseExponentialNotation ) { 
                    const UnicodeString *tmp;
                    tmp = &getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);
                    
                    if (!text.caseCompare(position, tmp->length(), *tmp, U_FOLD_CASE_DEFAULT))    
                    {
                        
                        int32_t pos = position + tmp->length();
                        char exponentSign = '+';

                        if (pos < textLength)
                        {
                            tmp = &getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                            if (!text.compare(pos, tmp->length(), *tmp))
                            {
                                pos += tmp->length();
                            }
                            else {
                                tmp = &getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                                if (!text.compare(pos, tmp->length(), *tmp))
                                {
                                    exponentSign = '-';
                                    pos += tmp->length();
                                }
                            }
                        }

                        UBool sawExponentDigit = FALSE;
                        while (pos < textLength) {
                            ch = text[(int32_t)pos];
                            digit = ch - zero;

                            if (digit < 0 || digit > 9) {
                                digit = u_charDigitValue(ch);
                            }
                            if (0 <= digit && digit <= 9) {
                                if (!sawExponentDigit) {
                                    parsedNum.append('E', err);
                                    parsedNum.append(exponentSign, err);
                                    sawExponentDigit = TRUE;
                                }
                                ++pos;
                                parsedNum.append((char)(digit + '0'), err);
                            } else {
                                break;
                            }
                        }

                        if (sawExponentDigit) {
                            position = pos; 
                        }

                        break; 
                    } else {
                        break;
                    }
                } else { 
                    break;
              }
            }
        }

        if (backup != -1)
        {
            position = backup;
        }

        if (strictParse && !sawDecimal) {
            if (lastGroup != -1 && position - lastGroup != fGroupingSize + 1) {
                strictFail = TRUE;
            }
        }

        if (strictFail) {
            

            parsePosition.setIndex(oldStart);
            parsePosition.setErrorIndex(position);
            debug("strictFail!");
            return FALSE;
        }

        

        
        
        
        
        if (!sawDigit && digitCount == 0) {
#ifdef FMT_DEBUG
            debug("none of text rec");
            printf("position=%d\n",position);
#endif
            parsePosition.setIndex(oldStart);
            parsePosition.setErrorIndex(oldStart);
            return FALSE;
        }
    }

    
    if (fFormatWidth > 0 && fPadPosition == kPadBeforeSuffix) {
        position = skipPadding(text, position);
    }

    int32_t posSuffixMatch = -1, negSuffixMatch = -1;

    
    if (posMatch >= 0 || (!strictParse && negMatch < 0)) {
        posSuffixMatch = compareAffix(text, position, FALSE, FALSE, posSuffix, currencyParsing, type, currency);
    }
    if (negMatch >= 0) {
        negSuffixMatch = compareAffix(text, position, TRUE, FALSE, negSuffix, currencyParsing, type, currency);
    }
    if (posSuffixMatch >= 0 && negSuffixMatch >= 0) {
        if (posSuffixMatch > negSuffixMatch) {
            negSuffixMatch = -1;
        } else if (negSuffixMatch > posSuffixMatch) {
            posSuffixMatch = -1;
        }
    }

    
    if (strictParse && ((posSuffixMatch >= 0) == (negSuffixMatch >= 0))) {
        parsePosition.setErrorIndex(position);
        debug("neither or both");
        return FALSE;
    }

    position += (posSuffixMatch >= 0 ? posSuffixMatch : (negSuffixMatch >= 0 ? negSuffixMatch : 0));

    
    if (fFormatWidth > 0 && fPadPosition == kPadAfterSuffix) {
        position = skipPadding(text, position);
    }

    parsePosition.setIndex(position);

    parsedNum.data()[0] = (posSuffixMatch >= 0 || (!strictParse && negMatch < 0 && negSuffixMatch < 0)) ? '+' : '-';
#ifdef FMT_DEBUG
printf("PP -> %d, SLOW = [%s]!    pp=%d, os=%d, err=%s\n", position, parsedNum.data(), parsePosition.getIndex(),oldStart,u_errorName(err));
#endif
  } 
  if(parsePosition.getIndex() == oldStart)
    {
#ifdef FMT_DEBUG
      printf(" PP didnt move, err\n");
#endif
        parsePosition.setErrorIndex(position);
        return FALSE;
    }
#if UCONFIG_HAVE_PARSEALLINPUT
  else if (fParseAllInput==UNUM_YES&&parsePosition.getIndex()!=textLength)
    {
#ifdef FMT_DEBUG
      printf(" PP didnt consume all (UNUM_YES), err\n");
#endif
        parsePosition.setErrorIndex(position);
        return FALSE;
    }
#endif
    
    
    
    digits.set(parsedNum.toStringPiece(),
               err,
               0
               );

    if (U_FAILURE(err)) {
#ifdef FMT_DEBUG
      printf(" err setting %s\n", u_errorName(err));
#endif
        parsePosition.setErrorIndex(position);
        return FALSE;
    }
    return TRUE;
}






int32_t DecimalFormat::skipPadding(const UnicodeString& text, int32_t position) const {
    int32_t padLen = U16_LENGTH(fPad);
    while (position < text.length() &&
           text.char32At(position) == fPad) {
        position += padLen;
    }
    return position;
}



















int32_t DecimalFormat::compareAffix(const UnicodeString& text,
                                    int32_t pos,
                                    UBool isNegative,
                                    UBool isPrefix,
                                    const UnicodeString* affixPat,
                                    UBool currencyParsing,
                                    int8_t type,
                                    UChar* currency) const
{
    const UnicodeString *patternToCompare;
    if (fCurrencyChoice != NULL || currency != NULL ||
        (fCurrencySignCount > fgCurrencySignCountZero && currencyParsing)) {

        if (affixPat != NULL) {
            return compareComplexAffix(*affixPat, text, pos, type, currency);
        }
    }

    if (isNegative) {
        if (isPrefix) {
            patternToCompare = &fNegativePrefix;
        }
        else {
            patternToCompare = &fNegativeSuffix;
        }
    }
    else {
        if (isPrefix) {
            patternToCompare = &fPositivePrefix;
        }
        else {
            patternToCompare = &fPositiveSuffix;
        }
    }
    return compareSimpleAffix(*patternToCompare, text, pos, isLenient());
}











int32_t DecimalFormat::compareSimpleAffix(const UnicodeString& affix,
                                          const UnicodeString& input,
                                          int32_t pos,
                                          UBool lenient) {
    int32_t start = pos;
    UChar32 affixChar = affix.char32At(0);
    int32_t affixLength = affix.length();
    int32_t inputLength = input.length();
    int32_t affixCharLength = U16_LENGTH(affixChar);
    UnicodeSet *affixSet;

    if (!lenient) {
        affixSet = DecimalFormatStaticSets::gStaticSets->fStrictDashEquivalents;
        
        
        
        
        if (affixCharLength == affixLength && affixSet->contains(affixChar))  {
            if (affixSet->contains(input.char32At(pos))) {
                return 1;
            }
        }

        for (int32_t i = 0; i < affixLength; ) {
            UChar32 c = affix.char32At(i);
            int32_t len = U16_LENGTH(c);
            if (PatternProps::isWhiteSpace(c)) {
                
                
                
                
                
                
                UBool literalMatch = FALSE;
                while (pos < inputLength &&
                       input.char32At(pos) == c) {
                    literalMatch = TRUE;
                    i += len;
                    pos += len;
                    if (i == affixLength) {
                        break;
                    }
                    c = affix.char32At(i);
                    len = U16_LENGTH(c);
                    if (!PatternProps::isWhiteSpace(c)) {
                        break;
                    }
                }

                
                i = skipPatternWhiteSpace(affix, i);

                
                
                
                int32_t s = pos;
                pos = skipUWhiteSpace(input, pos);
                if (pos == s && !literalMatch) {
                    return -1;
                }

                
                
                
                i = skipUWhiteSpace(affix, i);
            } else {
                if (pos < inputLength &&
                    input.char32At(pos) == c) {
                    i += len;
                    pos += len;
                } else {
                    return -1;
                }
            }
        }
    } else {
        UBool match = FALSE;
        
        affixSet = DecimalFormatStaticSets::gStaticSets->fDashEquivalents;

        if (affixCharLength == affixLength && affixSet->contains(affixChar))  {
            pos = skipUWhiteSpace(input, pos);
            
            if (affixSet->contains(input.char32At(pos))) {
                return pos - start + 1;
            }
        }

        for (int32_t i = 0; i < affixLength; )
        {
            
            i = skipUWhiteSpace(affix, i);
            pos = skipUWhiteSpace(input, pos);

            if (i >= affixLength || pos >= inputLength) {
                break;
            }

            UChar32 c = affix.char32At(i);
            int32_t len = U16_LENGTH(c);

            if (input.char32At(pos) != c) {
                return -1;
            }

            match = TRUE;
            i += len;
            pos += len;
        }

        if (affixLength > 0 && ! match) {
            return -1;
        }
    }
    return pos - start;
}





int32_t DecimalFormat::skipPatternWhiteSpace(const UnicodeString& text, int32_t pos) {
    const UChar* s = text.getBuffer();
    return (int32_t)(PatternProps::skipWhiteSpace(s + pos, text.length() - pos) - s);
}





int32_t DecimalFormat::skipUWhiteSpace(const UnicodeString& text, int32_t pos) {
    while (pos < text.length()) {
        UChar32 c = text.char32At(pos);
        if (!u_isUWhiteSpace(c)) {
            break;
        }
        pos += U16_LENGTH(c);
    }
    return pos;
}













int32_t DecimalFormat::compareComplexAffix(const UnicodeString& affixPat,
                                           const UnicodeString& text,
                                           int32_t pos,
                                           int8_t type,
                                           UChar* currency) const
{
    int32_t start = pos;
    U_ASSERT(currency != NULL ||
             (fCurrencyChoice != NULL && *getCurrency() != 0) ||
             fCurrencySignCount > fgCurrencySignCountZero);

    for (int32_t i=0;
         i<affixPat.length() && pos >= 0; ) {
        UChar32 c = affixPat.char32At(i);
        i += U16_LENGTH(c);

        if (c == kQuote) {
            U_ASSERT(i <= affixPat.length());
            c = affixPat.char32At(i);
            i += U16_LENGTH(c);

            const UnicodeString* affix = NULL;

            switch (c) {
            case kCurrencySign: {
                
                
                
                
                
                UBool intl = i<affixPat.length() &&
                    affixPat.char32At(i) == kCurrencySign;
                if (intl) {
                    ++i;
                }
                UBool plural = i<affixPat.length() &&
                    affixPat.char32At(i) == kCurrencySign;
                if (plural) {
                    ++i;
                    intl = FALSE;
                }
                
                
                
                
                const char* loc = fCurrencyPluralInfo->getLocale().getName();
                ParsePosition ppos(pos);
                UChar curr[4];
                UErrorCode ec = U_ZERO_ERROR;
                
                uprv_parseCurrency(loc, text, ppos, type, curr, ec);

                
                if (U_SUCCESS(ec) && ppos.getIndex() != pos) {
                    if (currency) {
                        u_strcpy(currency, curr);
                    } else {
                        
                        
                        
                        UChar effectiveCurr[4];
                        getEffectiveCurrency(effectiveCurr, ec);
                        if ( U_FAILURE(ec) || u_strncmp(curr,effectiveCurr,4) != 0 ) {
                        	pos = -1;
                        	continue;
                        }
                    }
                    pos = ppos.getIndex();
                } else if (!isLenient()){
                    pos = -1;
                }
                continue;
            }
            case kPatternPercent:
                affix = &getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                break;
            case kPatternPerMill:
                affix = &getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                break;
            case kPatternPlus:
                affix = &getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                break;
            case kPatternMinus:
                affix = &getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                break;
            default:
                
                break;
            }

            if (affix != NULL) {
                pos = match(text, pos, *affix);
                continue;
            }
        }

        pos = match(text, pos, c);
        if (PatternProps::isWhiteSpace(c)) {
            i = skipPatternWhiteSpace(affixPat, i);
        }
    }
    return pos - start;
}






int32_t DecimalFormat::match(const UnicodeString& text, int32_t pos, UChar32 ch) {
    if (PatternProps::isWhiteSpace(ch)) {
        
        
        int32_t s = pos;
        pos = skipPatternWhiteSpace(text, pos);
        if (pos == s) {
            return -1;
        }
        return pos;
    }
    return (pos >= 0 && text.char32At(pos) == ch) ?
        (pos + U16_LENGTH(ch)) : -1;
}






int32_t DecimalFormat::match(const UnicodeString& text, int32_t pos, const UnicodeString& str) {
    for (int32_t i=0; i<str.length() && pos >= 0; ) {
        UChar32 ch = str.char32At(i);
        i += U16_LENGTH(ch);
        if (PatternProps::isWhiteSpace(ch)) {
            i = skipPatternWhiteSpace(str, i);
        }
        pos = match(text, pos, ch);
    }
    return pos;
}

UBool DecimalFormat::matchSymbol(const UnicodeString &text, int32_t position, int32_t length, const UnicodeString &symbol,
                         UnicodeSet *sset, UChar32 schar)
{
    if (sset != NULL) {
        return sset->contains(schar);
    }

    return text.compare(position, length, symbol) == 0;
}

UBool DecimalFormat::matchDecimal(UChar32 symbolChar,
                            UBool sawDecimal,  UChar32 sawDecimalChar,
                             const UnicodeSet *sset, UChar32 schar) {
   if(sawDecimal) {
       return schar==sawDecimalChar;
   } else if(schar==symbolChar) {
       return TRUE;
   } else if(sset!=NULL) {
        return sset->contains(schar);
   } else {
       return FALSE;
   }
}

UBool DecimalFormat::matchGrouping(UChar32 groupingChar,
                            UBool sawGrouping, UChar32 sawGroupingChar,
                             const UnicodeSet *sset,
                             UChar32 , const UnicodeSet *decimalSet,
                             UChar32 schar) {
    if(sawGrouping) {
        return schar==sawGroupingChar;  
    } else if(schar==groupingChar) {
        return TRUE; 
    } else if(sset!=NULL) {
        return sset->contains(schar) &&  
           ((decimalSet==NULL) || !decimalSet->contains(schar)); 
    } else {
        return FALSE;
    }
}






const DecimalFormatSymbols*
DecimalFormat::getDecimalFormatSymbols() const
{
    return fSymbols;
}




void
DecimalFormat::adoptDecimalFormatSymbols(DecimalFormatSymbols* symbolsToAdopt)
{
    if (symbolsToAdopt == NULL) {
        return; 
    }

    UBool sameSymbols = FALSE;
    if (fSymbols != NULL) {
        sameSymbols = (UBool)(getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) ==
            symbolsToAdopt->getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) &&
            getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol) ==
            symbolsToAdopt->getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol));
        delete fSymbols;
    }

    fSymbols = symbolsToAdopt;
    if (!sameSymbols) {
        
        setCurrencyForSymbols();
    }
    expandAffixes(NULL);
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}




void
DecimalFormat::setDecimalFormatSymbols(const DecimalFormatSymbols& symbols)
{
    adoptDecimalFormatSymbols(new DecimalFormatSymbols(symbols));
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}


const CurrencyPluralInfo*
DecimalFormat::getCurrencyPluralInfo(void) const
{
    return fCurrencyPluralInfo;
}


void
DecimalFormat::adoptCurrencyPluralInfo(CurrencyPluralInfo* toAdopt)
{
    if (toAdopt != NULL) {
        delete fCurrencyPluralInfo;
        fCurrencyPluralInfo = toAdopt;
        
        if (fCurrencySignCount > fgCurrencySignCountZero) {
            UErrorCode status = U_ZERO_ERROR;
            if (fAffixPatternsForCurrency) {
                deleteHashForAffixPattern();
            }
            setupCurrencyAffixPatterns(status);
            if (fCurrencySignCount == fgCurrencySignCountInPluralFormat) {
                
                setupCurrencyAffixes(fFormatPattern, FALSE, TRUE, status);
            }
        }
    }
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}

void
DecimalFormat::setCurrencyPluralInfo(const CurrencyPluralInfo& info)
{
    adoptCurrencyPluralInfo(info.clone());
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}







void
DecimalFormat::setCurrencyForSymbols() {
    





    
    
    
    
    
    
    UErrorCode ec = U_ZERO_ERROR;
    const UChar* c = NULL;
    const char* loc = fSymbols->getLocale().getName();
    UChar intlCurrencySymbol[4];
    ucurr_forLocale(loc, intlCurrencySymbol, 4, &ec);
    UnicodeString currencySymbol;

    uprv_getStaticCurrencyName(intlCurrencySymbol, loc, currencySymbol, ec);
    if (U_SUCCESS(ec)
        && getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) == currencySymbol
        && getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol) == UnicodeString(intlCurrencySymbol))
    {
        
        
        c = intlCurrencySymbol;
    }
    ec = U_ZERO_ERROR; 
    setCurrencyInternally(c, ec);
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}





UnicodeString&
DecimalFormat::getPositivePrefix(UnicodeString& result) const
{
    result = fPositivePrefix;
    return result;
}




void
DecimalFormat::setPositivePrefix(const UnicodeString& newValue)
{
    fPositivePrefix = newValue;
    delete fPosPrefixPattern;
    fPosPrefixPattern = 0;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}




UnicodeString&
DecimalFormat::getNegativePrefix(UnicodeString& result) const
{
    result = fNegativePrefix;
    return result;
}




void
DecimalFormat::setNegativePrefix(const UnicodeString& newValue)
{
    fNegativePrefix = newValue;
    delete fNegPrefixPattern;
    fNegPrefixPattern = 0;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}




UnicodeString&
DecimalFormat::getPositiveSuffix(UnicodeString& result) const
{
    result = fPositiveSuffix;
    return result;
}




void
DecimalFormat::setPositiveSuffix(const UnicodeString& newValue)
{
    fPositiveSuffix = newValue;
    delete fPosSuffixPattern;
    fPosSuffixPattern = 0;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}




UnicodeString&
DecimalFormat::getNegativeSuffix(UnicodeString& result) const
{
    result = fNegativeSuffix;
    return result;
}




void
DecimalFormat::setNegativeSuffix(const UnicodeString& newValue)
{
    fNegativeSuffix = newValue;
    delete fNegSuffixPattern;
    fNegSuffixPattern = 0;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}







int32_t 
DecimalFormat::getMultiplier() const
{
    if (fMultiplier == NULL) {
        return 1;
    } else {
        return fMultiplier->getLong();
    }
}



void
DecimalFormat::setMultiplier(int32_t newValue)
{



    if (newValue == 0) {
        newValue = 1;     
    }
    if (newValue == 1) {
        delete fMultiplier;
        fMultiplier = NULL;
    } else {
        if (fMultiplier == NULL) {
            fMultiplier = new DigitList;
        }
        if (fMultiplier != NULL) {
            fMultiplier->set(newValue);
        }
    }
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}









double DecimalFormat::getRoundingIncrement() const {
    if (fRoundingIncrement == NULL) {
        return 0.0;
    } else {
        return fRoundingIncrement->getDouble();
    }
}










void DecimalFormat::setRoundingIncrement(double newValue) {
    if (newValue > 0.0) {
        if (fRoundingIncrement == NULL) {
            fRoundingIncrement = new DigitList();
        }
        if (fRoundingIncrement != NULL) {
            fRoundingIncrement->set(newValue);
            return;
        }
    }
    
    
    delete fRoundingIncrement;
    fRoundingIncrement = NULL;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}








DecimalFormat::ERoundingMode DecimalFormat::getRoundingMode() const {
    return fRoundingMode;
}









void DecimalFormat::setRoundingMode(ERoundingMode roundingMode) {
    fRoundingMode = roundingMode;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}










int32_t DecimalFormat::getFormatWidth() const {
    return fFormatWidth;
}













void DecimalFormat::setFormatWidth(int32_t width) {
    fFormatWidth = (width > 0) ? width : 0;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}

UnicodeString DecimalFormat::getPadCharacterString() const {
    return UnicodeString(fPad);
}

void DecimalFormat::setPadCharacter(const UnicodeString &padChar) {
    if (padChar.length() > 0) {
        fPad = padChar.char32At(0);
    }
    else {
        fPad = kDefaultPad;
    }
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}


















DecimalFormat::EPadPosition DecimalFormat::getPadPosition() const {
    return fPadPosition;
}




















void DecimalFormat::setPadPosition(EPadPosition padPos) {
    fPadPosition = padPos;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}










UBool DecimalFormat::isScientificNotation() {
    return fUseExponentialNotation;
}











void DecimalFormat::setScientificNotation(UBool useScientific) {
    fUseExponentialNotation = useScientific;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}










int8_t DecimalFormat::getMinimumExponentDigits() const {
    return fMinExponentDigits;
}












void DecimalFormat::setMinimumExponentDigits(int8_t minExpDig) {
    fMinExponentDigits = (int8_t)((minExpDig > 0) ? minExpDig : 1);
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}












UBool DecimalFormat::isExponentSignAlwaysShown() {
    return fExponentSignAlwaysShown;
}













void DecimalFormat::setExponentSignAlwaysShown(UBool expSignAlways) {
    fExponentSignAlwaysShown = expSignAlways;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}





int32_t
DecimalFormat::getGroupingSize() const
{
    return fGroupingSize;
}




void
DecimalFormat::setGroupingSize(int32_t newValue)
{
    fGroupingSize = newValue;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}



int32_t
DecimalFormat::getSecondaryGroupingSize() const
{
    return fGroupingSize2;
}



void
DecimalFormat::setSecondaryGroupingSize(int32_t newValue)
{
    fGroupingSize2 = newValue;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}




UBool
DecimalFormat::isDecimalSeparatorAlwaysShown() const
{
    return fDecimalSeparatorAlwaysShown;
}




void
DecimalFormat::setDecimalSeparatorAlwaysShown(UBool newValue)
{
    fDecimalSeparatorAlwaysShown = newValue;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}




UnicodeString&
DecimalFormat::toPattern(UnicodeString& result) const
{
    return toPattern(result, FALSE);
}




UnicodeString&
DecimalFormat::toLocalizedPattern(UnicodeString& result) const
{
    return toPattern(result, TRUE);
}















void DecimalFormat::expandAffixes(const UnicodeString* pluralCount) {
    FieldPositionHandler none;
    if (fPosPrefixPattern != 0) {
      expandAffix(*fPosPrefixPattern, fPositivePrefix, 0, none, FALSE, pluralCount);
    }
    if (fPosSuffixPattern != 0) {
      expandAffix(*fPosSuffixPattern, fPositiveSuffix, 0, none, FALSE, pluralCount);
    }
    if (fNegPrefixPattern != 0) {
      expandAffix(*fNegPrefixPattern, fNegativePrefix, 0, none, FALSE, pluralCount);
    }
    if (fNegSuffixPattern != 0) {
      expandAffix(*fNegSuffixPattern, fNegativeSuffix, 0, none, FALSE, pluralCount);
    }
#ifdef FMT_DEBUG
    UnicodeString s;
    s.append(UnicodeString("["))
      .append(DEREFSTR(fPosPrefixPattern)).append((UnicodeString)"|").append(DEREFSTR(fPosSuffixPattern))
      .append((UnicodeString)";") .append(DEREFSTR(fNegPrefixPattern)).append((UnicodeString)"|").append(DEREFSTR(fNegSuffixPattern))
        .append((UnicodeString)"]->[")
        .append(fPositivePrefix).append((UnicodeString)"|").append(fPositiveSuffix)
        .append((UnicodeString)";") .append(fNegativePrefix).append((UnicodeString)"|").append(fNegativeSuffix)
        .append((UnicodeString)"]\n");
    debugout(s);
#endif
}








































void DecimalFormat::expandAffix(const UnicodeString& pattern,
                                UnicodeString& affix,
                                double number,
                                FieldPositionHandler& handler,
                                UBool doFormat,
                                const UnicodeString* pluralCount) const {
    affix.remove();
    for (int i=0; i<pattern.length(); ) {
        UChar32 c = pattern.char32At(i);
        i += U16_LENGTH(c);
        if (c == kQuote) {
            c = pattern.char32At(i);
            i += U16_LENGTH(c);
            int beginIdx = affix.length();
            switch (c) {
            case kCurrencySign: {
                
                
                
                
                
                UBool intl = i<pattern.length() &&
                    pattern.char32At(i) == kCurrencySign;
                UBool plural = FALSE;
                if (intl) {
                    ++i;
                    plural = i<pattern.length() &&
                        pattern.char32At(i) == kCurrencySign;
                    if (plural) {
                        intl = FALSE;
                        ++i;
                    }
                }
                const UChar* currencyUChars = getCurrency();
                if (currencyUChars[0] != 0) {
                    UErrorCode ec = U_ZERO_ERROR;
                    if (plural && pluralCount != NULL) {
                        
                        
                        
                        
                        int32_t len;
                        CharString pluralCountChar;
                        pluralCountChar.appendInvariantChars(*pluralCount, ec);
                        UBool isChoiceFormat;
                        const UChar* s = ucurr_getPluralName(currencyUChars,
                            fSymbols != NULL ? fSymbols->getLocale().getName() :
                            Locale::getDefault().getName(), &isChoiceFormat,
                            pluralCountChar.data(), &len, &ec);
                        affix += UnicodeString(s, len);
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    } else if(intl) {
                        affix.append(currencyUChars, -1);
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    } else {
                        int32_t len;
                        UBool isChoiceFormat;
                        
                        const UChar* s = ucurr_getName(currencyUChars,
                            fSymbols != NULL ? fSymbols->getLocale().getName() : Locale::getDefault().getName(),
                            UCURR_SYMBOL_NAME, &isChoiceFormat, &len, &ec);
                        if (isChoiceFormat) {
                            
                            
                            
                            
                            if (!doFormat) {
                                
                                
                                
                                
                                if (fCurrencyChoice == NULL) {
                                    
                                    ChoiceFormat* fmt = new ChoiceFormat(UnicodeString(s), ec);
                                    if (U_SUCCESS(ec)) {
                                        umtx_lock(NULL);
                                        if (fCurrencyChoice == NULL) {
                                            
                                            ((DecimalFormat*)this)->fCurrencyChoice = fmt;
                                            fmt = NULL;
                                        }
                                        umtx_unlock(NULL);
                                        delete fmt;
                                    }
                                }
                                
                                
                                
                                
                                
                                
                                
                                affix.append(kCurrencySign);
                            } else {
                                if (fCurrencyChoice != NULL) {
                                    FieldPosition pos(0); 
                                    if (number < 0) {
                                        number = -number;
                                    }
                                    fCurrencyChoice->format(number, affix, pos);
                                } else {
                                    
                                    
                                    affix.append(currencyUChars, -1);
                                    handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                                }
                            }
                            continue;
                        }
                        affix += UnicodeString(s, len);
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    }
                } else {
                    if(intl) {
                        affix += getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
                    } else {
                        affix += getConstSymbol(DecimalFormatSymbols::kCurrencySymbol);
                    }
                    handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                }
                break;
            }
            case kPatternPercent:
                affix += getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                handler.addAttribute(kPercentField, beginIdx, affix.length());
                break;
            case kPatternPerMill:
                affix += getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                handler.addAttribute(kPermillField, beginIdx, affix.length());
                break;
            case kPatternPlus:
                affix += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                handler.addAttribute(kSignField, beginIdx, affix.length());
                break;
            case kPatternMinus:
                affix += getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                handler.addAttribute(kSignField, beginIdx, affix.length());
                break;
            default:
                affix.append(c);
                break;
            }
        }
        else {
            affix.append(c);
        }
    }
}







int32_t DecimalFormat::appendAffix(UnicodeString& buf, double number,
                                   FieldPositionHandler& handler,
                                   UBool isNegative, UBool isPrefix) const {
    
    if (fCurrencyChoice != 0 &&
        fCurrencySignCount != fgCurrencySignCountInPluralFormat) {
        const UnicodeString* affixPat;
        if (isPrefix) {
            affixPat = isNegative ? fNegPrefixPattern : fPosPrefixPattern;
        } else {
            affixPat = isNegative ? fNegSuffixPattern : fPosSuffixPattern;
        }
        if (affixPat) {
            UnicodeString affixBuf;
            expandAffix(*affixPat, affixBuf, number, handler, TRUE, NULL);
            buf.append(affixBuf);
            return affixBuf.length();
        }
        
    }

    const UnicodeString* affix;
    if (fCurrencySignCount == fgCurrencySignCountInPluralFormat) {
        UnicodeString pluralCount = fCurrencyPluralInfo->getPluralRules()->select(number);
        AffixesForCurrency* oneSet;
        if (fStyle == UNUM_CURRENCY_PLURAL) {
            oneSet = (AffixesForCurrency*)fPluralAffixesForCurrency->get(pluralCount);
        } else {
            oneSet = (AffixesForCurrency*)fAffixesForCurrency->get(pluralCount);
        }
        if (isPrefix) {
            affix = isNegative ? &oneSet->negPrefixForCurrency :
                                 &oneSet->posPrefixForCurrency;
        } else {
            affix = isNegative ? &oneSet->negSuffixForCurrency :
                                 &oneSet->posSuffixForCurrency;
        }
    } else {
        if (isPrefix) {
            affix = isNegative ? &fNegativePrefix : &fPositivePrefix;
        } else {
            affix = isNegative ? &fNegativeSuffix : &fPositiveSuffix;
        }
    }

    int32_t begin = (int) buf.length();

    buf.append(*affix);

    if (handler.isRecording()) {
      int32_t offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kCurrencySymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kCurrencySymbol);
        handler.addAttribute(kCurrencyField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
        handler.addAttribute(kCurrencyField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
        handler.addAttribute(kSignField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kPercentSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
        handler.addAttribute(kPercentField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kPerMillSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
        handler.addAttribute(kPermillField, begin + offset, begin + offset + aff.length());
      }
    }
    return affix->length();
}
















void DecimalFormat::appendAffixPattern(UnicodeString& appendTo,
                                       const UnicodeString* affixPattern,
                                       const UnicodeString& expAffix,
                                       UBool localized) const {
    if (affixPattern == 0) {
        appendAffixPattern(appendTo, expAffix, localized);
    } else {
        int i;
        for (int pos=0; pos<affixPattern->length(); pos=i) {
            i = affixPattern->indexOf(kQuote, pos);
            if (i < 0) {
                UnicodeString s;
                affixPattern->extractBetween(pos, affixPattern->length(), s);
                appendAffixPattern(appendTo, s, localized);
                break;
            }
            if (i > pos) {
                UnicodeString s;
                affixPattern->extractBetween(pos, i, s);
                appendAffixPattern(appendTo, s, localized);
            }
            UChar32 c = affixPattern->char32At(++i);
            ++i;
            if (c == kQuote) {
                appendTo.append(c).append(c);
                
            } else if (c == kCurrencySign &&
                       i<affixPattern->length() &&
                       affixPattern->char32At(i) == kCurrencySign) {
                ++i;
                appendTo.append(c).append(c);
            } else if (localized) {
                switch (c) {
                case kPatternPercent:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                    break;
                case kPatternPerMill:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                    break;
                case kPatternPlus:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                    break;
                case kPatternMinus:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                    break;
                default:
                    appendTo.append(c);
                }
            } else {
                appendTo.append(c);
            }
        }
    }
}






void
DecimalFormat::appendAffixPattern(UnicodeString& appendTo,
                                  const UnicodeString& affix,
                                  UBool localized) const {
    UBool needQuote;
    if(localized) {
        needQuote = affix.indexOf(getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPercentSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPerMillSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kDigitSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol)) >= 0
            || affix.indexOf(kCurrencySign) >= 0;
    }
    else {
        needQuote = affix.indexOf(kPatternZeroDigit) >= 0
            || affix.indexOf(kPatternGroupingSeparator) >= 0
            || affix.indexOf(kPatternDecimalSeparator) >= 0
            || affix.indexOf(kPatternPercent) >= 0
            || affix.indexOf(kPatternPerMill) >= 0
            || affix.indexOf(kPatternDigit) >= 0
            || affix.indexOf(kPatternSeparator) >= 0
            || affix.indexOf(kPatternExponent) >= 0
            || affix.indexOf(kPatternPlus) >= 0
            || affix.indexOf(kPatternMinus) >= 0
            || affix.indexOf(kCurrencySign) >= 0;
    }
    if (needQuote)
        appendTo += (UChar)0x0027 ;
    if (affix.indexOf((UChar)0x0027 ) < 0)
        appendTo += affix;
    else {
        for (int32_t j = 0; j < affix.length(); ) {
            UChar32 c = affix.char32At(j);
            j += U16_LENGTH(c);
            appendTo += c;
            if (c == 0x0027 )
                appendTo += c;
        }
    }
    if (needQuote)
        appendTo += (UChar)0x0027 ;
}



UnicodeString&
DecimalFormat::toPattern(UnicodeString& result, UBool localized) const
{
    if (fStyle == UNUM_CURRENCY_PLURAL) {
        
        
        
        
        
        
        result = fFormatPattern;
        return result;
    }
    result.remove();
    UChar32 zero, sigDigit = kPatternSignificantDigit;
    UnicodeString digit, group;
    int32_t i;
    int32_t roundingDecimalPos = 0; 
    UnicodeString roundingDigits;
    int32_t padPos = (fFormatWidth > 0) ? fPadPosition : -1;
    UnicodeString padSpec;
    UBool useSigDig = areSignificantDigitsUsed();

    if (localized) {
        digit.append(getConstSymbol(DecimalFormatSymbols::kDigitSymbol));
        group.append(getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol));
        zero = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
        if (useSigDig) {
            sigDigit = getConstSymbol(DecimalFormatSymbols::kSignificantDigitSymbol).char32At(0);
        }
    }
    else {
        digit.append((UChar)kPatternDigit);
        group.append((UChar)kPatternGroupingSeparator);
        zero = (UChar32)kPatternZeroDigit;
    }
    if (fFormatWidth > 0) {
        if (localized) {
            padSpec.append(getConstSymbol(DecimalFormatSymbols::kPadEscapeSymbol));
        }
        else {
            padSpec.append((UChar)kPatternPadEscape);
        }
        padSpec.append(fPad);
    }
    if (fRoundingIncrement != NULL) {
        for(i=0; i<fRoundingIncrement->getCount(); ++i) {
          roundingDigits.append(zero+(fRoundingIncrement->getDigitValue(i))); 
        }
        roundingDecimalPos = fRoundingIncrement->getDecimalAt();
    }
    for (int32_t part=0; part<2; ++part) {
        if (padPos == kPadBeforePrefix) {
            result.append(padSpec);
        }
        appendAffixPattern(result,
                    (part==0 ? fPosPrefixPattern : fNegPrefixPattern),
                    (part==0 ? fPositivePrefix : fNegativePrefix),
                    localized);
        if (padPos == kPadAfterPrefix && ! padSpec.isEmpty()) {
            result.append(padSpec);
        }
        int32_t sub0Start = result.length();
        int32_t g = isGroupingUsed() ? _max(0, fGroupingSize) : 0;
        if (g > 0 && fGroupingSize2 > 0 && fGroupingSize2 != fGroupingSize) {
            g += fGroupingSize2;
        }
        int32_t maxDig = 0, minDig = 0, maxSigDig = 0;
        if (useSigDig) {
            minDig = getMinimumSignificantDigits();
            maxDig = maxSigDig = getMaximumSignificantDigits();
        } else {
            minDig = getMinimumIntegerDigits();
            maxDig = getMaximumIntegerDigits();
        }
        if (fUseExponentialNotation) {
            if (maxDig > kMaxScientificIntegerDigits) {
                maxDig = 1;
            }
        } else if (useSigDig) {
            maxDig = _max(maxDig, g+1);
        } else {
            maxDig = _max(_max(g, getMinimumIntegerDigits()),
                          roundingDecimalPos) + 1;
        }
        for (i = maxDig; i > 0; --i) {
            if (!fUseExponentialNotation && i<maxDig &&
                isGroupingPosition(i)) {
                result.append(group);
            }
            if (useSigDig) {
                
                
                
                
                if (maxSigDig >= i && i > (maxSigDig - minDig)) {
                    result.append(sigDigit);
                } else {
                    result.append(digit);
                }
            } else {
                if (! roundingDigits.isEmpty()) {
                    int32_t pos = roundingDecimalPos - i;
                    if (pos >= 0 && pos < roundingDigits.length()) {
                        result.append((UChar) (roundingDigits.char32At(pos) - kPatternZeroDigit + zero));
                        continue;
                    }
                }
                if (i<=minDig) {
                    result.append(zero);
                } else {
                    result.append(digit);
                }
            }
        }
        if (!useSigDig) {
            if (getMaximumFractionDigits() > 0 || fDecimalSeparatorAlwaysShown) {
                if (localized) {
                    result += getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
                }
                else {
                    result.append((UChar)kPatternDecimalSeparator);
                }
            }
            int32_t pos = roundingDecimalPos;
            for (i = 0; i < getMaximumFractionDigits(); ++i) {
                if (! roundingDigits.isEmpty() && pos < roundingDigits.length()) {
                    if (pos < 0) {
                        result.append(zero);
                    }
                    else {
                        result.append((UChar)(roundingDigits.char32At(pos) - kPatternZeroDigit + zero));
                    }
                    ++pos;
                    continue;
                }
                if (i<getMinimumFractionDigits()) {
                    result.append(zero);
                }
                else {
                    result.append(digit);
                }
            }
        }
        if (fUseExponentialNotation) {
            if (localized) {
                result += getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);
            }
            else {
                result.append((UChar)kPatternExponent);
            }
            if (fExponentSignAlwaysShown) {
                if (localized) {
                    result += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                }
                else {
                    result.append((UChar)kPatternPlus);
                }
            }
            for (i=0; i<fMinExponentDigits; ++i) {
                result.append(zero);
            }
        }
        if (! padSpec.isEmpty() && !fUseExponentialNotation) {
            int32_t add = fFormatWidth - result.length() + sub0Start
                - ((part == 0)
                   ? fPositivePrefix.length() + fPositiveSuffix.length()
                   : fNegativePrefix.length() + fNegativeSuffix.length());
            while (add > 0) {
                result.insert(sub0Start, digit);
                ++maxDig;
                --add;
                
                
                
                if (add>1 && isGroupingPosition(maxDig)) {
                    result.insert(sub0Start, group);
                    --add;
                }
            }
        }
        if (fPadPosition == kPadBeforeSuffix && ! padSpec.isEmpty()) {
            result.append(padSpec);
        }
        if (part == 0) {
            appendAffixPattern(result, fPosSuffixPattern, fPositiveSuffix, localized);
            if (fPadPosition == kPadAfterSuffix && ! padSpec.isEmpty()) {
                result.append(padSpec);
            }
            UBool isDefault = FALSE;
            if ((fNegSuffixPattern == fPosSuffixPattern && 
                 fNegativeSuffix == fPositiveSuffix)
                || (fNegSuffixPattern != 0 && fPosSuffixPattern != 0 &&
                    *fNegSuffixPattern == *fPosSuffixPattern))
            {
                if (fNegPrefixPattern != NULL && fPosPrefixPattern != NULL)
                {
                    int32_t length = fPosPrefixPattern->length();
                    isDefault = fNegPrefixPattern->length() == (length+2) &&
                        (*fNegPrefixPattern)[(int32_t)0] == kQuote &&
                        (*fNegPrefixPattern)[(int32_t)1] == kPatternMinus &&
                        fNegPrefixPattern->compare(2, length, *fPosPrefixPattern, 0, length) == 0;
                }
                if (!isDefault &&
                    fNegPrefixPattern == NULL && fPosPrefixPattern == NULL)
                {
                    int32_t length = fPositivePrefix.length();
                    isDefault = fNegativePrefix.length() == (length+1) &&
                        fNegativePrefix.compare(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol)) == 0 &&
                        fNegativePrefix.compare(1, length, fPositivePrefix, 0, length) == 0;
                }
            }
            if (isDefault) {
                break; 
            } else {
                if (localized) {
                    result += getConstSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol);
                }
                else {
                    result.append((UChar)kPatternSeparator);
                }
            }
        } else {
            appendAffixPattern(result, fNegSuffixPattern, fNegativeSuffix, localized);
            if (fPadPosition == kPadAfterSuffix && ! padSpec.isEmpty()) {
                result.append(padSpec);
            }
        }
    }

    return result;
}



void
DecimalFormat::applyPattern(const UnicodeString& pattern, UErrorCode& status)
{
    UParseError parseError;
    applyPattern(pattern, FALSE, parseError, status);
}



void
DecimalFormat::applyPattern(const UnicodeString& pattern,
                            UParseError& parseError,
                            UErrorCode& status)
{
    applyPattern(pattern, FALSE, parseError, status);
}


void
DecimalFormat::applyLocalizedPattern(const UnicodeString& pattern, UErrorCode& status)
{
    UParseError parseError;
    applyPattern(pattern, TRUE,parseError,status);
}



void
DecimalFormat::applyLocalizedPattern(const UnicodeString& pattern,
                                     UParseError& parseError,
                                     UErrorCode& status)
{
    applyPattern(pattern, TRUE,parseError,status);
}



void
DecimalFormat::applyPatternWithoutExpandAffix(const UnicodeString& pattern,
                                              UBool localized,
                                              UParseError& parseError,
                                              UErrorCode& status)
{
    if (U_FAILURE(status))
    {
        return;
    }
    
    parseError.offset = -1;
    parseError.preContext[0] = parseError.postContext[0] = (UChar)0;

    
    UChar32 zeroDigit               = kPatternZeroDigit; 
    UChar32 sigDigit                = kPatternSignificantDigit; 
    UnicodeString groupingSeparator ((UChar)kPatternGroupingSeparator);
    UnicodeString decimalSeparator  ((UChar)kPatternDecimalSeparator);
    UnicodeString percent           ((UChar)kPatternPercent);
    UnicodeString perMill           ((UChar)kPatternPerMill);
    UnicodeString digit             ((UChar)kPatternDigit); 
    UnicodeString separator         ((UChar)kPatternSeparator);
    UnicodeString exponent          ((UChar)kPatternExponent);
    UnicodeString plus              ((UChar)kPatternPlus);
    UnicodeString minus             ((UChar)kPatternMinus);
    UnicodeString padEscape         ((UChar)kPatternPadEscape);
    
    if (localized) {
        zeroDigit = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
        sigDigit = getConstSymbol(DecimalFormatSymbols::kSignificantDigitSymbol).char32At(0);
        groupingSeparator.  remove().append(getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol));
        decimalSeparator.   remove().append(getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol));
        percent.            remove().append(getConstSymbol(DecimalFormatSymbols::kPercentSymbol));
        perMill.            remove().append(getConstSymbol(DecimalFormatSymbols::kPerMillSymbol));
        digit.              remove().append(getConstSymbol(DecimalFormatSymbols::kDigitSymbol));
        separator.          remove().append(getConstSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol));
        exponent.           remove().append(getConstSymbol(DecimalFormatSymbols::kExponentialSymbol));
        plus.               remove().append(getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol));
        minus.              remove().append(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol));
        padEscape.          remove().append(getConstSymbol(DecimalFormatSymbols::kPadEscapeSymbol));
    }
    UChar nineDigit = (UChar)(zeroDigit + 9);
    int32_t digitLen = digit.length();
    int32_t groupSepLen = groupingSeparator.length();
    int32_t decimalSepLen = decimalSeparator.length();

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
                
                
                
                
                
                
                
                
                
                if (pattern.compare(pos, digitLen, digit) == 0) {
                    if (zeroDigitCount > 0 || sigDigitCount > 0) {
                        ++digitRightCount;
                    } else {
                        ++digitLeftCount;
                    }
                    if (groupingCount >= 0 && decimalPos < 0) {
                        ++groupingCount;
                    }
                    pos += digitLen;
                } else if ((ch >= zeroDigit && ch <= nineDigit) ||
                           ch == sigDigit) {
                    if (digitRightCount > 0) {
                        
                        debug("Unexpected '0'")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    if (ch == sigDigit) {
                        ++sigDigitCount;
                    } else {
                        if (ch != zeroDigit && roundingPos < 0) {
                            roundingPos = digitLeftCount + zeroDigitCount;
                        }
                        if (roundingPos >= 0) {
                            roundingInc.append((char)(ch - zeroDigit + '0'));
                        }
                        ++zeroDigitCount;
                    }
                    if (groupingCount >= 0 && decimalPos < 0) {
                        ++groupingCount;
                    }
                    pos += U16_LENGTH(ch);
                } else if (pattern.compare(pos, groupSepLen, groupingSeparator) == 0) {
                    if (decimalPos >= 0) {
                        
                        debug("Grouping separator after decimal")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    groupingCount2 = groupingCount;
                    groupingCount = 0;
                    pos += groupSepLen;
                } else if (pattern.compare(pos, decimalSepLen, decimalSeparator) == 0) {
                    if (decimalPos >= 0) {
                        
                        debug("Multiple decimal separators")
                        status = U_MULTIPLE_DECIMAL_SEPARATORS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    
                    
                    
                    decimalPos = digitLeftCount + zeroDigitCount + digitRightCount;
                    pos += decimalSepLen;
                } else {
                    if (pattern.compare(pos, exponent.length(), exponent) == 0) {
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
                        pos += exponent.length();
                        
                        if (pos < patLen
                            && pattern.compare(pos, plus.length(), plus) == 0) {
                            expSignAlways = TRUE;
                            pos += plus.length();
                        }
                        
                        
                        expDigits = 0;
                        while (pos < patLen &&
                               pattern.char32At(pos) == zeroDigit) {
                            ++expDigits;
                            pos += U16_LENGTH(zeroDigit);
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
                
                
                

                
                
                
                if (!pattern.compare(pos, digitLen, digit) ||
                    !pattern.compare(pos, groupSepLen, groupingSeparator) ||
                    !pattern.compare(pos, decimalSepLen, decimalSeparator) ||
                    (ch >= zeroDigit && ch <= nineDigit) ||
                    ch == sigDigit) {
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
                            fCurrencySignCount = fgCurrencySignCountInPluralFormat;
                        } else {
                            fCurrencySignCount = fgCurrencySignCountInISOFormat;
                        }
                    } else {
                        fCurrencySignCount = fgCurrencySignCountInSymbolFormat;
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
                } else if (pattern.compare(pos, separator.length(), separator) == 0) {
                    
                    
                    if (subpart == 1 || part == 1) {
                        
                        debug("Unexpected separator")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    sub2Limit = pos;
                    isPartDone = TRUE; 
                    pos += separator.length();
                    break;
                } else if (pattern.compare(pos, percent.length(), percent) == 0) {
                    
                    if (multiplier != 1) {
                        
                        debug("Too many percent characters")
                        status = U_MULTIPLE_PERCENT_SYMBOLS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    affix->append(kQuote); 
                    affix->append(kPatternPercent); 
                    multiplier = 100;
                    pos += percent.length();
                    break;
                } else if (pattern.compare(pos, perMill.length(), perMill) == 0) {
                    
                    if (multiplier != 1) {
                        
                        debug("Too many perMill characters")
                        status = U_MULTIPLE_PERMILL_SYMBOLS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    affix->append(kQuote); 
                    affix->append(kPatternPerMill); 
                    multiplier = 1000;
                    pos += perMill.length();
                    break;
                } else if (pattern.compare(pos, padEscape.length(), padEscape) == 0) {
                    if (padPos >= 0 ||               
                        (pos+1) == pattern.length()) { 
                        debug("Multiple pad specifiers")
                        status = U_MULTIPLE_PAD_SPECIFIERS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    padPos = pos;
                    pos += padEscape.length();
                    padChar = pattern.char32At(pos);
                    pos += U16_LENGTH(padChar);
                    break;
                } else if (pattern.compare(pos, minus.length(), minus) == 0) {
                    affix->append(kQuote); 
                    affix->append(kPatternMinus);
                    pos += minus.length();
                    break;
                } else if (pattern.compare(pos, plus.length(), plus) == 0) {
                    affix->append(kQuote); 
                    affix->append(kPatternPlus);
                    pos += plus.length();
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
                padPos = kPadBeforePrefix;
            } else if (padPos+2 == sub0Start) {
                padPos = kPadAfterPrefix;
            } else if (padPos == sub0Limit) {
                padPos = kPadBeforeSuffix;
            } else if (padPos+2 == sub2Limit) {
                padPos = kPadAfterSuffix;
            } else {
                
                debug("Illegal pad position")
                status = U_ILLEGAL_PAD_POSITION;
                syntaxError(pattern,pos,parseError);
                return;
            }
        }

        if (part == 0) {
            delete fPosPrefixPattern;
            delete fPosSuffixPattern;
            delete fNegPrefixPattern;
            delete fNegSuffixPattern;
            fPosPrefixPattern = new UnicodeString(prefix);
            
            if (fPosPrefixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            fPosSuffixPattern = new UnicodeString(suffix);
            
            if (fPosSuffixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                delete fPosPrefixPattern;
                return;
            }
            fNegPrefixPattern = 0;
            fNegSuffixPattern = 0;

            fUseExponentialNotation = (expDigits >= 0);
            if (fUseExponentialNotation) {
                fMinExponentDigits = expDigits;
            }
            fExponentSignAlwaysShown = expSignAlways;
            int32_t digitTotalCount = digitLeftCount + zeroDigitCount + digitRightCount;
            
            
            
            
            int32_t effectiveDecimalPos = decimalPos >= 0 ? decimalPos : digitTotalCount;
            UBool isSigDig = (sigDigitCount > 0);
            setSignificantDigitsUsed(isSigDig);
            if (isSigDig) {
                setMinimumSignificantDigits(sigDigitCount);
                setMaximumSignificantDigits(sigDigitCount + digitRightCount);
            } else {
                int32_t minInt = effectiveDecimalPos - digitLeftCount;
                setMinimumIntegerDigits(minInt);
                setMaximumIntegerDigits(fUseExponentialNotation
                    ? digitLeftCount + getMinimumIntegerDigits()
                    : kDoubleIntegerDigits);
                setMaximumFractionDigits(decimalPos >= 0
                    ? (digitTotalCount - decimalPos) : 0);
                setMinimumFractionDigits(decimalPos >= 0
                    ? (digitLeftCount + zeroDigitCount - decimalPos) : 0);
            }
            setGroupingUsed(groupingCount > 0);
            fGroupingSize = (groupingCount > 0) ? groupingCount : 0;
            fGroupingSize2 = (groupingCount2 > 0 && groupingCount2 != groupingCount)
                ? groupingCount2 : 0;
            setMultiplier(multiplier);
            setDecimalSeparatorAlwaysShown(decimalPos == 0
                    || decimalPos == digitTotalCount);
            if (padPos >= 0) {
                fPadPosition = (EPadPosition) padPos;
                
                

                
                
                fFormatWidth = sub0Limit - sub0Start;
                fPad = padChar;
            } else {
                fFormatWidth = 0;
            }
            if (roundingPos >= 0) {
                roundingInc.setDecimalAt(effectiveDecimalPos - roundingPos);
                if (fRoundingIncrement != NULL) {
                    *fRoundingIncrement = roundingInc;
                } else {
                    fRoundingIncrement = new DigitList(roundingInc);
                    
                    if (fRoundingIncrement == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        delete fPosPrefixPattern;
                        delete fPosSuffixPattern;
                        return;
                    }
                }
                fRoundingMode = kRoundHalfEven;
            } else {
                setRoundingIncrement(0.0);
            }
        } else {
            fNegPrefixPattern = new UnicodeString(prefix);
            
            if (fNegPrefixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            fNegSuffixPattern = new UnicodeString(suffix);
            
            if (fNegSuffixPattern == 0) {
                delete fNegPrefixPattern;
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
    }

    if (pattern.length() == 0) {
        delete fNegPrefixPattern;
        delete fNegSuffixPattern;
        fNegPrefixPattern = NULL;
        fNegSuffixPattern = NULL;
        if (fPosPrefixPattern != NULL) {
            fPosPrefixPattern->remove();
        } else {
            fPosPrefixPattern = new UnicodeString();
            
            if (fPosPrefixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
        if (fPosSuffixPattern != NULL) {
            fPosSuffixPattern->remove();
        } else {
            fPosSuffixPattern = new UnicodeString();
            
            if (fPosSuffixPattern == 0) {
                delete fPosPrefixPattern;
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }

        setMinimumIntegerDigits(0);
        setMaximumIntegerDigits(kDoubleIntegerDigits);
        setMinimumFractionDigits(0);
        setMaximumFractionDigits(kDoubleFractionDigits);

        fUseExponentialNotation = FALSE;
        fCurrencySignCount = 0;
        setGroupingUsed(FALSE);
        fGroupingSize = 0;
        fGroupingSize2 = 0;
        setMultiplier(1);
        setDecimalSeparatorAlwaysShown(FALSE);
        fFormatWidth = 0;
        setRoundingIncrement(0.0);
    }

    
    
    
    if (fNegPrefixPattern == NULL ||
        (*fNegPrefixPattern == *fPosPrefixPattern
         && *fNegSuffixPattern == *fPosSuffixPattern)) {
        _copy_ptr(&fNegSuffixPattern, fPosSuffixPattern);
        if (fNegPrefixPattern == NULL) {
            fNegPrefixPattern = new UnicodeString();
            
            if (fNegPrefixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        } else {
            fNegPrefixPattern->remove();
        }
        fNegPrefixPattern->append(kQuote).append(kPatternMinus)
            .append(*fPosPrefixPattern);
    }
#ifdef FMT_DEBUG
    UnicodeString s;
    s.append((UnicodeString)"\"").append(pattern).append((UnicodeString)"\"->");
    debugout(s);
#endif

    
    fFormatPattern = pattern;
}


void
DecimalFormat::expandAffixAdjustWidth(const UnicodeString* pluralCount) {
    expandAffixes(pluralCount);
    if (fFormatWidth > 0) {
        
            
            
            fFormatWidth += fPositivePrefix.length() + fPositiveSuffix.length();
    }
}


void
DecimalFormat::applyPattern(const UnicodeString& pattern,
                            UBool localized,
                            UParseError& parseError,
                            UErrorCode& status)
{
    
    
    if (pattern.indexOf(kCurrencySign) != -1) {
        if (fCurrencyPluralInfo == NULL) {
            
            fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
        }
        if (fAffixPatternsForCurrency == NULL) {
            setupCurrencyAffixPatterns(status);
        }
        if (pattern.indexOf(fgTripleCurrencySign, 3, 0) != -1) {
            
            setupCurrencyAffixes(pattern, TRUE, FALSE, status);
        }
    }
    applyPatternWithoutExpandAffix(pattern, localized, parseError, status);
    expandAffixAdjustWidth(NULL);
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}


void
DecimalFormat::applyPatternInternally(const UnicodeString& pluralCount,
                                      const UnicodeString& pattern,
                                      UBool localized,
                                      UParseError& parseError,
                                      UErrorCode& status) {
    applyPatternWithoutExpandAffix(pattern, localized, parseError, status);
    expandAffixAdjustWidth(&pluralCount);
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}







void DecimalFormat::setMaximumIntegerDigits(int32_t newValue) {
    NumberFormat::setMaximumIntegerDigits(_min(newValue, kDoubleIntegerDigits));
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}






void DecimalFormat::setMinimumIntegerDigits(int32_t newValue) {
    NumberFormat::setMinimumIntegerDigits(_min(newValue, kDoubleIntegerDigits));
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}






void DecimalFormat::setMaximumFractionDigits(int32_t newValue) {
    NumberFormat::setMaximumFractionDigits(_min(newValue, kDoubleFractionDigits));
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}






void DecimalFormat::setMinimumFractionDigits(int32_t newValue) {
    NumberFormat::setMinimumFractionDigits(_min(newValue, kDoubleFractionDigits));
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}

int32_t DecimalFormat::getMinimumSignificantDigits() const {
    return fMinSignificantDigits;
}

int32_t DecimalFormat::getMaximumSignificantDigits() const {
    return fMaxSignificantDigits;
}

void DecimalFormat::setMinimumSignificantDigits(int32_t min) {
    if (min < 1) {
        min = 1;
    }
    
    int32_t max = _max(fMaxSignificantDigits, min);
    fMinSignificantDigits = min;
    fMaxSignificantDigits = max;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}

void DecimalFormat::setMaximumSignificantDigits(int32_t max) {
    if (max < 1) {
        max = 1;
    }
    
    U_ASSERT(fMinSignificantDigits >= 1);
    int32_t min = _min(fMinSignificantDigits, max);
    fMinSignificantDigits = min;
    fMaxSignificantDigits = max;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}

UBool DecimalFormat::areSignificantDigitsUsed() const {
    return fUseSignificantDigits;
}

void DecimalFormat::setSignificantDigitsUsed(UBool useSignificantDigits) {
    fUseSignificantDigits = useSignificantDigits;
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}

void DecimalFormat::setCurrencyInternally(const UChar* theCurrency,
                                          UErrorCode& ec) {
    
    
    
    

    
    

    
    UBool isCurr = (theCurrency && *theCurrency);

    double rounding = 0.0;
    int32_t frac = 0;
    if (fCurrencySignCount > fgCurrencySignCountZero && isCurr) {
        rounding = ucurr_getRoundingIncrement(theCurrency, &ec);
        frac = ucurr_getDefaultFractionDigits(theCurrency, &ec);
    }

    NumberFormat::setCurrency(theCurrency, ec);
    if (U_FAILURE(ec)) return;

    if (fCurrencySignCount > fgCurrencySignCountZero) {
        
        if (isCurr) {
            setRoundingIncrement(rounding);
            setMinimumFractionDigits(frac);
            setMaximumFractionDigits(frac);
        }
        expandAffixes(NULL);
    }
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}

void DecimalFormat::setCurrency(const UChar* theCurrency, UErrorCode& ec) {
    
    NumberFormat::setCurrency(theCurrency, ec);
    if (fFormatPattern.indexOf(fgTripleCurrencySign, 3, 0) != -1) {
        UnicodeString savedPtn = fFormatPattern;
        setupCurrencyAffixes(fFormatPattern, TRUE, TRUE, ec);
        UParseError parseErr;
        applyPattern(savedPtn, FALSE, parseErr, ec);
    }
    
    setCurrencyInternally(theCurrency, ec);
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}


void DecimalFormat::setCurrency(const UChar* theCurrency) {
    UErrorCode ec = U_ZERO_ERROR;
    setCurrency(theCurrency, ec);
#if UCONFIG_FORMAT_FASTPATHS_49
    handleChanged();
#endif
}

void DecimalFormat::getEffectiveCurrency(UChar* result, UErrorCode& ec) const {
    if (fSymbols == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    ec = U_ZERO_ERROR;
    const UChar* c = getCurrency();
    if (*c == 0) {
        const UnicodeString &intl =
            fSymbols->getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
        c = intl.getBuffer(); 
    }
    u_strncpy(result, c, 3);
    result[3] = 0;
}






int32_t
DecimalFormat::precision() const {
    if (areSignificantDigitsUsed()) {
        return getMaximumSignificantDigits();
    } else if (fUseExponentialNotation) {
        return getMinimumIntegerDigits() + getMaximumFractionDigits();
    } else {
        return getMaximumFractionDigits();
    }
}



Hashtable*
DecimalFormat::initHashForAffix(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(TRUE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if ( U_FAILURE(status) ) {
        delete hTable; 
        return NULL;
    }
    hTable->setValueComparator(decimfmtAffixValueComparator);
    return hTable;
}

Hashtable*
DecimalFormat::initHashForAffixPattern(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(TRUE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if ( U_FAILURE(status) ) {
        delete hTable; 
        return NULL;
    }
    hTable->setValueComparator(decimfmtAffixPatternValueComparator);
    return hTable;
}

void
DecimalFormat::deleteHashForAffix(Hashtable*& table)
{
    if ( table == NULL ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    while ( (element = table->nextElement(pos)) != NULL ) {
        const UHashTok valueTok = element->value;
        const AffixesForCurrency* value = (AffixesForCurrency*)valueTok.pointer;
        delete value;
    }
    delete table;
    table = NULL;
}



void
DecimalFormat::deleteHashForAffixPattern()
{
    if ( fAffixPatternsForCurrency == NULL ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    while ( (element = fAffixPatternsForCurrency->nextElement(pos)) != NULL ) {
        const UHashTok valueTok = element->value;
        const AffixPatternsForCurrency* value = (AffixPatternsForCurrency*)valueTok.pointer;
        delete value;
    }
    delete fAffixPatternsForCurrency;
    fAffixPatternsForCurrency = NULL;
}


void
DecimalFormat::copyHashForAffixPattern(const Hashtable* source,
                                       Hashtable* target,
                                       UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;
            const UHashTok valueTok = element->value;
            const AffixPatternsForCurrency* value = (AffixPatternsForCurrency*)valueTok.pointer;
            AffixPatternsForCurrency* copy = new AffixPatternsForCurrency(
                value->negPrefixPatternForCurrency,
                value->negSuffixPatternForCurrency,
                value->posPrefixPatternForCurrency,
                value->posSuffixPatternForCurrency,
                value->patternType);
            target->put(UnicodeString(*key), copy, status);
            if ( U_FAILURE(status) ) {
                return;
            }
        }
    }
}

DecimalFormat& DecimalFormat::setAttribute( UNumberFormatAttribute attr,
                                            int32_t newValue,
                                            UErrorCode &status) {
  if(U_FAILURE(status)) return *this;

  switch(attr) {
  case UNUM_LENIENT_PARSE:
    setLenient(newValue!=0);
    break;

    case UNUM_PARSE_INT_ONLY:
      setParseIntegerOnly(newValue!=0);
      break;
      
    case UNUM_GROUPING_USED:
      setGroupingUsed(newValue!=0);
      break;
        
    case UNUM_DECIMAL_ALWAYS_SHOWN:
      setDecimalSeparatorAlwaysShown(newValue!=0);
        break;
        
    case UNUM_MAX_INTEGER_DIGITS:
      setMaximumIntegerDigits(newValue);
        break;
        
    case UNUM_MIN_INTEGER_DIGITS:
      setMinimumIntegerDigits(newValue);
        break;
        
    case UNUM_INTEGER_DIGITS:
      setMinimumIntegerDigits(newValue);
      setMaximumIntegerDigits(newValue);
        break;
        
    case UNUM_MAX_FRACTION_DIGITS:
      setMaximumFractionDigits(newValue);
        break;
        
    case UNUM_MIN_FRACTION_DIGITS:
      setMinimumFractionDigits(newValue);
        break;
        
    case UNUM_FRACTION_DIGITS:
      setMinimumFractionDigits(newValue);
      setMaximumFractionDigits(newValue);
      break;
        
    case UNUM_SIGNIFICANT_DIGITS_USED:
      setSignificantDigitsUsed(newValue!=0);
        break;

    case UNUM_MAX_SIGNIFICANT_DIGITS:
      setMaximumSignificantDigits(newValue);
        break;
        
    case UNUM_MIN_SIGNIFICANT_DIGITS:
      setMinimumSignificantDigits(newValue);
        break;
        
    case UNUM_MULTIPLIER:
      setMultiplier(newValue);    
       break;
        
    case UNUM_GROUPING_SIZE:
      setGroupingSize(newValue);    
        break;
        
    case UNUM_ROUNDING_MODE:
      setRoundingMode((DecimalFormat::ERoundingMode)newValue);
        break;
        
    case UNUM_FORMAT_WIDTH:
      setFormatWidth(newValue);
        break;
        
    case UNUM_PADDING_POSITION:
        
      setPadPosition((DecimalFormat::EPadPosition)newValue);
        break;
        
    case UNUM_SECONDARY_GROUPING_SIZE:
      setSecondaryGroupingSize(newValue);
        break;

#if UCONFIG_HAVE_PARSEALLINPUT
    case UNUM_PARSE_ALL_INPUT:
      setParseAllInput((UNumberFormatAttributeValue)newValue);
        break;
#endif

    
    case UNUM_PARSE_NO_EXPONENT:
    case UNUM_FORMAT_FAIL_IF_MORE_THAN_MAX_DIGITS:
      if(!fBoolFlags.isValidValue(newValue)) {
          status = U_ILLEGAL_ARGUMENT_ERROR;
      } else {
          fBoolFlags.set(attr, newValue);
      }
      break;

    default:
      status = U_UNSUPPORTED_ERROR;
      break;
  }
  return *this;
}

int32_t DecimalFormat::getAttribute( UNumberFormatAttribute attr, 
                                     UErrorCode &status ) const {
  if(U_FAILURE(status)) return -1;
  switch(attr) {
    case UNUM_LENIENT_PARSE: 
        return isLenient();

    case UNUM_PARSE_INT_ONLY:
        return isParseIntegerOnly();
        
    case UNUM_GROUPING_USED:
        return isGroupingUsed();
        
    case UNUM_DECIMAL_ALWAYS_SHOWN:
        return isDecimalSeparatorAlwaysShown();    
        
    case UNUM_MAX_INTEGER_DIGITS:
        return getMaximumIntegerDigits();
        
    case UNUM_MIN_INTEGER_DIGITS:
        return getMinimumIntegerDigits();
        
    case UNUM_INTEGER_DIGITS:
        
        return getMinimumIntegerDigits();
        
    case UNUM_MAX_FRACTION_DIGITS:
        return getMaximumFractionDigits();
        
    case UNUM_MIN_FRACTION_DIGITS:
        return getMinimumFractionDigits();
        
    case UNUM_FRACTION_DIGITS:
        
        return getMinimumFractionDigits();
        
    case UNUM_SIGNIFICANT_DIGITS_USED:
        return areSignificantDigitsUsed();
        
    case UNUM_MAX_SIGNIFICANT_DIGITS:
        return getMaximumSignificantDigits();
        
    case UNUM_MIN_SIGNIFICANT_DIGITS:
        return getMinimumSignificantDigits();
        
    case UNUM_MULTIPLIER:
        return getMultiplier();    
        
    case UNUM_GROUPING_SIZE:
        return getGroupingSize();    
        
    case UNUM_ROUNDING_MODE:
        return getRoundingMode();
        
    case UNUM_FORMAT_WIDTH:
        return getFormatWidth();
        
    case UNUM_PADDING_POSITION:
        return getPadPosition();
        
    case UNUM_SECONDARY_GROUPING_SIZE:
        return getSecondaryGroupingSize();
        
    
    case UNUM_PARSE_NO_EXPONENT:
    case UNUM_FORMAT_FAIL_IF_MORE_THAN_MAX_DIGITS:
      return fBoolFlags.get(attr);

    default:
        status = U_UNSUPPORTED_ERROR;
        break;
  }

  return -1; 
}

#if UCONFIG_HAVE_PARSEALLINPUT
void DecimalFormat::setParseAllInput(UNumberFormatAttributeValue value) {
  fParseAllInput = value;
#if UCONFIG_FORMAT_FASTPATHS_49
  handleChanged();
#endif
}
#endif

void
DecimalFormat::copyHashForAffix(const Hashtable* source,
                                Hashtable* target,
                                UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;

            const UHashTok valueTok = element->value;
            const AffixesForCurrency* value = (AffixesForCurrency*)valueTok.pointer;
            AffixesForCurrency* copy = new AffixesForCurrency(
                value->negPrefixForCurrency,
                value->negSuffixForCurrency,
                value->posPrefixForCurrency,
                value->posSuffixForCurrency);
            target->put(UnicodeString(*key), copy, status);
            if ( U_FAILURE(status) ) {
                return;
            }
        }
    }
}

U_NAMESPACE_END

#endif 


