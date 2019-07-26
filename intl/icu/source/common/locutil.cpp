





#include "unicode/utypes.h"

#if !UCONFIG_NO_SERVICE || !UCONFIG_NO_TRANSLITERATION

#include "unicode/resbund.h"
#include "cmemory.h"
#include "ustrfmt.h"
#include "locutil.h"
#include "charstr.h"
#include "ucln_cmn.h"
#include "uassert.h"
#include "umutex.h"


static icu::Hashtable * LocaleUtility_cache = NULL;

#define UNDERSCORE_CHAR ((UChar)0x005f)
#define AT_SIGN_CHAR    ((UChar)64)
#define PERIOD_CHAR     ((UChar)46)








U_CDECL_BEGIN
static UBool U_CALLCONV service_cleanup(void) {
    if (LocaleUtility_cache) {
        delete LocaleUtility_cache;
        LocaleUtility_cache = NULL;
    }
    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN

UnicodeString&
LocaleUtility::canonicalLocaleString(const UnicodeString* id, UnicodeString& result)
{
  if (id == NULL) {
    result.setToBogus();
  } else {
    
    
    
    
    

    
    result = *id;
    int32_t i = 0;
    int32_t end = result.indexOf(AT_SIGN_CHAR);
    int32_t n = result.indexOf(PERIOD_CHAR);
    if (n >= 0 && n < end) {
        end = n;
    }
    if (end < 0) {
        end = result.length();
    }
    n = result.indexOf(UNDERSCORE_CHAR);
    if (n < 0) {
      n = end;
    }
    for (; i < n; ++i) {
      UChar c = result.charAt(i);
      if (c >= 0x0041 && c <= 0x005a) {
        c += 0x20;
        result.setCharAt(i, c);
      }
    }
    for (n = end; i < n; ++i) {
      UChar c = result.charAt(i);
      if (c >= 0x0061 && c <= 0x007a) {
        c -= 0x20;
        result.setCharAt(i, c);
      }
    }
  }
  return result;

#if 0
    
    
    
    

    
    
    
    

    
    result.setToBogus();
    if (id != 0) {
        int32_t buflen = id->length() + 8; 
        char* buf = (char*) uprv_malloc(buflen);
        char* canon = (buf == 0) ? 0 : (char*) uprv_malloc(buflen);
        if (buf != 0 && canon != 0) {
            U_ASSERT(id->extract(0, INT32_MAX, buf, buflen) < buflen);
            UErrorCode ec = U_ZERO_ERROR;
            uloc_canonicalize(buf, canon, buflen, &ec);
            if (U_SUCCESS(ec)) {
                result = UnicodeString(canon);
            }
        }
        uprv_free(buf);
        uprv_free(canon);
    }
    return result;
#endif
}

Locale&
LocaleUtility::initLocaleFromName(const UnicodeString& id, Locale& result)
{
    enum { BUFLEN = 128 }; 

    if (id.isBogus() || id.length() >= BUFLEN) {
        result.setToBogus();
    } else {
        

















        char buffer[BUFLEN];
        int32_t prev, i;
        prev = 0;
        for(;;) {
            i = id.indexOf((UChar)0x40, prev);
            if(i < 0) {
                
                id.extract(prev, INT32_MAX, buffer + prev, BUFLEN - prev, US_INV);
                break; 
            } else {
                
                id.extract(prev, i - prev, buffer + prev, BUFLEN - prev, US_INV);
                
                buffer[i] = '@';
                prev = i + 1;
            }
        }
        result = Locale::createFromName(buffer);
    }
    return result;
}

UnicodeString&
LocaleUtility::initNameFromLocale(const Locale& locale, UnicodeString& result)
{
    if (locale.isBogus()) {
        result.setToBogus();
    } else {
        result.append(UnicodeString(locale.getName(), -1, US_INV));
    }
    return result;
}

const Hashtable*
LocaleUtility::getAvailableLocaleNames(const UnicodeString& bundleID)
{
    
    
    
    
    
    

    UErrorCode status = U_ZERO_ERROR;
    Hashtable* cache;
    umtx_lock(NULL);
    cache = LocaleUtility_cache;
    umtx_unlock(NULL);

    if (cache == NULL) {
        cache = new Hashtable(status);
        if (cache == NULL || U_FAILURE(status)) {
            return NULL; 
        }
        cache->setValueDeleter(uhash_deleteHashtable);
        Hashtable* h; 
        umtx_lock(NULL);
        h = LocaleUtility_cache;
        if (h == NULL) {
            LocaleUtility_cache = h = cache;
            cache = NULL;
            ucln_common_registerCleanup(UCLN_COMMON_SERVICE, service_cleanup);
        }
        umtx_unlock(NULL);
        if(cache != NULL) {
          delete cache;
        }
        cache = h;
    }

    U_ASSERT(cache != NULL);

    Hashtable* htp;
    umtx_lock(NULL);
    htp = (Hashtable*) cache->get(bundleID);
    umtx_unlock(NULL);

    if (htp == NULL) {
        htp = new Hashtable(status);
        if (htp && U_SUCCESS(status)) {
            CharString cbundleID;
            cbundleID.appendInvariantChars(bundleID, status);
            const char* path = cbundleID.isEmpty() ? NULL : cbundleID.data();
            UEnumeration *uenum = ures_openAvailableLocales(path, &status);
            for (;;) {
                const UChar* id = uenum_unext(uenum, NULL, &status);
                if (id == NULL) {
                    break;
                }
                htp->put(UnicodeString(id), (void*)htp, status);
            }
            uenum_close(uenum);
            if (U_FAILURE(status)) {
                delete htp;
                return NULL;
            }
            umtx_lock(NULL);
            cache->put(bundleID, (void*)htp, status);
            umtx_unlock(NULL);
        }
    }
    return htp;
}

UBool
LocaleUtility::isFallbackOf(const UnicodeString& root, const UnicodeString& child)
{
    return child.indexOf(root) == 0 &&
      (child.length() == root.length() ||
       child.charAt(root.length()) == UNDERSCORE_CHAR);
}

U_NAMESPACE_END


#endif


