










#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsinterp.h"
#include "jsobj.h"

#include "builtin/Intl.h"
#include "vm/GlobalObject.h"
#include "vm/Stack.h"

#include "jsobjinlines.h"

#include "unicode/utypes.h"

using namespace js;




#if !ENABLE_INTL_API











static int32_t
u_strlen(const UChar *s)
{
    MOZ_NOT_REACHED("u_strlen: Intl API disabled");
    return 0;
}

struct UEnumeration;

static int32_t
uenum_count(UEnumeration *en, UErrorCode *status)
{
    MOZ_NOT_REACHED("uenum_count: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return 0;
}

static const char *
uenum_next(UEnumeration *en, int32_t *resultLength, UErrorCode *status)
{
    MOZ_NOT_REACHED("uenum_next: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

static void
uenum_close(UEnumeration *en)
{
    MOZ_NOT_REACHED("uenum_close: Intl API disabled");
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
ucol_countAvailable(void)
{
    MOZ_NOT_REACHED("ucol_countAvailable: Intl API disabled");
    return 0;
}

static const char *
ucol_getAvailable(int32_t localeIndex)
{
    MOZ_NOT_REACHED("ucol_getAvailable: Intl API disabled");
    return NULL;
}

static UCollator *
ucol_open(const char *loc, UErrorCode *status)
{
    MOZ_NOT_REACHED("ucol_open: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

static void
ucol_setAttribute(UCollator *coll, UColAttribute attr, UColAttributeValue value, UErrorCode *status)
{
    MOZ_NOT_REACHED("ucol_setAttribute: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
}

static UCollationResult
ucol_strcoll(const UCollator *coll, const UChar *source, int32_t sourceLength,
             const UChar *target, int32_t targetLength)
{
    MOZ_NOT_REACHED("ucol_strcoll: Intl API disabled");
    return (UCollationResult) 0;
}

static void
ucol_close(UCollator *coll)
{
    MOZ_NOT_REACHED("ucol_close: Intl API disabled");
}

static UEnumeration *
ucol_getKeywordValuesForLocale(const char *key, const char *locale, UBool commonlyUsed,
                               UErrorCode *status)
{
    MOZ_NOT_REACHED("ucol_getKeywordValuesForLocale: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

struct UParseError;
struct UFieldPosition;
typedef void *UNumberFormat;

enum UNumberFormatStyle {
    UNUM_DECIMAL = 1,
    UNUM_CURRENCY,
    UNUM_PERCENT,
    UNUM_CURRENCY_ISO,
    UNUM_CURRENCY_PLURAL,
};

enum UNumberFormatAttribute {
  UNUM_GROUPING_USED,
  UNUM_MIN_INTEGER_DIGITS,
  UNUM_MAX_FRACTION_DIGITS,
  UNUM_MIN_FRACTION_DIGITS,
  UNUM_SIGNIFICANT_DIGITS_USED,
  UNUM_MIN_SIGNIFICANT_DIGITS,
  UNUM_MAX_SIGNIFICANT_DIGITS,
};

enum UNumberFormatTextAttribute {
  UNUM_CURRENCY_CODE,
};

static int32_t
unum_countAvailable(void)
{
    MOZ_NOT_REACHED("unum_countAvailable: Intl API disabled");
    return 0;
}

static const char *
unum_getAvailable(int32_t localeIndex)
{
    MOZ_NOT_REACHED("unum_getAvailable: Intl API disabled");
    return NULL;
}

static UNumberFormat *
unum_open(UNumberFormatStyle style, const UChar *pattern, int32_t patternLength,
          const char *locale, UParseError *parseErr, UErrorCode *status)
{
    MOZ_NOT_REACHED("unum_open: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

static void
unum_setAttribute(UNumberFormat *fmt, UNumberFormatAttribute  attr, int32_t newValue)
{
    MOZ_NOT_REACHED("unum_setAttribute: Intl API disabled");
}

static int32_t
unum_formatDouble(const UNumberFormat *fmt, double number, UChar *result,
                  int32_t resultLength, UFieldPosition *pos, UErrorCode *status)
{
    MOZ_NOT_REACHED("unum_formatDouble: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return 0;
}

static void
unum_close(UNumberFormat *fmt)
{
    MOZ_NOT_REACHED("unum_close: Intl API disabled");
}

static void
unum_setTextAttribute(UNumberFormat *fmt, UNumberFormatTextAttribute tag, const UChar *newValue,
                      int32_t newValueLength, UErrorCode *status)
{
    MOZ_NOT_REACHED("unum_setTextAttribute: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
}

class Locale {
  public:
    Locale(const char *language, const char *country = 0, const char *variant = 0,
           const char *keywordsAndValues = 0);
};

Locale::Locale(const char *language, const char *country, const char *variant,
               const char *keywordsAndValues)
{
    MOZ_NOT_REACHED("Locale::Locale: Intl API disabled");
}

class NumberingSystem {
  public:
    static NumberingSystem *createInstance(const Locale &inLocale, UErrorCode &status);
    const char *getName();
};

NumberingSystem *
NumberingSystem::createInstance(const Locale &inLocale, UErrorCode &status)
{
    MOZ_NOT_REACHED("NumberingSystem::createInstance: Intl API disabled");
    status = U_UNSUPPORTED_ERROR;
    return NULL;
}

const char *
NumberingSystem::getName()
{
    MOZ_NOT_REACHED("NumberingSystem::getName: Intl API disabled");
    return NULL;
}

typedef void *UCalendar;

enum UCalendarType {
  UCAL_TRADITIONAL,
  UCAL_DEFAULT = UCAL_TRADITIONAL,
  UCAL_GREGORIAN
};

static UCalendar *
ucal_open(const UChar *zoneID, int32_t len, const char *locale,
          UCalendarType type, UErrorCode *status)
{
    MOZ_NOT_REACHED("ucal_open: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

static const char *
ucal_getType(const UCalendar *cal, UErrorCode *status)
{
    MOZ_NOT_REACHED("ucal_getType: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

static UEnumeration *
ucal_getKeywordValuesForLocale(const char *key, const char *locale,
                               UBool commonlyUsed, UErrorCode *status)
{
    MOZ_NOT_REACHED("ucal_getKeywordValuesForLocale: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

static void
ucal_close(UCalendar *cal)
{
    MOZ_NOT_REACHED("ucal_close: Intl API disabled");
}

typedef void *UDateTimePatternGenerator;

static UDateTimePatternGenerator *
udatpg_open(const char *locale, UErrorCode *pErrorCode)
{
    MOZ_NOT_REACHED("udatpg_open: Intl API disabled");
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return NULL;
}

static int32_t
udatpg_getBestPattern(UDateTimePatternGenerator *dtpg, const UChar *skeleton,
                      int32_t length, UChar *bestPattern, int32_t capacity,
                      UErrorCode *pErrorCode)
{
    MOZ_NOT_REACHED("udatpg_getBestPattern: Intl API disabled");
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return 0;
}

static void
udatpg_close(UDateTimePatternGenerator *dtpg)
{
    MOZ_NOT_REACHED("udatpg_close: Intl API disabled");
}

typedef void *UCalendar;
typedef void *UDateFormat;

enum UDateFormatStyle {
    UDAT_PATTERN = -2,
    UDAT_IGNORE = UDAT_PATTERN
};

static int32_t
udat_countAvailable(void)
{
    MOZ_NOT_REACHED("udat_countAvailable: Intl API disabled");
    return 0;
}

static const char *
udat_getAvailable(int32_t localeIndex)
{
    MOZ_NOT_REACHED("udat_getAvailable: Intl API disabled");
    return NULL;
}

static UDateFormat *
udat_open(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const char *locale,
          const UChar *tzID, int32_t tzIDLength, const UChar *pattern,
          int32_t patternLength, UErrorCode *status)
{
    MOZ_NOT_REACHED("udat_open: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return NULL;
}

static const UCalendar *
udat_getCalendar(const UDateFormat *fmt)
{
    MOZ_NOT_REACHED("udat_getCalendar: Intl API disabled");
    return NULL;
}

static void
ucal_setGregorianChange(UCalendar *cal, UDate date, UErrorCode *pErrorCode)
{
    MOZ_NOT_REACHED("ucal_setGregorianChange: Intl API disabled");
    *pErrorCode = U_UNSUPPORTED_ERROR;
}

static int32_t
udat_format(const UDateFormat *format, UDate dateToFormat, UChar *result,
            int32_t resultLength, UFieldPosition *position, UErrorCode *status)
{
    MOZ_NOT_REACHED("udat_format: Intl API disabled");
    *status = U_UNSUPPORTED_ERROR;
    return 0;
}

static void
udat_close(UDateFormat *format)
{
    MOZ_NOT_REACHED("udat_close: Intl API disabled");
}

#endif




static bool
IntlInitialize(JSContext *cx, HandleObject obj, Handle<PropertyName*> initializer,
               HandleValue locales, HandleValue options)
{
    RootedValue initializerValue(cx);
    if (!cx->global()->getIntrinsicValue(cx, initializer, &initializerValue))
        return false;
    JS_ASSERT(initializerValue.isObject());
    JS_ASSERT(initializerValue.toObject().isFunction());

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, 3, &args))
        return false;

    args.setCallee(initializerValue);
    args.setThis(NullValue());
    args[0] = ObjectValue(*obj);
    args[1] = locales;
    args[2] = options;

    return Invoke(cx, args);
}



static Class CollatorClass = {
    js_Object_str,
    0,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

#if JS_HAS_TOSOURCE
static JSBool
collator_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    vp->setString(cx->names().Collator);
    return true;
}
#endif

static JSFunctionSpec collator_static_methods[] = {
    {"supportedLocalesOf", JSOP_NULLWRAPPER, 1, JSFunction::INTERPRETED, "Intl_Collator_supportedLocalesOf"},
    JS_FS_END
};

static JSFunctionSpec collator_methods[] = {
    {"resolvedOptions", JSOP_NULLWRAPPER, 0, JSFunction::INTERPRETED, "Intl_Collator_resolvedOptions"},
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str, collator_toSource, 0, 0),
#endif
    JS_FS_END
};





static JSBool
Collator(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject obj(cx);

    bool construct = IsConstructing(args);
    if (!construct) {
        
        JSObject *intl = cx->global()->getOrCreateIntlObject(cx);
        if (!intl)
            return false;
        RootedValue self(cx, args.thisv());
        if (!self.isUndefined() && (!self.isObject() || self.toObject() != *intl)) {
            
            obj = ToObject(cx, self);
            if (!obj)
                return false;
            
            if (!obj->isExtensible())
                return Throw(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE);
        } else {
            
            construct = true;
        }
    }
    if (construct) {
        
        RootedObject proto(cx, cx->global()->getOrCreateCollatorPrototype(cx));
        if (!proto)
            return false;
        obj = NewObjectWithGivenProto(cx, &CollatorClass, proto, cx->global());
        if (!obj)
            return false;
    }

    
    RootedValue locales(cx, args.length() > 0 ? args[0] : UndefinedValue());
    RootedValue options(cx, args.length() > 1 ? args[1] : UndefinedValue());
    
    if (!IntlInitialize(cx, obj, cx->names().InitializeCollator, locales, options))
        return false;

    
    args.rval().setObject(*obj);
    return true;
}

static JSObject *
InitCollatorClass(JSContext *cx, HandleObject Intl, Handle<GlobalObject*> global)
{
    RootedFunction ctor(cx, global->createConstructor(cx, &Collator, cx->names().Collator, 0));
    if (!ctor)
        return NULL;

    RootedObject proto(cx, global->asGlobal().getOrCreateCollatorPrototype(cx));
    if (!proto)
        return NULL;
    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return NULL;

    
    if (!JS_DefineFunctions(cx, ctor, collator_static_methods))
        return NULL;

    
    if (!JS_DefineFunctions(cx, proto, collator_methods))
        return NULL;

    




    RootedValue getter(cx);
    if (!cx->global()->getIntrinsicValue(cx, cx->names().CollatorCompareGet, &getter))
        return NULL;
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!JSObject::defineProperty(cx, proto, cx->names().compare, undefinedValue,
                                  JS_DATA_TO_FUNC_PTR(JSPropertyOp, &getter.toObject()),
                                  NULL, JSPROP_GETTER)) {
        return NULL;
    }

    
    RootedValue locales(cx, UndefinedValue());
    RootedValue options(cx, UndefinedValue());
    if (!IntlInitialize(cx, proto, cx->names().InitializeCollator, locales, options))
        return NULL;

    
    RootedValue ctorValue(cx, ObjectValue(*ctor));
    if (!JSObject::defineProperty(cx, Intl, cx->names().Collator, ctorValue,
                                  JS_PropertyStub, JS_StrictPropertyStub, 0)) {
        return NULL;
    }

    return ctor;
}

bool
GlobalObject::initCollatorProto(JSContext *cx, Handle<GlobalObject*> global)
{
    RootedObject proto(cx, global->createBlankPrototype(cx, &CollatorClass));
    if (!proto)
        return false;
    global->setReservedSlot(COLLATOR_PROTO, ObjectValue(*proto));
    return true;
}




static Class NumberFormatClass = {
    js_Object_str,
    0,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

#if JS_HAS_TOSOURCE
static JSBool
numberFormat_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    vp->setString(cx->names().NumberFormat);
    return true;
}
#endif

static JSFunctionSpec numberFormat_static_methods[] = {
    {"supportedLocalesOf", JSOP_NULLWRAPPER, 1, JSFunction::INTERPRETED, "Intl_NumberFormat_supportedLocalesOf"},
    JS_FS_END
};

static JSFunctionSpec numberFormat_methods[] = {
    {"resolvedOptions", JSOP_NULLWRAPPER, 0, JSFunction::INTERPRETED, "Intl_NumberFormat_resolvedOptions"},
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str, numberFormat_toSource, 0, 0),
#endif
    JS_FS_END
};





static JSBool
NumberFormat(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject obj(cx);

    bool construct = IsConstructing(args);
    if (!construct) {
        
        JSObject *intl = cx->global()->getOrCreateIntlObject(cx);
        if (!intl)
            return false;
        RootedValue self(cx, args.thisv());
        if (!self.isUndefined() && (!self.isObject() || self.toObject() != *intl)) {
            
            obj = ToObject(cx, self);
            if (!obj)
                return false;
            
            if (!obj->isExtensible())
                return Throw(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE);
        } else {
            
            construct = true;
        }
    }
    if (construct) {
        
        RootedObject proto(cx, cx->global()->getOrCreateNumberFormatPrototype(cx));
        if (!proto)
            return false;
        obj = NewObjectWithGivenProto(cx, &NumberFormatClass, proto, cx->global());
        if (!obj)
            return false;
    }

    
    RootedValue locales(cx, args.length() > 0 ? args[0] : UndefinedValue());
    RootedValue options(cx, args.length() > 1 ? args[1] : UndefinedValue());
    
    if (!IntlInitialize(cx, obj, cx->names().InitializeNumberFormat, locales, options))
        return false;

    
    args.rval().setObject(*obj);
    return true;
}

static JSObject *
InitNumberFormatClass(JSContext *cx, HandleObject Intl, Handle<GlobalObject*> global)
{
    RootedFunction ctor(cx, global->createConstructor(cx, &NumberFormat, cx->names().NumberFormat, 0));
    if (!ctor)
        return NULL;

    RootedObject proto(cx, global->asGlobal().getOrCreateNumberFormatPrototype(cx));
    if (!proto)
        return NULL;
    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return NULL;

    
    if (!JS_DefineFunctions(cx, ctor, numberFormat_static_methods))
        return NULL;

    
    if (!JS_DefineFunctions(cx, proto, numberFormat_methods))
        return NULL;

    




    RootedValue getter(cx);
    if (!cx->global()->getIntrinsicValue(cx, cx->names().NumberFormatFormatGet, &getter))
        return NULL;
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!JSObject::defineProperty(cx, proto, cx->names().format, undefinedValue,
                                  JS_DATA_TO_FUNC_PTR(JSPropertyOp, &getter.toObject()),
                                  NULL, JSPROP_GETTER)) {
        return NULL;
    }

    
    RootedValue locales(cx, UndefinedValue());
    RootedValue options(cx, UndefinedValue());
    if (!IntlInitialize(cx, proto, cx->names().InitializeNumberFormat, locales, options))
        return NULL;

    
    RootedValue ctorValue(cx, ObjectValue(*ctor));
    if (!JSObject::defineProperty(cx, Intl, cx->names().NumberFormat, ctorValue,
                                  JS_PropertyStub, JS_StrictPropertyStub, 0)) {
        return NULL;
    }

    return ctor;
}

bool
GlobalObject::initNumberFormatProto(JSContext *cx, Handle<GlobalObject*> global)
{
    RootedObject proto(cx, global->createBlankPrototype(cx, &NumberFormatClass));
    if (!proto)
        return false;
    global->setReservedSlot(NUMBER_FORMAT_PROTO, ObjectValue(*proto));
    return true;
}




static Class DateTimeFormatClass = {
    js_Object_str,
    0,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

#if JS_HAS_TOSOURCE
static JSBool
dateTimeFormat_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    vp->setString(cx->names().DateTimeFormat);
    return true;
}
#endif

static JSFunctionSpec dateTimeFormat_static_methods[] = {
    {"supportedLocalesOf", JSOP_NULLWRAPPER, 1, JSFunction::INTERPRETED, "Intl_DateTimeFormat_supportedLocalesOf"},
    JS_FS_END
};

static JSFunctionSpec dateTimeFormat_methods[] = {
    {"resolvedOptions", JSOP_NULLWRAPPER, 0, JSFunction::INTERPRETED, "Intl_DateTimeFormat_resolvedOptions"},
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str, dateTimeFormat_toSource, 0, 0),
#endif
    JS_FS_END
};





static JSBool
DateTimeFormat(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject obj(cx);

    bool construct = IsConstructing(args);
    if (!construct) {
        
        JSObject *intl = cx->global()->getOrCreateIntlObject(cx);
        if (!intl)
            return false;
        RootedValue self(cx, args.thisv());
        if (!self.isUndefined() && (!self.isObject() || self.toObject() != *intl)) {
            
            obj = ToObject(cx, self);
            if (!obj)
                return false;
            
            if (!obj->isExtensible())
                return Throw(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE);
        } else {
            
            construct = true;
        }
    }
    if (construct) {
        
        RootedObject proto(cx, cx->global()->getOrCreateDateTimeFormatPrototype(cx));
        if (!proto)
            return false;
        obj = NewObjectWithGivenProto(cx, &DateTimeFormatClass, proto, cx->global());
        if (!obj)
            return false;
    }

    
    RootedValue locales(cx, args.length() > 0 ? args[0] : UndefinedValue());
    RootedValue options(cx, args.length() > 1 ? args[1] : UndefinedValue());
    
    if (!IntlInitialize(cx, obj, cx->names().InitializeDateTimeFormat, locales, options))
        return false;

    
    args.rval().setObject(*obj);
    return true;
}

static JSObject *
InitDateTimeFormatClass(JSContext *cx, HandleObject Intl, Handle<GlobalObject*> global)
{
    RootedFunction ctor(cx, global->createConstructor(cx, &DateTimeFormat, cx->names().DateTimeFormat, 0));
    if (!ctor)
        return NULL;

    RootedObject proto(cx, global->asGlobal().getOrCreateDateTimeFormatPrototype(cx));
    if (!proto)
        return NULL;
    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return NULL;

    
    if (!JS_DefineFunctions(cx, ctor, dateTimeFormat_static_methods))
        return NULL;

    
    if (!JS_DefineFunctions(cx, proto, dateTimeFormat_methods))
        return NULL;

    




    RootedValue getter(cx);
    if (!cx->global()->getIntrinsicValue(cx, cx->names().DateTimeFormatFormatGet, &getter))
        return NULL;
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!JSObject::defineProperty(cx, proto, cx->names().format, undefinedValue,
                                  JS_DATA_TO_FUNC_PTR(JSPropertyOp, &getter.toObject()),
                                  NULL, JSPROP_GETTER)) {
        return NULL;
    }

    
    RootedValue locales(cx, UndefinedValue());
    RootedValue options(cx, UndefinedValue());
    if (!IntlInitialize(cx, proto, cx->names().InitializeDateTimeFormat, locales, options))
        return NULL;

    
    RootedValue ctorValue(cx, ObjectValue(*ctor));
    if (!JSObject::defineProperty(cx, Intl, cx->names().DateTimeFormat, ctorValue,
                                  JS_PropertyStub, JS_StrictPropertyStub, 0)) {
        return NULL;
    }

    return ctor;
}

bool
GlobalObject::initDateTimeFormatProto(JSContext *cx, Handle<GlobalObject*> global)
{
    RootedObject proto(cx, global->createBlankPrototype(cx, &DateTimeFormatClass));
    if (!proto)
        return false;
    global->setReservedSlot(DATE_TIME_FORMAT_PROTO, ObjectValue(*proto));
    return true;
}




Class js::IntlClass = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Intl),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

#if JS_HAS_TOSOURCE
static JSBool
intl_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    vp->setString(cx->names().Intl);
    return true;
}
#endif

static JSFunctionSpec intl_static_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,  intl_toSource,        0, 0),
#endif
    JS_FS_END
};





JSObject *
js_InitIntlClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isGlobal());
    Rooted<GlobalObject*> global(cx, &obj->asGlobal());

    
    
    
    
    RootedObject Intl(cx, global->getOrCreateIntlObject(cx));
    if (!Intl)
        return NULL;

    RootedValue IntlValue(cx, ObjectValue(*Intl));
    if (!JSObject::defineProperty(cx, global, cx->names().Intl, IntlValue,
                                  JS_PropertyStub, JS_StrictPropertyStub, 0)) {
        return NULL;
    }

    if (!JS_DefineFunctions(cx, Intl, intl_static_methods))
        return NULL;

    
    
    
    if (!cx->runtime->isSelfHostingGlobal(cx->global())) {
        if (!InitCollatorClass(cx, Intl, global))
            return NULL;
        if (!InitNumberFormatClass(cx, Intl, global))
            return NULL;
        if (!InitDateTimeFormatClass(cx, Intl, global))
            return NULL;
    }

    MarkStandardClassInitializedNoProto(global, &IntlClass);

    return Intl;
}

bool
GlobalObject::initIntlObject(JSContext *cx, Handle<GlobalObject*> global)
{
    RootedObject Intl(cx);
    Intl = NewObjectWithGivenProto(cx, &IntlClass, global->getOrCreateObjectPrototype(cx),
                                   global, SingletonObject);
    if (!Intl)
        return false;

    global->setReservedSlot(JSProto_Intl, ObjectValue(*Intl));
    return true;
}
