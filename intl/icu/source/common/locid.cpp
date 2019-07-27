































#include "unicode/locid.h"
#include "unicode/uloc.h"
#include "putilimp.h"
#include "mutex.h"
#include "umutex.h"
#include "uassert.h"
#include "cmemory.h"
#include "cstring.h"
#include "uassert.h"
#include "uhash.h"
#include "ucln_cmn.h"
#include "ustr_imp.h"

U_CDECL_BEGIN
static UBool U_CALLCONV locale_cleanup(void);
U_CDECL_END

U_NAMESPACE_BEGIN

static Locale   *gLocaleCache = NULL;
static UInitOnce gLocaleCacheInitOnce = U_INITONCE_INITIALIZER;


static UMutex gDefaultLocaleMutex = U_MUTEX_INITIALIZER;
static UHashtable *gDefaultLocalesHashT = NULL;
static Locale *gDefaultLocale = NULL;

U_NAMESPACE_END

typedef enum ELocalePos {
    eENGLISH,
    eFRENCH,
    eGERMAN,
    eITALIAN,
    eJAPANESE,
    eKOREAN,
    eCHINESE,

    eFRANCE,
    eGERMANY,
    eITALY,
    eJAPAN,
    eKOREA,
    eCHINA,      
    eTAIWAN,
    eUK,
    eUS,
    eCANADA,
    eCANADA_FRENCH,
    eROOT,


    
    eMAX_LOCALES
} ELocalePos;

U_CFUNC int32_t locale_getKeywords(const char *localeID,
            char prev,
            char *keywords, int32_t keywordCapacity,
            char *values, int32_t valuesCapacity, int32_t *valLen,
            UBool valuesToo,
            UErrorCode *status);

U_CDECL_BEGIN



static void U_CALLCONV
deleteLocale(void *obj) {
    delete (icu::Locale *) obj;
}

static UBool U_CALLCONV locale_cleanup(void)
{
    U_NAMESPACE_USE

    delete [] gLocaleCache;
    gLocaleCache = NULL;
    gLocaleCacheInitOnce.reset();

    if (gDefaultLocalesHashT) {
        uhash_close(gDefaultLocalesHashT);   
        gDefaultLocalesHashT = NULL;
    }
    gDefaultLocale = NULL;
    return TRUE;
}


static void U_CALLCONV locale_init(UErrorCode &status) {
    U_NAMESPACE_USE

    U_ASSERT(gLocaleCache == NULL);
    gLocaleCache = new Locale[(int)eMAX_LOCALES];
    if (gLocaleCache == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    ucln_common_registerCleanup(UCLN_COMMON_LOCALE, locale_cleanup);
    gLocaleCache[eROOT]          = Locale("");
    gLocaleCache[eENGLISH]       = Locale("en");
    gLocaleCache[eFRENCH]        = Locale("fr");
    gLocaleCache[eGERMAN]        = Locale("de");
    gLocaleCache[eITALIAN]       = Locale("it");
    gLocaleCache[eJAPANESE]      = Locale("ja");
    gLocaleCache[eKOREAN]        = Locale("ko");
    gLocaleCache[eCHINESE]       = Locale("zh");
    gLocaleCache[eFRANCE]        = Locale("fr", "FR");
    gLocaleCache[eGERMANY]       = Locale("de", "DE");
    gLocaleCache[eITALY]         = Locale("it", "IT");
    gLocaleCache[eJAPAN]         = Locale("ja", "JP");
    gLocaleCache[eKOREA]         = Locale("ko", "KR");
    gLocaleCache[eCHINA]         = Locale("zh", "CN");
    gLocaleCache[eTAIWAN]        = Locale("zh", "TW");
    gLocaleCache[eUK]            = Locale("en", "GB");
    gLocaleCache[eUS]            = Locale("en", "US");
    gLocaleCache[eCANADA]        = Locale("en", "CA");
    gLocaleCache[eCANADA_FRENCH] = Locale("fr", "CA");
}

U_CDECL_END

U_NAMESPACE_BEGIN

Locale *locale_set_default_internal(const char *id, UErrorCode& status) {
    
    Mutex lock(&gDefaultLocaleMutex);

    UBool canonicalize = FALSE;

    
    
    
    
    if (id == NULL) {
        id = uprv_getDefaultLocaleID();   
        canonicalize = TRUE; 
    }

    char localeNameBuf[512];

    if (canonicalize) {
        uloc_canonicalize(id, localeNameBuf, sizeof(localeNameBuf)-1, &status);
    } else {
        uloc_getName(id, localeNameBuf, sizeof(localeNameBuf)-1, &status);
    }
    localeNameBuf[sizeof(localeNameBuf)-1] = 0;  
                                                 
                                                 
                                                 
    if (U_FAILURE(status)) {
        return gDefaultLocale;
    }

    if (gDefaultLocalesHashT == NULL) {
        gDefaultLocalesHashT = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &status);
        if (U_FAILURE(status)) {
            return gDefaultLocale;
        }
        uhash_setValueDeleter(gDefaultLocalesHashT, deleteLocale);
        ucln_common_registerCleanup(UCLN_COMMON_LOCALE, locale_cleanup);
    }

    Locale *newDefault = (Locale *)uhash_get(gDefaultLocalesHashT, localeNameBuf);
    if (newDefault == NULL) {
        newDefault = new Locale(Locale::eBOGUS);
        if (newDefault == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return gDefaultLocale;
        }
        newDefault->init(localeNameBuf, FALSE);
        uhash_put(gDefaultLocalesHashT, (char*) newDefault->getName(), newDefault, &status);
        if (U_FAILURE(status)) {
            return gDefaultLocale;
        }
    }
    gDefaultLocale = newDefault;
    return gDefaultLocale;
}

U_NAMESPACE_END


U_CFUNC void
locale_set_default(const char *id)
{
    U_NAMESPACE_USE
    UErrorCode status = U_ZERO_ERROR;
    locale_set_default_internal(id, status);
}


U_CFUNC const char *
locale_get_default(void)
{
    U_NAMESPACE_USE
    return Locale::getDefault().getName();
}


U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(Locale)




#define SEP_CHAR '_'

Locale::~Locale()
{
    if (baseName != fullName) {
        uprv_free(baseName);
    }
    baseName = NULL;
    
    if (fullName != fullNameBuffer)
    {
        uprv_free(fullName);
        fullName = NULL;
    }
}

Locale::Locale()
    : UObject(), fullName(fullNameBuffer), baseName(NULL)
{
    init(NULL, FALSE);
}






Locale::Locale(Locale::ELocaleType)
    : UObject(), fullName(fullNameBuffer), baseName(NULL)
{
    setToBogus();
}


Locale::Locale( const   char * newLanguage,
                const   char * newCountry,
                const   char * newVariant,
                const   char * newKeywords)
    : UObject(), fullName(fullNameBuffer), baseName(NULL)
{
    if( (newLanguage==NULL) && (newCountry == NULL) && (newVariant == NULL) )
    {
        init(NULL, FALSE); 
    }
    else
    {
        MaybeStackArray<char, ULOC_FULLNAME_CAPACITY> togo;
        int32_t size = 0;
        int32_t lsize = 0;
        int32_t csize = 0;
        int32_t vsize = 0;
        int32_t ksize = 0;
        char    *p;

        

        
        if ( newLanguage != NULL )
        {
            lsize = (int32_t)uprv_strlen(newLanguage);
            size = lsize;
        }

        
        if ( newCountry != NULL )
        {
            csize = (int32_t)uprv_strlen(newCountry);
            size += csize;
        }

        
        if ( newVariant != NULL )
        {
            
            while(newVariant[0] == SEP_CHAR)
            {
                newVariant++;
            }

            
            vsize = (int32_t)uprv_strlen(newVariant);
            while( (vsize>1) && (newVariant[vsize-1] == SEP_CHAR) )
            {
                vsize--;
            }
        }

        if( vsize > 0 )
        {
            size += vsize;
        }

        
        if ( vsize > 0 )
        {
            size += 2;  
        }
        else if ( csize > 0 )
        {
            size += 1;  
        }

        if ( newKeywords != NULL)
        {
            ksize = (int32_t)uprv_strlen(newKeywords);
            size += ksize + 1;
        }


        

        

        if (size >= togo.getCapacity())
        {
            
            if (togo.resize(size+1) == NULL) {
                init(NULL, FALSE);
            }
        }

        togo[0] = 0;

        
        p = togo.getAlias();
        if ( lsize != 0 )
        {
            uprv_strcpy(p, newLanguage);
            p += lsize;
        }

        if ( ( vsize != 0 ) || (csize != 0) )  
        {                                      
            *p++ = SEP_CHAR;
        }

        if ( csize != 0 )
        {
            uprv_strcpy(p, newCountry);
            p += csize;
        }

        if ( vsize != 0)
        {
            *p++ = SEP_CHAR; 

            uprv_strncpy(p, newVariant, vsize);  
            p += vsize;                          
            *p = 0; 
        }

        if ( ksize != 0)
        {
            if (uprv_strchr(newKeywords, '=')) {
                *p++ = '@'; 
            }
            else {
                *p++ = '_'; 
                if ( vsize == 0) {
                    *p++ = '_'; 
                }
            }
            uprv_strcpy(p, newKeywords);
            p += ksize;
        }

        
        
        init(togo.getAlias(), FALSE);
    }
}

Locale::Locale(const Locale &other)
    : UObject(other), fullName(fullNameBuffer), baseName(NULL)
{
    *this = other;
}

Locale &Locale::operator=(const Locale &other)
{
    if (this == &other) {
        return *this;
    }

    
    if (baseName != fullName) {
        uprv_free(baseName);
    }
    baseName = NULL;
    if(fullName != fullNameBuffer) {
        uprv_free(fullName);
        fullName = fullNameBuffer;
    }

    
    if(other.fullName != other.fullNameBuffer) {
        fullName = (char *)uprv_malloc(sizeof(char)*(uprv_strlen(other.fullName)+1));
        if (fullName == NULL) {
            return *this;
        }
    }
    
    uprv_strcpy(fullName, other.fullName);

    
    if (other.baseName == other.fullName) {
        baseName = fullName;
    } else {
        if (other.baseName) {
            baseName = uprv_strdup(other.baseName);
        }
    }

    
    uprv_strcpy(language, other.language);
    uprv_strcpy(script, other.script);
    uprv_strcpy(country, other.country);

    
    variantBegin = other.variantBegin;
    fIsBogus = other.fIsBogus;
    return *this;
}

Locale *
Locale::clone() const {
    return new Locale(*this);
}

UBool
Locale::operator==( const   Locale& other) const
{
    return (uprv_strcmp(other.fullName, fullName) == 0);
}

#define ISASCIIALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))


Locale& Locale::init(const char* localeID, UBool canonicalize)
{
    fIsBogus = FALSE;
    
    if (baseName != fullName) {
        uprv_free(baseName);
    }
    baseName = NULL;
    if(fullName != fullNameBuffer) {
        uprv_free(fullName);
        fullName = fullNameBuffer;
    }

    
    
    
    do {
        char *separator;
        char *field[5] = {0};
        int32_t fieldLen[5] = {0};
        int32_t fieldIdx;
        int32_t variantField;
        int32_t length;
        UErrorCode err;

        if(localeID == NULL) {
            
            return *this = getDefault();
        }

        
        language[0] = script[0] = country[0] = 0;

        
        err = U_ZERO_ERROR;
        length = canonicalize ?
            uloc_canonicalize(localeID, fullName, sizeof(fullNameBuffer), &err) :
            uloc_getName(localeID, fullName, sizeof(fullNameBuffer), &err);

        if(err == U_BUFFER_OVERFLOW_ERROR || length >= (int32_t)sizeof(fullNameBuffer)) {
            
            fullName = (char *)uprv_malloc(sizeof(char)*(length + 1));
            if(fullName == 0) {
                fullName = fullNameBuffer;
                break; 
            }
            err = U_ZERO_ERROR;
            length = canonicalize ?
                uloc_canonicalize(localeID, fullName, length+1, &err) :
                uloc_getName(localeID, fullName, length+1, &err);
        }
        if(U_FAILURE(err) || err == U_STRING_NOT_TERMINATED_WARNING) {
            
            break;
        }

        variantBegin = length;

        
        separator = field[0] = fullName;
        fieldIdx = 1;
        while ((separator = uprv_strchr(field[fieldIdx-1], SEP_CHAR)) && fieldIdx < (int32_t)(sizeof(field)/sizeof(field[0]))-1) {
            field[fieldIdx] = separator + 1;
            fieldLen[fieldIdx-1] = (int32_t)(separator - field[fieldIdx-1]);
            fieldIdx++;
        }
        
        separator = uprv_strchr(field[fieldIdx-1], '@');
        char* sep2 = uprv_strchr(field[fieldIdx-1], '.');
        if (separator!=NULL || sep2!=NULL) {
            if (separator==NULL || (sep2!=NULL && separator > sep2)) {
                separator = sep2;
            }
            fieldLen[fieldIdx-1] = (int32_t)(separator - field[fieldIdx-1]);
        } else {
            fieldLen[fieldIdx-1] = length - (int32_t)(field[fieldIdx-1] - fullName);
        }

        if (fieldLen[0] >= (int32_t)(sizeof(language)))
        {
            break; 
        }

        variantField = 1; 
        if (fieldLen[0] > 0) {
            
            uprv_memcpy(language, fullName, fieldLen[0]);
            language[fieldLen[0]] = 0;
        }
        if (fieldLen[1] == 4 && ISASCIIALPHA(field[1][0]) &&
                ISASCIIALPHA(field[1][1]) && ISASCIIALPHA(field[1][2]) &&
                ISASCIIALPHA(field[1][3])) {
            
            uprv_memcpy(script, field[1], fieldLen[1]);
            script[fieldLen[1]] = 0;
            variantField++;
        }

        if (fieldLen[variantField] == 2 || fieldLen[variantField] == 3) {
            
            uprv_memcpy(country, field[variantField], fieldLen[variantField]);
            country[fieldLen[variantField]] = 0;
            variantField++;
        } else if (fieldLen[variantField] == 0) {
            variantField++; 
        }

        if (fieldLen[variantField] > 0) {
            
            variantBegin = (int32_t)(field[variantField] - fullName);
        }

        err = U_ZERO_ERROR;
        initBaseName(err);
        if (U_FAILURE(err)) {
            break;
        }

        
        return *this;
    } while(0); 

    
    setToBogus();

    return *this;
}







void
Locale::initBaseName(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    U_ASSERT(baseName==NULL || baseName==fullName);
    const char *atPtr = uprv_strchr(fullName, '@');
    const char *eqPtr = uprv_strchr(fullName, '=');
    if (atPtr && eqPtr && atPtr < eqPtr) {
        
        int32_t baseNameLength = (int32_t)(atPtr - fullName);
        baseName = (char *)uprv_malloc(baseNameLength + 1);
        if (baseName == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        uprv_strncpy(baseName, fullName, baseNameLength);
        baseName[baseNameLength] = 0;

        
        
        
        if (variantBegin > baseNameLength) {
            variantBegin = baseNameLength;
        }
    } else {
        baseName = fullName;
    }
}


int32_t
Locale::hashCode() const
{
    return ustr_hashCharsN(fullName, uprv_strlen(fullName));
}

void
Locale::setToBogus() {
    
    if(baseName != fullName) {
        uprv_free(baseName);
    }
    baseName = NULL;
    if(fullName != fullNameBuffer) {
        uprv_free(fullName);
        fullName = fullNameBuffer;
    }
    *fullNameBuffer = 0;
    *language = 0;
    *script = 0;
    *country = 0;
    fIsBogus = TRUE;
}

const Locale& U_EXPORT2
Locale::getDefault()
{
    {
        Mutex lock(&gDefaultLocaleMutex);
        if (gDefaultLocale != NULL) {
            return *gDefaultLocale;
        }
    }
    UErrorCode status = U_ZERO_ERROR;
    return *locale_set_default_internal(NULL, status);
}



void U_EXPORT2
Locale::setDefault( const   Locale&     newLocale,
                            UErrorCode&  status)
{
    if (U_FAILURE(status)) {
        return;
    }

    


    const char *localeID = newLocale.getName();
    locale_set_default_internal(localeID, status);
}

Locale U_EXPORT2
Locale::createFromName (const char *name)
{
    if (name) {
        Locale l("");
        l.init(name, FALSE);
        return l;
    }
    else {
        return getDefault();
    }
}

Locale U_EXPORT2
Locale::createCanonical(const char* name) {
    Locale loc("");
    loc.init(name, TRUE);
    return loc;
}

const char *
Locale::getISO3Language() const
{
    return uloc_getISO3Language(fullName);
}


const char *
Locale::getISO3Country() const
{
    return uloc_getISO3Country(fullName);
}








uint32_t
Locale::getLCID() const
{
    return uloc_getLCID(fullName);
}

const char* const* U_EXPORT2 Locale::getISOCountries()
{
    return uloc_getISOCountries();
}

const char* const* U_EXPORT2 Locale::getISOLanguages()
{
    return uloc_getISOLanguages();
}


void Locale::setFromPOSIXID(const char *posixID)
{
    init(posixID, TRUE);
}

const Locale & U_EXPORT2
Locale::getRoot(void)
{
    return getLocale(eROOT);
}

const Locale & U_EXPORT2
Locale::getEnglish(void)
{
    return getLocale(eENGLISH);
}

const Locale & U_EXPORT2
Locale::getFrench(void)
{
    return getLocale(eFRENCH);
}

const Locale & U_EXPORT2
Locale::getGerman(void)
{
    return getLocale(eGERMAN);
}

const Locale & U_EXPORT2
Locale::getItalian(void)
{
    return getLocale(eITALIAN);
}

const Locale & U_EXPORT2
Locale::getJapanese(void)
{
    return getLocale(eJAPANESE);
}

const Locale & U_EXPORT2
Locale::getKorean(void)
{
    return getLocale(eKOREAN);
}

const Locale & U_EXPORT2
Locale::getChinese(void)
{
    return getLocale(eCHINESE);
}

const Locale & U_EXPORT2
Locale::getSimplifiedChinese(void)
{
    return getLocale(eCHINA);
}

const Locale & U_EXPORT2
Locale::getTraditionalChinese(void)
{
    return getLocale(eTAIWAN);
}


const Locale & U_EXPORT2
Locale::getFrance(void)
{
    return getLocale(eFRANCE);
}

const Locale & U_EXPORT2
Locale::getGermany(void)
{
    return getLocale(eGERMANY);
}

const Locale & U_EXPORT2
Locale::getItaly(void)
{
    return getLocale(eITALY);
}

const Locale & U_EXPORT2
Locale::getJapan(void)
{
    return getLocale(eJAPAN);
}

const Locale & U_EXPORT2
Locale::getKorea(void)
{
    return getLocale(eKOREA);
}

const Locale & U_EXPORT2
Locale::getChina(void)
{
    return getLocale(eCHINA);
}

const Locale & U_EXPORT2
Locale::getPRC(void)
{
    return getLocale(eCHINA);
}

const Locale & U_EXPORT2
Locale::getTaiwan(void)
{
    return getLocale(eTAIWAN);
}

const Locale & U_EXPORT2
Locale::getUK(void)
{
    return getLocale(eUK);
}

const Locale & U_EXPORT2
Locale::getUS(void)
{
    return getLocale(eUS);
}

const Locale & U_EXPORT2
Locale::getCanada(void)
{
    return getLocale(eCANADA);
}

const Locale & U_EXPORT2
Locale::getCanadaFrench(void)
{
    return getLocale(eCANADA_FRENCH);
}

const Locale &
Locale::getLocale(int locid)
{
    Locale *localeCache = getLocaleCache();
    U_ASSERT((locid < eMAX_LOCALES)&&(locid>=0));
    if (localeCache == NULL) {
        
        
        locid = 0;
    }
    return localeCache[locid]; 
}





Locale *
Locale::getLocaleCache(void)
{
    UErrorCode status = U_ZERO_ERROR;
    umtx_initOnce(gLocaleCacheInitOnce, locale_init, status);
    return gLocaleCache;
}

class KeywordEnumeration : public StringEnumeration {
private:
    char *keywords;
    char *current;
    int32_t length;
    UnicodeString currUSKey;
    static const char fgClassID;

public:
    static UClassID U_EXPORT2 getStaticClassID(void) { return (UClassID)&fgClassID; }
    virtual UClassID getDynamicClassID(void) const { return getStaticClassID(); }
public:
    KeywordEnumeration(const char *keys, int32_t keywordLen, int32_t currentIndex, UErrorCode &status)
        : keywords((char *)&fgClassID), current((char *)&fgClassID), length(0) {
        if(U_SUCCESS(status) && keywordLen != 0) {
            if(keys == NULL || keywordLen < 0) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
            } else {
                keywords = (char *)uprv_malloc(keywordLen+1);
                if (keywords == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                }
                else {
                    uprv_memcpy(keywords, keys, keywordLen);
                    keywords[keywordLen] = 0;
                    current = keywords + currentIndex;
                    length = keywordLen;
                }
            }
        }
    }

    virtual ~KeywordEnumeration();

    virtual StringEnumeration * clone() const
    {
        UErrorCode status = U_ZERO_ERROR;
        return new KeywordEnumeration(keywords, length, (int32_t)(current - keywords), status);
    }

    virtual int32_t count(UErrorCode &) const {
        char *kw = keywords;
        int32_t result = 0;
        while(*kw) {
            result++;
            kw += uprv_strlen(kw)+1;
        }
        return result;
    }

    virtual const char* next(int32_t* resultLength, UErrorCode& status) {
        const char* result;
        int32_t len;
        if(U_SUCCESS(status) && *current != 0) {
            result = current;
            len = (int32_t)uprv_strlen(current);
            current += len+1;
            if(resultLength != NULL) {
                *resultLength = len;
            }
        } else {
            if(resultLength != NULL) {
                *resultLength = 0;
            }
            result = NULL;
        }
        return result;
    }

    virtual const UnicodeString* snext(UErrorCode& status) {
        int32_t resultLength = 0;
        const char *s = next(&resultLength, status);
        return setChars(s, resultLength, status);
    }

    virtual void reset(UErrorCode& ) {
        current = keywords;
    }
};

const char KeywordEnumeration::fgClassID = '\0';

KeywordEnumeration::~KeywordEnumeration() {
    uprv_free(keywords);
}

StringEnumeration *
Locale::createKeywords(UErrorCode &status) const
{
    char keywords[256];
    int32_t keywordCapacity = 256;
    StringEnumeration *result = NULL;

    const char* variantStart = uprv_strchr(fullName, '@');
    const char* assignment = uprv_strchr(fullName, '=');
    if(variantStart) {
        if(assignment > variantStart) {
            int32_t keyLen = locale_getKeywords(variantStart+1, '@', keywords, keywordCapacity, NULL, 0, NULL, FALSE, &status);
            if(keyLen) {
                result = new KeywordEnumeration(keywords, keyLen, 0, status);
            }
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    return result;
}

int32_t
Locale::getKeywordValue(const char* keywordName, char *buffer, int32_t bufLen, UErrorCode &status) const
{
    return uloc_getKeywordValue(fullName, keywordName, buffer, bufLen, &status);
}

void
Locale::setKeywordValue(const char* keywordName, const char* keywordValue, UErrorCode &status)
{
    uloc_setKeywordValue(keywordName, keywordValue, fullName, ULOC_FULLNAME_CAPACITY, &status);
    if (U_SUCCESS(status) && baseName == fullName) {
        
        initBaseName(status);
    }
}

const char *
Locale::getBaseName() const {
    return baseName;
}


U_NAMESPACE_END
