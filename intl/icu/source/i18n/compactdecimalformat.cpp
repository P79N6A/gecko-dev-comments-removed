









#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "charstr.h"
#include "cstring.h"
#include "digitlst.h"
#include "mutex.h"
#include "unicode/compactdecimalformat.h"
#include "unicode/numsys.h"
#include "unicode/plurrule.h"
#include "unicode/ures.h"
#include "ucln_in.h"
#include "uhash.h"
#include "umutex.h"
#include "unicode/ures.h"
#include "uresimp.h"


static UHashtable* gCompactDecimalData = NULL;
static UMutex gCompactDecimalMetaLock = U_MUTEX_INITIALIZER;

U_NAMESPACE_BEGIN

static const int32_t MAX_DIGITS = 15;
static const char gOther[] = "other";
static const char gLatnTag[] = "latn";
static const char gNumberElementsTag[] = "NumberElements";
static const char gDecimalFormatTag[] = "decimalFormat";
static const char gPatternsShort[] = "patternsShort";
static const char gPatternsLong[] = "patternsLong";
static const char gRoot[] = "root";

static const UChar u_0 = 0x30;
static const UChar u_apos = 0x27;

static const UChar kZero[] = {u_0};


enum QuoteState {
  OUTSIDE,
  INSIDE_EMPTY,
  INSIDE_FULL
};

enum FallbackFlags {
  ANY = 0,
  MUST = 1,
  NOT_ROOT = 2
  
};




struct CDFUnit : public UMemory {
  UnicodeString prefix;
  UnicodeString suffix;
  inline CDFUnit() : prefix(), suffix() {
    prefix.setToBogus();
  }
  inline ~CDFUnit() {}
  inline UBool isSet() const {
    return !prefix.isBogus();
  }
  inline void markAsSet() {
    prefix.remove();
  }
};



class CDFLocaleStyleData : public UMemory {
 public:
  
  
  
  double divisors[MAX_DIGITS];
  
  
  
  
  
  
  
  UHashtable* unitsByVariant;
  inline CDFLocaleStyleData() : unitsByVariant(NULL) {}
  ~CDFLocaleStyleData();
  
  void Init(UErrorCode& status);
  inline UBool isBogus() const {
    return unitsByVariant == NULL;
  }
  void setToBogus();
 private:
  CDFLocaleStyleData(const CDFLocaleStyleData&);
  CDFLocaleStyleData& operator=(const CDFLocaleStyleData&);
};


struct CDFLocaleData : public UMemory {
  CDFLocaleStyleData shortData;
  CDFLocaleStyleData longData;
  inline CDFLocaleData() : shortData(), longData() { }
  inline ~CDFLocaleData() { }
  
  void Init(UErrorCode& status);
};

U_NAMESPACE_END

U_CDECL_BEGIN

static UBool U_CALLCONV cdf_cleanup(void) {
  if (gCompactDecimalData != NULL) {
    uhash_close(gCompactDecimalData);
    gCompactDecimalData = NULL;
  }
  return TRUE;
}

static void U_CALLCONV deleteCDFUnits(void* ptr) {
  delete [] (icu::CDFUnit*) ptr;
}

static void U_CALLCONV deleteCDFLocaleData(void* ptr) {
  delete (icu::CDFLocaleData*) ptr;
}

U_CDECL_END

U_NAMESPACE_BEGIN

static UBool divisors_equal(const double* lhs, const double* rhs);
static const CDFLocaleStyleData* getCDFLocaleStyleData(const Locale& inLocale, UNumberCompactStyle style, UErrorCode& status);

static const CDFLocaleStyleData* extractDataByStyleEnum(const CDFLocaleData& data, UNumberCompactStyle style, UErrorCode& status);
static CDFLocaleData* loadCDFLocaleData(const Locale& inLocale, UErrorCode& status);
static void initCDFLocaleData(const Locale& inLocale, CDFLocaleData* result, UErrorCode& status);
static UResourceBundle* tryGetDecimalFallback(const UResourceBundle* numberSystemResource, const char* style, UResourceBundle** fillIn, FallbackFlags flags, UErrorCode& status);
static UResourceBundle* tryGetByKeyWithFallback(const UResourceBundle* rb, const char* path, UResourceBundle** fillIn, FallbackFlags flags, UErrorCode& status);
static UBool isRoot(const UResourceBundle* rb, UErrorCode& status);
static void initCDFLocaleStyleData(const UResourceBundle* decimalFormatBundle, CDFLocaleStyleData* result, UErrorCode& status);
static void populatePower10(const UResourceBundle* power10Bundle, CDFLocaleStyleData* result, UErrorCode& status);
static int32_t populatePrefixSuffix(const char* variant, int32_t log10Value, const UnicodeString& formatStr, UHashtable* result, UErrorCode& status);
static UBool onlySpaces(UnicodeString u);
static void fixQuotes(UnicodeString& s);
static void fillInMissing(CDFLocaleStyleData* result);
static int32_t computeLog10(double x, UBool inRange);
static CDFUnit* createCDFUnit(const char* variant, int32_t log10Value, UHashtable* table, UErrorCode& status);
static const CDFUnit* getCDFUnitFallback(const UHashtable* table, const UnicodeString& variant, int32_t log10Value);

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CompactDecimalFormat)

CompactDecimalFormat::CompactDecimalFormat(
    const DecimalFormat& decimalFormat,
    const UHashtable* unitsByVariant,
    const double* divisors,
    PluralRules* pluralRules)
  : DecimalFormat(decimalFormat), _unitsByVariant(unitsByVariant), _divisors(divisors), _pluralRules(pluralRules) {
}

CompactDecimalFormat::CompactDecimalFormat(const CompactDecimalFormat& source)
    : DecimalFormat(source), _unitsByVariant(source._unitsByVariant), _divisors(source._divisors), _pluralRules(source._pluralRules->clone()) {
}

CompactDecimalFormat* U_EXPORT2
CompactDecimalFormat::createInstance(
    const Locale& inLocale, UNumberCompactStyle style, UErrorCode& status) {
  LocalPointer<DecimalFormat> decfmt((DecimalFormat*) NumberFormat::makeInstance(inLocale, UNUM_DECIMAL, TRUE, status));
  if (U_FAILURE(status)) {
    return NULL;
  }
  LocalPointer<PluralRules> pluralRules(PluralRules::forLocale(inLocale, status));
  if (U_FAILURE(status)) {
    return NULL;
  }
  const CDFLocaleStyleData* data = getCDFLocaleStyleData(inLocale, style, status);
  if (U_FAILURE(status)) {
    return NULL;
  }
  CompactDecimalFormat* result =
      new CompactDecimalFormat(*decfmt, data->unitsByVariant, data->divisors, pluralRules.getAlias());
  if (result == NULL) {
    status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  pluralRules.orphan();
  result->setMaximumSignificantDigits(3);
  result->setSignificantDigitsUsed(TRUE);
  result->setGroupingUsed(FALSE);
  return result;
}

CompactDecimalFormat&
CompactDecimalFormat::operator=(const CompactDecimalFormat& rhs) {
  if (this != &rhs) {
    DecimalFormat::operator=(rhs);
    _unitsByVariant = rhs._unitsByVariant;
    _divisors = rhs._divisors;
    delete _pluralRules;
    _pluralRules = rhs._pluralRules->clone();
  }
  return *this;
}

CompactDecimalFormat::~CompactDecimalFormat() {
  delete _pluralRules;
}


Format*
CompactDecimalFormat::clone(void) const {
  return new CompactDecimalFormat(*this);
}

UBool
CompactDecimalFormat::operator==(const Format& that) const {
  if (this == &that) {
    return TRUE;
  }
  return (DecimalFormat::operator==(that) && eqHelper((const CompactDecimalFormat&) that));
}

UBool
CompactDecimalFormat::eqHelper(const CompactDecimalFormat& that) const {
  return uhash_equals(_unitsByVariant, that._unitsByVariant) && divisors_equal(_divisors, that._divisors) && (*_pluralRules == *that._pluralRules);
}

UnicodeString&
CompactDecimalFormat::format(
    double number,
    UnicodeString& appendTo,
    FieldPosition& pos) const {
  DigitList orig, rounded;
  orig.set(number);
  UBool isNegative;
  UErrorCode status = U_ZERO_ERROR;
  _round(orig, rounded, isNegative, status);
  if (U_FAILURE(status)) {
    return appendTo;
  }
  double roundedDouble = rounded.getDouble();
  if (isNegative) {
    roundedDouble = -roundedDouble;
  }
  int32_t baseIdx = computeLog10(roundedDouble, TRUE);
  double numberToFormat = roundedDouble / _divisors[baseIdx];
  UnicodeString variant = _pluralRules->select(numberToFormat);
  if (isNegative) {
    numberToFormat = -numberToFormat;
  }
  const CDFUnit* unit = getCDFUnitFallback(_unitsByVariant, variant, baseIdx);
  appendTo += unit->prefix;
  DecimalFormat::format(numberToFormat, appendTo, pos);
  appendTo += unit->suffix;
  return appendTo;
}

UnicodeString&
CompactDecimalFormat::format(
    double ,
    UnicodeString& appendTo,
    FieldPositionIterator* ,
    UErrorCode& status) const {
  status = U_UNSUPPORTED_ERROR;
  return appendTo;
}

UnicodeString&
CompactDecimalFormat::format(
    int64_t number,
    UnicodeString& appendTo,
    FieldPosition& pos) const {
  return format((double) number, appendTo, pos);
}

UnicodeString&
CompactDecimalFormat::format(
    int64_t ,
    UnicodeString& appendTo,
    FieldPositionIterator* ,
    UErrorCode& status) const {
  status = U_UNSUPPORTED_ERROR;
  return appendTo;
}

UnicodeString&
CompactDecimalFormat::format(
    const StringPiece& ,
    UnicodeString& appendTo,
    FieldPositionIterator* ,
    UErrorCode& status) const {
  status = U_UNSUPPORTED_ERROR;
  return appendTo;
}

UnicodeString&
CompactDecimalFormat::format(
    const DigitList& ,
    UnicodeString& appendTo,
    FieldPositionIterator* ,
    UErrorCode& status) const {
  status = U_UNSUPPORTED_ERROR;
  return appendTo;
}

UnicodeString&
CompactDecimalFormat::format(const DigitList& ,
                             UnicodeString& appendTo,
                             FieldPosition& ,
                             UErrorCode& status) const {
  status = U_UNSUPPORTED_ERROR;
  return appendTo;
}

void
CompactDecimalFormat::parse(
    const UnicodeString& ,
    Formattable& ,
    ParsePosition& ) const {
}

void
CompactDecimalFormat::parse(
    const UnicodeString& ,
    Formattable& ,
    UErrorCode& status) const {
  status = U_UNSUPPORTED_ERROR;
}

CurrencyAmount*
CompactDecimalFormat::parseCurrency(
    const UnicodeString& ,
    ParsePosition& ) const {
  return NULL;
}

void CDFLocaleStyleData::Init(UErrorCode& status) {
  if (unitsByVariant != NULL) {
    return;
  }
  unitsByVariant = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &status);
  if (U_FAILURE(status)) {
    return;
  }
  uhash_setKeyDeleter(unitsByVariant, uprv_free);
  uhash_setValueDeleter(unitsByVariant, deleteCDFUnits);
}

CDFLocaleStyleData::~CDFLocaleStyleData() {
  setToBogus();
}

void CDFLocaleStyleData::setToBogus() {
  if (unitsByVariant != NULL) {
    uhash_close(unitsByVariant);
    unitsByVariant = NULL;
  }
}

void CDFLocaleData::Init(UErrorCode& status) {
  shortData.Init(status);
  if (U_FAILURE(status)) {
    return;
  }
  longData.Init(status);
}


static UBool divisors_equal(const double* lhs, const double* rhs) {
  for (int32_t i = 0; i < MAX_DIGITS; ++i) {
    if (lhs[i] != rhs[i]) {
      return FALSE;
    }
  }
  return TRUE;
}







static const CDFLocaleStyleData* getCDFLocaleStyleData(const Locale& inLocale, UNumberCompactStyle style, UErrorCode& status) {
  if (U_FAILURE(status)) {
    return NULL;
  }
  CDFLocaleData* result = NULL;
  const char* key = inLocale.getName();
  {
    Mutex lock(&gCompactDecimalMetaLock);
    if (gCompactDecimalData == NULL) {
      gCompactDecimalData = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &status);
      if (U_FAILURE(status)) {
        return NULL;
      }
      uhash_setKeyDeleter(gCompactDecimalData, uprv_free);
      uhash_setValueDeleter(gCompactDecimalData, deleteCDFLocaleData);
      ucln_i18n_registerCleanup(UCLN_I18N_CDFINFO, cdf_cleanup);
    } else {
      result = (CDFLocaleData*) uhash_get(gCompactDecimalData, key);
    }
  }
  if (result != NULL) {
    return extractDataByStyleEnum(*result, style, status);
  }

  result = loadCDFLocaleData(inLocale, status);
  if (U_FAILURE(status)) {
    return NULL;
  }

  {
    Mutex lock(&gCompactDecimalMetaLock);
    CDFLocaleData* temp = (CDFLocaleData*) uhash_get(gCompactDecimalData, key);
    if (temp != NULL) {
      delete result;
      result = temp;
    } else {
      uhash_put(gCompactDecimalData, uprv_strdup(key), (void*) result, &status);
      if (U_FAILURE(status)) {
        return NULL;
      }
    }
  }
  return extractDataByStyleEnum(*result, style, status);
}

static const CDFLocaleStyleData* extractDataByStyleEnum(const CDFLocaleData& data, UNumberCompactStyle style, UErrorCode& status) {
  switch (style) {
    case UNUM_SHORT:
      return &data.shortData;
    case UNUM_LONG:
      if (!data.longData.isBogus()) {
        return &data.longData;
      }
      return &data.shortData;
    default:
      status = U_ILLEGAL_ARGUMENT_ERROR;
      return NULL;
  }
}



static CDFLocaleData* loadCDFLocaleData(const Locale& inLocale, UErrorCode& status) {
  if (U_FAILURE(status)) {
    return NULL;
  }
  CDFLocaleData* result = new CDFLocaleData;
  if (result == NULL) {
    status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }
  result->Init(status);
  if (U_FAILURE(status)) {
    delete result;
    return NULL;
  }

  initCDFLocaleData(inLocale, result, status);
  if (U_FAILURE(status)) {
    delete result;
    return NULL;
  }
  return result;
}









static void initCDFLocaleData(const Locale& inLocale, CDFLocaleData* result, UErrorCode& status) {
  LocalPointer<NumberingSystem> ns(NumberingSystem::createInstance(inLocale, status));
  if (U_FAILURE(status)) {
    return;
  }
  const char* numberingSystemName = ns->getName();
  UResourceBundle* rb = ures_open(NULL, inLocale.getName(), &status);
  rb = ures_getByKeyWithFallback(rb, gNumberElementsTag, rb, &status);
  if (U_FAILURE(status)) {
    ures_close(rb);
    return;
  }
  UResourceBundle* shortDataFillIn = NULL;
  UResourceBundle* longDataFillIn = NULL;
  UResourceBundle* shortData = NULL;
  UResourceBundle* longData = NULL;

  if (uprv_strcmp(numberingSystemName, gLatnTag) != 0) {
    LocalUResourceBundlePointer localResource(
        tryGetByKeyWithFallback(rb, numberingSystemName, NULL, NOT_ROOT, status));
    shortData = tryGetDecimalFallback(
        localResource.getAlias(), gPatternsShort, &shortDataFillIn, NOT_ROOT, status);
    longData = tryGetDecimalFallback(
        localResource.getAlias(), gPatternsLong, &longDataFillIn, NOT_ROOT, status);
  }
  if (U_FAILURE(status)) {
    ures_close(shortDataFillIn);
    ures_close(longDataFillIn);
    ures_close(rb);
    return;
  }

  
  
  if (shortData == NULL) {
    LocalUResourceBundlePointer latnResource(tryGetByKeyWithFallback(rb, gLatnTag, NULL, MUST, status));
    shortData = tryGetDecimalFallback(latnResource.getAlias(), gPatternsShort, &shortDataFillIn, MUST, status);
    if (longData == NULL) {
      longData = tryGetDecimalFallback(latnResource.getAlias(), gPatternsLong, &longDataFillIn, ANY, status);
      if (longData != NULL && isRoot(longData, status) && !isRoot(shortData, status)) {
        longData = NULL;
      }
    }
  }
  initCDFLocaleStyleData(shortData, &result->shortData, status);
  ures_close(shortDataFillIn);
  if (U_FAILURE(status)) {
    ures_close(longDataFillIn);
    ures_close(rb);
  }

  if (longData == NULL) {
    result->longData.setToBogus();
  } else {
    initCDFLocaleStyleData(longData, &result->longData, status);
  }
  ures_close(longDataFillIn);
  ures_close(rb);
}






static UResourceBundle* tryGetDecimalFallback(const UResourceBundle* numberSystemResource, const char* style, UResourceBundle** fillIn, FallbackFlags flags, UErrorCode& status) {
  UResourceBundle* first = tryGetByKeyWithFallback(numberSystemResource, style, fillIn, flags, status);
  UResourceBundle* second = tryGetByKeyWithFallback(first, gDecimalFormatTag, fillIn, flags, status);
  if (fillIn == NULL) {
    ures_close(first);
  }
  return second;
}






































static UResourceBundle* tryGetByKeyWithFallback(const UResourceBundle* rb, const char* path, UResourceBundle** fillIn, FallbackFlags flags, UErrorCode& status) {
  if (U_FAILURE(status)) {
    return NULL;
  }
  UBool must = (flags & MUST);
  if (rb == NULL) {
    if (must) {
      status = U_MISSING_RESOURCE_ERROR;
    }
    return NULL;
  }
  UResourceBundle* result = NULL;
  UResourceBundle* ownedByUs = NULL;
  if (fillIn == NULL) {
    ownedByUs = ures_getByKeyWithFallback(rb, path, NULL, &status);
    result = ownedByUs;
  } else {
    *fillIn = ures_getByKeyWithFallback(rb, path, *fillIn, &status);
    result = *fillIn;
  }
  if (U_FAILURE(status)) {
    ures_close(ownedByUs);
    if (status == U_MISSING_RESOURCE_ERROR && !must) {
      status = U_ZERO_ERROR;
    }
    return NULL;
  }
  flags = (FallbackFlags) (flags & ~MUST);
  switch (flags) {
    case NOT_ROOT:
      {
        UBool bRoot = isRoot(result, status);
        if (bRoot || U_FAILURE(status)) {
          ures_close(ownedByUs);
          if (must && (status == U_ZERO_ERROR)) {
            status = U_MISSING_RESOURCE_ERROR;
          }
          return NULL;
        }
        return result;
      }
    case ANY:
      return result;
    default:
      ures_close(ownedByUs);
      status = U_ILLEGAL_ARGUMENT_ERROR;
      return NULL;
  }
}

static UBool isRoot(const UResourceBundle* rb, UErrorCode& status) {
  const char* actualLocale = ures_getLocaleByType(
      rb, ULOC_ACTUAL_LOCALE, &status);
  if (U_FAILURE(status)) {
    return FALSE;
  }
  return uprv_strcmp(actualLocale, gRoot) == 0;
}





static void initCDFLocaleStyleData(const UResourceBundle* decimalFormatBundle, CDFLocaleStyleData* result, UErrorCode& status) {
  if (U_FAILURE(status)) {
    return;
  }
  
  int32_t size = ures_getSize(decimalFormatBundle);
  UResourceBundle* power10 = NULL;
  for (int32_t i = 0; i < size; ++i) {
    power10 = ures_getByIndex(decimalFormatBundle, i, power10, &status);
    if (U_FAILURE(status)) {
      ures_close(power10);
      return;
    }
    populatePower10(power10, result, status);
    if (U_FAILURE(status)) {
      ures_close(power10);
      return;
    }
  }
  ures_close(power10);
  fillInMissing(result);
}



static void populatePower10(const UResourceBundle* power10Bundle, CDFLocaleStyleData* result, UErrorCode& status) {
  if (U_FAILURE(status)) {
    return;
  }
  char* endPtr = NULL;
  double power10 = uprv_strtod(ures_getKey(power10Bundle), &endPtr);
  if (*endPtr != 0) {
    status = U_INTERNAL_PROGRAM_ERROR;
    return;
  }
  int32_t log10Value = computeLog10(power10, FALSE);
  
  if (log10Value == MAX_DIGITS) {
    return;
  }
  int32_t size = ures_getSize(power10Bundle);
  int32_t numZeros = 0;
  UBool otherVariantDefined = FALSE;
  UResourceBundle* variantBundle = NULL;
  
  for (int32_t i = 0; i < size; ++i) {
    variantBundle = ures_getByIndex(power10Bundle, i, variantBundle, &status);
    if (U_FAILURE(status)) {
      ures_close(variantBundle);
      return;
    }
    const char* variant = ures_getKey(variantBundle);
    int32_t resLen;
    const UChar* formatStrP = ures_getString(variantBundle, &resLen, &status);
    if (U_FAILURE(status)) {
      ures_close(variantBundle);
      return;
    }
    UnicodeString formatStr(false, formatStrP, resLen);
    if (uprv_strcmp(variant, gOther) == 0) {
      otherVariantDefined = TRUE;
    }
    int32_t nz = populatePrefixSuffix(
        variant, log10Value, formatStr, result->unitsByVariant, status);
    if (U_FAILURE(status)) {
      ures_close(variantBundle);
      return;
    }
    if (nz != numZeros) {
      
      
      if (numZeros != 0) {
        status = U_INTERNAL_PROGRAM_ERROR;
        ures_close(variantBundle);
        return;
      }
      numZeros = nz;
    }
  }
  ures_close(variantBundle);
  
  if (!otherVariantDefined) {
    status = U_INTERNAL_PROGRAM_ERROR;
    return;
  }
  double divisor = power10;
  for (int32_t i = 1; i < numZeros; ++i) {
    divisor /= 10.0;
  }
  result->divisors[log10Value] = divisor;
}










static int32_t populatePrefixSuffix(
    const char* variant, int32_t log10Value, const UnicodeString& formatStr, UHashtable* result, UErrorCode& status) {
  if (U_FAILURE(status)) {
    return 0;
  }
  int32_t firstIdx = formatStr.indexOf(kZero, UPRV_LENGTHOF(kZero), 0);
  
  if (firstIdx == -1) {
    status = U_INTERNAL_PROGRAM_ERROR;
    return 0;
  }
  int32_t lastIdx = formatStr.lastIndexOf(kZero, UPRV_LENGTHOF(kZero), firstIdx);
  CDFUnit* unit = createCDFUnit(variant, log10Value, result, status);
  if (U_FAILURE(status)) {
    return 0;
  }
  
  unit->prefix = formatStr.tempSubString(0, firstIdx);
  fixQuotes(unit->prefix);
  
  unit->suffix = formatStr.tempSubString(lastIdx + 1);
  fixQuotes(unit->suffix);

  
  
  if (onlySpaces(unit->prefix) && onlySpaces(unit->suffix)) {
    return log10Value + 1;
  }

  
  int32_t idx = firstIdx + 1;
  while (idx <= lastIdx && formatStr.charAt(idx) == u_0) {
    ++idx;
  }
  return (idx - firstIdx);
}

static UBool onlySpaces(UnicodeString u) {
  return u.trim().length() == 0;
}



static void fixQuotes(UnicodeString& s) {
  QuoteState state = OUTSIDE;
  int32_t len = s.length();
  int32_t dest = 0;
  for (int32_t i = 0; i < len; ++i) {
    UChar ch = s.charAt(i);
    if (ch == u_apos) {
      if (state == INSIDE_EMPTY) {
        s.setCharAt(dest, ch);
        ++dest;
      }
    } else {
      s.setCharAt(dest, ch);
      ++dest;
    }

    
    switch (state) {
      case OUTSIDE:
        state = ch == u_apos ? INSIDE_EMPTY : OUTSIDE;
        break;
      case INSIDE_EMPTY:
      case INSIDE_FULL:
        state = ch == u_apos ? OUTSIDE : INSIDE_FULL;
        break;
      default:
        break;
    }
  }
  s.truncate(dest);
}


















static void fillInMissing(CDFLocaleStyleData* result) {
  const CDFUnit* otherUnits =
      (const CDFUnit*) uhash_get(result->unitsByVariant, gOther);
  UBool definedInCLDR[MAX_DIGITS];
  double lastDivisor = 1.0;
  for (int32_t i = 0; i < MAX_DIGITS; ++i) {
    if (!otherUnits[i].isSet()) {
      result->divisors[i] = lastDivisor;
      definedInCLDR[i] = FALSE;
    } else {
      lastDivisor = result->divisors[i];
      definedInCLDR[i] = TRUE;
    }
  }
  
  int32_t pos = UHASH_FIRST;
  const UHashElement* element = uhash_nextElement(result->unitsByVariant, &pos);
  for (;element != NULL; element = uhash_nextElement(result->unitsByVariant, &pos)) {
    CDFUnit* units = (CDFUnit*) element->value.pointer;
    for (int32_t i = 0; i < MAX_DIGITS; ++i) {
      if (definedInCLDR[i]) {
        if (!units[i].isSet()) {
          units[i] = otherUnits[i];
        }
      } else {
        if (i == 0) {
          units[0].markAsSet();
        } else {
          units[i] = units[i - 1];
        }
      }
    }
  }
}





static int32_t computeLog10(double x, UBool inRange) {
  int32_t result = 0;
  int32_t max = inRange ? MAX_DIGITS - 1 : MAX_DIGITS;
  while (x >= 10.0) {
    x /= 10.0;
    ++result;
    if (result == max) {
      break;
    }
  }
  return result;
}




static CDFUnit* createCDFUnit(const char* variant, int32_t log10Value, UHashtable* table, UErrorCode& status) {
  if (U_FAILURE(status)) {
    return NULL;
  }
  CDFUnit *cdfUnit = (CDFUnit*) uhash_get(table, variant);
  if (cdfUnit == NULL) {
    cdfUnit = new CDFUnit[MAX_DIGITS];
    if (cdfUnit == NULL) {
      status = U_MEMORY_ALLOCATION_ERROR;
      return NULL;
    }
    uhash_put(table, uprv_strdup(variant), cdfUnit, &status);
    if (U_FAILURE(status)) {
      return NULL;
    }
  }
  CDFUnit* result = &cdfUnit[log10Value];
  result->markAsSet();
  return result;
}





static const CDFUnit* getCDFUnitFallback(const UHashtable* table, const UnicodeString& variant, int32_t log10Value) {
  CharString cvariant;
  UErrorCode status = U_ZERO_ERROR;
  const CDFUnit *cdfUnit = NULL;
  cvariant.appendInvariantChars(variant, status);
  if (!U_FAILURE(status)) {
    cdfUnit = (const CDFUnit*) uhash_get(table, cvariant.data());
  }
  if (cdfUnit == NULL) {
    cdfUnit = (const CDFUnit*) uhash_get(table, gOther);
  }
  return &cdfUnit[log10Value];
}

U_NAMESPACE_END
#endif
