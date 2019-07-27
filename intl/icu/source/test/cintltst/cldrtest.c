





#include "cintltst.h"
#include "unicode/ures.h"
#include "unicode/ucurr.h"
#include "unicode/ustring.h"
#include "unicode/uset.h"
#include "unicode/udat.h"
#include "unicode/uscript.h"
#include "unicode/ulocdata.h"
#include "cstring.h"
#include "locmap.h"
#include "uresimp.h"





static USet*
createFlattenSet(USet *origSet, UErrorCode *status) {


    USet *newSet = NULL;
    int32_t origItemCount = 0;
    int32_t idx, graphmeSize;
    UChar32 start, end;
    UChar graphme[64];
    if (U_FAILURE(*status)) {
        log_err("createFlattenSet called with %s\n", u_errorName(*status));
        return NULL;
    }
    newSet = uset_open(1, 0);
    origItemCount = uset_getItemCount(origSet);
    for (idx = 0; idx < origItemCount; idx++) {
        graphmeSize = uset_getItem(origSet, idx,
            &start, &end,
            graphme, (int32_t)(sizeof(graphme)/sizeof(graphme[0])),
            status);
        if (U_FAILURE(*status)) {
            log_err("ERROR: uset_getItem returned %s\n", u_errorName(*status));
            *status = U_ZERO_ERROR;
        }
        if (graphmeSize) {
            uset_addAllCodePoints(newSet, graphme, graphmeSize);
        }
        else {
            uset_addRange(newSet, start, end);
        }
    }
    uset_closeOver(newSet,USET_CASE_INSENSITIVE);
    return newSet;
}

static UBool
isCurrencyPreEuro(const char* currencyKey){
    if( strcmp(currencyKey, "PTE") == 0 ||
        strcmp(currencyKey, "ESP") == 0 ||
        strcmp(currencyKey, "LUF") == 0 ||
        strcmp(currencyKey, "GRD") == 0 ||
        strcmp(currencyKey, "BEF") == 0 ||
        strcmp(currencyKey, "ITL") == 0 ||
        strcmp(currencyKey, "EEK") == 0){
            return TRUE;
    }
    return FALSE;
}
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
static void
TestKeyInRootRecursive(UResourceBundle *root, const char *rootName,
                       UResourceBundle *currentBundle, const char *locale) {
    UErrorCode errorCode = U_ZERO_ERROR;
    UResourceBundle *subRootBundle = NULL, *subBundle = NULL, *arr = NULL;

    ures_resetIterator(root);
    ures_resetIterator(currentBundle);
    while (ures_hasNext(currentBundle)) {
        const char *subBundleKey = NULL;
        const char *currentBundleKey = NULL;

        errorCode = U_ZERO_ERROR;
        currentBundleKey = ures_getKey(currentBundle);
        (void)currentBundleKey;    
        subBundle = ures_getNextResource(currentBundle, NULL, &errorCode);
        if (U_FAILURE(errorCode)) {
            log_err("Can't open a resource for lnocale %s. Error: %s\n", locale, u_errorName(errorCode));
            continue;
        }
        subBundleKey = ures_getKey(subBundle);


        subRootBundle = ures_getByKey(root, subBundleKey, NULL, &errorCode);
        if (U_FAILURE(errorCode)) {
            log_err("Can't open a resource with key \"%s\" in \"%s\" from %s for locale \"%s\"\n",
                    subBundleKey,
                    ures_getKey(currentBundle),
                    rootName,
                    locale);
            ures_close(subBundle);
            continue;
        }
        if (ures_getType(subRootBundle) != ures_getType(subBundle)) {
            log_err("key \"%s\" in \"%s\" has a different type from root for locale \"%s\"\n"
                    "\troot=%d, locale=%d\n",
                    subBundleKey,
                    ures_getKey(currentBundle),
                    locale,
                    ures_getType(subRootBundle),
                    ures_getType(subBundle));
            ures_close(subBundle);
            continue;
        }
        else if (ures_getType(subBundle) == URES_INT_VECTOR) {
            int32_t minSize;
            int32_t subBundleSize;
            int32_t idx;
            UBool sameArray = TRUE;
            const int32_t *subRootBundleArr = ures_getIntVector(subRootBundle, &minSize, &errorCode);
            const int32_t *subBundleArr = ures_getIntVector(subBundle, &subBundleSize, &errorCode);

            if (minSize > subBundleSize) {
                minSize = subBundleSize;
                log_err("Arrays are different size with key \"%s\" in \"%s\" from root for locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            }

            for (idx = 0; idx < minSize && sameArray; idx++) {
                if (subRootBundleArr[idx] != subBundleArr[idx]) {
                    sameArray = FALSE;
                }
                if (strcmp(subBundleKey, "DateTimeElements") == 0
                    && (subBundleArr[idx] < 1 || 7 < subBundleArr[idx]))
                {
                    log_err("Value out of range with key \"%s\" at index %d in \"%s\" for locale \"%s\"\n",
                            subBundleKey,
                            idx,
                            ures_getKey(currentBundle),
                            locale);
                }
            }
            
            if (sameArray
                && !(strcmp(locale, "es_US") == 0 && strcmp(subBundleKey, "DateTimeElements") == 0))
            {
                log_err("Integer vectors are the same with key \"%s\" in \"%s\" from root for locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            }
        }
        else if (ures_getType(subBundle) == URES_ARRAY) {
            UResourceBundle *subSubBundle = ures_getByIndex(subBundle, 0, NULL, &errorCode);
            UResourceBundle *subSubRootBundle = ures_getByIndex(subRootBundle, 0, NULL, &errorCode);

            if (U_SUCCESS(errorCode)
                && (ures_getType(subSubBundle) == URES_ARRAY || ures_getType(subSubRootBundle) == URES_ARRAY))
            {
                
                TestKeyInRootRecursive(subRootBundle, rootName, subBundle, locale);
            }
            else {
                int32_t minSize = ures_getSize(subRootBundle);
                int32_t idx;
                UBool sameArray = TRUE;

                if (minSize > ures_getSize(subBundle)) {
                    minSize = ures_getSize(subBundle);
                }

                if ((subBundleKey == NULL
                    || (subBundleKey != NULL &&  strcmp(subBundleKey, "LocaleScript") != 0 && !isCurrencyPreEuro(subBundleKey)))
                    && ures_getSize(subRootBundle) != ures_getSize(subBundle))
                {
                    log_err("Different size array with key \"%s\" in \"%s\" from root for locale \"%s\"\n"
                            "\troot array size=%d, locale array size=%d\n",
                            subBundleKey,
                            ures_getKey(currentBundle),
                            locale,
                            ures_getSize(subRootBundle),
                            ures_getSize(subBundle));
                }
                








                for (idx = 0; idx < minSize; idx++) {
                    int32_t rootStrLen, localeStrLen;
                    const UChar *rootStr = ures_getStringByIndex(subRootBundle,idx,&rootStrLen,&errorCode);
                    const UChar *localeStr = ures_getStringByIndex(subBundle,idx,&localeStrLen,&errorCode);
                    if (rootStr && localeStr && U_SUCCESS(errorCode)) {
                        if (u_strcmp(rootStr, localeStr) != 0) {
                            sameArray = FALSE;
                        }
                    }
                    else {
                        if ( rootStrLen > 1 && rootStr[0] == 0x41 && rootStr[1] >= 0x30 && rootStr[1] <= 0x39 ) {
                           
                           
                          errorCode = U_ZERO_ERROR;
                          arr = ures_getByIndex(subBundle,idx,NULL,&errorCode);
                          if (U_FAILURE(errorCode)) {
                              log_err("Got a NULL string with key \"%s\" in \"%s\" at index %d for root or locale \"%s\"\n",
                                      subBundleKey,
                                      ures_getKey(currentBundle),
                                      idx,
                                      locale);
                              continue;
                          }
                          if (ures_getType(arr) != URES_ARRAY || ures_getSize(arr) != (int32_t)rootStr[1] - 0x30) {
                              log_err("Got something other than a string or array of size %d for key \"%s\" in \"%s\" at index %d for root or locale \"%s\"\n",
                                      rootStr[1] - 0x30,
                                      subBundleKey,
                                      ures_getKey(currentBundle),
                                      idx,
                                      locale);
                              ures_close(arr);
                              continue;
                          }
                          localeStr = ures_getStringByIndex(arr,0,&localeStrLen,&errorCode);
                          ures_close(arr);
                          if (U_FAILURE(errorCode)) {
                              log_err("Got something other than a string or array for key \"%s\" in \"%s\" at index %d for root or locale \"%s\"\n",
                                      subBundleKey,
                                      ures_getKey(currentBundle),
                                      idx,
                                      locale);
                              continue;
                          }
                        } else {
                            log_err("Got a NULL string with key \"%s\" in \"%s\" at index %d for root or locale \"%s\"\n",
                                subBundleKey,
                                ures_getKey(currentBundle),
                                idx,
                                locale);
                            continue;
                        }
                    }
                    if (localeStr[0] == (UChar)0x20) {
                        log_err("key \"%s\" at index %d in \"%s\" starts with a space in locale \"%s\"\n",
                                subBundleKey,
                                idx,
                                ures_getKey(currentBundle),
                                locale);
                    }
                    else if ((localeStr[localeStrLen - 1] == (UChar)0x20) && (strcmp(subBundleKey,"separator") != 0)) {
                        log_err("key \"%s\" at index %d in \"%s\" ends with a space in locale \"%s\"\n",
                                subBundleKey,
                                idx,
                                ures_getKey(currentBundle),
                                locale);
                    }
                    else if (subBundleKey != NULL
                        && strcmp(subBundleKey, "DateTimePatterns") == 0)
                    {
                        int32_t quoted = 0;
                        const UChar *localeStrItr = localeStr;
                        while (*localeStrItr) {
                            if (*localeStrItr == (UChar)0x27 ) {
                                quoted++;
                            }
                            else if ((quoted % 2) == 0) {
                                
                                if (4 <= idx && idx <= 7
                                    && (*localeStrItr == (UChar)0x6B 
                                    || *localeStrItr == (UChar)0x48 
                                    || *localeStrItr == (UChar)0x6D 
                                    || *localeStrItr == (UChar)0x73 
                                    || *localeStrItr == (UChar)0x53 
                                    || *localeStrItr == (UChar)0x61 
                                    || *localeStrItr == (UChar)0x68 
                                    || *localeStrItr == (UChar)0x7A ))
                                {
                                    log_err("key \"%s\" at index %d has time pattern chars in date for locale \"%s\"\n",
                                            subBundleKey,
                                            idx,
                                            locale);
                                }
                                else if (0 <= idx && idx <= 3
                                    && (*localeStrItr == (UChar)0x47 
                                    || *localeStrItr == (UChar)0x79 
                                    || *localeStrItr == (UChar)0x4D 
                                    || *localeStrItr == (UChar)0x64 
                                    || *localeStrItr == (UChar)0x45 
                                    || *localeStrItr == (UChar)0x44 
                                    || *localeStrItr == (UChar)0x46 
                                    || *localeStrItr == (UChar)0x77 
                                    || *localeStrItr == (UChar)0x57 ))
                                {
                                    log_err("key \"%s\" at index %d has date pattern chars in time for locale \"%s\"\n",
                                            subBundleKey,
                                            idx,
                                            locale);
                                }
                            }
                            localeStrItr++;
                        }
                    }
                    else if (idx == 4 && subBundleKey != NULL
                        && strcmp(subBundleKey, "NumberElements") == 0
                        && u_charDigitValue(localeStr[0]) != 0)
                    {
                        log_err("key \"%s\" at index %d has a non-zero based number for locale \"%s\"\n",
                                subBundleKey,
                                idx,
                                locale);
                    }
                }
                (void)sameArray;    






            }
            ures_close(subSubBundle);
            ures_close(subSubRootBundle);
        }
        else if (ures_getType(subBundle) == URES_STRING) {
            int32_t len = 0;
            const UChar *string = ures_getString(subBundle, &len, &errorCode);
            if (U_FAILURE(errorCode) || string == NULL) {
                log_err("Can't open a string with key \"%s\" in \"%s\" for locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            } else if (string[0] == (UChar)0x20) {
                log_err("key \"%s\" in \"%s\" starts with a space in locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            
            } else if (string[len - 1] == (UChar)0x20 && (strcmp(subBundleKey,"separator"))) {
                log_err("key \"%s\" in \"%s\" ends with a space in locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            } else if (strcmp(subBundleKey, "localPatternChars") == 0) {
                



                





                int32_t i,j;
#if !UCONFIG_NO_FORMATTING
                if (len != UDAT_FIELD_COUNT) {
                    log_err("key \"%s\" has the wrong number of characters in locale \"%s\"\n",
                            subBundleKey,
                            locale);
                }
#endif
                
                for (i=0; i<len; ++i) {
                    if (!((string[i] >= 65 && string[i] <= 90) ||
                          (string[i] >= 97 && string[i] <= 122))) {
                        log_err("key \"%s\" has illegal character '%c' in locale \"%s\"\n",
                                subBundleKey,
                                (char) string[i],
                                locale);
                    }
                    
                    for (j=0; j<i; ++j) {
                        if (string[j] == string[i]) {
                            log_err("key \"%s\" has duplicate character '%c' in locale \"%s\"\n",
                                    subBundleKey,
                                    (char) string[i],
                                    locale);
                        }
                    }
                }
            }
            
            

#if 0
            else if (strcmp(locale, "root") != 0 && errorCode == U_ZERO_ERROR) {

                const UChar *rootString = ures_getString(subRootBundle, &len, &errorCode);
                if (U_FAILURE(errorCode) || rootString == NULL) {
                    log_err("Can't open a string with key \"%s\" in \"%s\" in root\n",
                            ures_getKey(subRootBundle),
                            ures_getKey(currentBundle));
                    continue;
                } else if (u_strcmp(string, rootString) == 0) {
                    if (strcmp(locale, "de_CH") != 0 && strcmp(subBundleKey, "Countries") != 0 &&
                        strcmp(subBundleKey, "Version") != 0) {
                        log_err("Found duplicate data with key \"%s\" in \"%s\" in locale \"%s\"\n",
                                ures_getKey(subRootBundle),
                                ures_getKey(currentBundle),
                                locale);
                    }
                    else {
                        
                        
                        log_verbose("Skipping key %s in %s\n", subBundleKey, locale);
                    }
                }
            }
#endif
        }
        else if (ures_getType(subBundle) == URES_TABLE) {
            if (strcmp(subBundleKey, "availableFormats")!=0) {
                
                TestKeyInRootRecursive(subRootBundle, rootName, subBundle, locale);
            }
            else {
                log_verbose("Skipping key %s in %s\n", subBundleKey, locale);
            }
        }
        else if (ures_getType(subBundle) == URES_BINARY || ures_getType(subBundle) == URES_INT) {
            
            
            if (strcmp(subBundleKey, "MeasurementSystem") != 0) {
                log_verbose("Skipping key \"%s\" in \"%s\" for locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            }
            
        }
        else {
            log_err("Type %d for key \"%s\" in \"%s\" is unknown for locale \"%s\"\n",
                    ures_getType(subBundle),
                    subBundleKey,
                    ures_getKey(currentBundle),
                    locale);
        }
        ures_close(subRootBundle);
        ures_close(subBundle);
    }
}
#endif

static void
testLCID(UResourceBundle *currentBundle,
         const char *localeName)
{
    UErrorCode status = U_ZERO_ERROR;
    uint32_t expectedLCID;
    char lcidStringC[64] = {0};
    int32_t len;

    expectedLCID = uloc_getLCID(localeName);
    if (expectedLCID == 0) {
        log_verbose("INFO:    %-5s does not have any LCID mapping\n",
            localeName);
        return;
    }

    status = U_ZERO_ERROR;
    len = uprv_convertToPosix(expectedLCID, lcidStringC, sizeof(lcidStringC)/sizeof(lcidStringC[0]) - 1, &status);
    if (U_FAILURE(status)) {
        log_err("ERROR:   %.4x does not have a POSIX mapping due to %s\n",
            expectedLCID, u_errorName(status));
    }
    lcidStringC[len] = 0;

    if(strcmp(localeName, lcidStringC) != 0) {
        char langName[1024];
        char langLCID[1024];
        uloc_getLanguage(localeName, langName, sizeof(langName), &status);
        uloc_getLanguage(lcidStringC, langLCID, sizeof(langLCID), &status);

        if (strcmp(langName, langLCID) == 0) {
            log_verbose("WARNING: %-5s resolves to %s (0x%.4x)\n",
                localeName, lcidStringC, expectedLCID);
        }
        else {
            log_err("ERROR:   %-5s has 0x%.4x and the number resolves wrongfully to %s\n",
                localeName, expectedLCID, lcidStringC);
        }
    }
}

#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
static void
TestLocaleStructure(void) {
    
    
    
    
    

    UResourceBundle *root, *currentLocale;
    int32_t locCount = uloc_countAvailable();
    int32_t locIndex;
    UErrorCode errorCode = U_ZERO_ERROR;
    const char *currLoc, *resolvedLoc;

    




















    if (locCount <= 1) {
        log_data_err("At least root needs to be installed\n");
    }

    root = ures_openDirect(loadTestData(&errorCode), "structLocale", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Can't open structLocale\n");
        return;
    }
    for (locIndex = 0; locIndex < locCount; locIndex++) {
        errorCode=U_ZERO_ERROR;
        currLoc = uloc_getAvailable(locIndex);
        currentLocale = ures_open(NULL, currLoc, &errorCode);
        if(errorCode != U_ZERO_ERROR) {
            if(U_SUCCESS(errorCode)) {
                

                log_err("ERROR: Locale %-5s not installed, and it should be, err %s\n",
                    uloc_getAvailable(locIndex), u_errorName(errorCode));
            } else {
                log_err("%%%%%%% Unexpected error %d in %s %%%%%%%",
                    u_errorName(errorCode),
                    uloc_getAvailable(locIndex));
            }
            ures_close(currentLocale);
            continue;
        }
        ures_getStringByKey(currentLocale, "Version", NULL, &errorCode);
        if(errorCode != U_ZERO_ERROR) {
            log_err("No version information is available for locale %s, and it should be!\n",
                currLoc);
        }
        else if (ures_getStringByKey(currentLocale, "Version", NULL, &errorCode)[0] == (UChar)(0x78)) {
            log_verbose("WARNING: The locale %s is experimental! It shouldn't be listed as an installed locale.\n",
                currLoc);
        }
        resolvedLoc = ures_getLocaleByType(currentLocale, ULOC_ACTUAL_LOCALE, &errorCode);
        if (strcmp(resolvedLoc, currLoc) != 0) {
            

            log_err("Locale resolves to different locale. Is %s an alias of %s?\n",
                currLoc, resolvedLoc);
        }
        TestKeyInRootRecursive(root, "root", currentLocale, currLoc);

        testLCID(currentLocale, currLoc);

        ures_close(currentLocale);
    }

    ures_close(root);
}
#endif

static void
compareArrays(const char *keyName,
              UResourceBundle *fromArray, const char *fromLocale,
              UResourceBundle *toArray, const char *toLocale,
              int32_t start, int32_t end)
{
    int32_t fromSize = ures_getSize(fromArray);
    int32_t toSize = ures_getSize(fromArray);
    int32_t idx;
    UErrorCode errorCode = U_ZERO_ERROR;

    if (fromSize > toSize) {
        fromSize = toSize;
        log_err("Arrays are different size from \"%s\" to \"%s\"\n",
                fromLocale,
                toLocale);
    }

    for (idx = start; idx <= end; idx++) {
        const UChar *fromBundleStr = ures_getStringByIndex(fromArray, idx, NULL, &errorCode);
        const UChar *toBundleStr = ures_getStringByIndex(toArray, idx, NULL, &errorCode);
        if (fromBundleStr && toBundleStr && u_strcmp(fromBundleStr, toBundleStr) != 0)
        {
            log_err("Difference for %s at index %d from %s= \"%s\" to %s= \"%s\"\n",
                    keyName,
                    idx,
                    fromLocale,
                    austrdup(fromBundleStr),
                    toLocale,
                    austrdup(toBundleStr));
        }
    }
}

static void
compareConsistentCountryInfo(const char *fromLocale, const char *toLocale) {
    UErrorCode errorCode = U_ZERO_ERROR;
    UResourceBundle *fromArray, *toArray;
    UResourceBundle *fromLocaleBund = ures_open(NULL, fromLocale, &errorCode);
    UResourceBundle *toLocaleBund = ures_open(NULL, toLocale, &errorCode);
    UResourceBundle *toCalendar, *fromCalendar, *toGregorian, *fromGregorian;

    if(U_FAILURE(errorCode)) {
        log_err("Can't open resource bundle %s or %s - %s\n", fromLocale, toLocale, u_errorName(errorCode));
        return;
    }
    fromCalendar = ures_getByKey(fromLocaleBund, "calendar", NULL, &errorCode);
    fromGregorian = ures_getByKeyWithFallback(fromCalendar, "gregorian", NULL, &errorCode);

    toCalendar = ures_getByKey(toLocaleBund, "calendar", NULL, &errorCode);
    toGregorian = ures_getByKeyWithFallback(toCalendar, "gregorian", NULL, &errorCode);

    fromArray = ures_getByKey(fromLocaleBund, "CurrencyElements", NULL, &errorCode);
    toArray = ures_getByKey(toLocaleBund, "CurrencyElements", NULL, &errorCode);
    if (strcmp(fromLocale, "en_CA") != 0)
    {
        
        compareArrays("CurrencyElements", fromArray, fromLocale, toArray, toLocale, 1, 2);
    }
    ures_close(fromArray);
    ures_close(toArray);

    fromArray = ures_getByKey(fromLocaleBund, "NumberPatterns", NULL, &errorCode);
    toArray = ures_getByKey(toLocaleBund, "NumberPatterns", NULL, &errorCode);
    if (strcmp(fromLocale, "en_CA") != 0)
    {
        compareArrays("NumberPatterns", fromArray, fromLocale, toArray, toLocale, 0, 3);
    }
    ures_close(fromArray);
    ures_close(toArray);

    









    fromArray = ures_getByKey(fromLocaleBund, "NumberElements", NULL, &errorCode);
    toArray = ures_getByKey(toLocaleBund, "NumberElements", NULL, &errorCode);
    if (strcmp(fromLocale, "en_CA") != 0)
    {
        compareArrays("NumberElements", fromArray, fromLocale, toArray, toLocale, 0, 3);
        
        compareArrays("NumberElements", fromArray, fromLocale, toArray, toLocale, 5, 10);
    }
    ures_close(fromArray);
    ures_close(toArray);
    ures_close(fromCalendar);
    ures_close(toCalendar);
    ures_close(fromGregorian);
    ures_close(toGregorian);

    ures_close(fromLocaleBund);
    ures_close(toLocaleBund);
}

static void
TestConsistentCountryInfo(void) {

    int32_t locCount = uloc_countAvailable();
    int32_t fromLocIndex, toLocIndex;

    int32_t fromCountryLen, toCountryLen;
    char fromCountry[ULOC_FULLNAME_CAPACITY], toCountry[ULOC_FULLNAME_CAPACITY];

    int32_t fromVariantLen, toVariantLen;
    char fromVariant[ULOC_FULLNAME_CAPACITY], toVariant[ULOC_FULLNAME_CAPACITY];

    UErrorCode errorCode = U_ZERO_ERROR;

    for (fromLocIndex = 0; fromLocIndex < locCount; fromLocIndex++) {
        const char *fromLocale = uloc_getAvailable(fromLocIndex);

        errorCode=U_ZERO_ERROR;
        fromCountryLen = uloc_getCountry(fromLocale, fromCountry, ULOC_FULLNAME_CAPACITY, &errorCode);
        if (fromCountryLen <= 0) {
            
            continue;
        }
        fromVariantLen = uloc_getVariant(fromLocale, fromVariant, ULOC_FULLNAME_CAPACITY, &errorCode);
        if (fromVariantLen > 0) {
            
            continue;
        }
        


        for (toLocIndex = fromLocIndex + 1; toLocIndex < locCount; toLocIndex++) {
            const char *toLocale = uloc_getAvailable(toLocIndex);

            toCountryLen = uloc_getCountry(toLocale, toCountry, ULOC_FULLNAME_CAPACITY, &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("Unknown failure fromLocale=%s toLocale=%s errorCode=%s\n",
                    fromLocale, toLocale, u_errorName(errorCode));
                continue;
            }

            if (toCountryLen <= 0) {
                
                continue;
            }
            toVariantLen = uloc_getVariant(toLocale, toVariant, ULOC_FULLNAME_CAPACITY, &errorCode);
            if (toVariantLen > 0) {
                
                
                continue;
            }
            if (strcmp(fromCountry, toCountry) == 0) {
                log_verbose("comparing fromLocale=%s toLocale=%s\n",
                    fromLocale, toLocale);
                compareConsistentCountryInfo(fromLocale, toLocale);
            }
        }
    }
}

static int32_t
findStringSetMismatch(const char *currLoc, const UChar *string, int32_t langSize,
                      USet * mergedExemplarSet,
                      UBool ignoreNumbers, UChar* badCharPtr) {
    UErrorCode errorCode = U_ZERO_ERROR;
    USet *exemplarSet;
    int32_t strIdx;
    if (mergedExemplarSet == NULL) {
        return -1;
    }
    exemplarSet = createFlattenSet(mergedExemplarSet, &errorCode);
    if (U_FAILURE(errorCode)) {
        log_err("%s: error createFlattenSet returned %s\n", currLoc, u_errorName(errorCode));
        return -1;
    }

    for (strIdx = 0; strIdx < langSize; strIdx++) {
        if (!uset_contains(exemplarSet, string[strIdx])
            && string[strIdx] != 0x0020 && string[strIdx] != 0x00A0 && string[strIdx] != 0x002e && string[strIdx] != 0x002c && string[strIdx] != 0x002d && string[strIdx] != 0x0027 && string[strIdx] != 0x005B && string[strIdx] != 0x005D && string[strIdx] != 0x2019 && string[strIdx] != 0x0f0b
            && string[strIdx] != 0x200C && string[strIdx] != 0x200D) {
            if (!ignoreNumbers || (ignoreNumbers && (string[strIdx] < 0x30 || string[strIdx] > 0x39))) {
                uset_close(exemplarSet);
                if (badCharPtr) {
                    *badCharPtr = string[strIdx];
                }
                return strIdx;
            }
        }
    }
    uset_close(exemplarSet);
    if (badCharPtr) {
        *badCharPtr = 0;
    }
    return -1;
}

static int32_t
myUCharsToChars(const UChar* us, char* cs, int32_t len){
    int32_t i=0;
    for(; i< len; i++){
        if(us[i] < 0x7f){
            cs[i] = (char)us[i];
        }else{
            return -1;
        }
    }
    return i;
}
static void
findSetMatch( UScriptCode *scriptCodes, int32_t scriptsLen,
              USet *exemplarSet,
              const char  *locale){
    USet *scripts[10]= {0};
    char pattern[256] = { '[', ':', 0x000 };
    int32_t patternLen;
    UChar uPattern[256] = {0};
    UErrorCode status = U_ZERO_ERROR;
    int32_t i;

    
    for(i = 0; i<scriptsLen; i++){
        strcat(pattern, uscript_getShortName(scriptCodes[i]));
        strcat(pattern, ":]");
        patternLen = (int32_t)strlen(pattern);
        u_charsToUChars(pattern, uPattern, patternLen);
        scripts[i] = uset_openPattern(uPattern, patternLen, &status);
        if(U_FAILURE(status)){
            log_err("Could not create set for pattern %s. Error: %s\n", pattern, u_errorName(status));
            return;
        }
        pattern[2] = 0;
    }
    if (strcmp(locale, "uk") == 0 || strcmp(locale, "uk_UA") == 0) {
        
        uset_add(scripts[0], 0x2bc);
    }
    if(U_SUCCESS(status)){
        UBool existsInScript = FALSE;
        



        int32_t count = uset_getItemCount(exemplarSet);

        for( i=0; i < count; i++){
            UChar32 start = 0;
            UChar32 end = 0;
            UChar *str = NULL;
            int32_t strCapacity = 0;

            strCapacity = uset_getItem(exemplarSet, i, &start, &end, str, strCapacity, &status);
            if(U_SUCCESS(status)){
                int32_t j;
                if(strCapacity == 0){
                    
                     for( j = 0; j < scriptsLen; j++){
                        if(uset_containsRange(scripts[j], start, end) == TRUE){
                            existsInScript = TRUE;
                        }
                    }
                    if(existsInScript == FALSE){
                        for( j = 0; j < scriptsLen; j++){
                            UChar toPattern[500]={'\0'};
                            char pat[500]={'\0'};
                            int32_t len = uset_toPattern(scripts[j], toPattern, 500, TRUE, &status);
                            len = myUCharsToChars(toPattern, pat, len);
                            log_err("uset_indexOf(\\u%04X)=%i uset_indexOf(\\u%04X)=%i\n", start, uset_indexOf(scripts[0], start), end, uset_indexOf(scripts[0], end));
                            if(len!=-1){
                                log_err("Pattern: %s\n",pat);
                            }
                        }
                        log_err("ExemplarCharacters and LocaleScript containment test failed for locale %s. \n", locale);
                    }
                }else{
                    strCapacity++; 
                    
                    str = (UChar*) malloc(U_SIZEOF_UCHAR * strCapacity);
                    strCapacity =  uset_getItem(exemplarSet, i, &start, &end, str, strCapacity, &status);
                    


                    for( j = 0; j < scriptsLen; j++){
                        if(uset_containsString(scripts[j],str, strCapacity) == TRUE){
                            existsInScript = TRUE;
                        }
                    }
                    if(existsInScript == FALSE){
                        log_err("ExemplarCharacters and LocaleScript containment test failed for locale %s. \n", locale);
                    }
                }
            }
        }

    }

    
    for(i = 0; i<scriptsLen; i++){
        uset_close(scripts[i]);
    }
}

static void VerifyTranslation(void) {
    UResourceBundle *root, *currentLocale;
    int32_t locCount = uloc_countAvailable();
    int32_t locIndex;
    UErrorCode errorCode = U_ZERO_ERROR;
    const char *currLoc;
    UScriptCode scripts[USCRIPT_CODE_LIMIT];
    int32_t numScripts;
    int32_t idx;
    int32_t end;
    UResourceBundle *resArray;

    if (locCount <= 1) {
        log_data_err("At least root needs to be installed\n");
    }

    root = ures_openDirect(NULL, "root", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Can't open root\n");
        return;
    }
    for (locIndex = 0; locIndex < locCount; locIndex++) {
        USet * mergedExemplarSet = NULL;
        errorCode=U_ZERO_ERROR;
        currLoc = uloc_getAvailable(locIndex);
        currentLocale = ures_open(NULL, currLoc, &errorCode);
        if(errorCode != U_ZERO_ERROR) {
            if(U_SUCCESS(errorCode)) {
                

                log_err("ERROR: Locale %-5s not installed, and it should be!\n",
                    uloc_getAvailable(locIndex));
            } else {
                log_err("%%%%%%% Unexpected error %d in %s %%%%%%%",
                    u_errorName(errorCode),
                    uloc_getAvailable(locIndex));
            }
            ures_close(currentLocale);
            continue;
        }
        {
            UErrorCode exemplarStatus = U_ZERO_ERROR;
            ULocaleData * uld = ulocdata_open(currLoc, &exemplarStatus);
            if (U_SUCCESS(exemplarStatus)) {
                USet * exemplarSet = ulocdata_getExemplarSet(uld, NULL, USET_ADD_CASE_MAPPINGS, ULOCDATA_ES_STANDARD, &exemplarStatus);
                if (U_SUCCESS(exemplarStatus)) {
                    mergedExemplarSet = uset_cloneAsThawed(exemplarSet);
                    uset_close(exemplarSet);
                    exemplarSet = ulocdata_getExemplarSet(uld, NULL, USET_ADD_CASE_MAPPINGS, ULOCDATA_ES_AUXILIARY, &exemplarStatus);
                    if (U_SUCCESS(exemplarStatus)) {
                        uset_addAll(mergedExemplarSet, exemplarSet);
                        uset_close(exemplarSet);
                    }
                    exemplarStatus = U_ZERO_ERROR;
                    exemplarSet = ulocdata_getExemplarSet(uld, NULL, 0, ULOCDATA_ES_PUNCTUATION, &exemplarStatus);
                    if (U_SUCCESS(exemplarStatus)) {
                        uset_addAll(mergedExemplarSet, exemplarSet);
                        uset_close(exemplarSet);
                    }
                } else {
                    log_err("error ulocdata_getExemplarSet (main) for locale %s returned %s\n", currLoc, u_errorName(errorCode));
                }
                ulocdata_close(uld);
            } else {
                log_err("error ulocdata_open for locale %s returned %s\n", currLoc, u_errorName(errorCode));
            }
        }
        if (mergedExemplarSet == NULL ) {
            log_verbose("skipping test for %s\n", currLoc);
        }
        
        
        
        else {
            UChar langBuffer[128];
            int32_t langSize;
            int32_t strIdx;
            UChar badChar;
            langSize = uloc_getDisplayLanguage(currLoc, currLoc, langBuffer, sizeof(langBuffer)/sizeof(langBuffer[0]), &errorCode);
            if (U_FAILURE(errorCode)) {
                log_err("error uloc_getDisplayLanguage returned %s\n", u_errorName(errorCode));
            }
            else {
                strIdx = findStringSetMismatch(currLoc, langBuffer, langSize, mergedExemplarSet, FALSE, &badChar);
                if (strIdx >= 0) {
                    log_err("getDisplayLanguage(%s) at index %d returned characters not in the exemplar characters: %04X.\n",
                        currLoc, strIdx, badChar);
                }
            }
            langSize = uloc_getDisplayCountry(currLoc, currLoc, langBuffer, sizeof(langBuffer)/sizeof(langBuffer[0]), &errorCode);
            if (U_FAILURE(errorCode)) {
                log_err("error uloc_getDisplayCountry returned %s\n", u_errorName(errorCode));
            }
            {
                UResourceBundle* cal = ures_getByKey(currentLocale, "calendar", NULL, &errorCode);
                UResourceBundle* greg = ures_getByKeyWithFallback(cal, "gregorian", NULL, &errorCode);
                UResourceBundle* names = ures_getByKeyWithFallback(greg,  "dayNames", NULL, &errorCode);
                UResourceBundle* format = ures_getByKeyWithFallback(names,  "format", NULL, &errorCode);
                resArray = ures_getByKeyWithFallback(format,  "wide", NULL, &errorCode);

                if (U_FAILURE(errorCode)) {
                    log_err("error ures_getByKey returned %s\n", u_errorName(errorCode));
                }
                if (getTestOption(QUICK_OPTION)) {
                    end = 1;
                }
                else {
                    end = ures_getSize(resArray);
                }


                for (idx = 0; idx < end; idx++) {
                    const UChar *fromBundleStr = ures_getStringByIndex(resArray, idx, &langSize, &errorCode);
                    if (U_FAILURE(errorCode)) {
                        log_err("error ures_getStringByIndex(%d) returned %s\n", idx, u_errorName(errorCode));
                        continue;
                    }
                    strIdx = findStringSetMismatch(currLoc, fromBundleStr, langSize, mergedExemplarSet, TRUE, &badChar);
                    if (strIdx >= 0) {
                        log_err("getDayNames(%s, %d) at index %d returned characters not in the exemplar characters: %04X.\n",
                            currLoc, idx, strIdx, badChar);
                    }
                }
                ures_close(resArray);
                ures_close(format);
                ures_close(names);

                names = ures_getByKeyWithFallback(greg, "monthNames", NULL, &errorCode);
                format = ures_getByKeyWithFallback(names,"format", NULL, &errorCode);
                resArray = ures_getByKeyWithFallback(format, "wide", NULL, &errorCode);
                if (U_FAILURE(errorCode)) {
                    log_err("error ures_getByKey returned %s\n", u_errorName(errorCode));
                }
                if (getTestOption(QUICK_OPTION)) {
                    end = 1;
                }
                else {
                    end = ures_getSize(resArray);
                }

                for (idx = 0; idx < end; idx++) {
                    const UChar *fromBundleStr = ures_getStringByIndex(resArray, idx, &langSize, &errorCode);
                    if (U_FAILURE(errorCode)) {
                        log_err("error ures_getStringByIndex(%d) returned %s\n", idx, u_errorName(errorCode));
                        continue;
                    }
                    strIdx = findStringSetMismatch(currLoc, fromBundleStr, langSize, mergedExemplarSet, TRUE, &badChar);
                    if (strIdx >= 0) {
                        log_err("getMonthNames(%s, %d) at index %d returned characters not in the exemplar characters: %04X.\n",
                            currLoc, idx, strIdx, badChar);
                    }
                }
                ures_close(resArray);
                ures_close(format);
                ures_close(names);
                ures_close(greg);
                ures_close(cal);
            }
            errorCode = U_ZERO_ERROR;
            numScripts = uscript_getCode(currLoc, scripts, sizeof(scripts)/sizeof(scripts[0]), &errorCode);
            if (strcmp(currLoc, "yi") == 0 && numScripts > 0 && log_knownIssue("11217", "Fix result of uscript_getCode for yi: USCRIPT_YI -> USCRIPT_HEBREW")) {
                scripts[0] = USCRIPT_HEBREW;
            }
            if (numScripts == 0) {
                log_err("uscript_getCode(%s) doesn't work.\n", currLoc);
            }else if(scripts[0] == USCRIPT_COMMON){
                log_err("uscript_getCode(%s) returned USCRIPT_COMMON.\n", currLoc);
            }

            
           {
                ULocaleData *uld = ulocdata_open(currLoc,&errorCode);
                USet *exemplarSet =  ulocdata_getExemplarSet(uld, NULL, 0, ULOCDATA_ES_STANDARD, &errorCode);
                
                findSetMatch(scripts, numScripts, exemplarSet, currLoc);
                uset_close(exemplarSet);
                ulocdata_close(uld);
            }

           
           {
               int32_t height=0, width=0;
               ulocdata_getPaperSize(currLoc, &height, &width, &errorCode);
               if(U_FAILURE(errorCode)){
                   log_err("ulocdata_getPaperSize failed for locale %s with error: %s \n", currLoc, u_errorName(errorCode));
               }
               if(strstr(currLoc, "_US")!=NULL && height != 279 && width != 216 ){
                   log_err("ulocdata_getPaperSize did not return expected data for locale %s \n", currLoc);
               }
           }
            
           {
               char fullLoc[ULOC_FULLNAME_CAPACITY];
               UMeasurementSystem measurementSystem;
               int32_t height = 0, width = 0;

               uloc_addLikelySubtags(currLoc, fullLoc, ULOC_FULLNAME_CAPACITY, &errorCode);

               errorCode = U_ZERO_ERROR;
               measurementSystem = ulocdata_getMeasurementSystem(currLoc, &errorCode);
               if (U_FAILURE(errorCode)) {
                   log_err("ulocdata_getMeasurementSystem failed for locale %s with error: %s \n", currLoc, u_errorName(errorCode));
               } else {
                   if ( strstr(fullLoc, "_US")!=NULL || strstr(fullLoc, "_MM")!=NULL || strstr(fullLoc, "_LR")!=NULL ) {
                       if(measurementSystem != UMS_US){
                            log_err("ulocdata_getMeasurementSystem did not return expected data for locale %s \n", currLoc);
                       }
                   } else if ( strstr(fullLoc, "_GB")!=NULL ) {
                       if(measurementSystem != UMS_UK){
                            log_err("ulocdata_getMeasurementSystem did not return expected data for locale %s \n", currLoc);
                       }
                   } else if (measurementSystem != UMS_SI) {
                       log_err("ulocdata_getMeasurementSystem did not return expected data for locale %s \n", currLoc);
                   }
               }
               
               errorCode = U_ZERO_ERROR;
               ulocdata_getPaperSize(currLoc, &height, &width, &errorCode);
               if (U_FAILURE(errorCode)) {
                   log_err("ulocdata_getPaperSize failed for locale %s with error: %s \n", currLoc, u_errorName(errorCode));
               } else {
                   if ( strstr(fullLoc, "_US")!=NULL || strstr(fullLoc, "_BZ")!=NULL || strstr(fullLoc, "_CA")!=NULL || strstr(fullLoc, "_CL")!=NULL ||
                        strstr(fullLoc, "_CO")!=NULL || strstr(fullLoc, "_CR")!=NULL || strstr(fullLoc, "_GT")!=NULL || strstr(fullLoc, "_MX")!=NULL ||
                        strstr(fullLoc, "_NI")!=NULL || strstr(fullLoc, "_PA")!=NULL || strstr(fullLoc, "_PH")!=NULL || strstr(fullLoc, "_PR")!=NULL ||
                        strstr(fullLoc, "_SV")!=NULL || strstr(fullLoc, "_VE")!=NULL ) {
                       if (height != 279 || width != 216) {
                            log_err("ulocdata_getPaperSize did not return expected data for locale %s \n", currLoc);
                       }
                   } else if (height != 297 || width != 210) {
                       log_err("ulocdata_getPaperSize did not return expected data for locale %s \n", currLoc);
                   }
               }
           }
        }
        if (mergedExemplarSet != NULL) {
            uset_close(mergedExemplarSet);
        }
        ures_close(currentLocale);
    }

    ures_close(root);
}


#define MAX_SCRIPTS_PER_LOCALE 8

static void TestExemplarSet(void){
    int32_t i, j, k, m, n;
    int32_t equalCount = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UEnumeration* avail;
    USet* exemplarSets[2];
    USet* unassignedSet;
    UScriptCode code[MAX_SCRIPTS_PER_LOCALE];
    USet* codeSets[MAX_SCRIPTS_PER_LOCALE];
    int32_t codeLen;
    char cbuf[32]; 
    UChar ubuf[64]; 
    UBool existsInScript;
    int32_t itemCount;
    int32_t strLen;
    UChar32 start, end;

    unassignedSet = NULL;
    exemplarSets[0] = NULL;
    exemplarSets[1] = NULL;
    for (i=0; i<MAX_SCRIPTS_PER_LOCALE; ++i) {
        codeSets[i] = NULL;
    }

    avail = ures_openAvailableLocales(NULL, &ec);
    if (!assertSuccess("ures_openAvailableLocales", &ec)) goto END;
    n = uenum_count(avail, &ec);
    if (!assertSuccess("uenum_count", &ec)) goto END;

    u_uastrcpy(ubuf, "[:unassigned:]");
    unassignedSet = uset_openPattern(ubuf, -1, &ec);
    if (!assertSuccess("uset_openPattern", &ec)) goto END;

    for(i=0; i<n; i++){
        const char* locale = uenum_next(avail, NULL, &ec);
        if (!assertSuccess("uenum_next", &ec)) goto END;
        log_verbose("%s\n", locale);
        for (k=0; k<2; ++k) {
            uint32_t option = (k==0) ? 0 : USET_CASE_INSENSITIVE;
            ULocaleData *uld = ulocdata_open(locale,&ec);
            USet* exemplarSet = ulocdata_getExemplarSet(uld,NULL, option, ULOCDATA_ES_STANDARD, &ec);
            uset_close(exemplarSets[k]);
            ulocdata_close(uld);
            exemplarSets[k] = exemplarSet;
            if (!assertSuccess("ulocaledata_getExemplarSet", &ec)) goto END;

            if (uset_containsSome(exemplarSet, unassignedSet)) {
                log_err("ExemplarSet contains unassigned characters for locale : %s\n", locale);
            }
            codeLen = uscript_getCode(locale, code, 8, &ec);
            if (strcmp(locale, "yi") == 0 && codeLen > 0 && log_knownIssue("11217", "Fix result of uscript_getCode for yi: USCRIPT_YI -> USCRIPT_HEBREW")) {
                code[0] = USCRIPT_HEBREW;
            }
            if (!assertSuccess("uscript_getCode", &ec)) goto END;

            for (j=0; j<MAX_SCRIPTS_PER_LOCALE; ++j) {
                uset_close(codeSets[j]);
                codeSets[j] = NULL;
            }
            for (j=0; j<codeLen; ++j) {
                uprv_strcpy(cbuf, "[:");
                if(code[j]==-1){
                    log_err("USCRIPT_INVALID_CODE returned for locale: %s\n", locale);
                    continue;
                }
                uprv_strcat(cbuf, uscript_getShortName(code[j]));
                uprv_strcat(cbuf, ":]");
                u_uastrcpy(ubuf, cbuf);
                codeSets[j] = uset_openPattern(ubuf, -1, &ec);
            }
            if (!assertSuccess("uset_openPattern", &ec)) goto END;

            existsInScript = FALSE;
            itemCount = uset_getItemCount(exemplarSet);
            for (m=0; m<itemCount && !existsInScript; ++m) {
                strLen = uset_getItem(exemplarSet, m, &start, &end, ubuf,
                                      sizeof(ubuf)/sizeof(ubuf[0]), &ec);
                
                if (!assertSuccess("uset_getItem", &ec)) goto END;
                if (strLen == 0) {
                    for (j=0; j<codeLen; ++j) {
                        if (codeSets[j]!=NULL && uset_containsRange(codeSets[j], start, end)) {
                            existsInScript = TRUE;
                            break;
                        }
                    }
                } else {
                    for (j=0; j<codeLen; ++j) {
                        if (codeSets[j]!=NULL && uset_containsString(codeSets[j], ubuf, strLen)) {
                            existsInScript = TRUE;
                            break;
                        }
                    }
                }
            }

            if (existsInScript == FALSE){
                log_err("ExemplarSet containment failed for locale : %s\n", locale);
            }
        }
        assertTrue("case-folded is a superset",
                   uset_containsAll(exemplarSets[1], exemplarSets[0]));
        if (uset_equals(exemplarSets[1], exemplarSets[0])) {
            ++equalCount;
        }
    }
    

    assertTrue("case-folded is sometimes a strict superset, and sometimes equal",
               equalCount > 0 && equalCount < n);

 END:
    uenum_close(avail);
    uset_close(exemplarSets[0]);
    uset_close(exemplarSets[1]);
    uset_close(unassignedSet);
    for (i=0; i<MAX_SCRIPTS_PER_LOCALE; ++i) {
        uset_close(codeSets[i]);
    }
}

enum { kUBufMax = 32 };
static void TestLocaleDisplayPattern(void){
    UErrorCode status;
    UChar pattern[kUBufMax] = {0,};
    UChar separator[kUBufMax] = {0,};
    ULocaleData *uld;
    static const UChar enExpectPat[] = { 0x007B,0x0030,0x007D,0x0020,0x0028,0x007B,0x0031,0x007D,0x0029,0 }; 
    static const UChar enExpectSep[] = { 0x002C,0x0020,0 }; 
    static const UChar zhExpectPat[] = { 0x007B,0x0030,0x007D,0xFF08,0x007B,0x0031,0x007D,0xFF09,0 };
    static const UChar zhExpectSep[] = { 0x3001,0 };

    status = U_ZERO_ERROR;
    uld = ulocdata_open("en", &status);
    if(U_FAILURE(status)){
        log_data_err("ulocdata_open en error %s", u_errorName(status));
    } else {
        ulocdata_getLocaleDisplayPattern(uld, pattern, kUBufMax, &status);
        if (U_FAILURE(status)){
            log_err("ulocdata_getLocaleDisplayPattern en error %s", u_errorName(status));
        } else if (u_strcmp(pattern, enExpectPat) != 0) {
             log_err("ulocdata_getLocaleDisplayPattern en returns unexpected pattern");
        }
        status = U_ZERO_ERROR;
        ulocdata_getLocaleSeparator(uld, separator, kUBufMax, &status);
        if (U_FAILURE(status)){
            log_err("ulocdata_getLocaleSeparator en error %s", u_errorName(status));
        } else if (u_strcmp(separator, enExpectSep) != 0) {
             log_err("ulocdata_getLocaleSeparator en returns unexpected string ");
        }
        ulocdata_close(uld);
    }

    status = U_ZERO_ERROR;
    uld = ulocdata_open("zh", &status);
    if(U_FAILURE(status)){
        log_data_err("ulocdata_open zh error %s", u_errorName(status));
    } else {
        ulocdata_getLocaleDisplayPattern(uld, pattern, kUBufMax, &status);
        if (U_FAILURE(status)){
            log_err("ulocdata_getLocaleDisplayPattern zh error %s", u_errorName(status));
        } else if (u_strcmp(pattern, zhExpectPat) != 0) {
             log_err("ulocdata_getLocaleDisplayPattern zh returns unexpected pattern");
        }
        status = U_ZERO_ERROR;
        ulocdata_getLocaleSeparator(uld, separator, kUBufMax, &status);
        if (U_FAILURE(status)){
            log_err("ulocdata_getLocaleSeparator zh error %s", u_errorName(status));
        } else if (u_strcmp(separator, zhExpectSep) != 0) {
             log_err("ulocdata_getLocaleSeparator zh returns unexpected string ");
        }
        ulocdata_close(uld);
    }
}

static void TestCoverage(void){
    ULocaleDataDelimiterType types[] = {
     ULOCDATA_QUOTATION_START,     
     ULOCDATA_QUOTATION_END,       
     ULOCDATA_ALT_QUOTATION_START, 
     ULOCDATA_ALT_QUOTATION_END,   
     ULOCDATA_DELIMITER_COUNT
    };
    int i;
    UBool sub;
    UErrorCode status = U_ZERO_ERROR;
    ULocaleData *uld = ulocdata_open(uloc_getDefault(), &status);

    if(U_FAILURE(status)){
        log_data_err("ulocdata_open error");
        return;
    }


    for(i = 0; i < ULOCDATA_DELIMITER_COUNT; i++){
        UChar result[32] = {0,};
        status = U_ZERO_ERROR;
        ulocdata_getDelimiter(uld, types[i], result, 32, &status);
        if (U_FAILURE(status)){
            log_err("ulocdata_getgetDelimiter error with type %d", types[i]);
        }
    }

    sub = ulocdata_getNoSubstitute(uld);
    ulocdata_setNoSubstitute(uld,sub);
    ulocdata_close(uld);
}

static void TestIndexChars(void) {
    


    UErrorCode status = U_ZERO_ERROR;
    ULocaleData  *uld;
    USet *exemplarChars;
    USet *indexChars;

    uld = ulocdata_open("en", &status);
    exemplarChars = uset_openEmpty();
    indexChars = uset_openEmpty();
    ulocdata_getExemplarSet(uld, exemplarChars, 0, ULOCDATA_ES_STANDARD, &status);
    ulocdata_getExemplarSet(uld, indexChars, 0, ULOCDATA_ES_INDEX, &status);
    if (U_FAILURE(status)) {
        log_data_err("File %s, line %d, Failure opening exemplar chars: %s", __FILE__, __LINE__, u_errorName(status));
        goto close_sets;
    }
    
    
    if ((uset_contains(exemplarChars, (UChar32)0x41) || uset_contains(indexChars, (UChar32)0x61))) {
        log_err("File %s, line %d, Exemplar characters incorrect.", __FILE__, __LINE__ );
        goto close_sets;
    }
    if (!(uset_contains(exemplarChars, (UChar32)0x61) && uset_contains(indexChars, (UChar32)0x41) )) {
        log_err("File %s, line %d, Exemplar characters incorrect.", __FILE__, __LINE__ );
        goto close_sets;
    }

  close_sets:
    uset_close(exemplarChars);
    uset_close(indexChars);
    ulocdata_close(uld);
}



#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
static void TestCurrencyList(void){
#if !UCONFIG_NO_FORMATTING
    UErrorCode errorCode = U_ZERO_ERROR;
    int32_t structLocaleCount, currencyCount;
    UEnumeration *en = ucurr_openISOCurrencies(UCURR_ALL, &errorCode);
    const char *isoCode, *structISOCode;
    UResourceBundle *subBundle;
    UResourceBundle *currencies = ures_openDirect(loadTestData(&errorCode), "structLocale", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Can't open structLocale\n");
        return;
    }
    currencies = ures_getByKey(currencies, "Currencies", currencies, &errorCode);
    currencyCount = uenum_count(en, &errorCode);
    structLocaleCount = ures_getSize(currencies);
    if (currencyCount != structLocaleCount) {
        log_err("structLocale(%d) and ISO4217(%d) currency list are out of sync.\n", structLocaleCount, currencyCount);
#if U_CHARSET_FAMILY == U_ASCII_FAMILY
        ures_resetIterator(currencies);
        while ((isoCode = uenum_next(en, NULL, &errorCode)) != NULL && ures_hasNext(currencies)) {
            subBundle = ures_getNextResource(currencies, NULL, &errorCode);
            structISOCode = ures_getKey(subBundle);
            ures_close(subBundle);
            if (strcmp(structISOCode, isoCode) != 0) {
                log_err("First difference found at structLocale(%s) and ISO4217(%s).\n", structISOCode, isoCode);
                break;
            }
        }
#endif
    }
    ures_close(currencies);
    uenum_close(en);
#endif
}
#endif

static void TestAvailableIsoCodes(void){
#if !UCONFIG_NO_FORMATTING
    UErrorCode errorCode = U_ZERO_ERROR;
    const char* eurCode = "EUR";
    const char* usdCode = "USD";
    const char* lastCode = "RHD";
    const char* zzzCode = "ZZZ";
    UDate date1950 = (UDate)-630720000000.0;
    UDate date1970 = (UDate)0.0;            
    UDate date1975 = (UDate)173448000000.0; 
    UDate date1978 = (UDate)260172000000.0; 
    UDate date1981 = (UDate)346896000000.0; 
    UDate date1992 = (UDate)693792000000.0; 
    UChar* isoCode = (UChar*)malloc(sizeof(UChar) * (uprv_strlen(usdCode) + 1));

    
    u_charsToUChars(eurCode, isoCode, uprv_strlen(usdCode) + 1);
    if (ucurr_isAvailable(isoCode, U_DATE_MIN, U_DATE_MAX, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) is not found.\n", eurCode);
    }

    u_charsToUChars(usdCode, isoCode, uprv_strlen(zzzCode) + 1);
    if (ucurr_isAvailable(isoCode, U_DATE_MIN, U_DATE_MAX, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) is not found.\n", usdCode);
    }

    u_charsToUChars(zzzCode, isoCode, uprv_strlen(zzzCode) + 1);
    if (ucurr_isAvailable(isoCode, U_DATE_MIN, U_DATE_MAX, &errorCode) == TRUE) {
       log_err("FAIL: ISO code (%s) is reported as available, but it doesn't exist.\n", zzzCode);
    }

    u_charsToUChars(lastCode, isoCode, uprv_strlen(zzzCode) + 1);
    if (ucurr_isAvailable(isoCode, U_DATE_MIN, U_DATE_MAX, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) is not found.\n", lastCode);
    }

    
    
    
    if (ucurr_isAvailable(isoCode, date1970, U_DATE_MAX, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) was available in time range >1970-01-01.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, date1975, U_DATE_MAX, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) was available in time range >1975.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, date1981, U_DATE_MAX, &errorCode) == TRUE) {
       log_err("FAIL: ISO code (%s) was not available in time range >1981.\n", lastCode);
    }

    
    if (ucurr_isAvailable(isoCode, U_DATE_MIN, date1970, &errorCode) == TRUE) {
       log_err("FAIL: ISO code (%s) was not available in time range <1970.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, U_DATE_MIN, date1975, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) was available in time range <1975.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, U_DATE_MIN, date1981, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) was available in time range <1981.\n", lastCode);
    }

    
    if (ucurr_isAvailable(isoCode, date1975, date1978, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) was available in time range 1975-1978.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, date1970, date1975, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) was available in time range 1970-1975.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, date1975, date1981, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) was available in time range 1975-1981.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, date1970,  date1981, &errorCode) == FALSE) {
       log_data_err("FAIL: ISO code (%s) was available in time range 1970-1981.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, date1981,  date1992, &errorCode) == TRUE) {
       log_err("FAIL: ISO code (%s) was not available in time range 1981-1992.\n", lastCode);
    }

    if (ucurr_isAvailable(isoCode, date1950,  date1970, &errorCode) == TRUE) {
       log_err("FAIL: ISO code (%s) was not available in time range 1950-1970.\n", lastCode);
    }

    
    if (ucurr_isAvailable(isoCode, date1975,  date1970, &errorCode) == TRUE) {
       log_err("FAIL: Wrong range 1975-1970 for ISO code (%s) was not reported.\n", lastCode);
    } else if (errorCode != U_ILLEGAL_ARGUMENT_ERROR) {
       log_data_err("FAIL: Error code not reported for wrong range 1975-1970 for ISO code (%s).\n", lastCode);
    }

    free(isoCode);
#endif
}

#define TESTCASE(name) addTest(root, &name, "tsutil/cldrtest/" #name)

void addCLDRTest(TestNode** root);

void addCLDRTest(TestNode** root)
{
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
    TESTCASE(TestLocaleStructure);
    TESTCASE(TestCurrencyList);
#endif
    TESTCASE(TestConsistentCountryInfo);
    TESTCASE(VerifyTranslation);
    TESTCASE(TestExemplarSet);
    TESTCASE(TestLocaleDisplayPattern);
    TESTCASE(TestCoverage);
    TESTCASE(TestIndexChars);
    TESTCASE(TestAvailableIsoCodes);
}

