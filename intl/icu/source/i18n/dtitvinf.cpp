









#include "unicode/dtitvinf.h"


#if !UCONFIG_NO_FORMATTING





#ifdef DTITVINF_DEBUG 
#include <iostream>
#endif

#include "cstring.h"
#include "unicode/msgfmt.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "dtitv_impl.h"
#include "hash.h"
#include "gregoimp.h"
#include "uresimp.h"
#include "hash.h"
#include "gregoimp.h"
#include "uresimp.h"


U_NAMESPACE_BEGIN


#ifdef DTITVINF_DEBUG 
#define PRINTMESG(msg) { std::cout << "(" << __FILE__ << ":" << __LINE__ << ") " << msg << "\n"; }
#endif

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DateIntervalInfo)

static const char gCalendarTag[]="calendar";
static const char gGregorianTag[]="gregorian";
static const char gIntervalDateTimePatternTag[]="intervalFormats";
static const char gFallbackPatternTag[]="fallback";


static const UChar gFirstPattern[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET};

static const UChar gSecondPattern[] = {LEFT_CURLY_BRACKET, DIGIT_ONE, RIGHT_CURLY_BRACKET};


static const UChar gDefaultFallbackPattern[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, EN_DASH, SPACE, LEFT_CURLY_BRACKET, DIGIT_ONE, RIGHT_CURLY_BRACKET, 0};



DateIntervalInfo::DateIntervalInfo(UErrorCode& status) 
:   fFallbackIntervalPattern(gDefaultFallbackPattern),
    fFirstDateInPtnIsLaterDate(false),
    fIntervalPatterns(NULL)
{
    fIntervalPatterns = initHash(status);
}



DateIntervalInfo::DateIntervalInfo(const Locale& locale, UErrorCode& status)
:   fFallbackIntervalPattern(gDefaultFallbackPattern),
    fFirstDateInPtnIsLaterDate(false),
    fIntervalPatterns(NULL)
{
    initializeData(locale, status);
}



void
DateIntervalInfo::setIntervalPattern(const UnicodeString& skeleton,
                                     UCalendarDateFields lrgDiffCalUnit,
                                     const UnicodeString& intervalPattern,
                                     UErrorCode& status) {
    
    if ( lrgDiffCalUnit == UCAL_HOUR_OF_DAY ) {
        setIntervalPatternInternally(skeleton, UCAL_AM_PM, intervalPattern, status);
        setIntervalPatternInternally(skeleton, UCAL_HOUR, intervalPattern, status);
    } else if ( lrgDiffCalUnit == UCAL_DAY_OF_MONTH ||
                lrgDiffCalUnit == UCAL_DAY_OF_WEEK ) {
        setIntervalPatternInternally(skeleton, UCAL_DATE, intervalPattern, status);
    } else {
        setIntervalPatternInternally(skeleton, lrgDiffCalUnit, intervalPattern, status);
    }
}


void
DateIntervalInfo::setFallbackIntervalPattern(
                                    const UnicodeString& fallbackPattern,
                                    UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t firstPatternIndex = fallbackPattern.indexOf(gFirstPattern, 
                        sizeof(gFirstPattern)/sizeof(gFirstPattern[0]), 0);
    int32_t secondPatternIndex = fallbackPattern.indexOf(gSecondPattern, 
                        sizeof(gSecondPattern)/sizeof(gSecondPattern[0]), 0);
    if ( firstPatternIndex == -1 || secondPatternIndex == -1 ) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if ( firstPatternIndex > secondPatternIndex ) { 
        fFirstDateInPtnIsLaterDate = true;
    }
    fFallbackIntervalPattern = fallbackPattern;
}



DateIntervalInfo::DateIntervalInfo(const DateIntervalInfo& dtitvinf)
:   UObject(dtitvinf),
    fIntervalPatterns(NULL)
{
    *this = dtitvinf;
}
    


DateIntervalInfo&
DateIntervalInfo::operator=(const DateIntervalInfo& dtitvinf) {
    if ( this == &dtitvinf ) {
        return *this;
    }
    
    UErrorCode status = U_ZERO_ERROR;
    deleteHash(fIntervalPatterns);
    fIntervalPatterns = initHash(status);
    copyHash(dtitvinf.fIntervalPatterns, fIntervalPatterns, status);
    if ( U_FAILURE(status) ) {
        return *this;
    } 

    fFallbackIntervalPattern = dtitvinf.fFallbackIntervalPattern;
    fFirstDateInPtnIsLaterDate = dtitvinf.fFirstDateInPtnIsLaterDate;
    return *this;
}


DateIntervalInfo*
DateIntervalInfo::clone() const {
    return new DateIntervalInfo(*this);
}


DateIntervalInfo::~DateIntervalInfo() {
    deleteHash(fIntervalPatterns);
    fIntervalPatterns = NULL;
}


UBool
DateIntervalInfo::operator==(const DateIntervalInfo& other) const {
    UBool equal = ( 
      fFallbackIntervalPattern == other.fFallbackIntervalPattern &&
      fFirstDateInPtnIsLaterDate == other.fFirstDateInPtnIsLaterDate );

    if ( equal == TRUE ) {
        equal = fIntervalPatterns->equals(*(other.fIntervalPatterns));
    }

    return equal;
}


UnicodeString&
DateIntervalInfo::getIntervalPattern(const UnicodeString& skeleton,
                                     UCalendarDateFields field,
                                     UnicodeString& result,
                                     UErrorCode& status) const {
    if ( U_FAILURE(status) ) {
        return result;
    }
   
    const UnicodeString* patternsOfOneSkeleton = (UnicodeString*) fIntervalPatterns->get(skeleton);
    if ( patternsOfOneSkeleton != NULL ) {
        IntervalPatternIndex index = calendarFieldToIntervalIndex(field, status);
        if ( U_FAILURE(status) ) {
            return result;
        }
        const UnicodeString& intervalPattern =  patternsOfOneSkeleton[index];
        if ( !intervalPattern.isEmpty() ) {
            result = intervalPattern;
        }
    }
    return result;
}


UBool
DateIntervalInfo::getDefaultOrder() const {
    return fFirstDateInPtnIsLaterDate;
}


UnicodeString&
DateIntervalInfo::getFallbackIntervalPattern(UnicodeString& result) const {
    result = fFallbackIntervalPattern;
    return result;
}

#define ULOC_LOCALE_IDENTIFIER_CAPACITY (ULOC_FULLNAME_CAPACITY + 1 + ULOC_KEYWORD_AND_VALUES_CAPACITY)

void 
DateIntervalInfo::initializeData(const Locale& locale, UErrorCode& err)
{
  fIntervalPatterns = initHash(err);
  if ( U_FAILURE(err) ) {
      return;
  }
  const char *locName = locale.getName();
  char parentLocale[ULOC_FULLNAME_CAPACITY];
  uprv_strcpy(parentLocale, locName);
  UErrorCode status = U_ZERO_ERROR;
  Hashtable skeletonSet(FALSE, status);
  if ( U_FAILURE(status) ) {
      return;
  }

  
  const char * calendarTypeToUse = gGregorianTag; 
  char         calendarType[ULOC_KEYWORDS_CAPACITY]; 
  char         localeWithCalendarKey[ULOC_LOCALE_IDENTIFIER_CAPACITY];
  
  (void)ures_getFunctionalEquivalent(localeWithCalendarKey, ULOC_LOCALE_IDENTIFIER_CAPACITY, NULL,
                                     "calendar", "calendar", locName, NULL, FALSE, &status);
  localeWithCalendarKey[ULOC_LOCALE_IDENTIFIER_CAPACITY-1] = 0; 
  
  int32_t calendarTypeLen = uloc_getKeywordValue(localeWithCalendarKey, "calendar", calendarType, ULOC_KEYWORDS_CAPACITY, &status);
  if (U_SUCCESS(status) && calendarTypeLen < ULOC_KEYWORDS_CAPACITY) {
    calendarTypeToUse = calendarType;
  }
  status = U_ZERO_ERROR;
  
  do {
    UResourceBundle *rb, *calBundle, *calTypeBundle, *itvDtPtnResource;
    rb = ures_open(NULL, parentLocale, &status);
    if ( U_FAILURE(status) ) {
        break;
    }
    calBundle = ures_getByKey(rb, gCalendarTag, NULL, &status); 
    calTypeBundle = ures_getByKey(calBundle, calendarTypeToUse, NULL, &status);
    itvDtPtnResource = ures_getByKeyWithFallback(calTypeBundle, 
                         gIntervalDateTimePatternTag, NULL, &status);

    if ( U_SUCCESS(status) ) {
        
        const UChar* resStr;
        int32_t resStrLen = 0;
        resStr = ures_getStringByKeyWithFallback(itvDtPtnResource, 
                                             gFallbackPatternTag, 
                                             &resStrLen, &status);
        if ( U_SUCCESS(status) ) {
            UnicodeString pattern = UnicodeString(TRUE, resStr, resStrLen);
            setFallbackIntervalPattern(pattern, status);
        }

        int32_t size = ures_getSize(itvDtPtnResource);
        int32_t index;
        for ( index = 0; index < size; ++index ) {
            LocalUResourceBundlePointer oneRes(ures_getByIndex(itvDtPtnResource, index, 
                                                     NULL, &status));
            if ( U_SUCCESS(status) ) {
                const char* skeleton = ures_getKey(oneRes.getAlias());
                if (skeleton == NULL) {
                    continue;
                }
                UnicodeString skeletonUniStr(skeleton, -1, US_INV);
                if ( skeletonSet.geti(skeletonUniStr) == 1 ) {
                    continue;
                }
                skeletonSet.puti(skeletonUniStr, 1, status);
                if ( uprv_strcmp(skeleton, gFallbackPatternTag) == 0 ) {
                    continue;  
                }

                LocalUResourceBundlePointer intervalPatterns(ures_getByKey(
                                     itvDtPtnResource, skeleton, NULL, &status));

                if ( U_FAILURE(status) ) {
                    break;
                }
                if ( intervalPatterns == NULL ) {
                    continue;
                }

                const char* key;
                int32_t ptnNum = ures_getSize(intervalPatterns.getAlias());
                int32_t ptnIndex;
                for ( ptnIndex = 0; ptnIndex < ptnNum; ++ptnIndex ) {
                    UnicodeString pattern =
                        ures_getNextUnicodeString(intervalPatterns.getAlias(), &key, &status);
                    if ( U_FAILURE(status) ) {
                        break;
                    }
        
                    UCalendarDateFields calendarField = UCAL_FIELD_COUNT;
                    if ( !uprv_strcmp(key, "y") ) {
                        calendarField = UCAL_YEAR;
                    } else if ( !uprv_strcmp(key, "M") ) {
                        calendarField = UCAL_MONTH;
                    } else if ( !uprv_strcmp(key, "d") ) {
                        calendarField = UCAL_DATE;
                    } else if ( !uprv_strcmp(key, "a") ) {
                        calendarField = UCAL_AM_PM;
                    } else if ( !uprv_strcmp(key, "h") || !uprv_strcmp(key, "H") ) {
                        calendarField = UCAL_HOUR;
                    } else if ( !uprv_strcmp(key, "m") ) {
                        calendarField = UCAL_MINUTE;
                    }
                    if ( calendarField != UCAL_FIELD_COUNT ) {
                        setIntervalPatternInternally(skeletonUniStr, calendarField, pattern,status);
                    }
                }
            }
        }
    }
    ures_close(itvDtPtnResource);
    ures_close(calTypeBundle);
    ures_close(calBundle);

    status = U_ZERO_ERROR;
    
    
    
    int32_t locNameLen;
    const UChar * parentUName = ures_getStringByKey(rb, "%%Parent", &locNameLen, &status);
    if (U_SUCCESS(status) && status != U_USING_FALLBACK_WARNING && locNameLen < ULOC_FULLNAME_CAPACITY) {
        u_UCharsToChars(parentUName, parentLocale, locNameLen + 1);
    } else {
        status = U_ZERO_ERROR;
        
        const char *curLocaleName=ures_getLocaleByType(rb, ULOC_ACTUAL_LOCALE, &status);
        if ( U_FAILURE(status) ) {
            curLocaleName = parentLocale;
            status = U_ZERO_ERROR;
        }
        uloc_getParent(curLocaleName, parentLocale, ULOC_FULLNAME_CAPACITY, &status);
        if (U_FAILURE(err) || err == U_STRING_NOT_TERMINATED_WARNING) {
            parentLocale[0] = 0; 
            status = U_ZERO_ERROR;
        }
    }
    
    ures_close(rb);
    
    
    
  } while ( parentLocale[0] != 0 && uprv_strcmp(parentLocale,"root")!=0 );
}



void
DateIntervalInfo::setIntervalPatternInternally(const UnicodeString& skeleton,
                                      UCalendarDateFields lrgDiffCalUnit,
                                      const UnicodeString& intervalPattern,
                                      UErrorCode& status) {
    IntervalPatternIndex index = calendarFieldToIntervalIndex(lrgDiffCalUnit,status);
    if ( U_FAILURE(status) ) {
        return;
    }
    UnicodeString* patternsOfOneSkeleton = (UnicodeString*)(fIntervalPatterns->get(skeleton));
    UBool emptyHash = false;
    if ( patternsOfOneSkeleton == NULL ) {
        patternsOfOneSkeleton = new UnicodeString[kIPI_MAX_INDEX];
        emptyHash = true;
    }
    
    patternsOfOneSkeleton[index] = intervalPattern;
    if ( emptyHash == TRUE ) {
        fIntervalPatterns->put(skeleton, patternsOfOneSkeleton, status);
    }
}



void 
DateIntervalInfo::parseSkeleton(const UnicodeString& skeleton, 
                                int32_t* skeletonFieldWidth) {
    const int8_t PATTERN_CHAR_BASE = 0x41;
    int32_t i;
    for ( i = 0; i < skeleton.length(); ++i ) {
        
        int8_t ch = (int8_t)skeleton.charAt(i);  
        ++skeletonFieldWidth[ch - PATTERN_CHAR_BASE];
    }
}



UBool 
DateIntervalInfo::stringNumeric(int32_t fieldWidth, int32_t anotherFieldWidth,
                                char patternLetter) {
    if ( patternLetter == 'M' ) {
        if ( (fieldWidth <= 2 && anotherFieldWidth > 2) ||
             (fieldWidth > 2 && anotherFieldWidth <= 2 )) {
            return true;
        }
    }
    return false;
}



const UnicodeString* 
DateIntervalInfo::getBestSkeleton(const UnicodeString& skeleton,
                                  int8_t& bestMatchDistanceInfo) const {
#ifdef DTITVINF_DEBUG
    char result[1000];
    char result_1[1000];
    char mesg[2000];
    skeleton.extract(0,  skeleton.length(), result, "UTF-8");
    sprintf(mesg, "in getBestSkeleton: skeleton: %s; \n", result);
    PRINTMESG(mesg)
#endif


    int32_t inputSkeletonFieldWidth[] =
    {
    
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0, 0, 0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    };

    int32_t skeletonFieldWidth[] =
    {
    
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0, 0, 0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    };

    const int32_t DIFFERENT_FIELD = 0x1000;
    const int32_t STRING_NUMERIC_DIFFERENCE = 0x100;
    const int32_t BASE = 0x41;
    const UChar CHAR_V = 0x0076;
    const UChar CHAR_Z = 0x007A;

    
    
    
    UBool replaceZWithV = false;
    const UnicodeString* inputSkeleton = &skeleton; 
    UnicodeString copySkeleton;
    if ( skeleton.indexOf(CHAR_Z) != -1 ) {
        copySkeleton = skeleton;
        copySkeleton.findAndReplace(UnicodeString(CHAR_Z), UnicodeString(CHAR_V));
        inputSkeleton = &copySkeleton;
        replaceZWithV = true;
    }

    parseSkeleton(*inputSkeleton, inputSkeletonFieldWidth);
    int32_t bestDistance = MAX_POSITIVE_INT;
    const UnicodeString* bestSkeleton = NULL;

    
    
    
    
    bestMatchDistanceInfo = 0;
    int8_t fieldLength = sizeof(skeletonFieldWidth)/sizeof(skeletonFieldWidth[0]);

    int32_t pos = -1;
    const UHashElement* elem = NULL;
    while ( (elem = fIntervalPatterns->nextElement(pos)) != NULL ) {
        const UHashTok keyTok = elem->key;
        UnicodeString* skeleton = (UnicodeString*)keyTok.pointer;
#ifdef DTITVINF_DEBUG
    skeleton->extract(0,  skeleton->length(), result, "UTF-8");
    sprintf(mesg, "available skeletons: skeleton: %s; \n", result);
    PRINTMESG(mesg)
#endif

        
        int8_t i;
        for ( i = 0; i < fieldLength; ++i ) {
            skeletonFieldWidth[i] = 0;    
        }
        parseSkeleton(*skeleton, skeletonFieldWidth);
        
        int32_t distance = 0;
        int8_t fieldDifference = 1;
        for ( i = 0; i < fieldLength; ++i ) {
            int32_t inputFieldWidth = inputSkeletonFieldWidth[i];
            int32_t fieldWidth = skeletonFieldWidth[i];
            if ( inputFieldWidth == fieldWidth ) {
                continue;
            }
            if ( inputFieldWidth == 0 ) {
                fieldDifference = -1;
                distance += DIFFERENT_FIELD;
            } else if ( fieldWidth == 0 ) {
                fieldDifference = -1;
                distance += DIFFERENT_FIELD;
            } else if (stringNumeric(inputFieldWidth, fieldWidth, 
                                     (char)(i+BASE) ) ) {
                distance += STRING_NUMERIC_DIFFERENCE;
            } else {
                distance += (inputFieldWidth > fieldWidth) ? 
                            (inputFieldWidth - fieldWidth) : 
                            (fieldWidth - inputFieldWidth);
            }
        }
        if ( distance < bestDistance ) {
            bestSkeleton = skeleton;
            bestDistance = distance;
            bestMatchDistanceInfo = fieldDifference;
        }
        if ( distance == 0 ) {
            bestMatchDistanceInfo = 0;
            break;
        }
    }
    if ( replaceZWithV && bestMatchDistanceInfo != -1 ) {
        bestMatchDistanceInfo = 2;
    }
    return bestSkeleton;
}



DateIntervalInfo::IntervalPatternIndex
DateIntervalInfo::calendarFieldToIntervalIndex(UCalendarDateFields field, 
                                               UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return kIPI_MAX_INDEX;
    }
    IntervalPatternIndex index = kIPI_MAX_INDEX;
    switch ( field ) {
      case UCAL_ERA:
        index = kIPI_ERA;
        break;
      case UCAL_YEAR:
        index = kIPI_YEAR;
        break;
      case UCAL_MONTH:
        index = kIPI_MONTH;
        break;
      case UCAL_DATE:
      case UCAL_DAY_OF_WEEK:
      
        index = kIPI_DATE;
        break;
      case UCAL_AM_PM:
        index = kIPI_AM_PM;
        break;
      case UCAL_HOUR:
      case UCAL_HOUR_OF_DAY:
        index = kIPI_HOUR;
        break;
      case UCAL_MINUTE:
        index = kIPI_MINUTE;
        break;
      default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return index;
}



void
DateIntervalInfo::deleteHash(Hashtable* hTable) 
{
    if ( hTable == NULL ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    while ( (element = hTable->nextElement(pos)) != NULL ) {
        const UHashTok valueTok = element->value;
        const UnicodeString* value = (UnicodeString*)valueTok.pointer;
        delete[] value;
    }
    delete fIntervalPatterns;
}


U_CDECL_BEGIN 








static UBool U_CALLCONV dtitvinfHashTableValueComparator(UHashTok val1, UHashTok val2);

static UBool 
U_CALLCONV dtitvinfHashTableValueComparator(UHashTok val1, UHashTok val2) {
    const UnicodeString* pattern1 = (UnicodeString*)val1.pointer;
    const UnicodeString* pattern2 = (UnicodeString*)val2.pointer;
    UBool ret = TRUE;
    int8_t i;
    for ( i = 0; i < DateIntervalInfo::kMaxIntervalPatternIndex && ret == TRUE; ++i ) {
        ret = (pattern1[i] == pattern2[i]);
    }
    return ret;
}

U_CDECL_END


Hashtable*
DateIntervalInfo::initHash(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(FALSE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if ( U_FAILURE(status) ) {
        delete hTable; 
        return NULL;
    }
    hTable->setValueComparator(dtitvinfHashTableValueComparator);
    return hTable;
}


void
DateIntervalInfo::copyHash(const Hashtable* source,
                           Hashtable* target,
                           UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;
            const UHashTok valueTok = element->value;
            const UnicodeString* value = (UnicodeString*)valueTok.pointer;
            UnicodeString* copy = new UnicodeString[kIPI_MAX_INDEX];
            int8_t i;
            for ( i = 0; i < kIPI_MAX_INDEX; ++i ) {
                copy[i] = value[i];
            }
            target->put(UnicodeString(*key), copy, status);
            if ( U_FAILURE(status) ) {
                return;
            }
        }
    }
}


U_NAMESPACE_END

#endif
