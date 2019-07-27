























#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/numfmt.h"
#include "unicode/locid.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/ustring.h"
#include "unicode/ucurr.h"
#include "unicode/curramt.h"
#include "unicode/numsys.h"
#include "unicode/rbnf.h"
#include "unicode/localpointer.h"
#include "unicode/udisplaycontext.h"
#include "charstr.h"
#include "winnmfmt.h"
#include "uresimp.h"
#include "uhash.h"
#include "cmemory.h"
#include "servloc.h"
#include "ucln_in.h"
#include "cstring.h"
#include "putilimp.h"
#include "uassert.h"
#include "umutex.h"
#include "mutex.h"
#include "digitlst.h"
#include <float.h>
#include "sharednumberformat.h"
#include "unifiedcache.h"



#ifdef FMT_DEBUG
#include <stdio.h>
static inline void debugout(UnicodeString s) {
    char buf[2000];
    s.extract((int32_t) 0, s.length(), buf);
    printf("%s", buf);
}
#define debug(x) printf("%s", x);
#else
#define debugout(x)
#define debug(x)
#endif



static const UChar gLastResortDecimalPat[] = {
    0x23, 0x2C, 0x23, 0x23, 0x30, 0x2E, 0x23, 0x23, 0x23, 0 
};
static const UChar gLastResortCurrencyPat[] = {
    0xA4, 0xA0, 0x23, 0x2C, 0x23, 0x23, 0x30, 0x2E, 0x30, 0x30, 0 
};
static const UChar gLastResortPercentPat[] = {
    0x23, 0x2C, 0x23, 0x23, 0x30, 0x25, 0 
};
static const UChar gLastResortScientificPat[] = {
    0x23, 0x45, 0x30, 0 
};
static const UChar gLastResortIsoCurrencyPat[] = {
    0xA4, 0xA4, 0xA0, 0x23, 0x2C, 0x23, 0x23, 0x30, 0x2E, 0x30, 0x30, 0  
};
static const UChar gLastResortPluralCurrencyPat[] = {
    0x23, 0x2C, 0x23, 0x23, 0x30, 0x2E, 0x23, 0x23, 0x23, 0x20, 0xA4, 0xA4, 0xA4, 0 
};
static const UChar gLastResortAccountingCurrencyPat[] =  {
    0xA4, 0xA0, 0x23, 0x2C, 0x23, 0x23, 0x30, 0x2E, 0x30, 0x30, 0 
};

static const UChar gSingleCurrencySign[] = {0xA4, 0};
static const UChar gDoubleCurrencySign[] = {0xA4, 0xA4, 0};

static const UChar gSlash = 0x2f;





const int32_t icu::NumberFormat::gDefaultMaxIntegerDigits = 2000000000;
const int32_t icu::NumberFormat::gDefaultMinIntegerDigits = 127;

static const UChar * const gLastResortNumberPatterns[UNUM_FORMAT_STYLE_COUNT] = {
    NULL,  
    gLastResortDecimalPat,  
    gLastResortCurrencyPat,  
    gLastResortPercentPat,  
    gLastResortScientificPat,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    gLastResortIsoCurrencyPat,  
    gLastResortPluralCurrencyPat,  
    gLastResortAccountingCurrencyPat, 
    gLastResortCurrencyPat,  
};



static const char *gNumberElements = "NumberElements";
static const char *gLatn = "latn";
static const char *gPatterns = "patterns";
static const char *gFormatKeys[UNUM_FORMAT_STYLE_COUNT] = {
    NULL,  
    "decimalFormat",  
    "currencyFormat",  
    "percentFormat",  
    "scientificFormat",  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    NULL,  
    
    
    
    
    "currencyFormat",  
    "currencyFormat",  
    "accountingFormat",  
    "currencyFormat"  
};


static UHashtable * NumberingSystem_cache = NULL;
static UMutex nscacheMutex = U_MUTEX_INITIALIZER;
static icu::UInitOnce gNSCacheInitOnce = U_INITONCE_INITIALIZER;

#if !UCONFIG_NO_SERVICE
static icu::ICULocaleService* gService = NULL;
static icu::UInitOnce gServiceInitOnce = U_INITONCE_INITIALIZER;
#endif




U_CDECL_BEGIN
static void U_CALLCONV
deleteNumberingSystem(void *obj) {
    delete (icu::NumberingSystem *)obj;
}

static UBool U_CALLCONV numfmt_cleanup(void) {
#if !UCONFIG_NO_SERVICE
    gServiceInitOnce.reset();
    if (gService) {
        delete gService;
        gService = NULL;
    }
#endif
    gNSCacheInitOnce.reset();
    if (NumberingSystem_cache) {
        
        uhash_close(NumberingSystem_cache);
        NumberingSystem_cache = NULL;
    }
    return TRUE;
}
U_CDECL_END





U_NAMESPACE_BEGIN

UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(NumberFormat)

#if !UCONFIG_NO_SERVICE


NumberFormatFactory::~NumberFormatFactory() {}
SimpleNumberFormatFactory::SimpleNumberFormatFactory(const Locale& locale, UBool visible)
    : _visible(visible)
{
    LocaleUtility::initNameFromLocale(locale, _id);
}

SimpleNumberFormatFactory::~SimpleNumberFormatFactory() {}

UBool SimpleNumberFormatFactory::visible(void) const {
    return _visible;
}

const UnicodeString *
SimpleNumberFormatFactory::getSupportedIDs(int32_t &count, UErrorCode& status) const
{
    if (U_SUCCESS(status)) {
        count = 1;
        return &_id;
    }
    count = 0;
    return NULL;
}
#endif 



NumberFormat::NumberFormat()
:   fGroupingUsed(TRUE),
    fMaxIntegerDigits(gDefaultMaxIntegerDigits),
    fMinIntegerDigits(1),
    fMaxFractionDigits(3), 
    fMinFractionDigits(0),
    fParseIntegerOnly(FALSE),
    fLenient(FALSE),
    fCapitalizationContext(UDISPCTX_CAPITALIZATION_NONE)
{
    fCurrency[0] = 0;
}



NumberFormat::~NumberFormat()
{
}

SharedNumberFormat::~SharedNumberFormat() {
    delete ptr;
}




NumberFormat::NumberFormat(const NumberFormat &source)
:   Format(source)
{
    *this = source;
}




NumberFormat&
NumberFormat::operator=(const NumberFormat& rhs)
{
    if (this != &rhs)
    {
        Format::operator=(rhs);
        fGroupingUsed = rhs.fGroupingUsed;
        fMaxIntegerDigits = rhs.fMaxIntegerDigits;
        fMinIntegerDigits = rhs.fMinIntegerDigits;
        fMaxFractionDigits = rhs.fMaxFractionDigits;
        fMinFractionDigits = rhs.fMinFractionDigits;
        fParseIntegerOnly = rhs.fParseIntegerOnly;
        u_strncpy(fCurrency, rhs.fCurrency, 4);
        fLenient = rhs.fLenient;
        fCapitalizationContext = rhs.fCapitalizationContext;
    }
    return *this;
}



UBool
NumberFormat::operator==(const Format& that) const
{
    
    NumberFormat* other = (NumberFormat*)&that;

#ifdef FMT_DEBUG
    
    
    UBool first = TRUE;
    if (!Format::operator==(that)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Format::!=");
    }
    if (!(fMaxIntegerDigits == other->fMaxIntegerDigits &&
          fMinIntegerDigits == other->fMinIntegerDigits)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Integer digits !=");
    }
    if (!(fMaxFractionDigits == other->fMaxFractionDigits &&
          fMinFractionDigits == other->fMinFractionDigits)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Fraction digits !=");
    }
    if (!(fGroupingUsed == other->fGroupingUsed)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fGroupingUsed != ");
    }
    if (!(fParseIntegerOnly == other->fParseIntegerOnly)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fParseIntegerOnly != ");
    }
    if (!(u_strcmp(fCurrency, other->fCurrency) == 0)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fCurrency !=");
    }
    if (!(fLenient == other->fLenient)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fLenient != ");
    }
    if (!(fCapitalizationContext == other->fCapitalizationContext)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("fCapitalizationContext != ");
    }
    if (!first) { printf(" ]"); }
#endif

    return ((this == &that) ||
            ((Format::operator==(that) &&
              fMaxIntegerDigits == other->fMaxIntegerDigits &&
              fMinIntegerDigits == other->fMinIntegerDigits &&
              fMaxFractionDigits == other->fMaxFractionDigits &&
              fMinFractionDigits == other->fMinFractionDigits &&
              fGroupingUsed == other->fGroupingUsed &&
              fParseIntegerOnly == other->fParseIntegerOnly &&
              u_strcmp(fCurrency, other->fCurrency) == 0 &&
              fLenient == other->fLenient &&
              fCapitalizationContext == other->fCapitalizationContext)));
}





UnicodeString&
NumberFormat::format(double ,
                     UnicodeString& toAppendTo,
                     FieldPositionIterator* ,
                     UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return toAppendTo;
}





UnicodeString&
NumberFormat::format(int32_t ,
                     UnicodeString& toAppendTo,
                     FieldPositionIterator* ,
                     UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return toAppendTo;
}





UnicodeString&
NumberFormat::format(int64_t ,
                     UnicodeString& toAppendTo,
                     FieldPositionIterator* ,
                     UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return toAppendTo;
}



UnicodeString&
NumberFormat::format(double number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode &status) const {
    if(U_SUCCESS(status)) {
        return format(number,appendTo,pos);
    } else {
        return appendTo;
    }
}

UnicodeString&
NumberFormat::format(int32_t number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode &status) const {
    if(U_SUCCESS(status)) {
        return format(number,appendTo,pos);
    } else {
        return appendTo;
    }
}

UnicodeString&
NumberFormat::format(int64_t number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode &status) const {
    if(U_SUCCESS(status)) {
        return format(number,appendTo,pos);
    } else {
        return appendTo;
    }
}


















UnicodeString&
NumberFormat::format(const StringPiece &decimalNum,
                     UnicodeString& toAppendTo,
                     FieldPositionIterator* fpi,
                     UErrorCode& status) const
{
    Formattable f;
    f.setDecimalNumber(decimalNum, status);
    format(f, toAppendTo, fpi, status);
    return toAppendTo;
}













class ArgExtractor {
  const Formattable* num;
  UChar save[4];
  UBool fWasCurrency;

 public:
  ArgExtractor(const NumberFormat& nf, const Formattable& obj, UErrorCode& status);
  ~ArgExtractor();

  const Formattable* number(void) const;
  const UChar *iso(void) const;
  UBool wasCurrency(void) const;
};

inline const Formattable*
ArgExtractor::number(void) const {
  return num;
}

inline UBool
ArgExtractor::wasCurrency(void) const {
  return fWasCurrency;
}

inline const UChar *
ArgExtractor::iso(void) const {
  return save;
}

ArgExtractor::ArgExtractor(const NumberFormat& , const Formattable& obj, UErrorCode& )
  : num(&obj), fWasCurrency(FALSE) {

    const UObject* o = obj.getObject(); 
    const CurrencyAmount* amt;
    if (o != NULL && (amt = dynamic_cast<const CurrencyAmount*>(o)) != NULL) {
        
        
        
        u_strcpy(save, amt->getISOCurrency());
        num = &amt->getNumber();
        fWasCurrency=TRUE;
    } else {
      save[0]=0;
    }
}

ArgExtractor::~ArgExtractor() {
}

UnicodeString& NumberFormat::format(const DigitList &number,
                      UnicodeString& appendTo,
                      FieldPositionIterator* posIter,
                      UErrorCode& status) const {
    
    
    
    if (U_FAILURE(status)) {
        return appendTo;
    }
    double dnum = number.getDouble();
    format(dnum, appendTo, posIter, status);
    return appendTo;
}



UnicodeString&
NumberFormat::format(const DigitList &number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode &status) const { 
    
    
    
    if (U_FAILURE(status)) {
        return appendTo;
    }
    double dnum = number.getDouble();
    format(dnum, appendTo, pos, status);
    return appendTo;
}

UnicodeString&
NumberFormat::format(const Formattable& obj,
                        UnicodeString& appendTo,
                        FieldPosition& pos,
                        UErrorCode& status) const
{
    if (U_FAILURE(status)) return appendTo;

    ArgExtractor arg(*this, obj, status);
    const Formattable *n = arg.number();
    const UChar *iso = arg.iso();

    if(arg.wasCurrency() && u_strcmp(iso, getCurrency())) {
      
      
      LocalPointer<NumberFormat> cloneFmt((NumberFormat*)this->clone());
      cloneFmt->setCurrency(iso, status);
      
      return cloneFmt->format(*n, appendTo, pos, status);
    }

    if (n->isNumeric() && n->getDigitList() != NULL) {
        
        
        
        
        
        
        
        
        format(*n->getDigitList(), appendTo, pos, status);
    } else {
        switch (n->getType()) {
        case Formattable::kDouble:
            format(n->getDouble(), appendTo, pos);
            break;
        case Formattable::kLong:
            format(n->getLong(), appendTo, pos);
            break;
        case Formattable::kInt64:
            format(n->getInt64(), appendTo, pos);
            break;
        default:
            status = U_INVALID_FORMAT_ERROR;
            break;
        }
    }

    return appendTo;
}





UnicodeString&
NumberFormat::format(const Formattable& obj,
                        UnicodeString& appendTo,
                        FieldPositionIterator* posIter,
                        UErrorCode& status) const
{
    if (U_FAILURE(status)) return appendTo;

    ArgExtractor arg(*this, obj, status);
    const Formattable *n = arg.number();
    const UChar *iso = arg.iso();

    if(arg.wasCurrency() && u_strcmp(iso, getCurrency())) {
      
      
      LocalPointer<NumberFormat> cloneFmt((NumberFormat*)this->clone());
      cloneFmt->setCurrency(iso, status);
      
      return cloneFmt->format(*n, appendTo, posIter, status);
    }

    if (n->isNumeric() && n->getDigitList() != NULL) {
        
        format(*n->getDigitList(), appendTo, posIter, status);
    } else {
        switch (n->getType()) {
        case Formattable::kDouble:
            format(n->getDouble(), appendTo, posIter, status);
            break;
        case Formattable::kLong:
            format(n->getLong(), appendTo, posIter, status);
            break;
        case Formattable::kInt64:
            format(n->getInt64(), appendTo, posIter, status);
            break;
        default:
            status = U_INVALID_FORMAT_ERROR;
            break;
        }
    }

    return appendTo;
}



UnicodeString&
NumberFormat::format(int64_t number,
                     UnicodeString& appendTo,
                     FieldPosition& pos) const
{
    
    return format((int32_t)number, appendTo, pos);
}





void
NumberFormat::parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& parse_pos) const
{
    parse(source, result, parse_pos);
}




UnicodeString&
NumberFormat::format(double number, UnicodeString& appendTo) const
{
    FieldPosition pos(0);
    return format(number, appendTo, pos);
}




UnicodeString&
NumberFormat::format(int32_t number, UnicodeString& appendTo) const
{
    FieldPosition pos(0);
    return format(number, appendTo, pos);
}




UnicodeString&
NumberFormat::format(int64_t number, UnicodeString& appendTo) const
{
    FieldPosition pos(0);
    return format(number, appendTo, pos);
}







void
NumberFormat::parse(const UnicodeString& text,
                        Formattable& result,
                        UErrorCode& status) const
{
    if (U_FAILURE(status)) return;

    ParsePosition parsePosition(0);
    parse(text, result, parsePosition);
    if (parsePosition.getIndex() == 0) {
        status = U_INVALID_FORMAT_ERROR;
    }
}

CurrencyAmount* NumberFormat::parseCurrency(const UnicodeString& text,
                                            ParsePosition& pos) const {
    
    Formattable parseResult;
    int32_t start = pos.getIndex();
    parse(text, parseResult, pos);
    if (pos.getIndex() != start) {
        UChar curr[4];
        UErrorCode ec = U_ZERO_ERROR;
        getEffectiveCurrency(curr, ec);
        if (U_SUCCESS(ec)) {
            LocalPointer<CurrencyAmount> currAmt(new CurrencyAmount(parseResult, curr, ec), ec);
            if (U_FAILURE(ec)) {
                pos.setIndex(start); 
            } else {
                return currAmt.orphan();
            }
        }
    }
    return NULL;
}




void
NumberFormat::setParseIntegerOnly(UBool value)
{
    fParseIntegerOnly = value;
}




void
NumberFormat::setLenient(UBool enable)
{
    fLenient = enable;
}




NumberFormat* U_EXPORT2
NumberFormat::createInstance(UErrorCode& status)
{
    return createInstance(Locale::getDefault(), UNUM_DECIMAL, status);
}




NumberFormat* U_EXPORT2
NumberFormat::createInstance(const Locale& inLocale, UErrorCode& status)
{
    return createInstance(inLocale, UNUM_DECIMAL, status);
}




NumberFormat* U_EXPORT2
NumberFormat::createCurrencyInstance(UErrorCode& status)
{
    return createCurrencyInstance(Locale::getDefault(),  status);
}




NumberFormat* U_EXPORT2
NumberFormat::createCurrencyInstance(const Locale& inLocale, UErrorCode& status)
{
    return createInstance(inLocale, UNUM_CURRENCY, status);
}




NumberFormat* U_EXPORT2
NumberFormat::createPercentInstance(UErrorCode& status)
{
    return createInstance(Locale::getDefault(), UNUM_PERCENT, status);
}




NumberFormat* U_EXPORT2
NumberFormat::createPercentInstance(const Locale& inLocale, UErrorCode& status)
{
    return createInstance(inLocale, UNUM_PERCENT, status);
}




NumberFormat* U_EXPORT2
NumberFormat::createScientificInstance(UErrorCode& status)
{
    return createInstance(Locale::getDefault(), UNUM_SCIENTIFIC, status);
}




NumberFormat* U_EXPORT2
NumberFormat::createScientificInstance(const Locale& inLocale, UErrorCode& status)
{
    return createInstance(inLocale, UNUM_SCIENTIFIC, status);
}



const Locale* U_EXPORT2
NumberFormat::getAvailableLocales(int32_t& count)
{
    return Locale::getAvailableLocales(count);
}







#if !UCONFIG_NO_SERVICE



class ICUNumberFormatFactory : public ICUResourceBundleFactory {
public:
    virtual ~ICUNumberFormatFactory();
protected:
    virtual UObject* handleCreate(const Locale& loc, int32_t kind, const ICUService* , UErrorCode& status) const {
        return NumberFormat::makeInstance(loc, (UNumberFormatStyle)kind, status);
    }
};

ICUNumberFormatFactory::~ICUNumberFormatFactory() {}



class NFFactory : public LocaleKeyFactory {
private:
    NumberFormatFactory* _delegate;
    Hashtable* _ids;

public:
    NFFactory(NumberFormatFactory* delegate)
        : LocaleKeyFactory(delegate->visible() ? VISIBLE : INVISIBLE)
        , _delegate(delegate)
        , _ids(NULL)
    {
    }

    virtual ~NFFactory();

    virtual UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const
    {
        if (handlesKey(key, status)) {
            const LocaleKey& lkey = (const LocaleKey&)key;
            Locale loc;
            lkey.canonicalLocale(loc);
            int32_t kind = lkey.kind();

            UObject* result = _delegate->createFormat(loc, (UNumberFormatStyle)kind);
            if (result == NULL) {
                result = service->getKey((ICUServiceKey&)key , NULL, this, status);
            }
            return result;
        }
        return NULL;
    }

protected:
    




    virtual const Hashtable* getSupportedIDs(UErrorCode& status) const
    {
        if (U_SUCCESS(status)) {
            if (!_ids) {
                int32_t count = 0;
                const UnicodeString * const idlist = _delegate->getSupportedIDs(count, status);
                ((NFFactory*)this)->_ids = new Hashtable(status); 
                if (_ids) {
                    for (int i = 0; i < count; ++i) {
                        _ids->put(idlist[i], (void*)this, status);
                    }
                }
            }
            return _ids;
        }
        return NULL;
    }
};

NFFactory::~NFFactory()
{
    delete _delegate;
    delete _ids;
}

class ICUNumberFormatService : public ICULocaleService {
public:
    ICUNumberFormatService()
        : ICULocaleService(UNICODE_STRING_SIMPLE("Number Format"))
    {
        UErrorCode status = U_ZERO_ERROR;
        registerFactory(new ICUNumberFormatFactory(), status);
    }

    virtual ~ICUNumberFormatService();

    virtual UObject* cloneInstance(UObject* instance) const {
        return ((NumberFormat*)instance)->clone();
    }

    virtual UObject* handleDefault(const ICUServiceKey& key, UnicodeString* , UErrorCode& status) const {
        LocaleKey& lkey = (LocaleKey&)key;
        int32_t kind = lkey.kind();
        Locale loc;
        lkey.currentLocale(loc);
        return NumberFormat::makeInstance(loc, (UNumberFormatStyle)kind, status);
    }

    virtual UBool isDefault() const {
        return countFactories() == 1;
    }
};

ICUNumberFormatService::~ICUNumberFormatService() {}



static void U_CALLCONV initNumberFormatService() {
    U_ASSERT(gService == NULL);
    ucln_i18n_registerCleanup(UCLN_I18N_NUMFMT, numfmt_cleanup);
    gService = new ICUNumberFormatService();
}

static ICULocaleService*
getNumberFormatService(void)
{
    umtx_initOnce(gServiceInitOnce, &initNumberFormatService);
    return gService;
}

static UBool haveService() {
    return !gServiceInitOnce.isReset() && (getNumberFormatService() != NULL);
}



URegistryKey U_EXPORT2
NumberFormat::registerFactory(NumberFormatFactory* toAdopt, UErrorCode& status)
{
  ICULocaleService *service = getNumberFormatService();
  if (service) {
	  NFFactory *tempnnf = new NFFactory(toAdopt);
	  if (tempnnf != NULL) {
		  return service->registerFactory(tempnnf, status);
	  }
  }
  status = U_MEMORY_ALLOCATION_ERROR;
  return NULL;
}



UBool U_EXPORT2
NumberFormat::unregister(URegistryKey key, UErrorCode& status)
{
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (haveService()) {
        return gService->unregister(key, status);
    } else {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
}


StringEnumeration* U_EXPORT2
NumberFormat::getAvailableLocales(void)
{
  ICULocaleService *service = getNumberFormatService();
  if (service) {
      return service->getAvailableLocales();
  }
  return NULL; 
}
#endif 


NumberFormat*
NumberFormat::internalCreateInstance(const Locale& loc, UNumberFormatStyle kind, UErrorCode& status) {
#if !UCONFIG_NO_SERVICE
    if (haveService()) {
        return (NumberFormat*)gService->get(loc, kind, status);
    }
#endif
    return makeInstance(loc, kind, status);
}

NumberFormat* U_EXPORT2
NumberFormat::createInstance(const Locale& loc, UNumberFormatStyle kind, UErrorCode& status) {
    if (kind != UNUM_DECIMAL) {
        return internalCreateInstance(loc, kind, status);
    }
    const SharedNumberFormat *shared = createSharedInstance(loc, kind, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    NumberFormat *result = static_cast<NumberFormat *>((*shared)->clone());
    shared->removeRef();
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}
    





UBool
NumberFormat::isGroupingUsed() const
{
    return fGroupingUsed;
}





void
NumberFormat::setGroupingUsed(UBool newValue)
{
    fGroupingUsed = newValue;
}





int32_t NumberFormat::getMaximumIntegerDigits() const
{
    return fMaxIntegerDigits;
}





void
NumberFormat::setMaximumIntegerDigits(int32_t newValue)
{
    fMaxIntegerDigits = uprv_max(0, uprv_min(newValue, gDefaultMaxIntegerDigits));
    if(fMinIntegerDigits > fMaxIntegerDigits)
        fMinIntegerDigits = fMaxIntegerDigits;
}





int32_t
NumberFormat::getMinimumIntegerDigits() const
{
    return fMinIntegerDigits;
}





void
NumberFormat::setMinimumIntegerDigits(int32_t newValue)
{
    fMinIntegerDigits = uprv_max(0, uprv_min(newValue, gDefaultMinIntegerDigits));
    if(fMinIntegerDigits > fMaxIntegerDigits)
        fMaxIntegerDigits = fMinIntegerDigits;
}





int32_t
NumberFormat::getMaximumFractionDigits() const
{
    return fMaxFractionDigits;
}





void
NumberFormat::setMaximumFractionDigits(int32_t newValue)
{
    fMaxFractionDigits = uprv_max(0, uprv_min(newValue, gDefaultMaxIntegerDigits));
    if(fMaxFractionDigits < fMinFractionDigits)
        fMinFractionDigits = fMaxFractionDigits;
}





int32_t
NumberFormat::getMinimumFractionDigits() const
{
    return fMinFractionDigits;
}





void
NumberFormat::setMinimumFractionDigits(int32_t newValue)
{
    fMinFractionDigits = uprv_max(0, uprv_min(newValue, gDefaultMinIntegerDigits));
    if (fMaxFractionDigits < fMinFractionDigits)
        fMaxFractionDigits = fMinFractionDigits;
}



void NumberFormat::setCurrency(const UChar* theCurrency, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return;
    }
    if (theCurrency) {
        u_strncpy(fCurrency, theCurrency, 3);
        fCurrency[3] = 0;
    } else {
        fCurrency[0] = 0;
    }
}

const UChar* NumberFormat::getCurrency() const {
    return fCurrency;
}

void NumberFormat::getEffectiveCurrency(UChar* result, UErrorCode& ec) const {
    const UChar* c = getCurrency();
    if (*c != 0) {
        u_strncpy(result, c, 3);
        result[3] = 0;
    } else {
        const char* loc = getLocaleID(ULOC_VALID_LOCALE, ec);
        if (loc == NULL) {
            loc = uloc_getDefault();
        }
        ucurr_forLocale(loc, result, 4, &ec);
    }
}




void NumberFormat::setContext(UDisplayContext value, UErrorCode& status)
{
    if (U_FAILURE(status))
        return;
    if ( (UDisplayContextType)((uint32_t)value >> 8) == UDISPCTX_TYPE_CAPITALIZATION ) {
        fCapitalizationContext = value;
    } else {
        status = U_ILLEGAL_ARGUMENT_ERROR;
   }
}


UDisplayContext NumberFormat::getContext(UDisplayContextType type, UErrorCode& status) const
{
    if (U_FAILURE(status))
        return (UDisplayContext)0;
    if (type != UDISPCTX_TYPE_CAPITALIZATION) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return (UDisplayContext)0;
    }
    return fCapitalizationContext;
}






static void U_CALLCONV nscacheInit() {
    U_ASSERT(NumberingSystem_cache == NULL);
    ucln_i18n_registerCleanup(UCLN_I18N_NUMFMT, numfmt_cleanup);
    UErrorCode status = U_ZERO_ERROR;
    NumberingSystem_cache = uhash_open(uhash_hashLong,
                                       uhash_compareLong,
                                       NULL,
                                       &status);
    if (U_FAILURE(status)) {
        
        NumberingSystem_cache = NULL;
        return;
    }
    uhash_setValueDeleter(NumberingSystem_cache, deleteNumberingSystem);
}

template<> U_I18N_API
const SharedNumberFormat *LocaleCacheKey<SharedNumberFormat>::createObject(
        const void * , UErrorCode &status) const {
    const char *localeId = fLoc.getName();
    NumberFormat *nf = NumberFormat::internalCreateInstance(
            localeId, UNUM_DECIMAL, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    SharedNumberFormat *result = new SharedNumberFormat(nf);
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        delete nf;
        return NULL;
    }
    result->addRef();
    return result;
}

const SharedNumberFormat* U_EXPORT2
NumberFormat::createSharedInstance(const Locale& loc, UNumberFormatStyle kind, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    if (kind != UNUM_DECIMAL) {
        status = U_UNSUPPORTED_ERROR;
        return NULL;
    }
    const SharedNumberFormat *result = NULL;
    UnifiedCache::getByLocale(loc, result, status);
    return result;
}

UBool
NumberFormat::isStyleSupported(UNumberFormatStyle style) {
    return gLastResortNumberPatterns[style] != NULL;
}

NumberFormat*
NumberFormat::makeInstance(const Locale& desiredLocale,
                           UNumberFormatStyle style,
                           UErrorCode& status) {
  return makeInstance(desiredLocale, style, false, status);
}

NumberFormat*
NumberFormat::makeInstance(const Locale& desiredLocale,
                           UNumberFormatStyle style,
                           UBool mustBeDecimalFormat,
                           UErrorCode& status) {
    if (U_FAILURE(status)) return NULL;

    if (style < 0 || style >= UNUM_FORMAT_STYLE_COUNT) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    
    
    
    
    
    
    if (!isStyleSupported(style)) {
        status = U_UNSUPPORTED_ERROR;
        return NULL;
    }

#if U_PLATFORM_USES_ONLY_WIN32_API
    if (!mustBeDecimalFormat) {
        char buffer[8];
        int32_t count = desiredLocale.getKeywordValue("compat", buffer, sizeof(buffer), status);

        
        if (U_SUCCESS(status) && count > 0 && uprv_strcmp(buffer, "host") == 0) {
            Win32NumberFormat *f = NULL;
            UBool curr = TRUE;

            switch (style) {
            case UNUM_DECIMAL:
                curr = FALSE;
                

            case UNUM_CURRENCY:
            case UNUM_CURRENCY_ISO: 
            case UNUM_CURRENCY_PLURAL:
            case UNUM_CURRENCY_ACCOUNTING:
            case UNUM_CASH_CURRENCY:
                f = new Win32NumberFormat(desiredLocale, curr, status);

                if (U_SUCCESS(status)) {
                    return f;
                }

                delete f;
                break;
            default:
                break;
            }
        }
    }
#endif
    
    umtx_initOnce(gNSCacheInitOnce, &nscacheInit);

    
    LocalPointer<NumberingSystem> ownedNs;
    NumberingSystem *ns = NULL;
    if (NumberingSystem_cache != NULL) {
        
        int32_t hashKey = desiredLocale.hashCode();

        Mutex lock(&nscacheMutex);
        ns = (NumberingSystem *)uhash_iget(NumberingSystem_cache, hashKey);
        if (ns == NULL) {
            ns = NumberingSystem::createInstance(desiredLocale,status);
            uhash_iput(NumberingSystem_cache, hashKey, (void*)ns, &status);
        }
    } else {
        ownedNs.adoptInstead(NumberingSystem::createInstance(desiredLocale,status));
        ns = ownedNs.getAlias();
    }

    
    if (U_FAILURE(status)) {
        return NULL;
    }

    if (mustBeDecimalFormat && ns->isAlgorithmic()) {
        status = U_UNSUPPORTED_ERROR;
        return NULL;
    }

    LocalPointer<DecimalFormatSymbols> symbolsToAdopt;
    UnicodeString pattern;
    LocalUResourceBundlePointer ownedResource(ures_open(NULL, desiredLocale.getName(), &status));
    if (U_FAILURE(status)) {
        return NULL;
    }
    else {
        
        symbolsToAdopt.adoptInsteadAndCheckErrorCode(new DecimalFormatSymbols(desiredLocale, status), status);
        if (U_FAILURE(status)) {
            return NULL;
        }

        UResourceBundle *resource = ownedResource.orphan();
        UResourceBundle *numElements = ures_getByKeyWithFallback(resource, gNumberElements, NULL, &status);
        resource = ures_getByKeyWithFallback(numElements, ns->getName(), resource, &status);
        resource = ures_getByKeyWithFallback(resource, gPatterns, resource, &status);
        ownedResource.adoptInstead(resource);

        int32_t patLen = 0;
        const UChar *patResStr = ures_getStringByKeyWithFallback(resource, gFormatKeys[style], &patLen, &status);

        
        if ( status == U_MISSING_RESOURCE_ERROR && uprv_strcmp(gLatn,ns->getName())) {  
            status = U_ZERO_ERROR;
            resource = ures_getByKeyWithFallback(numElements, gLatn, resource, &status);
            resource = ures_getByKeyWithFallback(resource, gPatterns, resource, &status);
            patResStr = ures_getStringByKeyWithFallback(resource, gFormatKeys[style], &patLen, &status);
        }

        ures_close(numElements);

        
        pattern.setTo(TRUE, patResStr, patLen);
    }
    if (U_FAILURE(status)) {
        return NULL;
    }
    if(style==UNUM_CURRENCY || style == UNUM_CURRENCY_ISO || style == UNUM_CURRENCY_ACCOUNTING 
        || style == UNUM_CASH_CURRENCY){
        const UChar* currPattern = symbolsToAdopt->getCurrencyPattern();
        if(currPattern!=NULL){
            pattern.setTo(currPattern, u_strlen(currPattern));
        }
    }


    NumberFormat *f;
    if (ns->isAlgorithmic()) {
        UnicodeString nsDesc;
        UnicodeString nsRuleSetGroup;
        UnicodeString nsRuleSetName;
        Locale nsLoc;
        URBNFRuleSetTag desiredRulesType = URBNF_NUMBERING_SYSTEM;

        nsDesc.setTo(ns->getDescription());
        int32_t firstSlash = nsDesc.indexOf(gSlash);
        int32_t lastSlash = nsDesc.lastIndexOf(gSlash);
        if ( lastSlash > firstSlash ) {
            CharString nsLocID;

            nsLocID.appendInvariantChars(nsDesc.tempSubString(0, firstSlash), status);
            nsRuleSetGroup.setTo(nsDesc,firstSlash+1,lastSlash-firstSlash-1);
            nsRuleSetName.setTo(nsDesc,lastSlash+1);

            nsLoc = Locale::createFromName(nsLocID.data());

            UnicodeString SpelloutRules = UNICODE_STRING_SIMPLE("SpelloutRules");
            if ( nsRuleSetGroup.compare(SpelloutRules) == 0 ) {
                desiredRulesType = URBNF_SPELLOUT;
            }
        } else {
            nsLoc = desiredLocale;
            nsRuleSetName.setTo(nsDesc);
        }

        RuleBasedNumberFormat *r = new RuleBasedNumberFormat(desiredRulesType,nsLoc,status);
        if (r == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        r->setDefaultRuleSet(nsRuleSetName,status);
        f = r;
    } else {
        
        
        if (style == UNUM_CURRENCY_ISO) {
            pattern.findAndReplace(UnicodeString(TRUE, gSingleCurrencySign, 1),
                                   UnicodeString(TRUE, gDoubleCurrencySign, 2));
        }

        
        DecimalFormatSymbols *syms = symbolsToAdopt.orphan();
        DecimalFormat* df = new DecimalFormat(pattern, syms, style, status);

        
        if (style == UNUM_CASH_CURRENCY){
            df->setCurrencyUsage(UCURR_USAGE_CASH, &status);
        }

        if (U_FAILURE(status)) {
            delete df;
            return NULL;
        }

        f = df;
        if (f == NULL) {
            delete syms;
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
    }

    f->setLocaleIDs(ures_getLocaleByType(ownedResource.getAlias(), ULOC_VALID_LOCALE, &status),
                    ures_getLocaleByType(ownedResource.getAlias(), ULOC_ACTUAL_LOCALE, &status));
    if (U_FAILURE(status)) {
        delete f;
        return NULL;
    }
    return f;
}

U_NAMESPACE_END

#endif 


