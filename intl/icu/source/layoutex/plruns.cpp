





#include "layout/LETypes.h"
#include "layout/loengine.h"
#include "layout/plruns.h"

#include "unicode/locid.h"

#include "layout/LayoutEngine.h"
#include "layout/RunArrays.h"

U_NAMESPACE_USE

U_CAPI pl_fontRuns * U_EXPORT2
pl_openFontRuns(const le_font **fonts,
                const le_int32 *limits,
                le_int32 count)
{
    return (pl_fontRuns *) new FontRuns((const LEFontInstance **) fonts, limits, count);
}

U_CAPI pl_fontRuns * U_EXPORT2
pl_openEmptyFontRuns(le_int32 initialCapacity)
{
    return (pl_fontRuns *) new FontRuns(initialCapacity);
}

U_CAPI void U_EXPORT2
pl_closeFontRuns(pl_fontRuns *fontRuns)
{
    FontRuns *fr = (FontRuns *) fontRuns;

    delete fr;
}

U_CAPI le_int32 U_EXPORT2
pl_getFontRunCount(const pl_fontRuns *fontRuns)
{
    const FontRuns *fr = (const FontRuns *) fontRuns;

    if (fr == NULL) {
        return -1;
    }

    return fr->getCount();
}

U_CAPI void U_EXPORT2
pl_resetFontRuns(pl_fontRuns *fontRuns)
{
    FontRuns *fr = (FontRuns *) fontRuns;

    if (fr != NULL) {
        fr->reset();
    }
}

U_CAPI le_int32 U_EXPORT2
pl_getFontRunLastLimit(const pl_fontRuns *fontRuns)
{
    const FontRuns *fr = (const FontRuns *) fontRuns;

    if (fr == NULL) {
        return -1;
    }

    return fr->getLimit();
}

U_CAPI le_int32 U_EXPORT2
pl_getFontRunLimit(const pl_fontRuns *fontRuns,
                   le_int32 run)
{
    const FontRuns *fr = (const FontRuns *) fontRuns;

    if (fr == NULL) {
        return -1;
    }

    return fr->getLimit(run);
}

U_CAPI const le_font * U_EXPORT2
pl_getFontRunFont(const pl_fontRuns *fontRuns,
                    le_int32 run)
{
    const FontRuns *fr = (const FontRuns *) fontRuns;

    if (fr == NULL) {
        return NULL;
    }

    return (const le_font *) fr->getFont(run);
}

U_CAPI le_int32 U_EXPORT2
pl_addFontRun(pl_fontRuns *fontRuns,
              const le_font *font,
              le_int32 limit)
{
    FontRuns *fr = (FontRuns *) fontRuns;

    if (fr == NULL) {
        return -1;
    }

    return fr->add((const LEFontInstance *) font, limit);
}

U_CAPI pl_valueRuns * U_EXPORT2
pl_openValueRuns(const le_int32 *values,
                 const le_int32 *limits,
                 le_int32 count)
{
    return (pl_valueRuns *) new ValueRuns(values, limits, count);
}

U_CAPI pl_valueRuns * U_EXPORT2
pl_openEmptyValueRuns(le_int32 initialCapacity)
{
    return (pl_valueRuns *) new ValueRuns(initialCapacity);
}

U_CAPI void U_EXPORT2
pl_closeValueRuns(pl_valueRuns *valueRuns)
{
    ValueRuns *vr = (ValueRuns *) valueRuns;

    delete vr;
}

U_CAPI le_int32 U_EXPORT2
pl_getValueRunCount(const pl_valueRuns *valueRuns)
{
    const ValueRuns *vr = (const ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->getCount();
}

U_CAPI void U_EXPORT2
pl_resetValueRuns(pl_valueRuns *valueRuns)
{
    ValueRuns *vr = (ValueRuns *) valueRuns;

    if (vr != NULL) {
        vr->reset();
    }
}

U_CAPI le_int32 U_EXPORT2
pl_getValueRunLastLimit(const pl_valueRuns *valueRuns)
{
    const ValueRuns *vr = (const ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->getLimit();
}

U_CAPI le_int32 U_EXPORT2
pl_getValueRunLimit(const pl_valueRuns *valueRuns,
                    le_int32 run)
{
    const ValueRuns *vr = (const ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->getLimit(run);
}

U_CAPI le_int32 U_EXPORT2
pl_getValueRunValue(const pl_valueRuns *valueRuns,
                     le_int32 run)
{
    const ValueRuns *vr = (const ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->getValue(run);
}

U_CAPI le_int32 U_EXPORT2
pl_addValueRun(pl_valueRuns *valueRuns,
               le_int32 value,
               le_int32 limit)
{
    ValueRuns *vr = (ValueRuns *) valueRuns;

    if (vr == NULL) {
        return -1;
    }

    return vr->add(value, limit);
}

U_NAMESPACE_BEGIN
class ULocRuns : public LocaleRuns
{
public:
    














    ULocRuns(const char **locales, const le_int32 *limits, le_int32 count);

    










    ULocRuns(le_int32 initialCapacity);

    




    virtual ~ULocRuns();

    












    const char *getLocaleName(le_int32 run) const;

    





















    le_int32 add(const char *locale, le_int32 limit);

    




    static inline UClassID getStaticClassID();

    




    virtual inline UClassID getDynamicClassID() const;

protected:
    virtual void init(le_int32 capacity);
    virtual void grow(le_int32 capacity);

private:

    inline ULocRuns();
    inline ULocRuns(const ULocRuns &other);
    inline ULocRuns &operator=(const ULocRuns & ) { return *this; };
    const char **fLocaleNames;
};

inline ULocRuns::ULocRuns()
    : LocaleRuns(0), fLocaleNames(NULL)
{
    
}

inline ULocRuns::ULocRuns(const ULocRuns & )
    : LocaleRuns(0), fLocaleNames(NULL)
{
    
}

static const Locale **getLocales(const char **localeNames, le_int32 count)
{
    Locale **locales = LE_NEW_ARRAY(Locale *, count);

    for (int i = 0; i < count; i += 1) {
        locales[i] = new Locale(Locale::createFromName(localeNames[i]));
    }

    return (const Locale **) locales;
}

ULocRuns::ULocRuns(const char **locales, const le_int32 *limits, le_int32 count)
    : LocaleRuns(getLocales(locales, count), limits, count), fLocaleNames(locales)
{
    
}

ULocRuns::ULocRuns(le_int32 initialCapacity)
    : LocaleRuns(initialCapacity), fLocaleNames(NULL)
{
    if(initialCapacity > 0) {
        fLocaleNames = LE_NEW_ARRAY(const char *, initialCapacity);
    }
}

ULocRuns::~ULocRuns()
{
    le_int32 count = getCount();

    for(int i = 0; i < count; i += 1) {
        delete fLocales[i];
    }

    if (fClientArrays) {
        LE_DELETE_ARRAY(fLocales);
        fLocales = NULL;
    } else {
        LE_DELETE_ARRAY(fLocaleNames);
        fLocaleNames = NULL;
    }
}

void ULocRuns::init(le_int32 capacity)
{
    LocaleRuns::init(capacity);
    fLocaleNames = LE_NEW_ARRAY(const char *, capacity);
}

void ULocRuns::grow(le_int32 capacity)
{
    LocaleRuns::grow(capacity);
    fLocaleNames = (const char **) LE_GROW_ARRAY(fLocaleNames, capacity);
}

le_int32 ULocRuns::add(const char *locale, le_int32 limit)
{
    Locale *loc = new Locale(Locale::createFromName(locale));
    le_int32 index = LocaleRuns::add(loc, limit);

    if (index >= 0) {
        char **localeNames = (char **) fLocaleNames;

        localeNames[index] = (char *) locale;
    }

    return index;
}

const char *ULocRuns::getLocaleName(le_int32 run) const
{
    if (run < 0 || run >= getCount()) {
        return NULL;
    }

    return fLocaleNames[run];
}
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ULocRuns)
U_NAMESPACE_END

U_CAPI pl_localeRuns * U_EXPORT2
pl_openLocaleRuns(const char **locales,
                  const le_int32 *limits,
                  le_int32 count)
{
    return (pl_localeRuns *) new ULocRuns(locales, limits, count);
}

U_CAPI pl_localeRuns * U_EXPORT2
pl_openEmptyLocaleRuns(le_int32 initialCapacity)
{
    return (pl_localeRuns *) new ULocRuns(initialCapacity);
}

U_CAPI void U_EXPORT2
pl_closeLocaleRuns(pl_localeRuns *localeRuns)
{
    ULocRuns *lr = (ULocRuns *) localeRuns;

    delete lr;
}

U_CAPI le_int32 U_EXPORT2
pl_getLocaleRunCount(const pl_localeRuns *localeRuns)
{
    const ULocRuns *lr = (const ULocRuns *) localeRuns;

    if (lr == NULL) {
        return -1;
    }

    return lr->getCount();
}

U_CAPI void U_EXPORT2
pl_resetLocaleRuns(pl_localeRuns *localeRuns)
{
    ULocRuns *lr = (ULocRuns *) localeRuns;

    if (lr != NULL) {
        lr->reset();
    }
}

U_CAPI le_int32 U_EXPORT2
pl_getLocaleRunLastLimit(const pl_localeRuns *localeRuns)
{
    const ULocRuns *lr = (const ULocRuns *) localeRuns;

    if (lr == NULL) {
        return -1;
    }

    return lr->getLimit();
}

U_CAPI le_int32 U_EXPORT2
pl_getLocaleRunLimit(const pl_localeRuns *localeRuns,
                     le_int32 run)
{
    const ULocRuns *lr = (const ULocRuns *) localeRuns;

    if (lr == NULL) {
        return -1;
    }

    return lr->getLimit(run);
}

U_CAPI const char * U_EXPORT2
pl_getLocaleRunLocale(const pl_localeRuns *localeRuns,
                      le_int32 run)
{
    const ULocRuns *lr = (const ULocRuns *) localeRuns;

    if (lr == NULL) {
        return NULL;
    }

    return lr->getLocaleName(run);
}

U_CAPI le_int32 U_EXPORT2
pl_addLocaleRun(pl_localeRuns *localeRuns,
                const char *locale,
                le_int32 limit)
{
    ULocRuns *lr = (ULocRuns *) localeRuns;

    if (lr == NULL) {
        return -1;
    }

    return lr->add(locale, limit);
}
