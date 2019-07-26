


















#include "unicode/utypes.h"
#include "unicode/brkiter.h"
#include "unicode/locid.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "putilimp.h"
#include "ulocimp.h"
#include "uresimp.h"
#include "ureslocs.h"
#include "ustr_imp.h"



U_NAMESPACE_BEGIN

UnicodeString&
Locale::getDisplayLanguage(UnicodeString& dispLang) const
{
    return this->getDisplayLanguage(getDefault(), dispLang);
}







UnicodeString&
Locale::getDisplayLanguage(const Locale &displayLocale,
                           UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayLanguage(fullName, displayLocale.fullName,
                                   buffer, result.getCapacity(),
                                   &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayLanguage(fullName, displayLocale.fullName,
                                       buffer, result.getCapacity(),
                                       &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

UnicodeString&
Locale::getDisplayScript(UnicodeString& dispScript) const
{
    return this->getDisplayScript(getDefault(), dispScript);
}

UnicodeString&
Locale::getDisplayScript(const Locale &displayLocale,
                          UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayScript(fullName, displayLocale.fullName,
                                  buffer, result.getCapacity(),
                                  &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayScript(fullName, displayLocale.fullName,
                                      buffer, result.getCapacity(),
                                      &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

UnicodeString&
Locale::getDisplayCountry(UnicodeString& dispCntry) const
{
    return this->getDisplayCountry(getDefault(), dispCntry);
}

UnicodeString&
Locale::getDisplayCountry(const Locale &displayLocale,
                          UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayCountry(fullName, displayLocale.fullName,
                                  buffer, result.getCapacity(),
                                  &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayCountry(fullName, displayLocale.fullName,
                                      buffer, result.getCapacity(),
                                      &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

UnicodeString&
Locale::getDisplayVariant(UnicodeString& dispVar) const
{
    return this->getDisplayVariant(getDefault(), dispVar);
}

UnicodeString&
Locale::getDisplayVariant(const Locale &displayLocale,
                          UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayVariant(fullName, displayLocale.fullName,
                                  buffer, result.getCapacity(),
                                  &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayVariant(fullName, displayLocale.fullName,
                                      buffer, result.getCapacity(),
                                      &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

UnicodeString&
Locale::getDisplayName( UnicodeString& name ) const
{
    return this->getDisplayName(getDefault(), name);
}

UnicodeString&
Locale::getDisplayName(const Locale &displayLocale,
                       UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayName(fullName, displayLocale.fullName,
                               buffer, result.getCapacity(),
                               &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayName(fullName, displayLocale.fullName,
                                   buffer, result.getCapacity(),
                                   &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

#if ! UCONFIG_NO_BREAK_ITERATION



UnicodeString& U_EXPORT2
BreakIterator::getDisplayName(const Locale& objectLocale,
                             UnicodeString& name)
{
    return objectLocale.getDisplayName(name);
}



UnicodeString& U_EXPORT2
BreakIterator::getDisplayName(const Locale& objectLocale,
                             const Locale& displayLocale,
                             UnicodeString& name)
{
    return objectLocale.getDisplayName(displayLocale, name);
}

#endif


U_NAMESPACE_END



U_NAMESPACE_USE





static const char _kLanguages[]       = "Languages";
static const char _kScripts[]         = "Scripts";
static const char _kScriptsStandAlone[] = "Scripts%stand-alone";
static const char _kCountries[]       = "Countries";
static const char _kVariants[]        = "Variants";
static const char _kKeys[]            = "Keys";
static const char _kTypes[]           = "Types";

static const char _kCurrency[]        = "currency";
static const char _kCurrencies[]      = "Currencies";
static const char _kLocaleDisplayPattern[] = "localeDisplayPattern";
static const char _kPattern[]         = "pattern";
static const char _kSeparator[]       = "separator";



static int32_t
_getStringOrCopyKey(const char *path, const char *locale,
                    const char *tableKey, 
                    const char* subTableKey,
                    const char *itemKey,
                    const char *substitute,
                    UChar *dest, int32_t destCapacity,
                    UErrorCode *pErrorCode) {
    const UChar *s = NULL;
    int32_t length = 0;

    if(itemKey==NULL) {
        
        UResourceBundle *rb;

        rb=ures_open(path, locale, pErrorCode);

        if(U_SUCCESS(*pErrorCode)) {
            s=ures_getStringByKey(rb, tableKey, &length, pErrorCode);
            
            ures_close(rb);
        }
    } else {
        
        if (!uprv_strncmp(tableKey, "Languages", 9) && uprv_strtol(itemKey, NULL, 10)) {
            *pErrorCode = U_MISSING_RESOURCE_ERROR;
        } else {
            
            s=uloc_getTableStringWithFallback(path, locale,
                                               tableKey, 
                                               subTableKey,
                                               itemKey,
                                               &length,
                                               pErrorCode);
        }
    }

    if(U_SUCCESS(*pErrorCode)) {
        int32_t copyLength=uprv_min(length, destCapacity);
        if(copyLength>0 && s != NULL) {
            u_memcpy(dest, s, copyLength);
        }
    } else {
        
        length=(int32_t)uprv_strlen(substitute);
        u_charsToUChars(substitute, dest, uprv_min(length, destCapacity));
        *pErrorCode=U_USING_DEFAULT_WARNING;
    }

    return u_terminateUChars(dest, destCapacity, length, pErrorCode);
}

typedef  int32_t U_CALLCONV UDisplayNameGetter(const char *, char *, int32_t, UErrorCode *);

static int32_t
_getDisplayNameForComponent(const char *locale,
                            const char *displayLocale,
                            UChar *dest, int32_t destCapacity,
                            UDisplayNameGetter *getter,
                            const char *tag,
                            UErrorCode *pErrorCode) {
    char localeBuffer[ULOC_FULLNAME_CAPACITY*4];
    int32_t length;
    UErrorCode localStatus;
    const char* root = NULL;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    localStatus = U_ZERO_ERROR;
    length=(*getter)(locale, localeBuffer, sizeof(localeBuffer), &localStatus);
    if(U_FAILURE(localStatus) || localStatus==U_STRING_NOT_TERMINATED_WARNING) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(length==0) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    }

    root = tag == _kCountries ? U_ICUDATA_REGION : U_ICUDATA_LANG;

    return _getStringOrCopyKey(root, displayLocale,
                               tag, NULL, localeBuffer,
                               localeBuffer,
                               dest, destCapacity,
                               pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayLanguage(const char *locale,
                        const char *displayLocale,
                        UChar *dest, int32_t destCapacity,
                        UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getLanguage, _kLanguages, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayScript(const char* locale,
                      const char* displayLocale,
                      UChar *dest, int32_t destCapacity,
                      UErrorCode *pErrorCode)
{
	UErrorCode err = U_ZERO_ERROR;
	int32_t res = _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getScript, _kScriptsStandAlone, &err);
	
	if ( err == U_USING_DEFAULT_WARNING ) {
        return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                    uloc_getScript, _kScripts, pErrorCode);
	} else {
		*pErrorCode = err;
		return res;
	}
}

U_INTERNAL int32_t U_EXPORT2
uloc_getDisplayScriptInContext(const char* locale,
                      const char* displayLocale,
                      UChar *dest, int32_t destCapacity,
                      UErrorCode *pErrorCode)
{
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                    uloc_getScript, _kScripts, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayCountry(const char *locale,
                       const char *displayLocale,
                       UChar *dest, int32_t destCapacity,
                       UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getCountry, _kCountries, pErrorCode);
}






U_CAPI int32_t U_EXPORT2
uloc_getDisplayVariant(const char *locale,
                       const char *displayLocale,
                       UChar *dest, int32_t destCapacity,
                       UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getVariant, _kVariants, pErrorCode);
}













U_CAPI int32_t U_EXPORT2
uloc_getDisplayName(const char *locale,
                    const char *displayLocale,
                    UChar *dest, int32_t destCapacity,
                    UErrorCode *pErrorCode)
{
    static const UChar defaultSeparator[3] = { 0x002c, 0x0020, 0x0000 }; 
    static const int32_t defaultSepLen = 2;
    static const UChar sub0[4] = { 0x007b, 0x0030, 0x007d , 0x0000 } ; 
    static const UChar sub1[4] = { 0x007b, 0x0031, 0x007d , 0x0000 } ; 
    static const int32_t subLen = 3;
    static const UChar defaultPattern[10] = {
        0x007b, 0x0030, 0x007d, 0x0020, 0x0028, 0x007b, 0x0031, 0x007d, 0x0029, 0x0000
    }; 
    static const int32_t defaultPatLen = 9;
    static const int32_t defaultSub0Pos = 0;
    static const int32_t defaultSub1Pos = 5;

    int32_t length; 

    const UChar *separator;
    int32_t sepLen = 0;
    const UChar *pattern;
    int32_t patLen = 0;
    int32_t sub0Pos, sub1Pos;

    UBool haveLang = TRUE; 

    UBool haveRest = TRUE; 

    UBool retry = FALSE; 

    int32_t langi = 0; 

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        UResourceBundle* locbundle=ures_open(U_ICUDATA_LANG, displayLocale, &status);
        UResourceBundle* dspbundle=ures_getByKeyWithFallback(locbundle, _kLocaleDisplayPattern,
                                                             NULL, &status);

        separator=ures_getStringByKeyWithFallback(dspbundle, _kSeparator, &sepLen, &status);
        pattern=ures_getStringByKeyWithFallback(dspbundle, _kPattern, &patLen, &status);

        ures_close(dspbundle);
        ures_close(locbundle);
    }

    
    if(sepLen == 0) {
       separator = defaultSeparator;
       sepLen = defaultSepLen;
    }

    if(patLen==0 || (patLen==defaultPatLen && !u_strncmp(pattern, defaultPattern, patLen))) {
        pattern=defaultPattern;
        patLen=defaultPatLen;
        sub0Pos=defaultSub0Pos;
        sub1Pos=defaultSub1Pos;
    } else { 
        UChar *p0=u_strstr(pattern, sub0);
        UChar *p1=u_strstr(pattern, sub1);
        if (p0==NULL || p1==NULL) {
            *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
        sub0Pos=p0-pattern;
        sub1Pos=p1-pattern;
        if (sub1Pos < sub0Pos) { 
            int32_t t=sub0Pos; sub0Pos=sub1Pos; sub1Pos=t;
            langi=1;
        }
    }

    






    do {
        UChar* p=dest;
        int32_t patPos=0; 
        int32_t langLen=0; 
        int32_t langPos=0; 
        int32_t restLen=0; 
        int32_t restPos=0; 
        UEnumeration* kenum = NULL; 

        
        if(sub0Pos) {
            if(destCapacity >= sub0Pos) {
                while (patPos < sub0Pos) {
                    *p++ = pattern[patPos++];
                }
            } else {
                patPos=sub0Pos;
            }
            length=sub0Pos;
        } else {
            length=0;
        }

        for(int32_t subi=0,resti=0;subi<2;) { 
            UBool subdone = FALSE; 

            

            int32_t cap=destCapacity-length;
            if (cap <= 0) {
                cap=0;
            } else {
                p=dest+length;
            }

            if (subi == langi) { 
                if(haveLang) {
                    langPos=length;
                    langLen=uloc_getDisplayLanguage(locale, displayLocale, p, cap, pErrorCode);
                    length+=langLen;
                    haveLang=langLen>0;
                }
                subdone=TRUE;
            } else { 
                if(!haveRest) {
                    subdone=TRUE;
                } else {
                    int32_t len; 
                    switch(resti++) {
                        case 0:
                            restPos=length;
                            len=uloc_getDisplayScriptInContext(locale, displayLocale, p, cap, pErrorCode);
                            break;
                        case 1:
                            len=uloc_getDisplayCountry(locale, displayLocale, p, cap, pErrorCode);
                            break;
                        case 2:
                            len=uloc_getDisplayVariant(locale, displayLocale, p, cap, pErrorCode);
                            break;
                        case 3:
                            kenum = uloc_openKeywords(locale, pErrorCode);
                            
                        default: {
                            const char* kw=uenum_next(kenum, &len, pErrorCode);
                            if (kw == NULL) {
                                uenum_close(kenum);
                                len=0; 
                                subdone=TRUE;
                            } else {
                                

                                len = uloc_getDisplayKeyword(kw, displayLocale, p, cap, pErrorCode);
                                if(len) {
                                    if(len < cap) {
                                        p[len]=0x3d; 
                                    }
                                    len+=1;

                                    
                                    cap-=len;
                                    if(cap <= 0) {
                                        cap=0;
                                    } else {
                                        p+=len;
                                    }
                                }
                                
                                if(*pErrorCode == U_BUFFER_OVERFLOW_ERROR) {
                                    *pErrorCode=U_ZERO_ERROR;
                                }
                                int32_t vlen = uloc_getDisplayKeywordValue(locale, kw, displayLocale,
                                                                           p, cap, pErrorCode);
                                if(len) {
                                    if(vlen==0) {
                                        --len; 
                                    }
                                    
                                    cap=destCapacity-length;
                                    if(cap <= 0) {
                                        cap=0;
                                    } else {
                                        p=dest+length;
                                    }
                                }
                                len+=vlen; 
                            }
                        } break;
                    } 

                    if (len>0) {
                        
                        if(len+sepLen<=cap) {
                            p+=len;
                            for(int32_t i=0;i<sepLen;++i) {
                                *p++=separator[i];
                            }
                        }
                        length+=len+sepLen;
                    } else if(subdone) {
                        
                        if (length!=restPos) {
                            length-=sepLen;
                        }
                        restLen=length-restPos;
                        haveRest=restLen>0;
                    }
                }
            }

            if(*pErrorCode == U_BUFFER_OVERFLOW_ERROR) {
                *pErrorCode=U_ZERO_ERROR;
            }

            if(subdone) {
                if(haveLang && haveRest) {
                    

                    int32_t padLen;
                    patPos+=subLen;
                    padLen=(subi==0 ? sub1Pos : patLen)-patPos;
                    if(length+padLen < destCapacity) {
                        p=dest+length;
                        for(int32_t i=0;i<padLen;++i) {
                            *p++=pattern[patPos++];
                        }
                    } else {
                        patPos+=padLen;
                    }
                    length+=padLen;
                } else if(subi==0) {
                    
                    sub0Pos=0;
                    length=0;
                } else if(length>0) {
                    
                    length=haveLang?langLen:restLen;
                    if(dest && sub0Pos!=0) {
                        if (sub0Pos+length<=destCapacity) {
                            

                            u_memmove(dest, dest+(haveLang?langPos:restPos), length);
                        } else {
                            
                            sub0Pos=0; 

                            retry=TRUE;
                        }
                    }
                }

                ++subi; 
            }
        }
    } while(retry);

    return u_terminateUChars(dest, destCapacity, length, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayKeyword(const char* keyword,
                       const char* displayLocale,
                       UChar* dest,
                       int32_t destCapacity,
                       UErrorCode* status){

    
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }


    
    return _getStringOrCopyKey(U_ICUDATA_LANG, displayLocale,
                               _kKeys, NULL, 
                               keyword, 
                               keyword,      
                               dest, destCapacity,
                               status);

}


#define UCURRENCY_DISPLAY_NAME_INDEX 1

U_CAPI int32_t U_EXPORT2
uloc_getDisplayKeywordValue(   const char* locale,
                               const char* keyword,
                               const char* displayLocale,
                               UChar* dest,
                               int32_t destCapacity,
                               UErrorCode* status){


    char keywordValue[ULOC_FULLNAME_CAPACITY*4];
    int32_t capacity = ULOC_FULLNAME_CAPACITY*4;
    int32_t keywordValueLen =0;

    
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    keywordValue[0]=0;
    keywordValueLen = uloc_getKeywordValue(locale, keyword, keywordValue, capacity, status);

    



    if(uprv_stricmp(keyword, _kCurrency)==0){

        int32_t dispNameLen = 0;
        const UChar *dispName = NULL;
        
        UResourceBundle *bundle     = ures_open(U_ICUDATA_CURR, displayLocale, status);
        UResourceBundle *currencies = ures_getByKey(bundle, _kCurrencies, NULL, status);
        UResourceBundle *currency   = ures_getByKeyWithFallback(currencies, keywordValue, NULL, status);
        
        dispName = ures_getStringByIndex(currency, UCURRENCY_DISPLAY_NAME_INDEX, &dispNameLen, status);
        
        
        ures_close(currency);
        ures_close(currencies);
        ures_close(bundle);
        
        if(U_FAILURE(*status)){
            if(*status == U_MISSING_RESOURCE_ERROR){
                
                *status = U_USING_DEFAULT_WARNING;
            }else{
                return 0;
            }
        }

        
        if(dispName != NULL){
            if(dispNameLen <= destCapacity){
                uprv_memcpy(dest, dispName, dispNameLen * U_SIZEOF_UCHAR);
                return u_terminateUChars(dest, destCapacity, dispNameLen, status);
            }else{
                *status = U_BUFFER_OVERFLOW_ERROR;
                return dispNameLen;
            }
        }else{
            
            if(keywordValueLen <= destCapacity){
                u_charsToUChars(keywordValue, dest, keywordValueLen);
                return u_terminateUChars(dest, destCapacity, keywordValueLen, status);
            }else{
                 *status = U_BUFFER_OVERFLOW_ERROR;
                return keywordValueLen;
            }
        }

        
    }else{

        return _getStringOrCopyKey(U_ICUDATA_LANG, displayLocale,
                                   _kTypes, keyword, 
                                   keywordValue,
                                   keywordValue,
                                   dest, destCapacity,
                                   status);
    }
}
