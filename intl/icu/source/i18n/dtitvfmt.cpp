









#include "utypeinfo.h"  

#include "unicode/dtitvfmt.h"

#if !UCONFIG_NO_FORMATTING




#include "cstring.h"
#include "unicode/msgfmt.h"
#include "unicode/dtptngen.h"
#include "unicode/dtitvinf.h"
#include "unicode/calendar.h"
#include "dtitv_impl.h"

#ifdef DTITVFMT_DEBUG 
#include <iostream>
#include "cstring.h"
#endif

#include "gregoimp.h"

U_NAMESPACE_BEGIN



#ifdef DTITVFMT_DEBUG 
#define PRINTMESG(msg) { std::cout << "(" << __FILE__ << ":" << __LINE__ << ") " << msg << "\n"; }
#endif


static const UChar gDateFormatSkeleton[][11] = {

{LOW_Y, CAP_M, CAP_M, CAP_M, CAP_M, CAP_E, CAP_E, CAP_E, CAP_E, LOW_D, 0},

{LOW_Y, CAP_M, CAP_M, CAP_M, CAP_M, LOW_D, 0},

{LOW_Y, CAP_M, CAP_M, CAP_M, LOW_D, 0},

{LOW_Y, CAP_M, LOW_D, 0} };


static const char gDateTimePatternsTag[]="DateTimePatterns";



static const UChar gLaterFirstPrefix[] = {LOW_L, LOW_A, LOW_T, LOW_E, LOW_S,LOW_T, CAP_F, LOW_I, LOW_R, LOW_S, LOW_T, COLON};


static const UChar gEarlierFirstPrefix[] = {LOW_E, LOW_A, LOW_R, LOW_L, LOW_I, LOW_E, LOW_S, LOW_T, CAP_F, LOW_I, LOW_R, LOW_S, LOW_T, COLON};


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DateIntervalFormat)



DateIntervalFormat* U_EXPORT2
DateIntervalFormat::createInstance(const UnicodeString& skeleton, 
                                   UErrorCode& status) {
    return createInstance(skeleton, Locale::getDefault(), status);
}


DateIntervalFormat* U_EXPORT2
DateIntervalFormat::createInstance(const UnicodeString& skeleton, 
                                   const Locale& locale, 
                                   UErrorCode& status) {
#ifdef DTITVFMT_DEBUG
    char result[1000];
    char result_1[1000];
    char mesg[2000];
    skeleton.extract(0,  skeleton.length(), result, "UTF-8");
    UnicodeString pat;
    ((SimpleDateFormat*)dtfmt)->toPattern(pat);
    pat.extract(0,  pat.length(), result_1, "UTF-8");
    sprintf(mesg, "skeleton: %s; pattern: %s\n", result, result_1);
    PRINTMESG(mesg)
#endif

    DateIntervalInfo* dtitvinf = new DateIntervalInfo(locale, status);
    return create(locale, dtitvinf, &skeleton, status);
}



DateIntervalFormat* U_EXPORT2
DateIntervalFormat::createInstance(const UnicodeString& skeleton,
                                   const DateIntervalInfo& dtitvinf,
                                   UErrorCode& status) {
    return createInstance(skeleton, Locale::getDefault(), dtitvinf, status);
}


DateIntervalFormat* U_EXPORT2
DateIntervalFormat::createInstance(const UnicodeString& skeleton,
                                   const Locale& locale,
                                   const DateIntervalInfo& dtitvinf,
                                   UErrorCode& status) {
    DateIntervalInfo* ptn = dtitvinf.clone();
    return create(locale, ptn, &skeleton, status);
}


DateIntervalFormat::DateIntervalFormat()
:   fInfo(NULL),
    fDateFormat(NULL),
    fFromCalendar(NULL),
    fToCalendar(NULL),
    fDtpng(NULL)
{}


DateIntervalFormat::DateIntervalFormat(const DateIntervalFormat& itvfmt)
:   Format(itvfmt),
    fInfo(NULL),
    fDateFormat(NULL),
    fFromCalendar(NULL),
    fToCalendar(NULL),
    fDtpng(NULL) {
    *this = itvfmt;
}


DateIntervalFormat&
DateIntervalFormat::operator=(const DateIntervalFormat& itvfmt) {
    if ( this != &itvfmt ) {
        delete fDateFormat;
        delete fInfo;
        delete fFromCalendar;
        delete fToCalendar;
        delete fDtpng;
        if ( itvfmt.fDateFormat ) {
            fDateFormat = (SimpleDateFormat*)itvfmt.fDateFormat->clone();
        } else {
            fDateFormat = NULL;
        }
        if ( itvfmt.fInfo ) {
            fInfo = itvfmt.fInfo->clone();
        } else {
            fInfo = NULL;
        }
        if ( itvfmt.fFromCalendar ) {
            fFromCalendar = itvfmt.fFromCalendar->clone();
        } else {
            fFromCalendar = NULL;
        }
        if ( itvfmt.fToCalendar ) {
            fToCalendar = itvfmt.fToCalendar->clone();
        } else {
            fToCalendar = NULL;
        }
        fSkeleton = itvfmt.fSkeleton;
        int8_t i;
        for ( i = 0; i< DateIntervalInfo::kIPI_MAX_INDEX; ++i ) {
            fIntervalPatterns[i] = itvfmt.fIntervalPatterns[i];
        }
        if (itvfmt.fDtpng) {
            fDtpng = itvfmt.fDtpng->clone();
        }
    }
    return *this;
}


DateIntervalFormat::~DateIntervalFormat() {
    delete fInfo;
    delete fDateFormat;
    delete fFromCalendar;
    delete fToCalendar;
    delete fDtpng;
}


Format*
DateIntervalFormat::clone(void) const {
    return new DateIntervalFormat(*this);
}


UBool
DateIntervalFormat::operator==(const Format& other) const {
    if (typeid(*this) == typeid(other)) {
        const DateIntervalFormat* fmt = (DateIntervalFormat*)&other;
#ifdef DTITVFMT_DEBUG
    UBool equal;
    equal = (this == fmt);
    
    equal = (*fInfo == *fmt->fInfo);
    equal = (*fDateFormat == *fmt->fDateFormat);
    equal = fFromCalendar->isEquivalentTo(*fmt->fFromCalendar) ;
    equal = fToCalendar->isEquivalentTo(*fmt->fToCalendar) ;
    equal = (fSkeleton == fmt->fSkeleton);
#endif
        UBool res;
        res =  ( this == fmt ) ||
               ( Format::operator==(other) && 
                 fInfo && 
                 ( *fInfo == *fmt->fInfo ) &&
                 fDateFormat &&
                 ( *fDateFormat == *fmt->fDateFormat ) &&
                 fFromCalendar &&
                 fFromCalendar->isEquivalentTo(*fmt->fFromCalendar) &&
                 fToCalendar &&
                 fToCalendar->isEquivalentTo(*fmt->fToCalendar) &&
                 fSkeleton == fmt->fSkeleton &&
                 fDtpng &&
                 (*fDtpng == *fmt->fDtpng) );
        int8_t i;
        for (i = 0; i< DateIntervalInfo::kIPI_MAX_INDEX && res == TRUE; ++i ) {
            res =   ( fIntervalPatterns[i].firstPart ==
                      fmt->fIntervalPatterns[i].firstPart) &&
                    ( fIntervalPatterns[i].secondPart ==
                      fmt->fIntervalPatterns[i].secondPart ) &&
                    ( fIntervalPatterns[i].laterDateFirst ==
                      fmt->fIntervalPatterns[i].laterDateFirst) ;
        }
        return res;
    } 
    return FALSE;
}



UnicodeString&
DateIntervalFormat::format(const Formattable& obj,
                           UnicodeString& appendTo,
                           FieldPosition& fieldPosition,
                           UErrorCode& status) const {
    if ( U_FAILURE(status) ) {
        return appendTo;
    }

    if ( obj.getType() == Formattable::kObject ) {
        const UObject* formatObj = obj.getObject();
        const DateInterval* interval = dynamic_cast<const DateInterval*>(formatObj);
        if (interval != NULL){
            return format(interval, appendTo, fieldPosition, status);
        }
    }
    status = U_ILLEGAL_ARGUMENT_ERROR;
    return appendTo;
}


UnicodeString&
DateIntervalFormat::format(const DateInterval* dtInterval,
                           UnicodeString& appendTo,
                           FieldPosition& fieldPosition,
                           UErrorCode& status) const {
    if ( U_FAILURE(status) ) {
        return appendTo;
    }

    if ( fFromCalendar != NULL && fToCalendar != NULL && 
         fDateFormat != NULL && fInfo != NULL ) {
        fFromCalendar->setTime(dtInterval->getFromDate(), status);
        fToCalendar->setTime(dtInterval->getToDate(), status);
        if ( U_SUCCESS(status) ) {
            return format(*fFromCalendar, *fToCalendar, appendTo,fieldPosition, status);
        }
    }
    return appendTo;
}


UnicodeString&
DateIntervalFormat::format(Calendar& fromCalendar,
                           Calendar& toCalendar,
                           UnicodeString& appendTo,
                           FieldPosition& pos,
                           UErrorCode& status) const {
    if ( U_FAILURE(status) ) {
        return appendTo;
    }

    
    
    if ( !fromCalendar.isEquivalentTo(toCalendar) ) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    
    UCalendarDateFields field = UCAL_FIELD_COUNT;

    if ( fromCalendar.get(UCAL_ERA,status) != toCalendar.get(UCAL_ERA,status)) {
        field = UCAL_ERA;
    } else if ( fromCalendar.get(UCAL_YEAR, status) != 
                toCalendar.get(UCAL_YEAR, status) ) {
        field = UCAL_YEAR;
    } else if ( fromCalendar.get(UCAL_MONTH, status) !=
                toCalendar.get(UCAL_MONTH, status) ) {
        field = UCAL_MONTH;
    } else if ( fromCalendar.get(UCAL_DATE, status) !=
                toCalendar.get(UCAL_DATE, status) ) {
        field = UCAL_DATE;
    } else if ( fromCalendar.get(UCAL_AM_PM, status) !=
                toCalendar.get(UCAL_AM_PM, status) ) {
        field = UCAL_AM_PM;
    } else if ( fromCalendar.get(UCAL_HOUR, status) !=
                toCalendar.get(UCAL_HOUR, status) ) {
        field = UCAL_HOUR;
    } else if ( fromCalendar.get(UCAL_MINUTE, status) !=
                toCalendar.get(UCAL_MINUTE, status) ) {
        field = UCAL_MINUTE;
    }

    if ( U_FAILURE(status) ) {
        return appendTo;
    }
    if ( field == UCAL_FIELD_COUNT ) {
        


        return fDateFormat->format(fromCalendar, appendTo, pos);
    }
    
    
    
    int32_t itvPtnIndex = DateIntervalInfo::calendarFieldToIntervalIndex(field,
                                                                        status);
    const PatternInfo& intervalPattern = fIntervalPatterns[itvPtnIndex];

    if ( intervalPattern.firstPart.isEmpty() &&
         intervalPattern.secondPart.isEmpty() ) {
        if ( fDateFormat->isFieldUnitIgnored(field) ) {
            



            return fDateFormat->format(fromCalendar, appendTo, pos);
        }
        return fallbackFormat(fromCalendar, toCalendar, appendTo, pos, status);
    }
    
    
    
    if ( intervalPattern.firstPart.isEmpty() ) {
        
        UnicodeString originalPattern;
        fDateFormat->toPattern(originalPattern);
        fDateFormat->applyPattern(intervalPattern.secondPart);
        appendTo = fallbackFormat(fromCalendar, toCalendar, appendTo, pos, status);
        fDateFormat->applyPattern(originalPattern);
        return appendTo;
    }
    Calendar* firstCal;
    Calendar* secondCal;
    if ( intervalPattern.laterDateFirst ) {
        firstCal = &toCalendar;
        secondCal = &fromCalendar;
    } else {
        firstCal = &fromCalendar;
        secondCal = &toCalendar;
    }
    
    
    UnicodeString originalPattern;
    fDateFormat->toPattern(originalPattern);
    fDateFormat->applyPattern(intervalPattern.firstPart);
    fDateFormat->format(*firstCal, appendTo, pos);
    if ( !intervalPattern.secondPart.isEmpty() ) {
        fDateFormat->applyPattern(intervalPattern.secondPart);
        fDateFormat->format(*secondCal, appendTo, pos);
    }
    fDateFormat->applyPattern(originalPattern);
    return appendTo;
}



void
DateIntervalFormat::parseObject(const UnicodeString& , 
                                Formattable& ,
                                ParsePosition& ) const {
    
    
    
}




const DateIntervalInfo*
DateIntervalFormat::getDateIntervalInfo() const {
    return fInfo;
}


void
DateIntervalFormat::setDateIntervalInfo(const DateIntervalInfo& newItvPattern,
                                        UErrorCode& status) {
    delete fInfo;
    fInfo = new DateIntervalInfo(newItvPattern);
    if ( fDateFormat ) {
        initializePattern(status);
    }
}


 
const DateFormat*
DateIntervalFormat::getDateFormat() const {
    return fDateFormat;
}


void
DateIntervalFormat::adoptTimeZone(TimeZone* zone)
{
    if (fDateFormat != NULL) {
        fDateFormat->adoptTimeZone(zone);
    }
    
    
    
    
    if (fFromCalendar) {
    	fFromCalendar->setTimeZone(*zone);
    }
    if (fToCalendar) {
    	fToCalendar->setTimeZone(*zone);
    }
}

void
DateIntervalFormat::setTimeZone(const TimeZone& zone)
{
    if (fDateFormat != NULL) {
        fDateFormat->setTimeZone(zone);
    }
    
    
    if (fFromCalendar) {
    	fFromCalendar->setTimeZone(zone);
    }
    if (fToCalendar) {
    	fToCalendar->setTimeZone(zone);
    }
}

const TimeZone&
DateIntervalFormat::getTimeZone() const
{
    if (fDateFormat != NULL) {
        return fDateFormat->getTimeZone();
    }
    
    return *(TimeZone::createDefault());
}

DateIntervalFormat::DateIntervalFormat(const Locale& locale,
                                       DateIntervalInfo* dtItvInfo,
                                       const UnicodeString* skeleton,
                                       UErrorCode& status) 
:   fInfo(NULL),
    fDateFormat(NULL),
    fFromCalendar(NULL),
    fToCalendar(NULL),
    fDtpng(NULL)
{
    if ( U_FAILURE(status) ) {
        delete dtItvInfo;
        return;
    }
    fDtpng = DateTimePatternGenerator::createInstance(locale, status);
    SimpleDateFormat* dtfmt = createSDFPatternInstance(*skeleton, locale, 
                                                    fDtpng, status);
    if ( U_FAILURE(status) ) {
        delete dtItvInfo;
        delete fDtpng;
        delete dtfmt;
        return;
    }
    if ( dtfmt == NULL || dtItvInfo == NULL || fDtpng == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        
        delete dtfmt;
        delete dtItvInfo;
        delete fDtpng;
        return;
    }
    if ( skeleton ) {
        fSkeleton = *skeleton;
    }
    fInfo = dtItvInfo;
    fDateFormat = dtfmt;
    if ( dtfmt->getCalendar() ) {
        fFromCalendar = dtfmt->getCalendar()->clone();
        fToCalendar = dtfmt->getCalendar()->clone();
    } else {
        fFromCalendar = NULL;
        fToCalendar = NULL;
    }
    initializePattern(status);
}


SimpleDateFormat* U_EXPORT2
DateIntervalFormat::createSDFPatternInstance(const UnicodeString& skeleton,
                                             const Locale& locale,
                                             DateTimePatternGenerator* dtpng,
                                             UErrorCode& status)
{
    if ( U_FAILURE(status) ) {
        return NULL;
    }

    const UnicodeString pattern = dtpng->getBestPattern(skeleton, status);
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    SimpleDateFormat* dtfmt = new SimpleDateFormat(pattern, locale, status);
    if ( U_FAILURE(status) ) {
        delete dtfmt;
        return NULL;
    }
    return dtfmt;
}


DateIntervalFormat* U_EXPORT2
DateIntervalFormat::create(const Locale& locale,
                           DateIntervalInfo* dtitvinf,
                           const UnicodeString* skeleton,
                           UErrorCode& status) {
    DateIntervalFormat* f = new DateIntervalFormat(locale, dtitvinf, 
                                                   skeleton, status);
    if ( f == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        delete dtitvinf;
    } else if ( U_FAILURE(status) ) {
        
        delete f;
        f = 0;
    }
    return f;
}





































void 
DateIntervalFormat::initializePattern(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    const Locale& locale = fDateFormat->getSmpFmtLocale();
    if ( fSkeleton.isEmpty() ) {
        UnicodeString fullPattern;
        fDateFormat->toPattern(fullPattern);
#ifdef DTITVFMT_DEBUG
    char result[1000];
    char result_1[1000];
    char mesg[2000];
    fSkeleton.extract(0,  fSkeleton.length(), result, "UTF-8");
    sprintf(mesg, "in getBestSkeleton: fSkeleton: %s; \n", result);
    PRINTMESG(mesg)
#endif
        
        
        fSkeleton = fDtpng->getSkeleton(fullPattern, status);
        if ( U_FAILURE(status) ) {
            return;    
        }
    }

    
    int8_t i;
    for ( i = 0; i < DateIntervalInfo::kIPI_MAX_INDEX; ++i ) {
        fIntervalPatterns[i].laterDateFirst = fInfo->getDefaultOrder();
    }

    


    UnicodeString dateSkeleton;
    UnicodeString timeSkeleton;
    UnicodeString normalizedTimeSkeleton;
    UnicodeString normalizedDateSkeleton;


    










    getDateTimeSkeleton(fSkeleton, dateSkeleton, normalizedDateSkeleton,
                        timeSkeleton, normalizedTimeSkeleton);

#ifdef DTITVFMT_DEBUG
    char result[1000];
    char result_1[1000];
    char mesg[2000];
    fSkeleton.extract(0,  fSkeleton.length(), result, "UTF-8");
    sprintf(mesg, "in getBestSkeleton: fSkeleton: %s; \n", result);
    PRINTMESG(mesg)
#endif


    UBool found = setSeparateDateTimePtn(normalizedDateSkeleton, 
                                         normalizedTimeSkeleton);

    if ( found == false ) {
        
        
        if ( timeSkeleton.length() != 0 ) {
            if ( dateSkeleton.length() == 0 ) {
                
                timeSkeleton.insert(0, gDateFormatSkeleton[DateFormat::kShort], -1);
                UnicodeString pattern = fDtpng->getBestPattern(timeSkeleton, status);
                if ( U_FAILURE(status) ) {
                    return;    
                }
                
                
                
                
                setPatternInfo(UCAL_DATE, NULL, &pattern, fInfo->getDefaultOrder()); 
                setPatternInfo(UCAL_MONTH, NULL, &pattern, fInfo->getDefaultOrder()); 
                setPatternInfo(UCAL_YEAR, NULL, &pattern, fInfo->getDefaultOrder()); 
            } else {
                
            }
        } else {
            
        }
        return;
    } 
    
    if ( timeSkeleton.length() == 0 ) {
        
    } else if ( dateSkeleton.length() == 0 ) {
        
        timeSkeleton.insert(0, gDateFormatSkeleton[DateFormat::kShort], -1);
        UnicodeString pattern = fDtpng->getBestPattern(timeSkeleton, status);
        if ( U_FAILURE(status) ) {
            return;    
        }
        
        
        
        
        setPatternInfo(UCAL_DATE, NULL, &pattern, fInfo->getDefaultOrder()); 
        setPatternInfo(UCAL_MONTH, NULL, &pattern, fInfo->getDefaultOrder()); 
        setPatternInfo(UCAL_YEAR, NULL, &pattern, fInfo->getDefaultOrder()); 
    } else {
        





        



        
        UnicodeString skeleton = fSkeleton;
        if ( !fieldExistsInSkeleton(UCAL_DATE, dateSkeleton) ) {
            
            skeleton.insert(0, LOW_D);
            setFallbackPattern(UCAL_DATE, skeleton, status);
        }
        if ( !fieldExistsInSkeleton(UCAL_MONTH, dateSkeleton) ) {
            
            skeleton.insert(0, CAP_M);
            setFallbackPattern(UCAL_MONTH, skeleton, status);
        }
        if ( !fieldExistsInSkeleton(UCAL_YEAR, dateSkeleton) ) {
            
            skeleton.insert(0, LOW_Y);
            setFallbackPattern(UCAL_YEAR, skeleton, status);
        }
        
        



        
        
        
        
        CalendarData* calData = new CalendarData(locale, NULL, status);

        if ( U_FAILURE(status) ) {
            delete calData;
            return;
        }

        if ( calData == NULL ) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
       
        const UResourceBundle* dateTimePatternsRes = calData->getByKey(
                                           gDateTimePatternsTag, status);
        int32_t dateTimeFormatLength;
        const UChar* dateTimeFormat = ures_getStringByIndex(
                                            dateTimePatternsRes,
                                            (int32_t)DateFormat::kDateTime,
                                            &dateTimeFormatLength, &status);
        if ( U_FAILURE(status) ) {
            return;
        }

        UnicodeString datePattern = fDtpng->getBestPattern(dateSkeleton, status);

        concatSingleDate2TimeInterval(dateTimeFormat, dateTimeFormatLength,
                                      datePattern, UCAL_AM_PM, status);
        concatSingleDate2TimeInterval(dateTimeFormat, dateTimeFormatLength,
                                      datePattern, UCAL_HOUR, status);
        concatSingleDate2TimeInterval(dateTimeFormat, dateTimeFormatLength,
                                      datePattern, UCAL_MINUTE, status);
        delete calData;
    }
}



void  U_EXPORT2 
DateIntervalFormat::getDateTimeSkeleton(const UnicodeString& skeleton, 
                                        UnicodeString& dateSkeleton, 
                                        UnicodeString& normalizedDateSkeleton, 
                                        UnicodeString& timeSkeleton,
                                        UnicodeString& normalizedTimeSkeleton) {
    
    
    int32_t ECount = 0;
    int32_t dCount = 0;
    int32_t MCount = 0;
    int32_t yCount = 0;
    int32_t hCount = 0;
    int32_t HCount = 0;
    int32_t mCount = 0;
    int32_t vCount = 0;
    int32_t zCount = 0;
    int32_t i;

    for (i = 0; i < skeleton.length(); ++i) {
        UChar ch = skeleton[i];
        switch ( ch ) {
          case CAP_E:
            dateSkeleton.append(ch);
            ++ECount;
            break;
          case LOW_D:
            dateSkeleton.append(ch);
            ++dCount;
            break;
          case CAP_M:
            dateSkeleton.append(ch);
            ++MCount;
            break;
          case LOW_Y:
            dateSkeleton.append(ch);
            ++yCount;
            break;
          case CAP_G:
          case CAP_Y:
          case LOW_U:
          case CAP_Q:
          case LOW_Q:
          case CAP_L:
          case LOW_L:
          case CAP_W:
          case LOW_W:
          case CAP_D:
          case CAP_F:
          case LOW_G:
          case LOW_E:
          case LOW_C:
            normalizedDateSkeleton.append(ch);
            dateSkeleton.append(ch);
            break;
          case LOW_A:
            
            timeSkeleton.append(ch);
            break;
          case LOW_H:
            timeSkeleton.append(ch);
            ++hCount;
            break;
          case CAP_H:
            timeSkeleton.append(ch);
            ++HCount;
            break;
          case LOW_M:
            timeSkeleton.append(ch);
            ++mCount;
            break;
          case LOW_Z:
            ++zCount;
            timeSkeleton.append(ch);
            break;
          case LOW_V:
            ++vCount;
            timeSkeleton.append(ch);
            break;
          case CAP_V:
          case CAP_Z:
          case LOW_K:
          case CAP_K:
          case LOW_J:
          case LOW_S:
          case CAP_S:
          case CAP_A:
            timeSkeleton.append(ch);
            normalizedTimeSkeleton.append(ch);
            break;     
        }
    }

    
    if ( yCount != 0 ) {
        normalizedDateSkeleton.append(LOW_Y);
    }
    if ( MCount != 0 ) {
        if ( MCount < 3 ) {
            normalizedDateSkeleton.append(CAP_M);
        } else {
            int32_t i;
            for ( i = 0; i < MCount && i < MAX_M_COUNT; ++i ) {
                 normalizedDateSkeleton.append(CAP_M);
            }
        }
    }
    if ( ECount != 0 ) {
        if ( ECount <= 3 ) {
            normalizedDateSkeleton.append(CAP_E);
        } else {
            int32_t i;
            for ( i = 0; i < ECount && i < MAX_E_COUNT; ++i ) {
                 normalizedDateSkeleton.append(CAP_E);
            }
        }
    }
    if ( dCount != 0 ) {
        normalizedDateSkeleton.append(LOW_D);
    }

    
    if ( HCount != 0 ) {
        normalizedTimeSkeleton.append(CAP_H);
    }
    else if ( hCount != 0 ) {
        normalizedTimeSkeleton.append(LOW_H);
    }
    if ( mCount != 0 ) {
        normalizedTimeSkeleton.append(LOW_M);
    }
    if ( zCount != 0 ) {
        normalizedTimeSkeleton.append(LOW_Z);
    }
    if ( vCount != 0 ) {
        normalizedTimeSkeleton.append(LOW_V);
    }
}


























UBool 
DateIntervalFormat::setSeparateDateTimePtn(
                                 const UnicodeString& dateSkeleton,
                                 const UnicodeString& timeSkeleton) {
    const UnicodeString* skeleton;
    
    
    
    
    
    if ( timeSkeleton.length() != 0  ) {
        skeleton = &timeSkeleton;
    } else {
        skeleton = &dateSkeleton;
    }

    







    
    int8_t differenceInfo = 0;
    const UnicodeString* bestSkeleton = fInfo->getBestSkeleton(*skeleton, 
                                                               differenceInfo);
    








    if ( bestSkeleton == NULL ) {
        return false; 
    } 
   
    
    
    
    
    
    if ( differenceInfo == -1 ) { 
        
        return false;
    }

    if ( timeSkeleton.length() == 0 ) {
        UnicodeString extendedSkeleton;
        UnicodeString extendedBestSkeleton;
        
        setIntervalPattern(UCAL_DATE, skeleton, bestSkeleton, differenceInfo,
                           &extendedSkeleton, &extendedBestSkeleton);

        UBool extended = setIntervalPattern(UCAL_MONTH, skeleton, bestSkeleton, 
                                     differenceInfo,
                                     &extendedSkeleton, &extendedBestSkeleton);
                                              
        if ( extended ) {
            bestSkeleton = &extendedBestSkeleton;
            skeleton = &extendedSkeleton;
        }
        setIntervalPattern(UCAL_YEAR, skeleton, bestSkeleton, differenceInfo,
                           &extendedSkeleton, &extendedBestSkeleton);
    } else {
        setIntervalPattern(UCAL_MINUTE, skeleton, bestSkeleton, differenceInfo);
        setIntervalPattern(UCAL_HOUR, skeleton, bestSkeleton, differenceInfo);
        setIntervalPattern(UCAL_AM_PM, skeleton, bestSkeleton, differenceInfo);
    }
    return true;
}



void
DateIntervalFormat::setFallbackPattern(UCalendarDateFields field,
                                       const UnicodeString& skeleton,
                                       UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    UnicodeString pattern = fDtpng->getBestPattern(skeleton, status);
    if ( U_FAILURE(status) ) {
        return;
    }
    setPatternInfo(field, NULL, &pattern, fInfo->getDefaultOrder());
}




void
DateIntervalFormat::setPatternInfo(UCalendarDateFields field, 
                                   const UnicodeString* firstPart,
                                   const UnicodeString* secondPart, 
                                   UBool laterDateFirst) {
    
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    
    int32_t itvPtnIndex = DateIntervalInfo::calendarFieldToIntervalIndex(field,
                                                                        status);
    if ( U_FAILURE(status) ) {
        return;
    }
    PatternInfo& ptn = fIntervalPatterns[itvPtnIndex];
    if ( firstPart ) {
        ptn.firstPart = *firstPart;
    }
    if ( secondPart ) {
        ptn.secondPart = *secondPart;
    }
    ptn.laterDateFirst = laterDateFirst;
}

void
DateIntervalFormat::setIntervalPattern(UCalendarDateFields field,
                                       const UnicodeString& intervalPattern) {
    UBool order = fInfo->getDefaultOrder();
    setIntervalPattern(field, intervalPattern, order);
}


void
DateIntervalFormat::setIntervalPattern(UCalendarDateFields field,
                                       const UnicodeString& intervalPattern,
                                       UBool laterDateFirst) {
    const UnicodeString* pattern = &intervalPattern;
    UBool order = laterDateFirst;
    
    int8_t prefixLength = sizeof(gLaterFirstPrefix)/sizeof(gLaterFirstPrefix[0]);
    int8_t earliestFirstLength = sizeof(gEarlierFirstPrefix)/sizeof(gEarlierFirstPrefix[0]);
    UnicodeString realPattern;
    if ( intervalPattern.startsWith(gLaterFirstPrefix, prefixLength) ) {
        order = true;
        intervalPattern.extract(prefixLength, 
                                intervalPattern.length() - prefixLength,
                                realPattern);
        pattern = &realPattern;
    } else if ( intervalPattern.startsWith(gEarlierFirstPrefix,
                                           earliestFirstLength) ) {
        order = false;
        intervalPattern.extract(earliestFirstLength,
                                intervalPattern.length() - earliestFirstLength,
                                realPattern);
        pattern = &realPattern;
    }

    int32_t splitPoint = splitPatternInto2Part(*pattern);
    
    UnicodeString firstPart;
    UnicodeString secondPart;
    pattern->extract(0, splitPoint, firstPart);
    if ( splitPoint < pattern->length() ) {
        pattern->extract(splitPoint, pattern->length()-splitPoint, secondPart);
    }
    setPatternInfo(field, &firstPart, &secondPart, order);
}




























UBool
DateIntervalFormat::setIntervalPattern(UCalendarDateFields field,
                                       const UnicodeString* skeleton,
                                       const UnicodeString* bestSkeleton,
                                       int8_t differenceInfo,
                                       UnicodeString* extendedSkeleton,
                                       UnicodeString* extendedBestSkeleton) {
    UErrorCode status = U_ZERO_ERROR;
    
    UnicodeString pattern;
    fInfo->getIntervalPattern(*bestSkeleton, field, pattern, status);
    if ( pattern.isEmpty() ) {
        
        if ( SimpleDateFormat::isFieldUnitIgnored(*bestSkeleton, field) ) {
            
            return false;
        }

        
        
        
        
        if ( field == UCAL_AM_PM ) {
            fInfo->getIntervalPattern(*bestSkeleton, UCAL_HOUR, pattern,status);
            if ( !pattern.isEmpty() ) {
                setIntervalPattern(field, pattern);
            }
            return false;
        } 
        
        
        
        
        
        
        UChar fieldLetter = fgCalendarFieldToPatternLetter[field];
        if ( extendedSkeleton ) {
            *extendedSkeleton = *skeleton;
            *extendedBestSkeleton = *bestSkeleton;
            extendedSkeleton->insert(0, fieldLetter);
            extendedBestSkeleton->insert(0, fieldLetter);
            
            
            fInfo->getIntervalPattern(*extendedBestSkeleton,field,pattern,status);
            if ( pattern.isEmpty() && differenceInfo == 0 ) {
                
                
                const UnicodeString* tmpBest = fInfo->getBestSkeleton(
                                        *extendedBestSkeleton, differenceInfo);
                if ( tmpBest != 0 && differenceInfo != -1 ) {
                    fInfo->getIntervalPattern(*tmpBest, field, pattern, status);
                    bestSkeleton = tmpBest;
                }
            }
        }
    } 
    if ( !pattern.isEmpty() ) {
        if ( differenceInfo != 0 ) {
            UnicodeString adjustIntervalPattern;
            adjustFieldWidth(*skeleton, *bestSkeleton, pattern, differenceInfo,
                              adjustIntervalPattern);
            setIntervalPattern(field, adjustIntervalPattern);
        } else {
            setIntervalPattern(field, pattern);
        }
        if ( extendedSkeleton && !extendedSkeleton->isEmpty() ) {
            return TRUE;
        }
    }
    return FALSE;
}



int32_t  U_EXPORT2 
DateIntervalFormat::splitPatternInto2Part(const UnicodeString& intervalPattern) {
    UBool inQuote = false;
    UChar prevCh = 0;
    int32_t count = 0;

    



    UBool patternRepeated[] = 
    {
    
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0, 0, 0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    };

    int8_t PATTERN_CHAR_BASE = 0x41;
    
    



    int32_t i;
    UBool foundRepetition = false;
    for (i = 0; i < intervalPattern.length(); ++i) {
        UChar ch = intervalPattern.charAt(i);
        
        if (ch != prevCh && count > 0) {
            
            UBool repeated = patternRepeated[(int)(prevCh - PATTERN_CHAR_BASE)];
            if ( repeated == FALSE ) {
                patternRepeated[prevCh - PATTERN_CHAR_BASE] = TRUE;
            } else {
                foundRepetition = true;
                break;
            }
            count = 0;
        }
        if (ch == '\'') {
            
            
            if ((i+1) < intervalPattern.length() && 
                intervalPattern.charAt(i+1) == '\'') {
                ++i;
            } else {
                inQuote = ! inQuote;
            }
        } 
        else if (!inQuote && ((ch >= 0x0061  && ch <= 0x007A )
                    || (ch >= 0x0041  && ch <= 0x005A ))) {
            
            prevCh = ch;
            ++count;
        }
    }
    
    
    
    
    if ( count > 0 && foundRepetition == FALSE ) {
        if ( patternRepeated[(int)(prevCh - PATTERN_CHAR_BASE)] == FALSE ) {
            count = 0;
        }
    }
    return (i - count);
}



UnicodeString& 
DateIntervalFormat::fallbackFormat(Calendar& fromCalendar,
                                   Calendar& toCalendar,
                                   UnicodeString& appendTo,
                                   FieldPosition& pos,
                                   UErrorCode& status) const {
    if ( U_FAILURE(status) ) {
        return appendTo;
    }
    
    
    UnicodeString* earlierDate = new UnicodeString();
    *earlierDate = fDateFormat->format(fromCalendar, *earlierDate, pos);
    UnicodeString* laterDate = new UnicodeString();
    *laterDate = fDateFormat->format(toCalendar, *laterDate, pos);
    UnicodeString fallbackPattern;
    fInfo->getFallbackIntervalPattern(fallbackPattern);
    Formattable fmtArray[2];
    fmtArray[0].adoptString(earlierDate);
    fmtArray[1].adoptString(laterDate);
    
    UnicodeString fallback;
    MessageFormat::format(fallbackPattern, fmtArray, 2, fallback, status);
    if ( U_SUCCESS(status) ) {
        appendTo.append(fallback);
    }
    return appendTo;
}




UBool  U_EXPORT2 
DateIntervalFormat::fieldExistsInSkeleton(UCalendarDateFields field,
                                          const UnicodeString& skeleton)
{
    const UChar fieldChar = fgCalendarFieldToPatternLetter[field];
    return ( (skeleton.indexOf(fieldChar) == -1)?FALSE:TRUE ) ;
}



void  U_EXPORT2 
DateIntervalFormat::adjustFieldWidth(const UnicodeString& inputSkeleton,
                 const UnicodeString& bestMatchSkeleton,
                 const UnicodeString& bestIntervalPattern,
                 int8_t differenceInfo,
                 UnicodeString& adjustedPtn) {
    adjustedPtn = bestIntervalPattern;
    int32_t inputSkeletonFieldWidth[] = 
    {
    
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0, 0, 0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    };

    int32_t bestMatchSkeletonFieldWidth[] = 
    {
    
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0, 0, 0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    };

    DateIntervalInfo::parseSkeleton(inputSkeleton, inputSkeletonFieldWidth);
    DateIntervalInfo::parseSkeleton(bestMatchSkeleton, bestMatchSkeletonFieldWidth);
    if ( differenceInfo == 2 ) {
        adjustedPtn.findAndReplace(UnicodeString((UChar)0x76 ),
                                   UnicodeString((UChar)0x7a ));
    }

    UBool inQuote = false;
    UChar prevCh = 0;
    int32_t count = 0;

    const int8_t PATTERN_CHAR_BASE = 0x41;
    
    
    int32_t adjustedPtnLength = adjustedPtn.length();
    int32_t i;
    for (i = 0; i < adjustedPtnLength; ++i) {
        UChar ch = adjustedPtn.charAt(i);
        if (ch != prevCh && count > 0) {
            
            UChar skeletonChar = prevCh;
            if ( skeletonChar ==  CAP_L ) {
                
                
                
                skeletonChar = CAP_M;
            }
            int32_t fieldCount = bestMatchSkeletonFieldWidth[(int)(skeletonChar - PATTERN_CHAR_BASE)];
            int32_t inputFieldCount = inputSkeletonFieldWidth[(int)(skeletonChar - PATTERN_CHAR_BASE)];
            if ( fieldCount == count && inputFieldCount > fieldCount ) {
                count = inputFieldCount - fieldCount;
                int32_t j;
                for ( j = 0; j < count; ++j ) {
                    adjustedPtn.insert(i, prevCh);    
                }                    
                i += count;
                adjustedPtnLength += count;
            }
            count = 0;
        }
        if (ch == '\'') {
            
            
            if ((i+1) < adjustedPtn.length() && adjustedPtn.charAt(i+1) == '\'') {
                ++i;
            } else {
                inQuote = ! inQuote;
            }
        } 
        else if ( ! inQuote && ((ch >= 0x0061  && ch <= 0x007A ) 
                    || (ch >= 0x0041  && ch <= 0x005A ))) {
            
            prevCh = ch;
            ++count;
        }
    }
    if ( count > 0 ) {
        
        
        UChar skeletonChar = prevCh;
        if ( skeletonChar == CAP_L ) {
            
            
            
            skeletonChar = CAP_M;
        }
        int32_t fieldCount = bestMatchSkeletonFieldWidth[(int)(skeletonChar - PATTERN_CHAR_BASE)];
        int32_t inputFieldCount = inputSkeletonFieldWidth[(int)(skeletonChar - PATTERN_CHAR_BASE)];
        if ( fieldCount == count && inputFieldCount > fieldCount ) {
            count = inputFieldCount - fieldCount;
            int32_t j;
            for ( j = 0; j < count; ++j ) {
                adjustedPtn.append(prevCh);    
            }                    
        }
    }
}



void 
DateIntervalFormat::concatSingleDate2TimeInterval(const UChar* format,
                                              int32_t formatLen,
                                              const UnicodeString& datePattern,
                                              UCalendarDateFields field,
                                              UErrorCode& status) {
    
    int32_t itvPtnIndex = DateIntervalInfo::calendarFieldToIntervalIndex(field,
                                                                        status);
    if ( U_FAILURE(status) ) {
        return;
    }
    PatternInfo&  timeItvPtnInfo = fIntervalPatterns[itvPtnIndex];
    if ( !timeItvPtnInfo.firstPart.isEmpty() ) {
        
        UnicodeString* timeIntervalPattern = new UnicodeString(timeItvPtnInfo.firstPart);
        timeIntervalPattern->append(timeItvPtnInfo.secondPart);
        UnicodeString* dateStr = new UnicodeString(datePattern);
        Formattable fmtArray[2];
        fmtArray[0].adoptString(timeIntervalPattern);
        fmtArray[1].adoptString(dateStr);
        UnicodeString combinedPattern;
        MessageFormat::format(UnicodeString(TRUE, format, formatLen),
                              fmtArray, 2, combinedPattern, status);
        if ( U_FAILURE(status) ) {
            return;
        }
        setIntervalPattern(field, combinedPattern, timeItvPtnInfo.laterDateFirst);
    } 
    
    
}



const UChar
DateIntervalFormat::fgCalendarFieldToPatternLetter[] =
{
     CAP_G, LOW_Y, CAP_M,
     LOW_W, CAP_W, LOW_D,
     CAP_D, CAP_E, CAP_F,
     LOW_A, LOW_H, CAP_H,
     LOW_M,
};


U_NAMESPACE_END

#endif
