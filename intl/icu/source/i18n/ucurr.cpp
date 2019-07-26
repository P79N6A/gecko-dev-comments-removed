






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ucurr.h"
#include "unicode/locid.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "unicode/choicfmt.h"
#include "unicode/parsepos.h"
#include "ustr_imp.h"
#include "cmemory.h"
#include "cstring.h"
#include "uassert.h"
#include "umutex.h"
#include "ucln_in.h"
#include "uenumimp.h"
#include "uhash.h"
#include "uresimp.h"
#include "ulist.h"
#include "ureslocs.h"


#ifdef UCURR_DEBUG
#include "stdio.h"
#endif

typedef struct IsoCodeEntry {
    const UChar *isoCode; 
    UDate from;
    UDate to;
} IsoCodeEntry;








static const int32_t LAST_RESORT_DATA[] = { 2, 0 };


static const int32_t POW10[] = { 1, 10, 100, 1000, 10000, 100000,
                                 1000000, 10000000, 100000000, 1000000000 };

static const int32_t MAX_POW10 = (sizeof(POW10)/sizeof(POW10[0])) - 1;

#define ISO_CURRENCY_CODE_LENGTH 3





static const char CURRENCY_DATA[] = "supplementalData";

static const char CURRENCY_META[] = "CurrencyMeta";


static const char CURRENCY_MAP[] = "CurrencyMap";


static const char DEFAULT_META[] = "DEFAULT";


static const char VAR_PRE_EURO[] = "PREEURO";


static const char VAR_EURO[] = "EURO";


static const char VAR_DELIM = '_';
static const char VAR_DELIM_STR[] = "_";




#define VARIANT_IS_EMPTY    0
#define VARIANT_IS_EURO     0x1
#define VARIANT_IS_PREEURO  0x2


static const char CURRENCIES[] = "Currencies";
static const char CURRENCYPLURALS[] = "CurrencyPlurals";





static const UChar CHOICE_FORMAT_MARK = 0x003D; 

static const UChar EUR_STR[] = {0x0045,0x0055,0x0052,0};


static UHashtable* gIsoCodes = NULL;
static UBool gIsoCodesInitialized = FALSE;

static UMutex gIsoCodesLock = U_MUTEX_INITIALIZER;







static UBool U_CALLCONV 
isoCodes_cleanup(void)
{
    if (gIsoCodes != NULL) {
        uhash_close(gIsoCodes);
        gIsoCodes = NULL;
    }
    gIsoCodesInitialized = FALSE;

    return TRUE;
}




static void U_CALLCONV
deleteIsoCodeEntry(void *obj) {
    IsoCodeEntry *entry = (IsoCodeEntry*)obj;
    uprv_free(entry);
}





static inline char*
myUCharsToChars(char* resultOfLen4, const UChar* currency) {
    u_UCharsToChars(currency, resultOfLen4, ISO_CURRENCY_CODE_LENGTH);
    resultOfLen4[ISO_CURRENCY_CODE_LENGTH] = 0;
    return resultOfLen4;
}







static const int32_t*
_findMetaData(const UChar* currency, UErrorCode& ec) {

    if (currency == 0 || *currency == 0) {
        if (U_SUCCESS(ec)) {
            ec = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return LAST_RESORT_DATA;
    }

    
    
    
    UResourceBundle* currencyData = ures_openDirect(U_ICUDATA_CURR, CURRENCY_DATA, &ec);
    UResourceBundle* currencyMeta = ures_getByKey(currencyData, CURRENCY_META, currencyData, &ec);

    if (U_FAILURE(ec)) {
        ures_close(currencyMeta);
        
        return LAST_RESORT_DATA;
    }

    
    char buf[ISO_CURRENCY_CODE_LENGTH+1];
    UErrorCode ec2 = U_ZERO_ERROR; 
    UResourceBundle* rb = ures_getByKey(currencyMeta, myUCharsToChars(buf, currency), NULL, &ec2);
      if (U_FAILURE(ec2)) {
        ures_close(rb);
        rb = ures_getByKey(currencyMeta,DEFAULT_META, NULL, &ec);
        if (U_FAILURE(ec)) {
            ures_close(currencyMeta);
            ures_close(rb);
            
            return LAST_RESORT_DATA;
        }
    }

    int32_t len;
    const int32_t *data = ures_getIntVector(rb, &len, &ec);
    if (U_FAILURE(ec) || len != 2) {
        
        if (U_SUCCESS(ec)) {
            ec = U_INVALID_FORMAT_ERROR;
        }
        ures_close(currencyMeta);
        ures_close(rb);
        return LAST_RESORT_DATA;
    }

    ures_close(currencyMeta);
    ures_close(rb);
    return data;
}







static uint32_t
idForLocale(const char* locale, char* countryAndVariant, int capacity, UErrorCode* ec)
{
    uint32_t variantType = 0;
    
    
    
    char variant[ULOC_FULLNAME_CAPACITY];
    uloc_getCountry(locale, countryAndVariant, capacity, ec);
    uloc_getVariant(locale, variant, sizeof(variant), ec);
    if (variant[0] != 0) {
        variantType = (uint32_t)(0 == uprv_strcmp(variant, VAR_EURO))
                   | ((uint32_t)(0 == uprv_strcmp(variant, VAR_PRE_EURO)) << 1);
        if (variantType)
        {
            uprv_strcat(countryAndVariant, VAR_DELIM_STR);
            uprv_strcat(countryAndVariant, variant);
        }
    }
    return variantType;
}









U_CDECL_BEGIN
static UBool U_CALLCONV currency_cleanup(void);
U_CDECL_END

#if !UCONFIG_NO_SERVICE
struct CReg;

static UMutex gCRegLock = U_MUTEX_INITIALIZER;
static CReg* gCRegHead = 0;

struct CReg : public icu::UMemory {
    CReg *next;
    UChar iso[ISO_CURRENCY_CODE_LENGTH+1];
    char  id[ULOC_FULLNAME_CAPACITY];

    CReg(const UChar* _iso, const char* _id)
        : next(0)
    {
        int32_t len = (int32_t)uprv_strlen(_id);
        if (len > (int32_t)(sizeof(id)-1)) {
            len = (sizeof(id)-1);
        }
        uprv_strncpy(id, _id, len);
        id[len] = 0;
        uprv_memcpy(iso, _iso, ISO_CURRENCY_CODE_LENGTH * sizeof(const UChar));
        iso[ISO_CURRENCY_CODE_LENGTH] = 0;
    }

    static UCurrRegistryKey reg(const UChar* _iso, const char* _id, UErrorCode* status)
    {
        if (status && U_SUCCESS(*status) && _iso && _id) {
            CReg* n = new CReg(_iso, _id);
            if (n) {
                umtx_lock(&gCRegLock);
                if (!gCRegHead) {
                    
                    ucln_i18n_registerCleanup(UCLN_I18N_CURRENCY, currency_cleanup);
                }
                n->next = gCRegHead;
                gCRegHead = n;
                umtx_unlock(&gCRegLock);
                return n;
            }
            *status = U_MEMORY_ALLOCATION_ERROR;
        }
        return 0;
    }

    static UBool unreg(UCurrRegistryKey key) {
        UBool found = FALSE;
        umtx_lock(&gCRegLock);

        CReg** p = &gCRegHead;
        while (*p) {
            if (*p == key) {
                *p = ((CReg*)key)->next;
                delete (CReg*)key;
                found = TRUE;
                break;
            }
            p = &((*p)->next);
        }

        umtx_unlock(&gCRegLock);
        return found;
    }

    static const UChar* get(const char* id) {
        const UChar* result = NULL;
        umtx_lock(&gCRegLock);
        CReg* p = gCRegHead;

        
        ucln_i18n_registerCleanup(UCLN_I18N_CURRENCY, currency_cleanup);
        while (p) {
            if (uprv_strcmp(id, p->id) == 0) {
                result = p->iso;
                break;
            }
            p = p->next;
        }
        umtx_unlock(&gCRegLock);
        return result;
    }

    
    static void cleanup(void) {
        while (gCRegHead) {
            CReg* n = gCRegHead;
            gCRegHead = gCRegHead->next;
            delete n;
        }
    }
};



U_CAPI UCurrRegistryKey U_EXPORT2
ucurr_register(const UChar* isoCode, const char* locale, UErrorCode *status)
{
    if (status && U_SUCCESS(*status)) {
        char id[ULOC_FULLNAME_CAPACITY];
        idForLocale(locale, id, sizeof(id), status);
        return CReg::reg(isoCode, id, status);
    }
    return NULL;
}



U_CAPI UBool U_EXPORT2
ucurr_unregister(UCurrRegistryKey key, UErrorCode* status)
{
    if (status && U_SUCCESS(*status)) {
        return CReg::unreg(key);
    }
    return FALSE;
}
#endif 









static UBool U_CALLCONV
currency_cache_cleanup(void);

U_CDECL_BEGIN
static UBool U_CALLCONV currency_cleanup(void) {
#if !UCONFIG_NO_SERVICE
    CReg::cleanup();
#endif
    


    currency_cache_cleanup();
    isoCodes_cleanup();

    return TRUE;
}
U_CDECL_END



U_CAPI int32_t U_EXPORT2
ucurr_forLocale(const char* locale,
                UChar* buff,
                int32_t buffCapacity,
                UErrorCode* ec)
{
    int32_t resLen = 0;
    const UChar* s = NULL;
    if (ec != NULL && U_SUCCESS(*ec)) {
        if ((buff && buffCapacity) || !buffCapacity) {
            UErrorCode localStatus = U_ZERO_ERROR;
            char id[ULOC_FULLNAME_CAPACITY];
            if ((resLen = uloc_getKeywordValue(locale, "currency", id, ULOC_FULLNAME_CAPACITY, &localStatus))) {
                
                if(buffCapacity > resLen) {
                    
                    T_CString_toUpperCase(id);
                    u_charsToUChars(id, buff, resLen);
                }
            } else {
                
                uint32_t variantType = idForLocale(locale, id, sizeof(id), ec);

                if (U_FAILURE(*ec)) {
                    return 0;
                }

#if !UCONFIG_NO_SERVICE
                const UChar* result = CReg::get(id);
                if (result) {
                    if(buffCapacity > u_strlen(result)) {
                        u_strcpy(buff, result);
                    }
                    return u_strlen(result);
                }
#endif
                
                char *idDelim = strchr(id, VAR_DELIM);
                if (idDelim) {
                    idDelim[0] = 0;
                }

                
                UResourceBundle *rb = ures_openDirect(U_ICUDATA_CURR, CURRENCY_DATA, &localStatus);
                UResourceBundle *cm = ures_getByKey(rb, CURRENCY_MAP, rb, &localStatus);
                UResourceBundle *countryArray = ures_getByKey(rb, id, cm, &localStatus);
                UResourceBundle *currencyReq = ures_getByIndex(countryArray, 0, NULL, &localStatus);
                s = ures_getStringByKey(currencyReq, "id", &resLen, &localStatus);

                






                if (U_SUCCESS(localStatus)) {
                    if ((variantType & VARIANT_IS_PREEURO) && u_strcmp(s, EUR_STR) == 0) {
                        currencyReq = ures_getByIndex(countryArray, 1, currencyReq, &localStatus);
                        s = ures_getStringByKey(currencyReq, "id", &resLen, &localStatus);
                    }
                    else if ((variantType & VARIANT_IS_EURO)) {
                        s = EUR_STR;
                    }
                }
                ures_close(countryArray);
                ures_close(currencyReq);

                if ((U_FAILURE(localStatus)) && strchr(id, '_') != 0)
                {
                    
                    uloc_getParent(locale, id, sizeof(id), ec);
                    *ec = U_USING_FALLBACK_WARNING;
                    return ucurr_forLocale(id, buff, buffCapacity, ec);
                }
                else if (*ec == U_ZERO_ERROR || localStatus != U_ZERO_ERROR) {
                    
                    *ec = localStatus;
                }
                if (U_SUCCESS(*ec)) {
                    if(buffCapacity > resLen) {
                        u_strcpy(buff, s);
                    }
                }
            }
            return u_terminateUChars(buff, buffCapacity, resLen, ec);
        } else {
            *ec = U_ILLEGAL_ARGUMENT_ERROR;
        }
    }
    return resLen;
}










static UBool fallback(char *loc) {
    if (!*loc) {
        return FALSE;
    }
    UErrorCode status = U_ZERO_ERROR;
    uloc_getParent(loc, loc, (int32_t)uprv_strlen(loc), &status);
 






    return TRUE;
}


U_CAPI const UChar* U_EXPORT2
ucurr_getName(const UChar* currency,
              const char* locale,
              UCurrNameStyle nameStyle,
              UBool* isChoiceFormat, 
              int32_t* len, 
              UErrorCode* ec) {

    
    
    
    
    
    
    
    
    
    

    if (U_FAILURE(*ec)) {
        return 0;
    }

    int32_t choice = (int32_t) nameStyle;
    if (choice < 0 || choice > 1) {
        *ec = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    
    
    
    

    
    

    
    
    UErrorCode ec2 = U_ZERO_ERROR;

    char loc[ULOC_FULLNAME_CAPACITY];
    uloc_getName(locale, loc, sizeof(loc), &ec2);
    if (U_FAILURE(ec2) || ec2 == U_STRING_NOT_TERMINATED_WARNING) {
        *ec = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    char buf[ISO_CURRENCY_CODE_LENGTH+1];
    myUCharsToChars(buf, currency);
    
    
    T_CString_toUpperCase(buf);
    
    const UChar* s = NULL;
    ec2 = U_ZERO_ERROR;
    UResourceBundle* rb = ures_open(U_ICUDATA_CURR, loc, &ec2);

    rb = ures_getByKey(rb, CURRENCIES, rb, &ec2);

    
    rb = ures_getByKeyWithFallback(rb, buf, rb, &ec2);

    s = ures_getStringByIndex(rb, choice, len, &ec2);
    ures_close(rb);

    
    
    if (U_SUCCESS(ec2)) {
        if (ec2 == U_USING_DEFAULT_WARNING
            || (ec2 == U_USING_FALLBACK_WARNING && *ec != U_USING_DEFAULT_WARNING)) {
            *ec = ec2;
        }
    }

    
    
    
    
    
    *isChoiceFormat = FALSE;
    if (U_SUCCESS(ec2)) {
        U_ASSERT(s != NULL);
        int32_t i=0;
        while (i < *len && s[i] == CHOICE_FORMAT_MARK && i < 2) {
            ++i;
        }
        *isChoiceFormat = (i == 1);
        if (i != 0) ++s; 
        return s;
    }

    
    *len = u_strlen(currency); 
    *ec = U_USING_DEFAULT_WARNING;
    return currency;
}

U_CAPI const UChar* U_EXPORT2
ucurr_getPluralName(const UChar* currency,
                    const char* locale,
                    UBool* isChoiceFormat,
                    const char* pluralCount,
                    int32_t* len, 
                    UErrorCode* ec) {
    
    
    
    
    
    
    
    
    
    

    if (U_FAILURE(*ec)) {
        return 0;
    }

    
    
    UErrorCode ec2 = U_ZERO_ERROR;

    char loc[ULOC_FULLNAME_CAPACITY];
    uloc_getName(locale, loc, sizeof(loc), &ec2);
    if (U_FAILURE(ec2) || ec2 == U_STRING_NOT_TERMINATED_WARNING) {
        *ec = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    char buf[ISO_CURRENCY_CODE_LENGTH+1];
    myUCharsToChars(buf, currency);

    const UChar* s = NULL;
    ec2 = U_ZERO_ERROR;
    UResourceBundle* rb = ures_open(U_ICUDATA_CURR, loc, &ec2);

    rb = ures_getByKey(rb, CURRENCYPLURALS, rb, &ec2);

    
    rb = ures_getByKeyWithFallback(rb, buf, rb, &ec2);

    s = ures_getStringByKeyWithFallback(rb, pluralCount, len, &ec2);
    if (U_FAILURE(ec2)) {
        
        ec2 = U_ZERO_ERROR;
        s = ures_getStringByKeyWithFallback(rb, "other", len, &ec2);     
        if (U_FAILURE(ec2)) {
            ures_close(rb);
            
            return ucurr_getName(currency, locale, UCURR_LONG_NAME, 
                                 isChoiceFormat, len, ec);
        }
    }
    ures_close(rb);

    
    
    if (U_SUCCESS(ec2)) {
        if (ec2 == U_USING_DEFAULT_WARNING
            || (ec2 == U_USING_FALLBACK_WARNING && *ec != U_USING_DEFAULT_WARNING)) {
            *ec = ec2;
        }
        U_ASSERT(s != NULL);
        return s;
    }

    
    *len = u_strlen(currency); 
    *ec = U_USING_DEFAULT_WARNING;
    return currency;
}





#define NEED_TO_BE_DELETED 0x1


#define MAX_CURRENCY_NAME_LEN 100

typedef struct {
    const char* IsoCode;  
    UChar* currencyName;  
    int32_t currencyNameLen;  
    int32_t flag;  
} CurrencyNameStruct;


#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)<(b)) ? (b) : (a))
#endif



static int U_CALLCONV currencyNameComparator(const void* a, const void* b) {
    const CurrencyNameStruct* currName_1 = (const CurrencyNameStruct*)a;
    const CurrencyNameStruct* currName_2 = (const CurrencyNameStruct*)b;
    for (int32_t i = 0; 
         i < MIN(currName_1->currencyNameLen, currName_2->currencyNameLen);
         ++i) {
        if (currName_1->currencyName[i] < currName_2->currencyName[i]) {
            return -1;
        }
        if (currName_1->currencyName[i] > currName_2->currencyName[i]) {
            return 1;
        }
    }
    if (currName_1->currencyNameLen < currName_2->currencyNameLen) {
        return -1;
    } else if (currName_1->currencyNameLen > currName_2->currencyNameLen) {
        return 1;
    }
    return 0;
}










static void
getCurrencyNameCount(const char* loc, int32_t* total_currency_name_count, int32_t* total_currency_symbol_count) {
    U_NAMESPACE_USE
    *total_currency_name_count = 0;
    *total_currency_symbol_count = 0;
    const UChar* s = NULL;
    char locale[ULOC_FULLNAME_CAPACITY];
    uprv_strcpy(locale, loc);
    for (;;) {
        UErrorCode ec2 = U_ZERO_ERROR;
        
        UResourceBundle* rb = ures_open(U_ICUDATA_CURR, locale, &ec2);
        UResourceBundle* curr = ures_getByKey(rb, CURRENCIES, NULL, &ec2);
        int32_t n = ures_getSize(curr);
        for (int32_t i=0; i<n; ++i) {
            UResourceBundle* names = ures_getByIndex(curr, i, NULL, &ec2);
            int32_t len;
            s = ures_getStringByIndex(names, UCURR_SYMBOL_NAME, &len, &ec2);
            UBool isChoice = FALSE;
            if (len > 0 && s[0] == CHOICE_FORMAT_MARK) {
                ++s;
                --len;
                if (len > 0 && s[0] != CHOICE_FORMAT_MARK) {
                    isChoice = TRUE;
                }
            }
            if (isChoice) {
                ChoiceFormat fmt(UnicodeString(TRUE, s, len), ec2);
                int32_t fmt_count;
                fmt.getFormats(fmt_count);
                *total_currency_symbol_count += fmt_count;
            } else {
                ++(*total_currency_symbol_count);  
            }

            ++(*total_currency_symbol_count); 
            ++(*total_currency_name_count); 
            ures_close(names);
        }

        
        UErrorCode ec3 = U_ZERO_ERROR;
        UResourceBundle* curr_p = ures_getByKey(rb, CURRENCYPLURALS, NULL, &ec3);
        n = ures_getSize(curr_p);
        for (int32_t i=0; i<n; ++i) {
            UResourceBundle* names = ures_getByIndex(curr_p, i, NULL, &ec3);
            *total_currency_name_count += ures_getSize(names);
            ures_close(names);
        }
        ures_close(curr_p);
        ures_close(curr);
        ures_close(rb);

        if (!fallback(locale)) {
            break;
        }
    }
}

static UChar* 
toUpperCase(const UChar* source, int32_t len, const char* locale) {
    UChar* dest = NULL;
    UErrorCode ec = U_ZERO_ERROR;
    int32_t destLen = u_strToUpper(dest, 0, source, len, locale, &ec);

    ec = U_ZERO_ERROR;
    dest = (UChar*)uprv_malloc(sizeof(UChar) * MAX(destLen, len));
    u_strToUpper(dest, destLen, source, len, locale, &ec);
    if (U_FAILURE(ec)) {
        uprv_memcpy(dest, source, sizeof(UChar) * len);
    } 
    return dest;
}








static void
collectCurrencyNames(const char* locale, 
                     CurrencyNameStruct** currencyNames, 
                     int32_t* total_currency_name_count, 
                     CurrencyNameStruct** currencySymbols, 
                     int32_t* total_currency_symbol_count, 
                     UErrorCode& ec) {
    U_NAMESPACE_USE
    
    UErrorCode ec2 = U_ZERO_ERROR;

    char loc[ULOC_FULLNAME_CAPACITY];
    uloc_getName(locale, loc, sizeof(loc), &ec2);
    if (U_FAILURE(ec2) || ec2 == U_STRING_NOT_TERMINATED_WARNING) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
    }

    
    getCurrencyNameCount(loc, total_currency_name_count, total_currency_symbol_count);

    *currencyNames = (CurrencyNameStruct*)uprv_malloc
        (sizeof(CurrencyNameStruct) * (*total_currency_name_count));
    *currencySymbols = (CurrencyNameStruct*)uprv_malloc
        (sizeof(CurrencyNameStruct) * (*total_currency_symbol_count));

    const UChar* s = NULL;  
    char* iso = NULL;  

    *total_currency_name_count = 0;
    *total_currency_symbol_count = 0;

    UErrorCode ec3 = U_ZERO_ERROR;
    UErrorCode ec4 = U_ZERO_ERROR;

    
    UHashtable* currencyIsoCodes = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &ec3);
    UHashtable* currencyPluralIsoCodes = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &ec4);
    for (int32_t localeLevel = 0; ; ++localeLevel) {
        ec2 = U_ZERO_ERROR;
        
        UResourceBundle* rb = ures_open(U_ICUDATA_CURR, loc, &ec2);
        UResourceBundle* curr = ures_getByKey(rb, CURRENCIES, NULL, &ec2);
        int32_t n = ures_getSize(curr);
        for (int32_t i=0; i<n; ++i) {
            UResourceBundle* names = ures_getByIndex(curr, i, NULL, &ec2);
            int32_t len;
            s = ures_getStringByIndex(names, UCURR_SYMBOL_NAME, &len, &ec2);
            
            iso = (char*)ures_getKey(names);
            if (localeLevel == 0) {
                uhash_put(currencyIsoCodes, iso, iso, &ec3); 
            } else {
                if (uhash_get(currencyIsoCodes, iso) != NULL) {
                    ures_close(names);
                    continue;
                } else {
                    uhash_put(currencyIsoCodes, iso, iso, &ec3); 
                }
            }
            UBool isChoice = FALSE;
            if (len > 0 && s[0] == CHOICE_FORMAT_MARK) {
                ++s;
                --len;
                if (len > 0 && s[0] != CHOICE_FORMAT_MARK) {
                    isChoice = TRUE;
                }
            }
            if (isChoice) {
                ChoiceFormat fmt(UnicodeString(TRUE, s, len), ec2);
                int32_t fmt_count;
                const UnicodeString* formats = fmt.getFormats(fmt_count);
                for (int i = 0; i < fmt_count; ++i) {
                    
                    int32_t length = formats[i].length();
                    UChar* name = (UChar*)uprv_malloc(sizeof(UChar)*length);
                    formats[i].extract(0, length, name);
                    (*currencySymbols)[*total_currency_symbol_count].IsoCode = iso;
                    (*currencySymbols)[*total_currency_symbol_count].currencyName = name;
                    (*currencySymbols)[*total_currency_symbol_count].flag = NEED_TO_BE_DELETED;
                    (*currencySymbols)[(*total_currency_symbol_count)++].currencyNameLen = length;
                }
            } else {
                
                (*currencySymbols)[*total_currency_symbol_count].IsoCode = iso;
                (*currencySymbols)[*total_currency_symbol_count].currencyName = (UChar*)s;
                (*currencySymbols)[*total_currency_symbol_count].flag = 0;
                (*currencySymbols)[(*total_currency_symbol_count)++].currencyNameLen = len;
            }

            
            s = ures_getStringByIndex(names, UCURR_LONG_NAME, &len, &ec2);
            (*currencyNames)[*total_currency_name_count].IsoCode = iso;
            UChar* upperName = toUpperCase(s, len, locale);
            (*currencyNames)[*total_currency_name_count].currencyName = upperName;
            (*currencyNames)[*total_currency_name_count].flag = NEED_TO_BE_DELETED;
            (*currencyNames)[(*total_currency_name_count)++].currencyNameLen = len;

            
            
            (*currencySymbols)[*total_currency_symbol_count].IsoCode = iso;
            (*currencySymbols)[*total_currency_symbol_count].currencyName = (UChar*)uprv_malloc(sizeof(UChar)*3);
            
            u_charsToUChars(iso, (*currencySymbols)[*total_currency_symbol_count].currencyName, 3);
            (*currencySymbols)[*total_currency_symbol_count].flag = NEED_TO_BE_DELETED;
            (*currencySymbols)[(*total_currency_symbol_count)++].currencyNameLen = 3;

            ures_close(names);
        }

        
        UErrorCode ec3 = U_ZERO_ERROR;
        UResourceBundle* curr_p = ures_getByKey(rb, CURRENCYPLURALS, NULL, &ec3);
        n = ures_getSize(curr_p);
        for (int32_t i=0; i<n; ++i) {
            UResourceBundle* names = ures_getByIndex(curr_p, i, NULL, &ec3);
            iso = (char*)ures_getKey(names);
            
            if (localeLevel == 0) {
                uhash_put(currencyPluralIsoCodes, iso, iso, &ec4); 
            } else {
                if (uhash_get(currencyPluralIsoCodes, iso) != NULL) {
                    ures_close(names);
                    continue;
                } else {
                    uhash_put(currencyPluralIsoCodes, iso, iso, &ec4); 
                }
            }
            int32_t num = ures_getSize(names);
            int32_t len;
            for (int32_t j = 0; j < num; ++j) {
                
                
                s = ures_getStringByIndex(names, j, &len, &ec3);
                (*currencyNames)[*total_currency_name_count].IsoCode = iso;
                UChar* upperName = toUpperCase(s, len, locale);
                (*currencyNames)[*total_currency_name_count].currencyName = upperName;
                (*currencyNames)[*total_currency_name_count].flag = NEED_TO_BE_DELETED;
                (*currencyNames)[(*total_currency_name_count)++].currencyNameLen = len;
            }
            ures_close(names);
        }
        ures_close(curr_p);
        ures_close(curr);
        ures_close(rb);

        if (!fallback(loc)) {
            break;
        }
    }

    uhash_close(currencyIsoCodes);
    uhash_close(currencyPluralIsoCodes);

    
    qsort(*currencyNames, *total_currency_name_count, 
          sizeof(CurrencyNameStruct), currencyNameComparator);
    qsort(*currencySymbols, *total_currency_symbol_count, 
          sizeof(CurrencyNameStruct), currencyNameComparator);

#ifdef UCURR_DEBUG
    printf("currency name count: %d\n", *total_currency_name_count);
    for (int32_t index = 0; index < *total_currency_name_count; ++index) {
        printf("index: %d\n", index);
        printf("iso: %s\n", (*currencyNames)[index].IsoCode);
        printf("currencyName:");
        for (int32_t i = 0; i < (*currencyNames)[index].currencyNameLen; ++i) {
            printf("%c", (unsigned char)(*currencyNames)[index].currencyName[i]);
        }
        printf("\n");
        printf("len: %d\n", (*currencyNames)[index].currencyNameLen);
    }
    printf("currency symbol count: %d\n", *total_currency_symbol_count);
    for (int32_t index = 0; index < *total_currency_symbol_count; ++index) {
        printf("index: %d\n", index);
        printf("iso: %s\n", (*currencySymbols)[index].IsoCode);
        printf("currencySymbol:");
        for (int32_t i = 0; i < (*currencySymbols)[index].currencyNameLen; ++i) {
            printf("%c", (unsigned char)(*currencySymbols)[index].currencyName[i]);
        }
        printf("\n");
        printf("len: %d\n", (*currencySymbols)[index].currencyNameLen);
    }
#endif
}







static int32_t
binarySearch(const CurrencyNameStruct* currencyNames, 
             int32_t indexInCurrencyNames,
             const UChar key,
             int32_t* begin, int32_t* end) {
#ifdef UCURR_DEBUG
    printf("key = %x\n", key);
#endif
   int32_t first = *begin;
   int32_t last = *end;
   while (first <= last) {
       int32_t mid = (first + last) / 2;  
       if (indexInCurrencyNames >= currencyNames[mid].currencyNameLen) {
           first = mid + 1;
       } else {
           if (key > currencyNames[mid].currencyName[indexInCurrencyNames]) {
               first = mid + 1;
           }
           else if (key < currencyNames[mid].currencyName[indexInCurrencyNames]) {
               last = mid - 1;
           }
           else {
                
                
                
                int32_t L = *begin;
                int32_t R = mid;

#ifdef UCURR_DEBUG
                printf("mid = %d\n", mid);
#endif
                while (L < R) {
                    int32_t M = (L + R) / 2;
#ifdef UCURR_DEBUG
                    printf("L = %d, R = %d, M = %d\n", L, R, M);
#endif
                    if (indexInCurrencyNames >= currencyNames[M].currencyNameLen) {
                        L = M + 1;
                    } else {
                        if (currencyNames[M].currencyName[indexInCurrencyNames] < key) {
                            L = M + 1;
                        } else {
#ifdef UCURR_DEBUG
                            U_ASSERT(currencyNames[M].currencyName[indexInCurrencyNames] == key);
#endif
                            R = M;
                        }
                    }
                }
#ifdef UCURR_DEBUG
                U_ASSERT(L == R);
#endif
                *begin = L;
#ifdef UCURR_DEBUG
                printf("begin = %d\n", *begin);
                U_ASSERT(currencyNames[*begin].currencyName[indexInCurrencyNames] == key);
#endif

                
                
                L = mid;
                R = *end;
                while (L < R) {
                    int32_t M = (L + R) / 2;
#ifdef UCURR_DEBUG
                    printf("L = %d, R = %d, M = %d\n", L, R, M);
#endif
                    if (currencyNames[M].currencyNameLen < indexInCurrencyNames) {
                        L = M + 1;
                    } else {
                        if (currencyNames[M].currencyName[indexInCurrencyNames] > key) {
                            R = M;
                        } else {
#ifdef UCURR_DEBUG
                            U_ASSERT(currencyNames[M].currencyName[indexInCurrencyNames] == key);
#endif
                            L = M + 1;
                        }
                    }
                }
#ifdef UCURR_DEBUG
                U_ASSERT(L == R);
#endif
                if (currencyNames[R].currencyName[indexInCurrencyNames] > key) {
                    *end = R - 1;
                } else {
                    *end = R;
                }
#ifdef UCURR_DEBUG
                printf("end = %d\n", *end);
#endif

                
                if (currencyNames[*begin].currencyNameLen == indexInCurrencyNames + 1) {
                    return *begin;  
                }
                return -1;  
           }
       }
   }
   *begin = -1;
   *end = -1;
   return -1;    
}










static void
linearSearch(const CurrencyNameStruct* currencyNames, 
             int32_t begin, int32_t end,
             const UChar* text, int32_t textLen,
             int32_t *maxMatchLen, int32_t* maxMatchIndex) {
    for (int32_t index = begin; index <= end; ++index) {
        int32_t len = currencyNames[index].currencyNameLen;
        if (len > *maxMatchLen && len <= textLen &&
            uprv_memcmp(currencyNames[index].currencyName, text, len * sizeof(UChar)) == 0) {
            *maxMatchIndex = index;
            *maxMatchLen = len;
#ifdef UCURR_DEBUG
            printf("maxMatchIndex = %d, maxMatchLen = %d\n",
                   *maxMatchIndex, *maxMatchLen);
#endif
        }
    }
}

#define LINEAR_SEARCH_THRESHOLD 10








static void
searchCurrencyName(const CurrencyNameStruct* currencyNames, 
                   int32_t total_currency_count,
                   const UChar* text, int32_t textLen, 
                   int32_t* maxMatchLen, int32_t* maxMatchIndex) {
    *maxMatchIndex = -1;
    *maxMatchLen = 0;
    int32_t matchIndex = -1;
    int32_t binarySearchBegin = 0;
    int32_t binarySearchEnd = total_currency_count - 1;
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (int32_t index = 0; index < textLen; ++index) {
        
        
        matchIndex = binarySearch(currencyNames, index,
                                  text[index],
                                  &binarySearchBegin, &binarySearchEnd);
        if (binarySearchBegin == -1) { 
            break;
        }
        if (matchIndex != -1) { 
            
            
            *maxMatchLen = index + 1;
            *maxMatchIndex = matchIndex;
        }
        if (binarySearchEnd - binarySearchBegin < LINEAR_SEARCH_THRESHOLD) {
            
            linearSearch(currencyNames, binarySearchBegin, binarySearchEnd,
                         text, textLen,
                         maxMatchLen, maxMatchIndex);
            break;
        }
    }
    return;
}


typedef struct {
    char locale[ULOC_FULLNAME_CAPACITY];  
    
    CurrencyNameStruct* currencyNames;  
    int32_t totalCurrencyNameCount;  
    
    CurrencyNameStruct* currencySymbols; 
    int32_t totalCurrencySymbolCount;  
    
    
    
    
    
    int32_t refCount;
} CurrencyNameCacheEntry;


#define CURRENCY_NAME_CACHE_NUM 10


static CurrencyNameCacheEntry* currCache[CURRENCY_NAME_CACHE_NUM] = {NULL};


static int8_t currentCacheEntryIndex = 0;


static void
deleteCurrencyNames(CurrencyNameStruct* currencyNames, int32_t count) {
    for (int32_t index = 0; index < count; ++index) {
        if ( (currencyNames[index].flag & NEED_TO_BE_DELETED) ) {
            uprv_free(currencyNames[index].currencyName);
        }
    }
    uprv_free(currencyNames);
}


static void
deleteCacheEntry(CurrencyNameCacheEntry* entry) {
    deleteCurrencyNames(entry->currencyNames, entry->totalCurrencyNameCount);
    deleteCurrencyNames(entry->currencySymbols, entry->totalCurrencySymbolCount);
    uprv_free(entry);
}



static UBool U_CALLCONV
currency_cache_cleanup(void) {
    for (int32_t i = 0; i < CURRENCY_NAME_CACHE_NUM; ++i) {
        if (currCache[i]) {
            deleteCacheEntry(currCache[i]);
            currCache[i] = 0;
        }
    }
    return TRUE;
}


U_CFUNC void
uprv_parseCurrency(const char* locale,
                   const icu::UnicodeString& text,
                   icu::ParsePosition& pos,
                   int8_t type,
                   UChar* result,
                   UErrorCode& ec)
{
    U_NAMESPACE_USE

    if (U_FAILURE(ec)) {
        return;
    }

    int32_t total_currency_name_count = 0;
    CurrencyNameStruct* currencyNames = NULL;
    int32_t total_currency_symbol_count = 0;
    CurrencyNameStruct* currencySymbols = NULL;
    CurrencyNameCacheEntry* cacheEntry = NULL;

    umtx_lock(NULL);
    
    
    int8_t  found = -1;
    for (int8_t i = 0; i < CURRENCY_NAME_CACHE_NUM; ++i) {
        if (currCache[i]!= NULL &&
            uprv_strcmp(locale, currCache[i]->locale) == 0) {
            found = i;
            break;
        }
    }
    if (found != -1) {
        cacheEntry = currCache[found];
        currencyNames = cacheEntry->currencyNames;
        total_currency_name_count = cacheEntry->totalCurrencyNameCount;
        currencySymbols = cacheEntry->currencySymbols;
        total_currency_symbol_count = cacheEntry->totalCurrencySymbolCount;
        ++(cacheEntry->refCount);
    }
    umtx_unlock(NULL);
    if (found == -1) {
        collectCurrencyNames(locale, &currencyNames, &total_currency_name_count, &currencySymbols, &total_currency_symbol_count, ec);
        if (U_FAILURE(ec)) {
            return;
        }
        umtx_lock(NULL);
        
        int8_t  found = -1;
        for (int8_t i = 0; i < CURRENCY_NAME_CACHE_NUM; ++i) {
            if (currCache[i]!= NULL &&
                uprv_strcmp(locale, currCache[i]->locale) == 0) {
                found = i;
                break;
            }
        }
        if (found == -1) {
            
            
            
            
            
            cacheEntry = currCache[currentCacheEntryIndex];
            if (cacheEntry) {
                --(cacheEntry->refCount);
                
                if (cacheEntry->refCount == 0) {
                    deleteCacheEntry(cacheEntry);
                }
            }
            cacheEntry = (CurrencyNameCacheEntry*)uprv_malloc(sizeof(CurrencyNameCacheEntry));
            currCache[currentCacheEntryIndex] = cacheEntry;
            uprv_strcpy(cacheEntry->locale, locale);
            cacheEntry->currencyNames = currencyNames;
            cacheEntry->totalCurrencyNameCount = total_currency_name_count;
            cacheEntry->currencySymbols = currencySymbols;
            cacheEntry->totalCurrencySymbolCount = total_currency_symbol_count;
            cacheEntry->refCount = 2; 
            currentCacheEntryIndex = (currentCacheEntryIndex + 1) % CURRENCY_NAME_CACHE_NUM;
            ucln_i18n_registerCleanup(UCLN_I18N_CURRENCY, currency_cache_cleanup);
            
        } else {
            deleteCurrencyNames(currencyNames, total_currency_name_count);
            deleteCurrencyNames(currencySymbols, total_currency_symbol_count);
            cacheEntry = currCache[found];
            currencyNames = cacheEntry->currencyNames;
            total_currency_name_count = cacheEntry->totalCurrencyNameCount;
            currencySymbols = cacheEntry->currencySymbols;
            total_currency_symbol_count = cacheEntry->totalCurrencySymbolCount;
            ++(cacheEntry->refCount);
        }
        umtx_unlock(NULL);
    }

    int32_t start = pos.getIndex();

    UChar inputText[MAX_CURRENCY_NAME_LEN];  
    UChar upperText[MAX_CURRENCY_NAME_LEN];  
    int32_t textLen = MIN(MAX_CURRENCY_NAME_LEN, text.length() - start);
    text.extract(start, textLen, inputText);
    UErrorCode ec1 = U_ZERO_ERROR;
    textLen = u_strToUpper(upperText, MAX_CURRENCY_NAME_LEN, inputText, textLen, locale, &ec1);

    int32_t max = 0;
    int32_t matchIndex = -1;
    
    searchCurrencyName(currencyNames, total_currency_name_count, 
                       upperText, textLen, &max, &matchIndex);

#ifdef UCURR_DEBUG
    printf("search in names, max = %d, matchIndex = %d\n", max, matchIndex);
#endif

    int32_t maxInSymbol = 0;
    int32_t matchIndexInSymbol = -1;
    if (type != UCURR_LONG_NAME) {  
        
        searchCurrencyName(currencySymbols, total_currency_symbol_count, 
                           inputText, textLen, 
                           &maxInSymbol, &matchIndexInSymbol);
    }

#ifdef UCURR_DEBUG
    printf("search in symbols, maxInSymbol = %d, matchIndexInSymbol = %d\n", maxInSymbol, matchIndexInSymbol);
#endif

    if (max >= maxInSymbol && matchIndex != -1) {
        u_charsToUChars(currencyNames[matchIndex].IsoCode, result, 4);
        pos.setIndex(start + max);
    } else if (maxInSymbol >= max && matchIndexInSymbol != -1) {
        u_charsToUChars(currencySymbols[matchIndexInSymbol].IsoCode, result, 4);
        pos.setIndex(start + maxInSymbol);
    } 

    
    umtx_lock(NULL);
    --(cacheEntry->refCount);
    if (cacheEntry->refCount == 0) {  
        deleteCacheEntry(cacheEntry);
    }
    umtx_unlock(NULL);
}












U_CFUNC void
uprv_getStaticCurrencyName(const UChar* iso, const char* loc,
                           icu::UnicodeString& result, UErrorCode& ec)
{
    U_NAMESPACE_USE

    UBool isChoiceFormat;
    int32_t len;
    const UChar* currname = ucurr_getName(iso, loc, UCURR_SYMBOL_NAME,
                                          &isChoiceFormat, &len, &ec);
    if (U_SUCCESS(ec)) {
        
        
        result.truncate(0);
        if (isChoiceFormat) {
            ChoiceFormat f(UnicodeString(TRUE, currname, len), ec);
            if (U_SUCCESS(ec)) {
                f.format(2.0, result);
            } else {
                result.setTo(iso, -1);
            }
        } else {
            result.setTo(currname, -1);
        }
    }
}

U_CAPI int32_t U_EXPORT2
ucurr_getDefaultFractionDigits(const UChar* currency, UErrorCode* ec) {
    return (_findMetaData(currency, *ec))[0];
}

U_CAPI double U_EXPORT2
ucurr_getRoundingIncrement(const UChar* currency, UErrorCode* ec) {
    const int32_t *data = _findMetaData(currency, *ec);

    
    if (data[0] < 0 || data[0] > MAX_POW10) {
        if (U_SUCCESS(*ec)) {
            *ec = U_INVALID_FORMAT_ERROR;
        }
        return 0.0;
    }

    
    
    if (data[1] < 2) {
        return 0.0;
    }

    
    
    return double(data[1]) / POW10[data[0]];
}

U_CDECL_BEGIN

typedef struct UCurrencyContext {
    uint32_t currType; 
    uint32_t listIdx;
} UCurrencyContext;







static const struct CurrencyList {
    const char *currency;
    uint32_t currType;
} gCurrencyList[] = {
    {"ADP", UCURR_COMMON|UCURR_DEPRECATED},
    {"AED", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"AFA", UCURR_COMMON|UCURR_DEPRECATED},
    {"AFN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ALK", UCURR_COMMON|UCURR_DEPRECATED},
    {"ALL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"AMD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ANG", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"AOA", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"AOK", UCURR_COMMON|UCURR_DEPRECATED},
    {"AON", UCURR_COMMON|UCURR_DEPRECATED},
    {"AOR", UCURR_COMMON|UCURR_DEPRECATED},
    {"ARA", UCURR_COMMON|UCURR_DEPRECATED},
    {"ARL", UCURR_COMMON|UCURR_DEPRECATED},
    {"ARM", UCURR_COMMON|UCURR_DEPRECATED},
    {"ARP", UCURR_COMMON|UCURR_DEPRECATED},
    {"ARS", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ATS", UCURR_COMMON|UCURR_DEPRECATED},
    {"AUD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"AWG", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"AZM", UCURR_COMMON|UCURR_DEPRECATED},
    {"AZN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BAD", UCURR_COMMON|UCURR_DEPRECATED},
    {"BAM", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BAN", UCURR_COMMON|UCURR_DEPRECATED},
    {"BBD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BDT", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BEC", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"BEF", UCURR_COMMON|UCURR_DEPRECATED},
    {"BEL", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"BGL", UCURR_COMMON|UCURR_DEPRECATED},
    {"BGM", UCURR_COMMON|UCURR_DEPRECATED},
    {"BGN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BGO", UCURR_COMMON|UCURR_DEPRECATED},
    {"BHD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BIF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BMD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BND", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BOB", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BOL", UCURR_COMMON|UCURR_DEPRECATED},
    {"BOP", UCURR_COMMON|UCURR_DEPRECATED},
    {"BOV", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"BRB", UCURR_COMMON|UCURR_DEPRECATED},
    {"BRC", UCURR_COMMON|UCURR_DEPRECATED},
    {"BRE", UCURR_COMMON|UCURR_DEPRECATED},
    {"BRL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BRN", UCURR_COMMON|UCURR_DEPRECATED},
    {"BRR", UCURR_COMMON|UCURR_DEPRECATED},
    {"BRZ", UCURR_COMMON|UCURR_DEPRECATED},
    {"BSD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BTN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BUK", UCURR_COMMON|UCURR_DEPRECATED},
    {"BWP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BYB", UCURR_COMMON|UCURR_DEPRECATED},
    {"BYR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"BZD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CAD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CDF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CHE", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"CHF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CHW", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"CLE", UCURR_COMMON|UCURR_DEPRECATED},
    {"CLF", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"CLP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CNX", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"CNY", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"COP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"COU", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"CRC", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CSD", UCURR_COMMON|UCURR_DEPRECATED},
    {"CSK", UCURR_COMMON|UCURR_DEPRECATED},
    {"CUC", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CUP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CVE", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"CYP", UCURR_COMMON|UCURR_DEPRECATED},
    {"CZK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"DDM", UCURR_COMMON|UCURR_DEPRECATED},
    {"DEM", UCURR_COMMON|UCURR_DEPRECATED},
    {"DJF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"DKK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"DOP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"DZD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ECS", UCURR_COMMON|UCURR_DEPRECATED},
    {"ECV", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"EEK", UCURR_COMMON|UCURR_DEPRECATED},
    {"EGP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"EQE", UCURR_COMMON|UCURR_DEPRECATED},
    {"ERN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ESA", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"ESB", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"ESP", UCURR_COMMON|UCURR_DEPRECATED},
    {"ETB", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"EUR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"FIM", UCURR_COMMON|UCURR_DEPRECATED},
    {"FJD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"FKP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"FRF", UCURR_COMMON|UCURR_DEPRECATED},
    {"GBP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"GEK", UCURR_COMMON|UCURR_DEPRECATED},
    {"GEL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"GHC", UCURR_COMMON|UCURR_DEPRECATED},
    {"GHS", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"GIP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"GMD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"GNF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"GNS", UCURR_COMMON|UCURR_DEPRECATED},
    {"GQE", UCURR_COMMON|UCURR_DEPRECATED},
    {"GRD", UCURR_COMMON|UCURR_DEPRECATED},
    {"GTQ", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"GWE", UCURR_COMMON|UCURR_DEPRECATED},
    {"GWP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"GYD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"HKD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"HNL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"HRD", UCURR_COMMON|UCURR_DEPRECATED},
    {"HRK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"HTG", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"HUF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"IDR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"IEP", UCURR_COMMON|UCURR_DEPRECATED},
    {"ILP", UCURR_COMMON|UCURR_DEPRECATED},
    {"ILR", UCURR_COMMON|UCURR_DEPRECATED},
    {"ILS", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"INR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"IQD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"IRR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ISJ", UCURR_COMMON|UCURR_DEPRECATED},
    {"ISK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ITL", UCURR_COMMON|UCURR_DEPRECATED},
    {"JMD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"JOD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"JPY", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KES", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KGS", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KHR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KMF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KPW", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KRH", UCURR_COMMON|UCURR_DEPRECATED},
    {"KRO", UCURR_COMMON|UCURR_DEPRECATED},
    {"KRW", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KWD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KYD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"KZT", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"LAK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"LBP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"LKR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"LRD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"LSL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"LSM", UCURR_COMMON|UCURR_DEPRECATED},
    {"LTL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"LTT", UCURR_COMMON|UCURR_DEPRECATED},
    {"LUC", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"LUF", UCURR_COMMON|UCURR_DEPRECATED},
    {"LUL", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"LVL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"LVR", UCURR_COMMON|UCURR_DEPRECATED},
    {"LYD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MAD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MAF", UCURR_COMMON|UCURR_DEPRECATED},
    {"MCF", UCURR_COMMON|UCURR_DEPRECATED},
    {"MDC", UCURR_COMMON|UCURR_DEPRECATED},
    {"MDL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MGA", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MGF", UCURR_COMMON|UCURR_DEPRECATED},
    {"MKD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MKN", UCURR_COMMON|UCURR_DEPRECATED},
    {"MLF", UCURR_COMMON|UCURR_DEPRECATED},
    {"MMK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MNT", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MOP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MRO", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MTL", UCURR_COMMON|UCURR_DEPRECATED},
    {"MTP", UCURR_COMMON|UCURR_DEPRECATED},
    {"MUR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MVP", UCURR_COMMON|UCURR_DEPRECATED},
    {"MVR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MWK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MXN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MXP", UCURR_COMMON|UCURR_DEPRECATED},
    {"MXV", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"MYR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MZE", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"MZM", UCURR_COMMON|UCURR_DEPRECATED},
    {"MZN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"NAD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"NGN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"NIC", UCURR_COMMON|UCURR_DEPRECATED},
    {"NIO", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"NLG", UCURR_COMMON|UCURR_DEPRECATED},
    {"NOK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"NPR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"NZD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"OMR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"PAB", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"PEI", UCURR_COMMON|UCURR_DEPRECATED},
    {"PEN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"PES", UCURR_COMMON|UCURR_DEPRECATED},
    {"PGK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"PHP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"PKR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"PLN", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"PLZ", UCURR_COMMON|UCURR_DEPRECATED},
    {"PTE", UCURR_COMMON|UCURR_DEPRECATED},
    {"PYG", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"QAR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"RHD", UCURR_COMMON|UCURR_DEPRECATED},
    {"ROL", UCURR_COMMON|UCURR_DEPRECATED},
    {"RON", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"RSD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"RUB", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"RUR", UCURR_COMMON|UCURR_DEPRECATED},
    {"RWF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SAR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SBD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SCR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SDD", UCURR_COMMON|UCURR_DEPRECATED},
    {"SDG", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SDP", UCURR_COMMON|UCURR_DEPRECATED},
    {"SEK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SGD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SHP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SIT", UCURR_COMMON|UCURR_DEPRECATED},
    {"SKK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SLL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SOS", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SRD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SRG", UCURR_COMMON|UCURR_DEPRECATED},
    {"STD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SUR", UCURR_COMMON|UCURR_DEPRECATED},
    {"SVC", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SYP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"SZL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"THB", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"TJR", UCURR_COMMON|UCURR_DEPRECATED},
    {"TJS", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"TMM", UCURR_COMMON|UCURR_DEPRECATED},
    {"TMT", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"TND", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"TOP", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"TPE", UCURR_COMMON|UCURR_DEPRECATED},
    {"TRL", UCURR_COMMON|UCURR_DEPRECATED},
    {"TRY", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"TTD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"TWD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"TZS", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"UAH", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"UAK", UCURR_COMMON|UCURR_DEPRECATED},
    {"UGS", UCURR_COMMON|UCURR_DEPRECATED},
    {"UGX", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"USD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"USN", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"USS", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"UYI", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"UYP", UCURR_COMMON|UCURR_DEPRECATED},
    {"UYU", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"UZS", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"VEB", UCURR_COMMON|UCURR_DEPRECATED},
    {"VEF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"VND", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"VNN", UCURR_COMMON|UCURR_DEPRECATED},
    {"VUV", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"WST", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"XAF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"XAG", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XAU", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XBA", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XBB", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XBC", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XBD", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XCD", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"XDR", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XEU", UCURR_UNCOMMON|UCURR_DEPRECATED},
    {"XFO", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XFU", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XOF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"XPD", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XPF", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"XPT", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XRE", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XTS", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"XXX", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"YDD", UCURR_COMMON|UCURR_DEPRECATED},
    {"YER", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"YUD", UCURR_COMMON|UCURR_DEPRECATED},
    {"YUM", UCURR_COMMON|UCURR_DEPRECATED},
    {"YUN", UCURR_COMMON|UCURR_DEPRECATED},
    {"YUR", UCURR_COMMON|UCURR_DEPRECATED},
    {"ZAL", UCURR_UNCOMMON|UCURR_NON_DEPRECATED},
    {"ZAR", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ZMK", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ZRN", UCURR_COMMON|UCURR_DEPRECATED},
    {"ZRZ", UCURR_COMMON|UCURR_DEPRECATED},
    {"ZWL", UCURR_COMMON|UCURR_NON_DEPRECATED},
    {"ZWR", UCURR_COMMON|UCURR_DEPRECATED},
    {"ZWD", UCURR_COMMON|UCURR_DEPRECATED},
    { NULL, 0 } 
};

#define UCURR_MATCHES_BITMASK(variable, typeToMatch) \
    ((typeToMatch) == UCURR_ALL || ((variable) & (typeToMatch)) == (typeToMatch))

static int32_t U_CALLCONV
ucurr_countCurrencyList(UEnumeration *enumerator, UErrorCode * ) {
    UCurrencyContext *myContext = (UCurrencyContext *)(enumerator->context);
    uint32_t currType = myContext->currType;
    int32_t count = 0;

    
    for (int32_t idx = 0; gCurrencyList[idx].currency != NULL; idx++) {
        if (UCURR_MATCHES_BITMASK(gCurrencyList[idx].currType, currType)) {
            count++;
        }
    }
    return count;
}

static const char* U_CALLCONV
ucurr_nextCurrencyList(UEnumeration *enumerator,
                        int32_t* resultLength,
                        UErrorCode * )
{
    UCurrencyContext *myContext = (UCurrencyContext *)(enumerator->context);

    
    while (myContext->listIdx < (sizeof(gCurrencyList)/sizeof(gCurrencyList[0]))-1) {
        const struct CurrencyList *currItem = &gCurrencyList[myContext->listIdx++];
        if (UCURR_MATCHES_BITMASK(currItem->currType, myContext->currType))
        {
            if (resultLength) {
                *resultLength = 3; 
            }
            return currItem->currency;
        }
    }
    
    if (resultLength) {
        *resultLength = 0;
    }
    return NULL;
}

static void U_CALLCONV
ucurr_resetCurrencyList(UEnumeration *enumerator, UErrorCode * ) {
    ((UCurrencyContext *)(enumerator->context))->listIdx = 0;
}

static void U_CALLCONV
ucurr_closeCurrencyList(UEnumeration *enumerator) {
    uprv_free(enumerator->context);
    uprv_free(enumerator);
}

static void U_CALLCONV
ucurr_createCurrencyList(UErrorCode* status){
    UErrorCode localStatus = U_ZERO_ERROR;

    
    UResourceBundle *rb = ures_openDirect(U_ICUDATA_CURR, CURRENCY_DATA, &localStatus);
    UResourceBundle *currencyMapArray = ures_getByKey(rb, CURRENCY_MAP, rb, &localStatus);

    if (U_SUCCESS(localStatus)) {
        
        for (int32_t i=0; i<ures_getSize(currencyMapArray); i++) {
            
            UResourceBundle *currencyArray = ures_getByIndex(currencyMapArray, i, NULL, &localStatus);
            
            if (U_SUCCESS(localStatus)) {
                for (int32_t j=0; j<ures_getSize(currencyArray); j++) {
                    
                    UResourceBundle *currencyRes = ures_getByIndex(currencyArray, j, NULL, &localStatus);
                    IsoCodeEntry *entry = (IsoCodeEntry*)uprv_malloc(sizeof(IsoCodeEntry));
                    if (entry == NULL) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        return;
                    }

                    
                    int32_t isoLength = 0;
                    UResourceBundle *idRes = ures_getByKey(currencyRes, "id", NULL, &localStatus);
                    if (idRes == NULL) {
                        continue;
                    }
                    const UChar *isoCode = ures_getString(idRes, &isoLength, &localStatus);

                    
                    UDate fromDate = U_DATE_MIN;
                    UResourceBundle *fromRes = ures_getByKey(currencyRes, "from", NULL, &localStatus);

                    if (U_SUCCESS(localStatus)) {
                        int32_t fromLength = 0;
                        const int32_t *fromArray = ures_getIntVector(fromRes, &fromLength, &localStatus);
                        int64_t currDate64 = (int64_t)fromArray[0] << 32;
                        currDate64 |= ((int64_t)fromArray[1] & (int64_t)INT64_C(0x00000000FFFFFFFF));
                        fromDate = (UDate)currDate64;
                    }
                    ures_close(fromRes);

                    
                    UDate toDate = U_DATE_MAX;
                    localStatus = U_ZERO_ERROR;
                    UResourceBundle *toRes = ures_getByKey(currencyRes, "to", NULL, &localStatus);

                    if (U_SUCCESS(localStatus)) {
                        int32_t toLength = 0;
                        const int32_t *toArray = ures_getIntVector(toRes, &toLength, &localStatus);
                        int64_t currDate64 = (int64_t)toArray[0] << 32;
                        currDate64 |= ((int64_t)toArray[1] & (int64_t)INT64_C(0x00000000FFFFFFFF));
                        toDate = (UDate)currDate64;
                    }
                    ures_close(toRes);

                    ures_close(idRes);
                    ures_close(currencyRes);

                    entry->isoCode = isoCode;
                    entry->from = fromDate;
                    entry->to = toDate;

                    localStatus = U_ZERO_ERROR;
                    uhash_put(gIsoCodes, (UChar *)isoCode, entry, &localStatus);
                }
            } else {
                *status = localStatus;
            }
            ures_close(currencyArray);
        }
    } else {
        *status = localStatus;
    }

    ures_close(currencyMapArray);
}

static const UEnumeration gEnumCurrencyList = {
    NULL,
    NULL,
    ucurr_closeCurrencyList,
    ucurr_countCurrencyList,
    uenum_unextDefault,
    ucurr_nextCurrencyList,
    ucurr_resetCurrencyList
};
U_CDECL_END

U_CAPI UBool U_EXPORT2
ucurr_isAvailable(const UChar* isoCode, UDate from, UDate to, UErrorCode* eErrorCode) {
    UErrorCode status = U_ZERO_ERROR;
    UBool initialized;
    UMTX_CHECK(&gIsoCodesLock, gIsoCodesInitialized, initialized);

    if (!initialized) {
        umtx_lock(&gIsoCodesLock);
        gIsoCodes = uhash_open(uhash_hashUChars, uhash_compareUChars, NULL, &status);
        if (U_FAILURE(status)) {
            umtx_unlock(&gIsoCodesLock);
            return FALSE;
        }
        uhash_setValueDeleter(gIsoCodes, deleteIsoCodeEntry);

        ucln_i18n_registerCleanup(UCLN_I18N_CURRENCY, currency_cleanup);
        ucurr_createCurrencyList(&status);
        if (U_FAILURE(status)) {
            umtx_unlock(&gIsoCodesLock);
            return FALSE;
        }

        gIsoCodesInitialized = TRUE;
        umtx_unlock(&gIsoCodesLock);
    }

    umtx_lock(&gIsoCodesLock);
    IsoCodeEntry* result = (IsoCodeEntry *) uhash_get(gIsoCodes, isoCode);
    umtx_unlock(&gIsoCodesLock);

    if (result == NULL) {
        return FALSE;
    } else if (from > to) {
        *eErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    } else if  ((from > result->to) || (to < result->from)) {
        return FALSE;
    }

    return TRUE;
}

U_CAPI UEnumeration * U_EXPORT2
ucurr_openISOCurrencies(uint32_t currType, UErrorCode *pErrorCode) {
    UEnumeration *myEnum = NULL;
    UCurrencyContext *myContext;

    myEnum = (UEnumeration*)uprv_malloc(sizeof(UEnumeration));
    if (myEnum == NULL) {
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memcpy(myEnum, &gEnumCurrencyList, sizeof(UEnumeration));
    myContext = (UCurrencyContext*)uprv_malloc(sizeof(UCurrencyContext));
    if (myContext == NULL) {
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(myEnum);
        return NULL;
    }
    myContext->currType = currType;
    myContext->listIdx = 0;
    myEnum->context = myContext;
    return myEnum;
}

U_CAPI int32_t U_EXPORT2
ucurr_countCurrencies(const char* locale, 
                 UDate date, 
                 UErrorCode* ec)
{
    int32_t currCount = 0;

    if (ec != NULL && U_SUCCESS(*ec)) 
    {
        
        UErrorCode localStatus = U_ZERO_ERROR;
        char id[ULOC_FULLNAME_CAPACITY];
        uloc_getKeywordValue(locale, "currency", id, ULOC_FULLNAME_CAPACITY, &localStatus);
        
         idForLocale(locale, id, sizeof(id), ec);

        if (U_FAILURE(*ec))
        {
            return 0;
        }

        
        char *idDelim = strchr(id, VAR_DELIM);
        if (idDelim)
        {
            idDelim[0] = 0;
        }

        
        UResourceBundle *rb = ures_openDirect(U_ICUDATA_CURR, CURRENCY_DATA, &localStatus);
        UResourceBundle *cm = ures_getByKey(rb, CURRENCY_MAP, rb, &localStatus);

        
        UResourceBundle *countryArray = ures_getByKey(rb, id, cm, &localStatus);

        
        if (U_SUCCESS(localStatus))
        {
            for (int32_t i=0; i<ures_getSize(countryArray); i++)
            {
                
                UResourceBundle *currencyRes = ures_getByIndex(countryArray, i, NULL, &localStatus);

                
                int32_t fromLength = 0;
                UResourceBundle *fromRes = ures_getByKey(currencyRes, "from", NULL, &localStatus);
                const int32_t *fromArray = ures_getIntVector(fromRes, &fromLength, &localStatus);

                int64_t currDate64 = (int64_t)fromArray[0] << 32;
                currDate64 |= ((int64_t)fromArray[1] & (int64_t)INT64_C(0x00000000FFFFFFFF));
                UDate fromDate = (UDate)currDate64;

                if (ures_getSize(currencyRes)> 2)
                {
                    int32_t toLength = 0;
                    UResourceBundle *toRes = ures_getByKey(currencyRes, "to", NULL, &localStatus);
                    const int32_t *toArray = ures_getIntVector(toRes, &toLength, &localStatus);

                    currDate64 = (int64_t)toArray[0] << 32;
                    currDate64 |= ((int64_t)toArray[1] & (int64_t)INT64_C(0x00000000FFFFFFFF));
                    UDate toDate = (UDate)currDate64;

                    if ((fromDate <= date) && (date < toDate))
                    {
                        currCount++;
                    }

                    ures_close(toRes);
                }
                else
                {
                    if (fromDate <= date)
                    {
                        currCount++;
                    }
                }

                
                ures_close(currencyRes);
                ures_close(fromRes);

            } 
        } 

        ures_close(countryArray);

        
        if (*ec == U_ZERO_ERROR || localStatus != U_ZERO_ERROR)
        {
            
            
            *ec = localStatus;
        }

        if (U_SUCCESS(*ec))
        {
            
            return currCount;
        }

    }

    
    
    return 0;
}

U_CAPI int32_t U_EXPORT2 
ucurr_forLocaleAndDate(const char* locale, 
                UDate date, 
                int32_t index,
                UChar* buff, 
                int32_t buffCapacity, 
                UErrorCode* ec)
{
    int32_t resLen = 0;
	int32_t currIndex = 0;
    const UChar* s = NULL;

    if (ec != NULL && U_SUCCESS(*ec))
    {
        
        if ((buff && buffCapacity) || !buffCapacity )
        {
            
            UErrorCode localStatus = U_ZERO_ERROR;
            char id[ULOC_FULLNAME_CAPACITY];
            resLen = uloc_getKeywordValue(locale, "currency", id, ULOC_FULLNAME_CAPACITY, &localStatus);

            
             idForLocale(locale, id, sizeof(id), ec);
            if (U_FAILURE(*ec))
            {
                return 0;
            }

            
            char *idDelim = strchr(id, VAR_DELIM);
            if (idDelim)
            {
                idDelim[0] = 0;
            }

            
            UResourceBundle *rb = ures_openDirect(U_ICUDATA_CURR, CURRENCY_DATA, &localStatus);
            UResourceBundle *cm = ures_getByKey(rb, CURRENCY_MAP, rb, &localStatus);

            
            UResourceBundle *countryArray = ures_getByKey(rb, id, cm, &localStatus);

            
            bool matchFound = false;
            if (U_SUCCESS(localStatus))
            {
                if ((index <= 0) || (index> ures_getSize(countryArray)))
                {
                    
                    ures_close(countryArray);
                    return 0;
                }

                for (int32_t i=0; i<ures_getSize(countryArray); i++)
                {
                    
                    UResourceBundle *currencyRes = ures_getByIndex(countryArray, i, NULL, &localStatus);
                    s = ures_getStringByKey(currencyRes, "id", &resLen, &localStatus);

                    
                    int32_t fromLength = 0;
                    UResourceBundle *fromRes = ures_getByKey(currencyRes, "from", NULL, &localStatus);
                    const int32_t *fromArray = ures_getIntVector(fromRes, &fromLength, &localStatus);

                    int64_t currDate64 = (int64_t)fromArray[0] << 32;
                    currDate64 |= ((int64_t)fromArray[1] & (int64_t)INT64_C(0x00000000FFFFFFFF));
                    UDate fromDate = (UDate)currDate64;

                    if (ures_getSize(currencyRes)> 2)
                    {
                        int32_t toLength = 0;
                        UResourceBundle *toRes = ures_getByKey(currencyRes, "to", NULL, &localStatus);
                        const int32_t *toArray = ures_getIntVector(toRes, &toLength, &localStatus);

                        currDate64 = (int64_t)toArray[0] << 32;
                        currDate64 |= ((int64_t)toArray[1] & (int64_t)INT64_C(0x00000000FFFFFFFF));
                        UDate toDate = (UDate)currDate64;

                        if ((fromDate <= date) && (date < toDate))
                        {
                            currIndex++;
                            if (currIndex == index)
                            {
                                matchFound = true;
                            }
                        }

                        ures_close(toRes);
                    }
                    else
                    {
                        if (fromDate <= date)
                        {
                            currIndex++;
                            if (currIndex == index)
                            {
                                matchFound = true;
                            }
                        }
                    }

                    
                    ures_close(currencyRes);
                    ures_close(fromRes);

                    
                    if (matchFound)
                    {
                        break;
                    }

                } 
            }

            ures_close(countryArray);

            
            if (*ec == U_ZERO_ERROR || localStatus != U_ZERO_ERROR)
            {
                
                
                *ec = localStatus;
            }

            if (U_SUCCESS(*ec))
            {
                
                if((buffCapacity> resLen) && matchFound)
                {
                    
                    u_strcpy(buff, s);
                }
                else
                {
                    return 0;
                }
            }

            
            return u_terminateUChars(buff, buffCapacity, resLen, ec);
        }
        else
        {
            
            *ec = U_ILLEGAL_ARGUMENT_ERROR;
        }

    }

    
    
    return resLen;
}

static const UEnumeration defaultKeywordValues = {
    NULL,
    NULL,
    ulist_close_keyword_values_iterator,
    ulist_count_keyword_values,
    uenum_unextDefault,
    ulist_next_keyword_value, 
    ulist_reset_keyword_values_iterator
};

U_CAPI UEnumeration *U_EXPORT2 ucurr_getKeywordValuesForLocale(const char *key, const char *locale, UBool commonlyUsed, UErrorCode* status) {
    
    char prefRegion[ULOC_FULLNAME_CAPACITY] = "";
    int32_t prefRegionLength = 0;
    prefRegionLength = uloc_getCountry(locale, prefRegion, sizeof(prefRegion), status);
    if (prefRegionLength == 0) {
        char loc[ULOC_FULLNAME_CAPACITY] = "";
        uloc_addLikelySubtags(locale, loc, sizeof(loc), status);
        
        prefRegionLength = uloc_getCountry(loc, prefRegion, sizeof(prefRegion), status);
    }
    
    
    UList *values = ulist_createEmptyList(status);
    UList *otherValues = ulist_createEmptyList(status);
    UEnumeration *en = (UEnumeration *)uprv_malloc(sizeof(UEnumeration));
    if (U_FAILURE(*status) || en == NULL) {
        if (en == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        } else {
            uprv_free(en);
        }
        ulist_deleteList(values);
        ulist_deleteList(otherValues);
        return NULL;
    }
    memcpy(en, &defaultKeywordValues, sizeof(UEnumeration));
    en->context = values;
    
    UResourceBundle *bundle = ures_openDirect(U_ICUDATA_CURR, "supplementalData", status);
    ures_getByKey(bundle, "CurrencyMap", bundle, status);
    UResourceBundle bundlekey, regbndl, curbndl, to;
    ures_initStackObject(&bundlekey);
    ures_initStackObject(&regbndl);
    ures_initStackObject(&curbndl);
    ures_initStackObject(&to);
    
    while (U_SUCCESS(*status) && ures_hasNext(bundle)) {
        ures_getNextResource(bundle, &bundlekey, status);
        if (U_FAILURE(*status)) {
            break;
        }
        const char *region = ures_getKey(&bundlekey);
        UBool isPrefRegion = uprv_strcmp(region, prefRegion) == 0 ? TRUE : FALSE;
        if (!isPrefRegion && commonlyUsed) {
            
            
            
            continue;
        }
        ures_getByKey(bundle, region, &regbndl, status);
        if (U_FAILURE(*status)) {
            break;
        }
        while (U_SUCCESS(*status) && ures_hasNext(&regbndl)) {
            ures_getNextResource(&regbndl, &curbndl, status);
            if (ures_getType(&curbndl) != URES_TABLE) {
                
                continue;
            }
            char *curID = (char *)uprv_malloc(sizeof(char) * ULOC_KEYWORDS_CAPACITY);
            int32_t curIDLength = ULOC_KEYWORDS_CAPACITY;
            if (curID == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }

#if U_CHARSET_FAMILY==U_ASCII_FAMILY
            ures_getUTF8StringByKey(&curbndl, "id", curID, &curIDLength, TRUE, status);
            
#else
            {
                       const UChar* defString = ures_getStringByKey(&curbndl, "id", &curIDLength, status);
                       if(U_SUCCESS(*status)) {
			   if(curIDLength+1 > ULOC_KEYWORDS_CAPACITY) {
				*status = U_BUFFER_OVERFLOW_ERROR;
			   } else {
                           	u_UCharsToChars(defString, curID, curIDLength+1);
			   }
                       }
            }
#endif	

            if (U_FAILURE(*status)) {
                break;
            }
            UBool hasTo = FALSE;
            ures_getByKey(&curbndl, "to", &to, status);
            if (U_FAILURE(*status)) {
                
                *status = U_ZERO_ERROR;
            } else {
                hasTo = TRUE;
            }
            if (isPrefRegion && !hasTo && !ulist_containsString(values, curID, (int32_t)uprv_strlen(curID))) {
                
                ulist_addItemEndList(values, curID, TRUE, status);
            } else if (!ulist_containsString(otherValues, curID, (int32_t)uprv_strlen(curID)) && !commonlyUsed) {
                ulist_addItemEndList(otherValues, curID, TRUE, status);
            } else {
                uprv_free(curID);
            }
        }
        
    }
    if (U_SUCCESS(*status)) {
        if (commonlyUsed) {
            if (ulist_getListSize(values) == 0) {
                
                
                uenum_close(en);
                en = ucurr_getKeywordValuesForLocale(key, "und", TRUE, status);
            }
        } else {
            
            char *value = NULL;
            ulist_resetList(otherValues);
            while ((value = (char *)ulist_getNext(otherValues)) != NULL) {
                if (!ulist_containsString(values, value, (int32_t)uprv_strlen(value))) {
                    char *tmpValue = (char *)uprv_malloc(sizeof(char) * ULOC_KEYWORDS_CAPACITY);
                    uprv_memcpy(tmpValue, value, uprv_strlen(value) + 1);
                    ulist_addItemEndList(values, tmpValue, TRUE, status);
                    if (U_FAILURE(*status)) {
                        break;
                    }
                }
            }
        }
        
        ulist_resetList((UList *)(en->context));
    } else {
        ulist_deleteList(values);
        uprv_free(en);
        values = NULL;
        en = NULL;
    }
    ures_close(&to);
    ures_close(&curbndl);
    ures_close(&regbndl);
    ures_close(&bundlekey);
    ures_close(bundle);
    
    ulist_deleteList(otherValues);
    
    return en;
}


U_CAPI int32_t U_EXPORT2
ucurr_getNumericCode(const UChar* currency) {
    int32_t code = 0;
    if (currency && u_strlen(currency) == ISO_CURRENCY_CODE_LENGTH) {
        UErrorCode status = U_ZERO_ERROR;

        UResourceBundle *bundle = ures_openDirect(0, "currencyNumericCodes", &status);
        ures_getByKey(bundle, "codeMap", bundle, &status);
        if (U_SUCCESS(status)) {
            char alphaCode[ISO_CURRENCY_CODE_LENGTH+1];
            myUCharsToChars(alphaCode, currency);
            T_CString_toUpperCase(alphaCode);
            ures_getByKey(bundle, alphaCode, bundle, &status);
            int tmpCode = ures_getInt(bundle, &status);
            if (U_SUCCESS(status)) {
                code = tmpCode;
            }
        }
        ures_close(bundle);
    }
    return code;
}
#endif 


