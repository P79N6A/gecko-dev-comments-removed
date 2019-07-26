






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/basictz.h"
#include "gregoimp.h"
#include "uvector.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

#define MILLIS_PER_YEAR (365*24*60*60*1000.0)

BasicTimeZone::BasicTimeZone()
: TimeZone() {
}

BasicTimeZone::BasicTimeZone(const UnicodeString &id)
: TimeZone(id) {
}

BasicTimeZone::BasicTimeZone(const BasicTimeZone& source)
: TimeZone(source) {
}

BasicTimeZone::~BasicTimeZone() {
}

UBool
BasicTimeZone::hasEquivalentTransitions( BasicTimeZone& tz, UDate start, UDate end,
                                        UBool ignoreDstAmount, UErrorCode& status)  {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (hasSameRules(tz)) {
        return TRUE;
    }
    
    int32_t raw1, raw2, dst1, dst2;
    getOffset(start, FALSE, raw1, dst1, status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    tz.getOffset(start, FALSE, raw2, dst2, status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (ignoreDstAmount) {
        if ((raw1 + dst1 != raw2 + dst2)
            || (dst1 != 0 && dst2 == 0)
            || (dst1 == 0 && dst2 != 0)) {
            return FALSE;
        }
    } else {
        if (raw1 != raw2 || dst1 != dst2) {
            return FALSE;
        }            
    }
    
    UDate time = start;
    TimeZoneTransition tr1, tr2;
    while (TRUE) {
        UBool avail1 = getNextTransition(time, FALSE, tr1);
        UBool avail2 = tz.getNextTransition(time, FALSE, tr2);

        if (ignoreDstAmount) {
            
            while (TRUE) {
                if (avail1
                        && tr1.getTime() <= end
                        && (tr1.getFrom()->getRawOffset() + tr1.getFrom()->getDSTSavings()
                                == tr1.getTo()->getRawOffset() + tr1.getTo()->getDSTSavings())
                        && (tr1.getFrom()->getDSTSavings() != 0 && tr1.getTo()->getDSTSavings() != 0)) {
                    getNextTransition(tr1.getTime(), FALSE, tr1);
                } else {
                    break;
                }
            }
            while (TRUE) {
                if (avail2
                        && tr2.getTime() <= end
                        && (tr2.getFrom()->getRawOffset() + tr2.getFrom()->getDSTSavings()
                                == tr2.getTo()->getRawOffset() + tr2.getTo()->getDSTSavings())
                        && (tr2.getFrom()->getDSTSavings() != 0 && tr2.getTo()->getDSTSavings() != 0)) {
                    tz.getNextTransition(tr2.getTime(), FALSE, tr2);
                } else {
                    break;
                }
            }
        }

        UBool inRange1 = (avail1 && tr1.getTime() <= end);
        UBool inRange2 = (avail2 && tr2.getTime() <= end);
        if (!inRange1 && !inRange2) {
            
            break;
        }
        if (!inRange1 || !inRange2) {
            return FALSE;
        }
        if (tr1.getTime() != tr2.getTime()) {
            return FALSE;
        }
        if (ignoreDstAmount) {
            if (tr1.getTo()->getRawOffset() + tr1.getTo()->getDSTSavings()
                        != tr2.getTo()->getRawOffset() + tr2.getTo()->getDSTSavings()
                    || (tr1.getTo()->getDSTSavings() != 0 &&  tr2.getTo()->getDSTSavings() == 0)
                    || (tr1.getTo()->getDSTSavings() == 0 &&  tr2.getTo()->getDSTSavings() != 0)) {
                return FALSE;
            }
        } else {
            if (tr1.getTo()->getRawOffset() != tr2.getTo()->getRawOffset() ||
                tr1.getTo()->getDSTSavings() != tr2.getTo()->getDSTSavings()) {
                return FALSE;
            }
        }
        time = tr1.getTime();
    }
    return TRUE;
}

void
BasicTimeZone::getSimpleRulesNear(UDate date, InitialTimeZoneRule*& initial,
        AnnualTimeZoneRule*& std, AnnualTimeZoneRule*& dst, UErrorCode& status)  {
    initial = NULL;
    std = NULL;
    dst = NULL;
    if (U_FAILURE(status)) {
        return;
    }
    int32_t initialRaw, initialDst;
    UnicodeString initialName;

    AnnualTimeZoneRule *ar1 = NULL;
    AnnualTimeZoneRule *ar2 = NULL;
    UnicodeString name;

    UBool avail;
    TimeZoneTransition tr;
    
    avail = getNextTransition(date, FALSE, tr);
    if (avail) {
        tr.getFrom()->getName(initialName);
        initialRaw = tr.getFrom()->getRawOffset();
        initialDst = tr.getFrom()->getDSTSavings();

        
        
        UDate nextTransitionTime = tr.getTime();
        if (((tr.getFrom()->getDSTSavings() == 0 && tr.getTo()->getDSTSavings() != 0)
              || (tr.getFrom()->getDSTSavings() != 0 && tr.getTo()->getDSTSavings() == 0))
            && (date + MILLIS_PER_YEAR > nextTransitionTime)) {
 
            int32_t year, month, dom, dow, doy, mid;
            UDate d;

            
            Grego::timeToFields(nextTransitionTime + initialRaw + initialDst,
                year, month, dom, dow, doy, mid);
            int32_t weekInMonth = Grego::dayOfWeekInMonth(year, month, dom);
            
            DateTimeRule *dtr = new DateTimeRule(month, weekInMonth, dow, mid, DateTimeRule::WALL_TIME);
            tr.getTo()->getName(name);

            
            
            
            
            
            
            ar1 = new AnnualTimeZoneRule(name, initialRaw, tr.getTo()->getDSTSavings(),
                dtr, year, AnnualTimeZoneRule::MAX_YEAR);

            if (tr.getTo()->getRawOffset() == initialRaw) {
                
                avail = getNextTransition(nextTransitionTime, FALSE, tr);
                if (avail) {
                    
                    
                    if (((tr.getFrom()->getDSTSavings() == 0 && tr.getTo()->getDSTSavings() != 0)
                          || (tr.getFrom()->getDSTSavings() != 0 && tr.getTo()->getDSTSavings() == 0))
                         && nextTransitionTime + MILLIS_PER_YEAR > tr.getTime()) {

                        
                        Grego::timeToFields(tr.getTime() + tr.getFrom()->getRawOffset() + tr.getFrom()->getDSTSavings(),
                            year, month, dom, dow, doy, mid);
                        weekInMonth = Grego::dayOfWeekInMonth(year, month, dom);
                        
                        dtr = new DateTimeRule(month, weekInMonth, dow, mid, DateTimeRule::WALL_TIME);
                        tr.getTo()->getName(name);
                        ar2 = new AnnualTimeZoneRule(name, tr.getTo()->getRawOffset(), tr.getTo()->getDSTSavings(),
                            dtr, year - 1, AnnualTimeZoneRule::MAX_YEAR);

                        
                        avail = ar2->getPreviousStart(date, tr.getFrom()->getRawOffset(), tr.getFrom()->getDSTSavings(), TRUE, d);
                        if (!avail || d > date
                                || initialRaw != tr.getTo()->getRawOffset()
                                || initialDst != tr.getTo()->getDSTSavings()) {
                            
                            delete ar2;
                            ar2 = NULL;
                        }
                    }
                }
            }
            if (ar2 == NULL) {
                
                avail = getPreviousTransition(date, TRUE, tr);
                if (avail) {
                    
                    
                    if ((tr.getFrom()->getDSTSavings() == 0 && tr.getTo()->getDSTSavings() != 0)
                        || (tr.getFrom()->getDSTSavings() != 0 && tr.getTo()->getDSTSavings() == 0)) {

                        
                        Grego::timeToFields(tr.getTime() + tr.getFrom()->getRawOffset() + tr.getFrom()->getDSTSavings(),
                            year, month, dom, dow, doy, mid);
                        weekInMonth = Grego::dayOfWeekInMonth(year, month, dom);
                        dtr = new DateTimeRule(month, weekInMonth, dow, mid, DateTimeRule::WALL_TIME);
                        tr.getTo()->getName(name);

                        
                        
                        ar2 = new AnnualTimeZoneRule(name, initialRaw, initialDst,
                            dtr, ar1->getStartYear() - 1, AnnualTimeZoneRule::MAX_YEAR);

                        
                        avail = ar2->getNextStart(date, tr.getFrom()->getRawOffset(), tr.getFrom()->getDSTSavings(), FALSE, d);
                        if (!avail || d <= nextTransitionTime) {
                            
                            delete ar2;
                            ar2 = NULL;
                        }
                    }
                }
            }
            if (ar2 == NULL) {
                
                delete ar1;
                ar1 = NULL;
            } else {
                
                ar1->getName(initialName);
                initialRaw = ar1->getRawOffset();
                initialDst = ar1->getDSTSavings();
            }
        }
    }
    else {
        
        avail = getPreviousTransition(date, TRUE, tr);
        if (avail) {
            tr.getTo()->getName(initialName);
            initialRaw = tr.getTo()->getRawOffset();
            initialDst = tr.getTo()->getDSTSavings();
        } else {
            
            getOffset(date, FALSE, initialRaw, initialDst, status);
            if (U_FAILURE(status)) {
                return;
            }
        }
    }
    
    initial = new InitialTimeZoneRule(initialName, initialRaw, initialDst);

    
    if (ar1 != NULL && ar2 != NULL) {
        if (ar1->getDSTSavings() != 0) {
            dst = ar1;
            std = ar2;
        } else {
            std = ar1;
            dst = ar2;
        }
    }
}

void
BasicTimeZone::getTimeZoneRulesAfter(UDate start, InitialTimeZoneRule*& initial,
                                     UVector*& transitionRules, UErrorCode& status)  {
    if (U_FAILURE(status)) {
        return;
    }

    const InitialTimeZoneRule *orgini;
    const TimeZoneRule **orgtrs = NULL;
    TimeZoneTransition tzt;
    UBool avail;
    UVector *orgRules = NULL;
    int32_t ruleCount;
    TimeZoneRule *r = NULL;
    UBool *done = NULL;
    InitialTimeZoneRule *res_initial = NULL;
    UVector *filteredRules = NULL;
    UnicodeString name;
    int32_t i;
    UDate time, t;
    UDate *newTimes = NULL;
    UDate firstStart;
    UBool bFinalStd = FALSE, bFinalDst = FALSE;

    
    ruleCount = countTransitionRules(status);
    if (U_FAILURE(status)) {
        return;
    }
    orgRules = new UVector(ruleCount, status);
    if (U_FAILURE(status)) {
        return;
    }
    orgtrs = (const TimeZoneRule**)uprv_malloc(sizeof(TimeZoneRule*)*ruleCount);
    if (orgtrs == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto error;
    }
    getTimeZoneRules(orgini, orgtrs, ruleCount, status);
    if (U_FAILURE(status)) {
        goto error;
    }
    for (i = 0; i < ruleCount; i++) {
        orgRules->addElement(orgtrs[i]->clone(), status);
        if (U_FAILURE(status)) {
            goto error;
        }
    }
    uprv_free(orgtrs);
    orgtrs = NULL;

    avail = getPreviousTransition(start, TRUE, tzt);
    if (!avail) {
        
        initial = orgini->clone();
        transitionRules = orgRules;
        return;
    }

    done = (UBool*)uprv_malloc(sizeof(UBool)*ruleCount);
    if (done == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto error;
    }
    filteredRules = new UVector(status);
    if (U_FAILURE(status)) {
        goto error;
    }

    
    tzt.getTo()->getName(name);
    res_initial = new InitialTimeZoneRule(name, tzt.getTo()->getRawOffset(),
        tzt.getTo()->getDSTSavings());

    
    for (i = 0; i < ruleCount; i++) {
        r = (TimeZoneRule*)orgRules->elementAt(i);
        avail = r->getNextStart(start, res_initial->getRawOffset(), res_initial->getDSTSavings(), FALSE, time);
        done[i] = !avail;
    }

    time = start;
    while (!bFinalStd || !bFinalDst) {
        avail = getNextTransition(time, FALSE, tzt);
        if (!avail) {
            break;
        }
        UDate updatedTime = tzt.getTime();
        if (updatedTime == time) {
            
            
            
            status = U_INVALID_STATE_ERROR;
            goto error;
        }
        time = updatedTime;
 
        const TimeZoneRule *toRule = tzt.getTo();
        for (i = 0; i < ruleCount; i++) {
            r = (TimeZoneRule*)orgRules->elementAt(i);
            if (*r == *toRule) {
                break;
            }
        }
        if (i >= ruleCount) {
            
            status = U_INVALID_STATE_ERROR;
            goto error;
        }
        if (done[i]) {
            continue;
        }
        const TimeArrayTimeZoneRule *tar = dynamic_cast<const TimeArrayTimeZoneRule *>(toRule);
        const AnnualTimeZoneRule *ar;
        if (tar != NULL) {
            
            TimeZoneTransition tzt0;
            t = start;
            while (TRUE) {
                avail = getNextTransition(t, FALSE, tzt0);
                if (!avail) {
                    break;
                }
                if (*(tzt0.getTo()) == *tar) {
                    break;
                }
                t = tzt0.getTime();
            }
            if (avail) {
                
                tar->getFirstStart(tzt.getFrom()->getRawOffset(), tzt.getFrom()->getDSTSavings(), firstStart);
                if (firstStart > start) {
                    
                    filteredRules->addElement(tar->clone(), status);
                    if (U_FAILURE(status)) {
                        goto error;
                    }
                } else {
                    
                    int32_t startTimes;
                    DateTimeRule::TimeRuleType timeType;
                    int32_t idx;

                    startTimes = tar->countStartTimes();
                    timeType = tar->getTimeType();
                    for (idx = 0; idx < startTimes; idx++) {
                        tar->getStartTimeAt(idx, t);
                        if (timeType == DateTimeRule::STANDARD_TIME) {
                            t -= tzt.getFrom()->getRawOffset();
                        }
                        if (timeType == DateTimeRule::WALL_TIME) {
                            t -= tzt.getFrom()->getDSTSavings();
                        }
                        if (t > start) {
                            break;
                        }
                    }
                    int32_t asize = startTimes - idx;
                    if (asize > 0) {
                        newTimes = (UDate*)uprv_malloc(sizeof(UDate) * asize);
                        if (newTimes == NULL) {
                            status = U_MEMORY_ALLOCATION_ERROR;
                            goto error;
                        }
                        for (int32_t newidx = 0; newidx < asize; newidx++) {
                            tar->getStartTimeAt(idx + newidx, newTimes[newidx]);
                            if (U_FAILURE(status)) {
                                uprv_free(newTimes);
                                newTimes = NULL;
                                goto error;
                            }
                        }
                        tar->getName(name);
                        TimeArrayTimeZoneRule *newTar = new TimeArrayTimeZoneRule(name,
                            tar->getRawOffset(), tar->getDSTSavings(), newTimes, asize, timeType);
                        uprv_free(newTimes);
                        filteredRules->addElement(newTar, status);
                        if (U_FAILURE(status)) {
                            goto error;
                        }
                    }
                }
            }
        } else if ((ar = dynamic_cast<const AnnualTimeZoneRule *>(toRule)) != NULL) {
            ar->getFirstStart(tzt.getFrom()->getRawOffset(), tzt.getFrom()->getDSTSavings(), firstStart);
            if (firstStart == tzt.getTime()) {
                
                filteredRules->addElement(ar->clone(), status);
                if (U_FAILURE(status)) {
                    goto error;
                }
            } else {
                
                int32_t year, month, dom, dow, doy, mid;
                Grego::timeToFields(tzt.getTime(), year, month, dom, dow, doy, mid);
                
                ar->getName(name);
                AnnualTimeZoneRule *newAr = new AnnualTimeZoneRule(name, ar->getRawOffset(), ar->getDSTSavings(),
                    *(ar->getRule()), year, ar->getEndYear());
                filteredRules->addElement(newAr, status);
                if (U_FAILURE(status)) {
                    goto error;
                }
            }
            
            if (ar->getEndYear() == AnnualTimeZoneRule::MAX_YEAR) {
                
                
                if (ar->getDSTSavings() == 0) {
                    bFinalStd = TRUE;
                } else {
                    bFinalDst = TRUE;
                }
            }
        }
        done[i] = TRUE;
    }

    
    if (orgRules != NULL) {
        while (!orgRules->isEmpty()) {
            r = (TimeZoneRule*)orgRules->orphanElementAt(0);
            delete r;
        }
        delete orgRules;
    }
    if (done != NULL) {
        uprv_free(done);
    }

    initial = res_initial;
    transitionRules = filteredRules;
    return;

error:
    if (orgtrs != NULL) {
        uprv_free(orgtrs);
    }
    if (orgRules != NULL) {
        while (!orgRules->isEmpty()) {
            r = (TimeZoneRule*)orgRules->orphanElementAt(0);
            delete r;
        }
        delete orgRules;
    }
    if (done != NULL) {
        if (filteredRules != NULL) {
            while (!filteredRules->isEmpty()) {
                r = (TimeZoneRule*)filteredRules->orphanElementAt(0);
                delete r;
            }
            delete filteredRules;
        }
        delete res_initial;
        uprv_free(done);
    }

    initial = NULL;
    transitionRules = NULL;
}

void
BasicTimeZone::getOffsetFromLocal(UDate , int32_t , int32_t ,
                            int32_t& , int32_t& , UErrorCode& status)  {
    if (U_FAILURE(status)) {
        return;
    }
    status = U_UNSUPPORTED_ERROR;
}

U_NAMESPACE_END

#endif 


