










#include "builtin/Intl.h"

#include "mozilla/Range.h"

#include <string.h>

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsobj.h"

#if ENABLE_INTL_API
#include "unicode/locid.h"
#include "unicode/numsys.h"
#include "unicode/ucal.h"
#include "unicode/ucol.h"
#include "unicode/udat.h"
#include "unicode/udatpg.h"
#include "unicode/uenum.h"
#include "unicode/unum.h"
#include "unicode/ustring.h"
#endif
#include "vm/DateTime.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/Stack.h"
#include "vm/StringBuffer.h"

#include "jsobjinlines.h"

#include "vm/NativeObject-inl.h"

using namespace js;

using mozilla::IsFinite;
using mozilla::IsNegativeZero;

#if ENABLE_INTL_API
using icu::Locale;
using icu::NumberingSystem;
#endif













#if !ENABLE_INTL_API











static int32_t
u_strlen(const UChar* s)
{
    MOZ_CRASH("u_strlen: Intl API disabled");
}

struct UEnumeration;

static int32_t
uenum_count(UEnumeration* en, UErrorCode* status)
{
    MOZ_CRASH("uenum_count: Intl API disabled");
}

static const char*
uenum_next(UEnumeration* en, int32_t* resultLength, UErrorCode* status)
{
    MOZ_CRASH("uenum_next: Intl API disabled");
}

static void
uenum_close(UEnumeration* en)
{
    MOZ_CRASH("uenum_close: Intl API disabled");
}

struct UCollator;

enum UColAttribute {
     UCOL_ALTERNATE_HANDLING,
     UCOL_CASE_FIRST,
     UCOL_CASE_LEVEL,
     UCOL_NORMALIZATION_MODE,
     UCOL_STRENGTH,
     UCOL_NUMERIC_COLLATION,
};

enum UColAttributeValue {
  UCOL_DEFAULT = -1,
  UCOL_PRIMARY = 0,
  UCOL_SECONDARY = 1,
  UCOL_TERTIARY = 2,
  UCOL_OFF = 16,
  UCOL_ON = 17,
  UCOL_SHIFTED = 20,
  UCOL_LOWER_FIRST = 24,
  UCOL_UPPER_FIRST = 25,
};

enum UCollationResult {
  UCOL_EQUAL = 0,
  UCOL_GREATER = 1,
  UCOL_LESS = -1
};

static int32_t
ucol_countAvailable()
{
    MOZ_CRASH("ucol_countAvailable: Intl API disabled");
}

static const char*
ucol_getAvailable(int32_t localeIndex)
{
    MOZ_CRASH("ucol_getAvailable: Intl API disabled");
}

static UCollator*
ucol_open(const char* loc, UErrorCode* status)
{
    MOZ_CRASH("ucol_open: Intl API disabled");
}

static void
ucol_setAttribute(UCollator* coll, UColAttribute attr, UColAttributeValue value, UErrorCode* status)
{
    MOZ_CRASH("ucol_setAttribute: Intl API disabled");
}

static UCollationResult
ucol_strcoll(const UCollator* coll, const UChar* source, int32_t sourceLength,
             const UChar* target, int32_t targetLength)
{
    MOZ_CRASH("ucol_strcoll: Intl API disabled");
}

static void
ucol_close(UCollator* coll)
{
    MOZ_CRASH("ucol_close: Intl API disabled");
}

static UEnumeration*
ucol_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed,
                               UErrorCode* status)
{
    MOZ_CRASH("ucol_getKeywordValuesForLocale: Intl API disabled");
}

struct UParseError;
struct UFieldPosition;
typedef void* UNumberFormat;

enum UNumberFormatStyle {
    UNUM_DECIMAL = 1,
    UNUM_CURRENCY,
    UNUM_PERCENT,
    UNUM_CURRENCY_ISO,
    UNUM_CURRENCY_PLURAL,
};

enum UNumberFormatRoundingMode {
    UNUM_ROUND_HALFUP,
};

enum UNumberFormatAttribute {
  UNUM_GROUPING_USED,
  UNUM_MIN_INTEGER_DIGITS,
  UNUM_MAX_FRACTION_DIGITS,
  UNUM_MIN_FRACTION_DIGITS,
  UNUM_ROUNDING_MODE,
  UNUM_SIGNIFICANT_DIGITS_USED,
  UNUM_MIN_SIGNIFICANT_DIGITS,
  UNUM_MAX_SIGNIFICANT_DIGITS,
};

enum UNumberFormatTextAttribute {
  UNUM_CURRENCY_CODE,
};

static int32_t
unum_countAvailable()
{
    MOZ_CRASH("unum_countAvailable: Intl API disabled");
}

static const char*
unum_getAvailable(int32_t localeIndex)
{
    MOZ_CRASH("unum_getAvailable: Intl API disabled");
}

static UNumberFormat*
unum_open(UNumberFormatStyle style, const UChar* pattern, int32_t patternLength,
          const char* locale, UParseError* parseErr, UErrorCode* status)
{
    MOZ_CRASH("unum_open: Intl API disabled");
}

static void
unum_setAttribute(UNumberFormat* fmt, UNumberFormatAttribute  attr, int32_t newValue)
{
    MOZ_CRASH("unum_setAttribute: Intl API disabled");
}

static int32_t
unum_formatDouble(const UNumberFormat* fmt, double number, UChar* result,
                  int32_t resultLength, UFieldPosition* pos, UErrorCode* status)
{
    MOZ_CRASH("unum_formatDouble: Intl API disabled");
}

static void
unum_close(UNumberFormat* fmt)
{
    MOZ_CRASH("unum_close: Intl API disabled");
}

static void
unum_setTextAttribute(UNumberFormat* fmt, UNumberFormatTextAttribute tag, const UChar* newValue,
                      int32_t newValueLength, UErrorCode* status)
{
    MOZ_CRASH("unum_setTextAttribute: Intl API disabled");
}

class Locale {
  public:
    Locale(const char* language, const char* country = 0, const char* variant = 0,
           const char* keywordsAndValues = 0);
};

Locale::Locale(const char* language, const char* country, const char* variant,
               const char* keywordsAndValues)
{
    MOZ_CRASH("Locale::Locale: Intl API disabled");
}

class NumberingSystem {
  public:
    static NumberingSystem* createInstance(const Locale& inLocale, UErrorCode& status);
    const char* getName();
};

NumberingSystem*
NumberingSystem::createInstance(const Locale& inLocale, UErrorCode& status)
{
    MOZ_CRASH("NumberingSystem::createInstance: Intl API disabled");
}

const char*
NumberingSystem::getName()
{
    MOZ_CRASH("NumberingSystem::getName: Intl API disabled");
}

typedef void* UCalendar;

enum UCalendarType {
  UCAL_TRADITIONAL,
  UCAL_DEFAULT = UCAL_TRADITIONAL,
  UCAL_GREGORIAN
};

static UCalendar*
ucal_open(const UChar* zoneID, int32_t len, const char* locale,
          UCalendarType type, UErrorCode* status)
{
    MOZ_CRASH("ucal_open: Intl API disabled");
}

static const char*
ucal_getType(const UCalendar* cal, UErrorCode* status)
{
    MOZ_CRASH("ucal_getType: Intl API disabled");
}

static UEnumeration*
ucal_getKeywordValuesForLocale(const char* key, const char* locale,
                               UBool commonlyUsed, UErrorCode* status)
{
    MOZ_CRASH("ucal_getKeywordValuesForLocale: Intl API disabled");
}

static void
ucal_close(UCalendar* cal)
{
    MOZ_CRASH("ucal_close: Intl API disabled");
}

typedef void* UDateTimePatternGenerator;

static UDateTimePatternGenerator*
udatpg_open(const char* locale, UErrorCode* pErrorCode)
{
    MOZ_CRASH("udatpg_open: Intl API disabled");
}

static int32_t
udatpg_getBestPattern(UDateTimePatternGenerator* dtpg, const UChar* skeleton,
                      int32_t length, UChar* bestPattern, int32_t capacity,
                      UErrorCode* pErrorCode)
{
    MOZ_CRASH("udatpg_getBestPattern: Intl API disabled");
}

static void
udatpg_close(UDateTimePatternGenerator* dtpg)
{
    MOZ_CRASH("udatpg_close: Intl API disabled");
}

typedef void* UCalendar;
typedef void* UDateFormat;

enum UDateFormatStyle {
    UDAT_PATTERN = -2,
    UDAT_IGNORE = UDAT_PATTERN
};

static int32_t
udat_countAvailable()
{
    MOZ_CRASH("udat_countAvailable: Intl API disabled");
}

static const char*
udat_getAvailable(int32_t localeIndex)
{
    MOZ_CRASH("udat_getAvailable: Intl API disabled");
}

static UDateFormat*
udat_open(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const char* locale,
          const UChar* tzID, int32_t tzIDLength, const UChar* pattern,
          int32_t patternLength, UErrorCode* status)
{
    MOZ_CRASH("udat_open: Intl API disabled");
}

static const UCalendar*
udat_getCalendar(const UDateFormat* fmt)
{
    MOZ_CRASH("udat_getCalendar: Intl API disabled");
}

static void
ucal_setGregorianChange(UCalendar* cal, UDate date, UErrorCode* pErrorCode)
{
    MOZ_CRASH("ucal_setGregorianChange: Intl API disabled");
}

static int32_t
udat_format(const UDateFormat* format, UDate dateToFormat, UChar* result,
            int32_t resultLength, UFieldPosition* position, UErrorCode* status)
{
    MOZ_CRASH("udat_format: Intl API disabled");
}

static void
udat_close(UDateFormat* format)
{
    MOZ_CRASH("udat_close: Intl API disabled");
}

#endif




static bool
IntlInitialize(JSContext* cx, HandleObject obj, Handle<PropertyName*> initializer,
               HandleValue locales, HandleValue options)
{
    RootedValue initializerValue(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), initializer, &initializerValue))
        return false;
    MOZ_ASSERT(initializerValue.isObject());
    MOZ_ASSERT(initializerValue.toObject().is<JSFunction>());

    InvokeArgs args(cx);
    if (!args.init(3))
        return false;

    args.setCallee(initializerValue);
    args.setThis(NullValue());
    args[0].setObject(*obj);
    args[1].set(locales);
    args[2].set(options);

    return Invoke(cx, args);
}

static bool
CreateDefaultOptions(JSContext* cx, MutableHandleValue defaultOptions)
{
    RootedObject options(cx, NewObjectWithGivenProto<PlainObject>(cx, NullPtr()));
    if (!options)
        return false;
    defaultOptions.setObject(*options);
    return true;
}



typedef int32_t
(* CountAvailable)();

typedef const char*
(* GetAvailable)(int32_t localeIndex);

static bool
intl_availableLocales(JSContext* cx, CountAvailable countAvailable,
                      GetAvailable getAvailable, MutableHandleValue result)
{
    RootedObject locales(cx, NewObjectWithGivenProto<PlainObject>(cx, NullPtr()));
    if (!locales)
        return false;

#if ENABLE_INTL_API
    uint32_t count = countAvailable();
    RootedValue t(cx, BooleanValue(true));
    for (uint32_t i = 0; i < count; i++) {
        const char* locale = getAvailable(i);
        ScopedJSFreePtr<char> lang(JS_strdup(cx, locale));
        if (!lang)
            return false;
        char* p;
        while ((p = strchr(lang, '_')))
            *p = '-';
        RootedAtom a(cx, Atomize(cx, lang, strlen(lang)));
        if (!a)
            return false;
        if (!DefineProperty(cx, locales, a->asPropertyName(), t, nullptr, nullptr,
                            JSPROP_ENUMERATE))
        {
            return false;
        }
    }
#endif
    result.setObject(*locales);
    return true;
}




static bool
GetInternals(JSContext* cx, HandleObject obj, MutableHandleObject internals)
{
    RootedValue getInternalsValue(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), cx->names().getInternals, &getInternalsValue))
        return false;
    MOZ_ASSERT(getInternalsValue.isObject());
    MOZ_ASSERT(getInternalsValue.toObject().is<JSFunction>());

    InvokeArgs args(cx);
    if (!args.init(1))
        return false;

    args.setCallee(getInternalsValue);
    args.setThis(NullValue());
    args[0].setObject(*obj);

    if (!Invoke(cx, args))
        return false;
    internals.set(&args.rval().toObject());
    return true;
}

static bool
equal(const char* s1, const char* s2)
{
    return !strcmp(s1, s2);
}

static bool
equal(JSAutoByteString& s1, const char* s2)
{
    return !strcmp(s1.ptr(), s2);
}

static const char*
icuLocale(const char* locale)
{
    if (equal(locale, "und"))
        return ""; 
    return locale;
}






template <typename T>
class ScopedICUObject
{
    T* ptr_;
    void (* deleter_)(T*);

  public:
    ScopedICUObject(T* ptr, void (*deleter)(T*))
      : ptr_(ptr),
        deleter_(deleter)
    {}

    ~ScopedICUObject() {
        if (ptr_)
            deleter_(ptr_);
    }

    
    
    
    T* forget() {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }
};


static const size_t INITIAL_CHAR_BUFFER_SIZE = 32;



static void collator_finalize(FreeOp* fop, JSObject* obj);

static const uint32_t UCOLLATOR_SLOT = 0;
static const uint32_t COLLATOR_SLOTS_COUNT = 1;

static const Class CollatorClass = {
    js_Object_str,
    JSCLASS_HAS_RESERVED_SLOTS(COLLATOR_SLOTS_COUNT),
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    collator_finalize
};

#if JS_HAS_TOSOURCE
static bool
collator_toSource(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setString(cx->names().Collator);
    return true;
}
#endif

static const JSFunctionSpec collator_static_methods[] = {
    JS_SELF_HOSTED_FN("supportedLocalesOf", "Intl_Collator_supportedLocalesOf", 1, 0),
    JS_FS_END
};

static const JSFunctionSpec collator_methods[] = {
    JS_SELF_HOSTED_FN("resolvedOptions", "Intl_Collator_resolvedOptions", 0, 0),
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str, collator_toSource, 0, 0),
#endif
    JS_FS_END
};





static bool
Collator(JSContext* cx, CallArgs args, bool construct)
{
    RootedObject obj(cx);

    if (!construct) {
        
        JSObject* intl = cx->global()->getOrCreateIntlObject(cx);
        if (!intl)
            return false;
        RootedValue self(cx, args.thisv());
        if (!self.isUndefined() && (!self.isObject() || self.toObject() != *intl)) {
            
            obj = ToObject(cx, self);
            if (!obj)
                return false;

            
            bool extensible;
            if (!IsExtensible(cx, obj, &extensible))
                return false;
            if (!extensible)
                return Throw(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE);
        } else {
            
            construct = true;
        }
    }
    if (construct) {
        
        RootedObject proto(cx, cx->global()->getOrCreateCollatorPrototype(cx));
        if (!proto)
            return false;
        obj = NewObjectWithGivenProto(cx, &CollatorClass, proto);
        if (!obj)
            return false;

        obj->as<NativeObject>().setReservedSlot(UCOLLATOR_SLOT, PrivateValue(nullptr));
    }

    
    RootedValue locales(cx, args.length() > 0 ? args[0] : UndefinedValue());
    RootedValue options(cx, args.length() > 1 ? args[1] : UndefinedValue());

    
    if (!IntlInitialize(cx, obj, cx->names().InitializeCollator, locales, options))
        return false;

    
    args.rval().setObject(*obj);
    return true;
}

static bool
Collator(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return Collator(cx, args, args.isConstructing());
}

bool
js::intl_Collator(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    
    
    return Collator(cx, args, true);
}

static void
collator_finalize(FreeOp* fop, JSObject* obj)
{
    
    
    
    
    const Value& slot = obj->as<NativeObject>().getReservedSlot(UCOLLATOR_SLOT);
    if (!slot.isUndefined()) {
        if (UCollator* coll = static_cast<UCollator*>(slot.toPrivate()))
            ucol_close(coll);
    }
}

static JSObject*
InitCollatorClass(JSContext* cx, HandleObject Intl, Handle<GlobalObject*> global)
{
    RootedFunction ctor(cx, global->createConstructor(cx, &Collator, cx->names().Collator, 0));
    if (!ctor)
        return nullptr;

    RootedObject proto(cx, global->as<GlobalObject>().getOrCreateCollatorPrototype(cx));
    if (!proto)
        return nullptr;
    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return nullptr;

    
    if (!JS_DefineFunctions(cx, ctor, collator_static_methods))
        return nullptr;

    
    if (!JS_DefineFunctions(cx, proto, collator_methods))
        return nullptr;

    




    RootedValue getter(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), cx->names().CollatorCompareGet, &getter))
        return nullptr;
    if (!DefineProperty(cx, proto, cx->names().compare, UndefinedHandleValue,
                        JS_DATA_TO_FUNC_PTR(JSGetterOp, &getter.toObject()),
                        nullptr, JSPROP_GETTER | JSPROP_SHARED))
    {
        return nullptr;
    }

    RootedValue options(cx);
    if (!CreateDefaultOptions(cx, &options))
        return nullptr;

    
    if (!IntlInitialize(cx, proto, cx->names().InitializeCollator, UndefinedHandleValue, options))
        return nullptr;

    
    RootedValue ctorValue(cx, ObjectValue(*ctor));
    if (!DefineProperty(cx, Intl, cx->names().Collator, ctorValue, nullptr, nullptr, 0))
        return nullptr;

    return ctor;
}

bool
GlobalObject::initCollatorProto(JSContext* cx, Handle<GlobalObject*> global)
{
    RootedNativeObject proto(cx, global->createBlankPrototype(cx, &CollatorClass));
    if (!proto)
        return false;
    proto->setReservedSlot(UCOLLATOR_SLOT, PrivateValue(nullptr));
    global->setReservedSlot(COLLATOR_PROTO, ObjectValue(*proto));
    return true;
}

bool
js::intl_Collator_availableLocales(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 0);

    RootedValue result(cx);
    if (!intl_availableLocales(cx, ucol_countAvailable, ucol_getAvailable, &result))
        return false;
    args.rval().set(result);
    return true;
}

bool
js::intl_availableCollations(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isString());

    JSAutoByteString locale(cx, args[0].toString());
    if (!locale)
        return false;
    UErrorCode status = U_ZERO_ERROR;
    UEnumeration* values = ucol_getKeywordValuesForLocale("co", locale.ptr(), false, &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }
    ScopedICUObject<UEnumeration> toClose(values, uenum_close);

    uint32_t count = uenum_count(values, &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }

    RootedObject collations(cx, NewDenseEmptyArray(cx));
    if (!collations)
        return false;

    uint32_t index = 0;
    for (uint32_t i = 0; i < count; i++) {
        const char* collation = uenum_next(values, nullptr, &status);
        if (U_FAILURE(status)) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
            return false;
        }

        
        
        
        
        if (equal(collation, "standard") || equal(collation, "search"))
            continue;

        
        
        if (equal(collation, "dictionary"))
            collation = "dict";
        else if (equal(collation, "gb2312han"))
            collation = "gb2312";
        else if (equal(collation, "phonebook"))
            collation = "phonebk";
        else if (equal(collation, "traditional"))
            collation = "trad";

        RootedString jscollation(cx, JS_NewStringCopyZ(cx, collation));
        if (!jscollation)
            return false;
        RootedValue element(cx, StringValue(jscollation));
        if (!DefineElement(cx, collations, index++, element))
            return false;
    }

    args.rval().setObject(*collations);
    return true;
}





static UCollator*
NewUCollator(JSContext* cx, HandleObject collator)
{
    RootedValue value(cx);

    RootedObject internals(cx);
    if (!GetInternals(cx, collator, &internals))
        return nullptr;

    if (!GetProperty(cx, internals, internals, cx->names().locale, &value))
        return nullptr;
    JSAutoByteString locale(cx, value.toString());
    if (!locale)
        return nullptr;

    
    UColAttributeValue uStrength = UCOL_DEFAULT;
    UColAttributeValue uCaseLevel = UCOL_OFF;
    UColAttributeValue uAlternate = UCOL_DEFAULT;
    UColAttributeValue uNumeric = UCOL_OFF;
    
    UColAttributeValue uNormalization = UCOL_ON;
    UColAttributeValue uCaseFirst = UCOL_DEFAULT;

    if (!GetProperty(cx, internals, internals, cx->names().usage, &value))
        return nullptr;
    JSAutoByteString usage(cx, value.toString());
    if (!usage)
        return nullptr;
    if (equal(usage, "search")) {
        
        
        const char* oldLocale = locale.ptr();
        const char* p;
        size_t index;
        size_t localeLen = strlen(oldLocale);
        if ((p = strstr(oldLocale, "-x-")))
            index = p - oldLocale;
        else
            index = localeLen;

        const char* insert;
        if ((p = strstr(oldLocale, "-u-")) && static_cast<size_t>(p - oldLocale) < index) {
            index = p - oldLocale + 2;
            insert = "-co-search";
        } else {
            insert = "-u-co-search";
        }
        size_t insertLen = strlen(insert);
        char* newLocale = cx->pod_malloc<char>(localeLen + insertLen + 1);
        if (!newLocale)
            return nullptr;
        memcpy(newLocale, oldLocale, index);
        memcpy(newLocale + index, insert, insertLen);
        memcpy(newLocale + index + insertLen, oldLocale + index, localeLen - index + 1); 
        locale.clear();
        locale.initBytes(newLocale);
    }

    
    
    

    if (!GetProperty(cx, internals, internals, cx->names().sensitivity, &value))
        return nullptr;
    JSAutoByteString sensitivity(cx, value.toString());
    if (!sensitivity)
        return nullptr;
    if (equal(sensitivity, "base")) {
        uStrength = UCOL_PRIMARY;
    } else if (equal(sensitivity, "accent")) {
        uStrength = UCOL_SECONDARY;
    } else if (equal(sensitivity, "case")) {
        uStrength = UCOL_PRIMARY;
        uCaseLevel = UCOL_ON;
    } else {
        MOZ_ASSERT(equal(sensitivity, "variant"));
        uStrength = UCOL_TERTIARY;
    }

    if (!GetProperty(cx, internals, internals, cx->names().ignorePunctuation, &value))
        return nullptr;
    
    
    
    
    
    if (value.toBoolean())
        uAlternate = UCOL_SHIFTED;

    if (!GetProperty(cx, internals, internals, cx->names().numeric, &value))
        return nullptr;
    if (!value.isUndefined() && value.toBoolean())
        uNumeric = UCOL_ON;

    if (!GetProperty(cx, internals, internals, cx->names().caseFirst, &value))
        return nullptr;
    if (!value.isUndefined()) {
        JSAutoByteString caseFirst(cx, value.toString());
        if (!caseFirst)
            return nullptr;
        if (equal(caseFirst, "upper"))
            uCaseFirst = UCOL_UPPER_FIRST;
        else if (equal(caseFirst, "lower"))
            uCaseFirst = UCOL_LOWER_FIRST;
        else
            MOZ_ASSERT(equal(caseFirst, "false"));
    }

    UErrorCode status = U_ZERO_ERROR;
    UCollator* coll = ucol_open(icuLocale(locale.ptr()), &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return nullptr;
    }

    ucol_setAttribute(coll, UCOL_STRENGTH, uStrength, &status);
    ucol_setAttribute(coll, UCOL_CASE_LEVEL, uCaseLevel, &status);
    ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, uAlternate, &status);
    ucol_setAttribute(coll, UCOL_NUMERIC_COLLATION, uNumeric, &status);
    ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, uNormalization, &status);
    ucol_setAttribute(coll, UCOL_CASE_FIRST, uCaseFirst, &status);
    if (U_FAILURE(status)) {
        ucol_close(coll);
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return nullptr;
    }

    return coll;
}

static bool
intl_CompareStrings(JSContext* cx, UCollator* coll, HandleString str1, HandleString str2, MutableHandleValue result)
{
    MOZ_ASSERT(str1);
    MOZ_ASSERT(str2);

    if (str1 == str2) {
        result.setInt32(0);
        return true;
    }

    AutoStableStringChars stableChars1(cx);
    if (!stableChars1.initTwoByte(cx, str1))
        return false;

    AutoStableStringChars stableChars2(cx);
    if (!stableChars2.initTwoByte(cx, str2))
        return false;

    mozilla::Range<const char16_t> chars1 = stableChars1.twoByteRange();
    mozilla::Range<const char16_t> chars2 = stableChars2.twoByteRange();

    UCollationResult uresult = ucol_strcoll(coll,
                                            Char16ToUChar(chars1.start().get()), chars1.length(),
                                            Char16ToUChar(chars2.start().get()), chars2.length());
    int32_t res;
    switch (uresult) {
        case UCOL_LESS: res = -1; break;
        case UCOL_EQUAL: res = 0; break;
        case UCOL_GREATER: res = 1; break;
        default: MOZ_CRASH("ucol_strcoll returned bad UCollationResult");
    }
    result.setInt32(res);
    return true;
}

bool
js::intl_CompareStrings(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 3);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[1].isString());
    MOZ_ASSERT(args[2].isString());

    RootedObject collator(cx, &args[0].toObject());

    
    
    bool isCollatorInstance = collator->getClass() == &CollatorClass;
    UCollator* coll;
    if (isCollatorInstance) {
        coll = static_cast<UCollator*>(collator->as<NativeObject>().getReservedSlot(UCOLLATOR_SLOT).toPrivate());
        if (!coll) {
            coll = NewUCollator(cx, collator);
            if (!coll)
                return false;
            collator->as<NativeObject>().setReservedSlot(UCOLLATOR_SLOT, PrivateValue(coll));
        }
    } else {
        
        
        
        
        coll = NewUCollator(cx, collator);
        if (!coll)
            return false;
    }

    
    RootedString str1(cx, args[1].toString());
    RootedString str2(cx, args[2].toString());
    RootedValue result(cx);
    bool success = intl_CompareStrings(cx, coll, str1, str2, &result);

    if (!isCollatorInstance)
        ucol_close(coll);
    if (!success)
        return false;
    args.rval().set(result);
    return true;
}




static void numberFormat_finalize(FreeOp* fop, JSObject* obj);

static const uint32_t UNUMBER_FORMAT_SLOT = 0;
static const uint32_t NUMBER_FORMAT_SLOTS_COUNT = 1;

static const Class NumberFormatClass = {
    js_Object_str,
    JSCLASS_HAS_RESERVED_SLOTS(NUMBER_FORMAT_SLOTS_COUNT),
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    numberFormat_finalize
};

#if JS_HAS_TOSOURCE
static bool
numberFormat_toSource(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setString(cx->names().NumberFormat);
    return true;
}
#endif

static const JSFunctionSpec numberFormat_static_methods[] = {
    JS_SELF_HOSTED_FN("supportedLocalesOf", "Intl_NumberFormat_supportedLocalesOf", 1, 0),
    JS_FS_END
};

static const JSFunctionSpec numberFormat_methods[] = {
    JS_SELF_HOSTED_FN("resolvedOptions", "Intl_NumberFormat_resolvedOptions", 0, 0),
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str, numberFormat_toSource, 0, 0),
#endif
    JS_FS_END
};





static bool
NumberFormat(JSContext* cx, CallArgs args, bool construct)
{
    RootedObject obj(cx);

    if (!construct) {
        
        JSObject* intl = cx->global()->getOrCreateIntlObject(cx);
        if (!intl)
            return false;
        RootedValue self(cx, args.thisv());
        if (!self.isUndefined() && (!self.isObject() || self.toObject() != *intl)) {
            
            obj = ToObject(cx, self);
            if (!obj)
                return false;

            
            bool extensible;
            if (!IsExtensible(cx, obj, &extensible))
                return false;
            if (!extensible)
                return Throw(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE);
        } else {
            
            construct = true;
        }
    }
    if (construct) {
        
        RootedObject proto(cx, cx->global()->getOrCreateNumberFormatPrototype(cx));
        if (!proto)
            return false;
        obj = NewObjectWithGivenProto(cx, &NumberFormatClass, proto);
        if (!obj)
            return false;

        obj->as<NativeObject>().setReservedSlot(UNUMBER_FORMAT_SLOT, PrivateValue(nullptr));
    }

    
    RootedValue locales(cx, args.length() > 0 ? args[0] : UndefinedValue());
    RootedValue options(cx, args.length() > 1 ? args[1] : UndefinedValue());

    
    if (!IntlInitialize(cx, obj, cx->names().InitializeNumberFormat, locales, options))
        return false;

    
    args.rval().setObject(*obj);
    return true;
}

static bool
NumberFormat(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return NumberFormat(cx, args, args.isConstructing());
}

bool
js::intl_NumberFormat(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    
    
    
    return NumberFormat(cx, args, true);
}

static void
numberFormat_finalize(FreeOp* fop, JSObject* obj)
{
    
    
    
    
    const Value& slot = obj->as<NativeObject>().getReservedSlot(UNUMBER_FORMAT_SLOT);
    if (!slot.isUndefined()) {
        if (UNumberFormat* nf = static_cast<UNumberFormat*>(slot.toPrivate()))
            unum_close(nf);
    }
}

static JSObject*
InitNumberFormatClass(JSContext* cx, HandleObject Intl, Handle<GlobalObject*> global)
{
    RootedFunction ctor(cx, global->createConstructor(cx, &NumberFormat, cx->names().NumberFormat, 0));
    if (!ctor)
        return nullptr;

    RootedObject proto(cx, global->as<GlobalObject>().getOrCreateNumberFormatPrototype(cx));
    if (!proto)
        return nullptr;
    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return nullptr;

    
    if (!JS_DefineFunctions(cx, ctor, numberFormat_static_methods))
        return nullptr;

    
    if (!JS_DefineFunctions(cx, proto, numberFormat_methods))
        return nullptr;

    




    RootedValue getter(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), cx->names().NumberFormatFormatGet, &getter))
        return nullptr;
    if (!DefineProperty(cx, proto, cx->names().format, UndefinedHandleValue,
                        JS_DATA_TO_FUNC_PTR(JSGetterOp, &getter.toObject()),
                        nullptr, JSPROP_GETTER | JSPROP_SHARED))
    {
        return nullptr;
    }

    RootedValue options(cx);
    if (!CreateDefaultOptions(cx, &options))
        return nullptr;

    
    if (!IntlInitialize(cx, proto, cx->names().InitializeNumberFormat, UndefinedHandleValue, options))
        return nullptr;

    
    RootedValue ctorValue(cx, ObjectValue(*ctor));
    if (!DefineProperty(cx, Intl, cx->names().NumberFormat, ctorValue, nullptr, nullptr, 0))
        return nullptr;

    return ctor;
}

bool
GlobalObject::initNumberFormatProto(JSContext* cx, Handle<GlobalObject*> global)
{
    RootedNativeObject proto(cx, global->createBlankPrototype(cx, &NumberFormatClass));
    if (!proto)
        return false;
    proto->setReservedSlot(UNUMBER_FORMAT_SLOT, PrivateValue(nullptr));
    global->setReservedSlot(NUMBER_FORMAT_PROTO, ObjectValue(*proto));
    return true;
}

bool
js::intl_NumberFormat_availableLocales(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 0);

    RootedValue result(cx);
    if (!intl_availableLocales(cx, unum_countAvailable, unum_getAvailable, &result))
        return false;
    args.rval().set(result);
    return true;
}

bool
js::intl_numberingSystem(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isString());

    JSAutoByteString locale(cx, args[0].toString());
    if (!locale)
        return false;

    
    
    Locale ulocale(locale.ptr());
    UErrorCode status = U_ZERO_ERROR;
    NumberingSystem* numbers = NumberingSystem::createInstance(ulocale, status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }
    const char* name = numbers->getName();
    RootedString jsname(cx, JS_NewStringCopyZ(cx, name));
    delete numbers;
    if (!jsname)
        return false;
    args.rval().setString(jsname);
    return true;
}





static UNumberFormat*
NewUNumberFormat(JSContext* cx, HandleObject numberFormat)
{
    RootedValue value(cx);

    RootedObject internals(cx);
    if (!GetInternals(cx, numberFormat, &internals))
       return nullptr;

    if (!GetProperty(cx, internals, internals, cx->names().locale, &value))
        return nullptr;
    JSAutoByteString locale(cx, value.toString());
    if (!locale)
        return nullptr;

    
    UNumberFormatStyle uStyle = UNUM_DECIMAL;
    const UChar* uCurrency = nullptr;
    uint32_t uMinimumIntegerDigits = 1;
    uint32_t uMinimumFractionDigits = 0;
    uint32_t uMaximumFractionDigits = 3;
    int32_t uMinimumSignificantDigits = -1;
    int32_t uMaximumSignificantDigits = -1;
    bool uUseGrouping = true;

    
    RootedString currency(cx);
    AutoStableStringChars stableChars(cx);

    
    

    if (!GetProperty(cx, internals, internals, cx->names().style, &value))
        return nullptr;
    JSAutoByteString style(cx, value.toString());
    if (!style)
        return nullptr;

    if (equal(style, "currency")) {
        if (!GetProperty(cx, internals, internals, cx->names().currency, &value))
            return nullptr;
        currency = value.toString();
        MOZ_ASSERT(currency->length() == 3, "IsWellFormedCurrencyCode permits only length-3 strings");
        if (!currency->ensureFlat(cx) || !stableChars.initTwoByte(cx, currency))
            return nullptr;
        
        uCurrency = Char16ToUChar(stableChars.twoByteRange().start().get());
        if (!uCurrency)
            return nullptr;

        if (!GetProperty(cx, internals, internals, cx->names().currencyDisplay, &value))
            return nullptr;
        JSAutoByteString currencyDisplay(cx, value.toString());
        if (!currencyDisplay)
            return nullptr;
        if (equal(currencyDisplay, "code")) {
            uStyle = UNUM_CURRENCY_ISO;
        } else if (equal(currencyDisplay, "symbol")) {
            uStyle = UNUM_CURRENCY;
        } else {
            MOZ_ASSERT(equal(currencyDisplay, "name"));
            uStyle = UNUM_CURRENCY_PLURAL;
        }
    } else if (equal(style, "percent")) {
        uStyle = UNUM_PERCENT;
    } else {
        MOZ_ASSERT(equal(style, "decimal"));
        uStyle = UNUM_DECIMAL;
    }

    RootedId id(cx, NameToId(cx->names().minimumSignificantDigits));
    bool hasP;
    if (!HasProperty(cx, internals, id, &hasP))
        return nullptr;
    if (hasP) {
        if (!GetProperty(cx, internals, internals, cx->names().minimumSignificantDigits,
                         &value))
        {
            return nullptr;
        }
        uMinimumSignificantDigits = int32_t(value.toNumber());
        if (!GetProperty(cx, internals, internals, cx->names().maximumSignificantDigits,
                         &value))
        {
            return nullptr;
        }
        uMaximumSignificantDigits = int32_t(value.toNumber());
    } else {
        if (!GetProperty(cx, internals, internals, cx->names().minimumIntegerDigits,
                         &value))
        {
            return nullptr;
        }
        uMinimumIntegerDigits = int32_t(value.toNumber());
        if (!GetProperty(cx, internals, internals, cx->names().minimumFractionDigits,
                         &value))
        {
            return nullptr;
        }
        uMinimumFractionDigits = int32_t(value.toNumber());
        if (!GetProperty(cx, internals, internals, cx->names().maximumFractionDigits,
                         &value))
        {
            return nullptr;
        }
        uMaximumFractionDigits = int32_t(value.toNumber());
    }

    if (!GetProperty(cx, internals, internals, cx->names().useGrouping, &value))
        return nullptr;
    uUseGrouping = value.toBoolean();

    UErrorCode status = U_ZERO_ERROR;
    UNumberFormat* nf = unum_open(uStyle, nullptr, 0, icuLocale(locale.ptr()), nullptr, &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return nullptr;
    }
    ScopedICUObject<UNumberFormat> toClose(nf, unum_close);

    if (uCurrency) {
        unum_setTextAttribute(nf, UNUM_CURRENCY_CODE, uCurrency, 3, &status);
        if (U_FAILURE(status)) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
            return nullptr;
        }
    }
    if (uMinimumSignificantDigits != -1) {
        unum_setAttribute(nf, UNUM_SIGNIFICANT_DIGITS_USED, true);
        unum_setAttribute(nf, UNUM_MIN_SIGNIFICANT_DIGITS, uMinimumSignificantDigits);
        unum_setAttribute(nf, UNUM_MAX_SIGNIFICANT_DIGITS, uMaximumSignificantDigits);
    } else {
        unum_setAttribute(nf, UNUM_MIN_INTEGER_DIGITS, uMinimumIntegerDigits);
        unum_setAttribute(nf, UNUM_MIN_FRACTION_DIGITS, uMinimumFractionDigits);
        unum_setAttribute(nf, UNUM_MAX_FRACTION_DIGITS, uMaximumFractionDigits);
    }
    unum_setAttribute(nf, UNUM_GROUPING_USED, uUseGrouping);
    unum_setAttribute(nf, UNUM_ROUNDING_MODE, UNUM_ROUND_HALFUP);

    return toClose.forget();
}

static bool
intl_FormatNumber(JSContext* cx, UNumberFormat* nf, double x, MutableHandleValue result)
{
    
    if (IsNegativeZero(x))
        x = 0.0;

    Vector<char16_t, INITIAL_CHAR_BUFFER_SIZE> chars(cx);
    if (!chars.resize(INITIAL_CHAR_BUFFER_SIZE))
        return false;
    UErrorCode status = U_ZERO_ERROR;
    int size = unum_formatDouble(nf, x, Char16ToUChar(chars.begin()), INITIAL_CHAR_BUFFER_SIZE,
                                 nullptr, &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        if (!chars.resize(size))
            return false;
        status = U_ZERO_ERROR;
        unum_formatDouble(nf, x, Char16ToUChar(chars.begin()), size, nullptr, &status);
    }
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }

    JSString* str = NewStringCopyN<CanGC>(cx, chars.begin(), size);
    if (!str)
        return false;

    result.setString(str);
    return true;
}

bool
js::intl_FormatNumber(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[1].isNumber());

    RootedObject numberFormat(cx, &args[0].toObject());

    
    bool isNumberFormatInstance = numberFormat->getClass() == &NumberFormatClass;
    UNumberFormat* nf;
    if (isNumberFormatInstance) {
        nf = static_cast<UNumberFormat*>(numberFormat->as<NativeObject>().getReservedSlot(UNUMBER_FORMAT_SLOT).toPrivate());
        if (!nf) {
            nf = NewUNumberFormat(cx, numberFormat);
            if (!nf)
                return false;
            numberFormat->as<NativeObject>().setReservedSlot(UNUMBER_FORMAT_SLOT, PrivateValue(nf));
        }
    } else {
        
        
        
        
        nf = NewUNumberFormat(cx, numberFormat);
        if (!nf)
            return false;
    }

    
    RootedValue result(cx);
    bool success = intl_FormatNumber(cx, nf, args[1].toNumber(), &result);

    if (!isNumberFormatInstance)
        unum_close(nf);
    if (!success)
        return false;
    args.rval().set(result);
    return true;
}




static void dateTimeFormat_finalize(FreeOp* fop, JSObject* obj);

static const uint32_t UDATE_FORMAT_SLOT = 0;
static const uint32_t DATE_TIME_FORMAT_SLOTS_COUNT = 1;

static const Class DateTimeFormatClass = {
    js_Object_str,
    JSCLASS_HAS_RESERVED_SLOTS(DATE_TIME_FORMAT_SLOTS_COUNT),
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    dateTimeFormat_finalize
};

#if JS_HAS_TOSOURCE
static bool
dateTimeFormat_toSource(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setString(cx->names().DateTimeFormat);
    return true;
}
#endif

static const JSFunctionSpec dateTimeFormat_static_methods[] = {
    JS_SELF_HOSTED_FN("supportedLocalesOf", "Intl_DateTimeFormat_supportedLocalesOf", 1, 0),
    JS_FS_END
};

static const JSFunctionSpec dateTimeFormat_methods[] = {
    JS_SELF_HOSTED_FN("resolvedOptions", "Intl_DateTimeFormat_resolvedOptions", 0, 0),
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str, dateTimeFormat_toSource, 0, 0),
#endif
    JS_FS_END
};





static bool
DateTimeFormat(JSContext* cx, CallArgs args, bool construct)
{
    RootedObject obj(cx);

    if (!construct) {
        
        JSObject* intl = cx->global()->getOrCreateIntlObject(cx);
        if (!intl)
            return false;
        RootedValue self(cx, args.thisv());
        if (!self.isUndefined() && (!self.isObject() || self.toObject() != *intl)) {
            
            obj = ToObject(cx, self);
            if (!obj)
                return false;

            
            bool extensible;
            if (!IsExtensible(cx, obj, &extensible))
                return false;
            if (!extensible)
                return Throw(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE);
        } else {
            
            construct = true;
        }
    }
    if (construct) {
        
        RootedObject proto(cx, cx->global()->getOrCreateDateTimeFormatPrototype(cx));
        if (!proto)
            return false;
        obj = NewObjectWithGivenProto(cx, &DateTimeFormatClass, proto);
        if (!obj)
            return false;

        obj->as<NativeObject>().setReservedSlot(UDATE_FORMAT_SLOT, PrivateValue(nullptr));
    }

    
    RootedValue locales(cx, args.length() > 0 ? args[0] : UndefinedValue());
    RootedValue options(cx, args.length() > 1 ? args[1] : UndefinedValue());

    
    if (!IntlInitialize(cx, obj, cx->names().InitializeDateTimeFormat, locales, options))
        return false;

    
    args.rval().setObject(*obj);
    return true;
}

static bool
DateTimeFormat(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return DateTimeFormat(cx, args, args.isConstructing());
}

bool
js::intl_DateTimeFormat(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    
    
    
    return DateTimeFormat(cx, args, true);
}

static void
dateTimeFormat_finalize(FreeOp* fop, JSObject* obj)
{
    
    
    
    
    const Value& slot = obj->as<NativeObject>().getReservedSlot(UDATE_FORMAT_SLOT);
    if (!slot.isUndefined()) {
        if (UDateFormat* df = static_cast<UDateFormat*>(slot.toPrivate()))
            udat_close(df);
    }
}

static JSObject*
InitDateTimeFormatClass(JSContext* cx, HandleObject Intl, Handle<GlobalObject*> global)
{
    RootedFunction ctor(cx, global->createConstructor(cx, &DateTimeFormat, cx->names().DateTimeFormat, 0));
    if (!ctor)
        return nullptr;

    RootedObject proto(cx, global->as<GlobalObject>().getOrCreateDateTimeFormatPrototype(cx));
    if (!proto)
        return nullptr;
    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return nullptr;

    
    if (!JS_DefineFunctions(cx, ctor, dateTimeFormat_static_methods))
        return nullptr;

    
    if (!JS_DefineFunctions(cx, proto, dateTimeFormat_methods))
        return nullptr;

    




    RootedValue getter(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), cx->names().DateTimeFormatFormatGet, &getter))
        return nullptr;
    if (!DefineProperty(cx, proto, cx->names().format, UndefinedHandleValue,
                        JS_DATA_TO_FUNC_PTR(JSGetterOp, &getter.toObject()),
                        nullptr, JSPROP_GETTER | JSPROP_SHARED))
    {
        return nullptr;
    }

    RootedValue options(cx);
    if (!CreateDefaultOptions(cx, &options))
        return nullptr;

    
    if (!IntlInitialize(cx, proto, cx->names().InitializeDateTimeFormat, UndefinedHandleValue, options))
        return nullptr;

    
    RootedValue ctorValue(cx, ObjectValue(*ctor));
    if (!DefineProperty(cx, Intl, cx->names().DateTimeFormat, ctorValue, nullptr, nullptr, 0))
        return nullptr;

    return ctor;
}

bool
GlobalObject::initDateTimeFormatProto(JSContext* cx, Handle<GlobalObject*> global)
{
    RootedNativeObject proto(cx, global->createBlankPrototype(cx, &DateTimeFormatClass));
    if (!proto)
        return false;
    proto->setReservedSlot(UDATE_FORMAT_SLOT, PrivateValue(nullptr));
    global->setReservedSlot(DATE_TIME_FORMAT_PROTO, ObjectValue(*proto));
    return true;
}

bool
js::intl_DateTimeFormat_availableLocales(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 0);

    RootedValue result(cx);
    if (!intl_availableLocales(cx, udat_countAvailable, udat_getAvailable, &result))
        return false;
    args.rval().set(result);
    return true;
}



static const char*
bcp47CalendarName(const char* icuName)
{
    if (equal(icuName, "ethiopic-amete-alem"))
        return "ethioaa";
    if (equal(icuName, "gregorian"))
        return "gregory";
    if (equal(icuName, "islamic-civil"))
        return "islamicc";
    return icuName;
}

bool
js::intl_availableCalendars(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isString());

    JSAutoByteString locale(cx, args[0].toString());
    if (!locale)
        return false;

    RootedObject calendars(cx, NewDenseEmptyArray(cx));
    if (!calendars)
        return false;
    uint32_t index = 0;

    
    UErrorCode status = U_ZERO_ERROR;
    UCalendar* cal = ucal_open(nullptr, 0, locale.ptr(), UCAL_DEFAULT, &status);
    const char* calendar = ucal_getType(cal, &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }
    ucal_close(cal);
    RootedString jscalendar(cx, JS_NewStringCopyZ(cx, bcp47CalendarName(calendar)));
    if (!jscalendar)
        return false;
    RootedValue element(cx, StringValue(jscalendar));
    if (!DefineElement(cx, calendars, index++, element))
        return false;

    
    UEnumeration* values = ucal_getKeywordValuesForLocale("ca", locale.ptr(), false, &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }
    ScopedICUObject<UEnumeration> toClose(values, uenum_close);

    uint32_t count = uenum_count(values, &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }

    for (; count > 0; count--) {
        calendar = uenum_next(values, nullptr, &status);
        if (U_FAILURE(status)) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
            return false;
        }

        jscalendar = JS_NewStringCopyZ(cx, bcp47CalendarName(calendar));
        if (!jscalendar)
            return false;
        element = StringValue(jscalendar);
        if (!DefineElement(cx, calendars, index++, element))
            return false;
    }

    args.rval().setObject(*calendars);
    return true;
}

bool
js::intl_patternForSkeleton(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    MOZ_ASSERT(args[0].isString());
    MOZ_ASSERT(args[1].isString());

    JSAutoByteString locale(cx, args[0].toString());
    if (!locale)
        return false;

    JSFlatString* skeletonFlat = args[1].toString()->ensureFlat(cx);
    if (!skeletonFlat)
        return false;

    AutoStableStringChars stableChars(cx);
    if (!stableChars.initTwoByte(cx, skeletonFlat))
        return false;

    mozilla::Range<const char16_t> skeletonChars = stableChars.twoByteRange();
    uint32_t skeletonLen = u_strlen(Char16ToUChar(skeletonChars.start().get()));

    UErrorCode status = U_ZERO_ERROR;
    UDateTimePatternGenerator* gen = udatpg_open(icuLocale(locale.ptr()), &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }
    ScopedICUObject<UDateTimePatternGenerator> toClose(gen, udatpg_close);

    int32_t size = udatpg_getBestPattern(gen, Char16ToUChar(skeletonChars.start().get()),
                                         skeletonLen, nullptr, 0, &status);
    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }
    ScopedJSFreePtr<UChar> pattern(cx->pod_malloc<UChar>(size + 1));
    if (!pattern)
        return false;
    pattern[size] = '\0';
    status = U_ZERO_ERROR;
    udatpg_getBestPattern(gen, Char16ToUChar(skeletonChars.start().get()),
                          skeletonLen, pattern, size, &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }

    RootedString str(cx, JS_NewUCStringCopyZ(cx, reinterpret_cast<char16_t*>(pattern.get())));
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}





static UDateFormat*
NewUDateFormat(JSContext* cx, HandleObject dateTimeFormat)
{
    RootedValue value(cx);

    RootedObject internals(cx);
    if (!GetInternals(cx, dateTimeFormat, &internals))
       return nullptr;

    if (!GetProperty(cx, internals, internals, cx->names().locale, &value))
        return nullptr;
    JSAutoByteString locale(cx, value.toString());
    if (!locale)
        return nullptr;

    
    const UChar* uTimeZone = nullptr;
    uint32_t uTimeZoneLength = 0;
    const UChar* uPattern = nullptr;
    uint32_t uPatternLength = 0;

    
    
    

    RootedId id(cx, NameToId(cx->names().timeZone));
    bool hasP;
    if (!HasProperty(cx, internals, id, &hasP))
        return nullptr;

    AutoStableStringChars timeZoneChars(cx);
    if (hasP) {
        if (!GetProperty(cx, internals, internals, cx->names().timeZone, &value))
            return nullptr;
        if (!value.isUndefined()) {
            JSFlatString* flat = value.toString()->ensureFlat(cx);
            if (!flat || !timeZoneChars.initTwoByte(cx, flat))
                return nullptr;
            uTimeZone = Char16ToUChar(timeZoneChars.twoByteRange().start().get());
            if (!uTimeZone)
                return nullptr;
            uTimeZoneLength = u_strlen(uTimeZone);
        }
    }
    if (!GetProperty(cx, internals, internals, cx->names().pattern, &value))
        return nullptr;

    AutoStableStringChars patternChars(cx);
    JSFlatString* flat = value.toString()->ensureFlat(cx);
    if (!flat || !patternChars.initTwoByte(cx, flat))
        return nullptr;

    uPattern = Char16ToUChar(patternChars.twoByteRange().start().get());
    if (!uPattern)
        return nullptr;
    uPatternLength = u_strlen(uPattern);

    UErrorCode status = U_ZERO_ERROR;

    
    
    UDateFormat* df =
        udat_open(UDAT_PATTERN, UDAT_PATTERN, icuLocale(locale.ptr()), uTimeZone, uTimeZoneLength,
                  uPattern, uPatternLength, &status);
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return nullptr;
    }

    
    
    UCalendar* cal = const_cast<UCalendar*>(udat_getCalendar(df));
    ucal_setGregorianChange(cal, StartOfTime, &status);

    

    return df;
}

static bool
intl_FormatDateTime(JSContext* cx, UDateFormat* df, double x, MutableHandleValue result)
{
    if (!IsFinite(x)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_DATE_NOT_FINITE);
        return false;
    }

    Vector<char16_t, INITIAL_CHAR_BUFFER_SIZE> chars(cx);
    if (!chars.resize(INITIAL_CHAR_BUFFER_SIZE))
        return false;
    UErrorCode status = U_ZERO_ERROR;
    int size = udat_format(df, x, Char16ToUChar(chars.begin()), INITIAL_CHAR_BUFFER_SIZE,
                           nullptr, &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        if (!chars.resize(size))
            return false;
        status = U_ZERO_ERROR;
        udat_format(df, x, Char16ToUChar(chars.begin()), size, nullptr, &status);
    }
    if (U_FAILURE(status)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INTERNAL_INTL_ERROR);
        return false;
    }

    JSString* str = NewStringCopyN<CanGC>(cx, chars.begin(), size);
    if (!str)
        return false;

    result.setString(str);
    return true;
}

bool
js::intl_FormatDateTime(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[1].isNumber());

    RootedObject dateTimeFormat(cx, &args[0].toObject());

    
    bool isDateTimeFormatInstance = dateTimeFormat->getClass() == &DateTimeFormatClass;
    UDateFormat* df;
    if (isDateTimeFormatInstance) {
        df = static_cast<UDateFormat*>(dateTimeFormat->as<NativeObject>().getReservedSlot(UDATE_FORMAT_SLOT).toPrivate());
        if (!df) {
            df = NewUDateFormat(cx, dateTimeFormat);
            if (!df)
                return false;
            dateTimeFormat->as<NativeObject>().setReservedSlot(UDATE_FORMAT_SLOT, PrivateValue(df));
        }
    } else {
        
        
        
        
        df = NewUDateFormat(cx, dateTimeFormat);
        if (!df)
            return false;
    }

    
    RootedValue result(cx);
    bool success = intl_FormatDateTime(cx, df, args[1].toNumber(), &result);

    if (!isDateTimeFormatInstance)
        udat_close(df);
    if (!success)
        return false;
    args.rval().set(result);
    return true;
}




const Class js::IntlClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Intl)
};

#if JS_HAS_TOSOURCE
static bool
intl_toSource(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setString(cx->names().Intl);
    return true;
}
#endif

static const JSFunctionSpec intl_static_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,  intl_toSource,        0, 0),
#endif
    JS_FS_END
};





JSObject*
js::InitIntlClass(JSContext* cx, HandleObject obj)
{
    MOZ_ASSERT(obj->is<GlobalObject>());
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    
    
    
    
    RootedObject Intl(cx, global->getOrCreateIntlObject(cx));
    if (!Intl)
        return nullptr;

    RootedValue IntlValue(cx, ObjectValue(*Intl));
    if (!DefineProperty(cx, global, cx->names().Intl, IntlValue, nullptr, nullptr, 0))
        return nullptr;

    if (!JS_DefineFunctions(cx, Intl, intl_static_methods))
        return nullptr;

    if (!InitCollatorClass(cx, Intl, global))
        return nullptr;
    if (!InitNumberFormatClass(cx, Intl, global))
        return nullptr;
    if (!InitDateTimeFormatClass(cx, Intl, global))
        return nullptr;

    global->setConstructor(JSProto_Intl, ObjectValue(*Intl));

    return Intl;
}

bool
GlobalObject::initIntlObject(JSContext* cx, Handle<GlobalObject*> global)
{
    RootedObject Intl(cx);
    RootedObject proto(cx, global->getOrCreateObjectPrototype(cx));
    Intl = NewObjectWithGivenProto(cx, &IntlClass, proto, SingletonObject);
    if (!Intl)
        return false;

    global->setConstructor(JSProto_Intl, ObjectValue(*Intl));
    return true;
}
