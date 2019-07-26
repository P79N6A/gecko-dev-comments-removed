







#include "unicode/utypes.h"

#if !UCONFIG_NO_SERVICE

#include "unicode/resbund.h"
#include "uresimp.h"
#include "cmemory.h"
#include "servloc.h"
#include "ustrfmt.h"
#include "uhash.h"
#include "charstr.h"
#include "ucln_cmn.h"
#include "uassert.h"

#define UNDERSCORE_CHAR ((UChar)0x005f)
#define AT_SIGN_CHAR    ((UChar)64)
#define PERIOD_CHAR     ((UChar)46)


U_NAMESPACE_BEGIN

LocaleKeyFactory::LocaleKeyFactory(int32_t coverage)
  : _name()
  , _coverage(coverage)
{
}

LocaleKeyFactory::LocaleKeyFactory(int32_t coverage, const UnicodeString& name)
  : _name(name)
  , _coverage(coverage)
{
}

LocaleKeyFactory::~LocaleKeyFactory() {
}

UObject*
LocaleKeyFactory::create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const {
    if (handlesKey(key, status)) {
        const LocaleKey& lkey = (const LocaleKey&)key;
        int32_t kind = lkey.kind();
        Locale loc;
        lkey.currentLocale(loc);

        return handleCreate(loc, kind, service, status);
    }
    return NULL;
}

UBool
LocaleKeyFactory::handlesKey(const ICUServiceKey& key, UErrorCode& status) const {
    const Hashtable* supported = getSupportedIDs(status);
    if (supported) {
        UnicodeString id;
        key.currentID(id);
        return supported->get(id) != NULL;
    }
    return FALSE;
}

void
LocaleKeyFactory::updateVisibleIDs(Hashtable& result, UErrorCode& status) const {
    const Hashtable* supported = getSupportedIDs(status);
    if (supported) {
        UBool visible = (_coverage & 0x1) == 0;

        const UHashElement* elem = NULL;
        int32_t pos = 0;
        while ((elem = supported->nextElement(pos)) != NULL) {
            const UnicodeString& id = *((const UnicodeString*)elem->key.pointer);
            if (!visible) {
                result.remove(id);
            } else {
                result.put(id, (void*)this, status); 
                if (U_FAILURE(status)) {
                    break;
                }
            }
        }
    }
}

UnicodeString&
LocaleKeyFactory::getDisplayName(const UnicodeString& id, const Locale& locale, UnicodeString& result) const {
    if ((_coverage & 0x1) == 0) {
        
        
        
            Locale loc;
            LocaleUtility::initLocaleFromName(id, loc);
            return loc.getDisplayName(locale, result);
        
    }
    result.setToBogus();
    return result;
}

UObject*
LocaleKeyFactory::handleCreate(const Locale& , 
                   int32_t , 
                   const ICUService* , 
                   UErrorCode& ) const {
    return NULL;
}







const Hashtable*
LocaleKeyFactory::getSupportedIDs(UErrorCode& ) const {
    return NULL;
}

#ifdef SERVICE_DEBUG
UnicodeString&
LocaleKeyFactory::debug(UnicodeString& result) const
{
    debugClass(result);
    result.append(", name: ");
    result.append(_name);
    result.append(", coverage: ");
    result.append(_coverage);
    return result;
}

UnicodeString&
LocaleKeyFactory::debugClass(UnicodeString& result) const
{
  return result.append("LocaleKeyFactory");
}
#endif

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(LocaleKeyFactory)

U_NAMESPACE_END


#endif


