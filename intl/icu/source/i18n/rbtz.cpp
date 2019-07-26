






#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/rbtz.h"
#include "unicode/gregocal.h"
#include "uvector.h"
#include "gregoimp.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN




struct Transition {
    UDate time;
    TimeZoneRule* from;
    TimeZoneRule* to;
};

static UBool compareRules(UVector* rules1, UVector* rules2) {
    if (rules1 == NULL && rules2 == NULL) {
        return TRUE;
    } else if (rules1 == NULL || rules2 == NULL) {
        return FALSE;
    }
    int32_t size = rules1->size();
    if (size != rules2->size()) {
        return FALSE;
    }
    for (int32_t i = 0; i < size; i++) {
        TimeZoneRule *r1 = (TimeZoneRule*)rules1->elementAt(i);
        TimeZoneRule *r2 = (TimeZoneRule*)rules2->elementAt(i);
        if (*r1 != *r2) {
            return FALSE;
        }
    }
    return TRUE;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RuleBasedTimeZone)

RuleBasedTimeZone::RuleBasedTimeZone(const UnicodeString& id, InitialTimeZoneRule* initialRule)
: BasicTimeZone(id), fInitialRule(initialRule), fHistoricRules(NULL), fFinalRules(NULL),
  fHistoricTransitions(NULL), fUpToDate(FALSE) {
}

RuleBasedTimeZone::RuleBasedTimeZone(const RuleBasedTimeZone& source)
: BasicTimeZone(source), fInitialRule(source.fInitialRule->clone()),
  fHistoricTransitions(NULL), fUpToDate(FALSE) {
    fHistoricRules = copyRules(source.fHistoricRules);
    fFinalRules = copyRules(source.fFinalRules);
    if (source.fUpToDate) {
        UErrorCode status = U_ZERO_ERROR;
        complete(status);
    }
}

RuleBasedTimeZone::~RuleBasedTimeZone() {
    deleteTransitions();
    deleteRules();
}

RuleBasedTimeZone&
RuleBasedTimeZone::operator=(const RuleBasedTimeZone& right) {
    if (*this != right) {
        BasicTimeZone::operator=(right);
        deleteRules();
        fInitialRule = right.fInitialRule->clone();
        fHistoricRules = copyRules(right.fHistoricRules);
        fFinalRules = copyRules(right.fFinalRules);
        deleteTransitions();
        fUpToDate = FALSE;
    }
    return *this;
}

UBool
RuleBasedTimeZone::operator==(const TimeZone& that) const {
    if (this == &that) {
        return TRUE;
    }
    if (typeid(*this) != typeid(that)
        || BasicTimeZone::operator==(that) == FALSE) {
        return FALSE;
    }
    RuleBasedTimeZone *rbtz = (RuleBasedTimeZone*)&that;
    if (*fInitialRule != *(rbtz->fInitialRule)) {
        return FALSE;
    }
    if (compareRules(fHistoricRules, rbtz->fHistoricRules)
        && compareRules(fFinalRules, rbtz->fFinalRules)) {
        return TRUE;
    }
    return FALSE;
}

UBool
RuleBasedTimeZone::operator!=(const TimeZone& that) const {
    return !operator==(that);
}

void
RuleBasedTimeZone::addTransitionRule(TimeZoneRule* rule, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    AnnualTimeZoneRule* atzrule = dynamic_cast<AnnualTimeZoneRule*>(rule);
    if (atzrule != NULL && atzrule->getEndYear() == AnnualTimeZoneRule::MAX_YEAR) {
        
        if (fFinalRules == NULL) {
            fFinalRules = new UVector(status);
            if (U_FAILURE(status)) {
                return;
            }
        } else if (fFinalRules->size() >= 2) {
            
            status = U_INVALID_STATE_ERROR;
            return;
        }
        fFinalRules->addElement((void*)rule, status);
    } else {
        
        if (fHistoricRules == NULL) {
            fHistoricRules = new UVector(status);
            if (U_FAILURE(status)) {
                return;
            }
        }
        fHistoricRules->addElement((void*)rule, status);
    }
    
    fUpToDate = FALSE;
}

void
RuleBasedTimeZone::complete(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (fUpToDate) {
        return;
    }
    
    
    if (fFinalRules != NULL && fFinalRules->size() != 2) {
        status = U_INVALID_STATE_ERROR;
        return;
    }

    UBool *done = NULL;
    
    if (fHistoricRules != NULL || fFinalRules != NULL) {
        TimeZoneRule *curRule = fInitialRule;
        UDate lastTransitionTime = MIN_MILLIS;

        
        
        if (fHistoricRules != NULL && fHistoricRules->size() > 0) {
            int32_t i;
            int32_t historicCount = fHistoricRules->size();
            done = (UBool*)uprv_malloc(sizeof(UBool) * historicCount);
            if (done == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            for (i = 0; i < historicCount; i++) {
                done[i] = FALSE;
            }
            while (TRUE) {
                int32_t curStdOffset = curRule->getRawOffset();
                int32_t curDstSavings = curRule->getDSTSavings();
                UDate nextTransitionTime = MAX_MILLIS;
                TimeZoneRule *nextRule = NULL;
                TimeZoneRule *r = NULL;
                UBool avail;
                UDate tt;
                UnicodeString curName, name;
                curRule->getName(curName);

                for (i = 0; i < historicCount; i++) {
                    if (done[i]) {
                        continue;
                    }
                    r = (TimeZoneRule*)fHistoricRules->elementAt(i);
                    avail = r->getNextStart(lastTransitionTime, curStdOffset, curDstSavings, false, tt);
                    if (!avail) {
                        
                        done[i] = TRUE;
                    } else {
                        r->getName(name);
                        if (*r == *curRule ||
                            (name == curName && r->getRawOffset() == curRule->getRawOffset()
                            && r->getDSTSavings() == curRule->getDSTSavings())) {
                            continue;
                        }
                        if (tt < nextTransitionTime) {
                            nextTransitionTime = tt;
                            nextRule = r;
                        }
                    }
                }

                if (nextRule ==  NULL) {
                    
                    UBool bDoneAll = TRUE;
                    for (int32_t j = 0; j < historicCount; j++) {
                        if (!done[j]) {
                            bDoneAll = FALSE;
                            break;
                        }
                    }
                    if (bDoneAll) {
                        break;
                    }
                }

                if (fFinalRules != NULL) {
                    
                    for (i = 0; i < 2 ; i++) {
                        TimeZoneRule *fr = (TimeZoneRule*)fFinalRules->elementAt(i);
                        if (*fr == *curRule) {
                            continue;
                        }
                        r = (TimeZoneRule*)fFinalRules->elementAt(i);
                        avail = r->getNextStart(lastTransitionTime, curStdOffset, curDstSavings, false, tt);
                        if (avail) {
                            if (tt < nextTransitionTime) {
                                nextTransitionTime = tt;
                                nextRule = r;
                            }
                        }
                    }
                }

                if (nextRule == NULL) {
                    
                    break;
                }

                if (fHistoricTransitions == NULL) {
                    fHistoricTransitions = new UVector(status);
                    if (U_FAILURE(status)) {
                        goto cleanup;
                    }
                }
                Transition *trst = (Transition*)uprv_malloc(sizeof(Transition));
                if (trst == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    goto cleanup;
                }
                trst->time = nextTransitionTime;
                trst->from = curRule;
                trst->to = nextRule;
                fHistoricTransitions->addElement(trst, status);
                if (U_FAILURE(status)) {
                    goto cleanup;
                }
                lastTransitionTime = nextTransitionTime;
                curRule = nextRule;
            }
        }
        if (fFinalRules != NULL) {
            if (fHistoricTransitions == NULL) {
                fHistoricTransitions = new UVector(status);
                if (U_FAILURE(status)) {
                    goto cleanup;
                }
            }
            
            TimeZoneRule *rule0 = (TimeZoneRule*)fFinalRules->elementAt(0);
            TimeZoneRule *rule1 = (TimeZoneRule*)fFinalRules->elementAt(1);
            UDate tt0, tt1;
            UBool avail0 = rule0->getNextStart(lastTransitionTime, curRule->getRawOffset(), curRule->getDSTSavings(), false, tt0);
            UBool avail1 = rule1->getNextStart(lastTransitionTime, curRule->getRawOffset(), curRule->getDSTSavings(), false, tt1);
            if (!avail0 || !avail1) {
                
                status = U_INVALID_STATE_ERROR;
                goto cleanup;
            }
            Transition *final0 = (Transition*)uprv_malloc(sizeof(Transition));
            if (final0 == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            Transition *final1 = (Transition*)uprv_malloc(sizeof(Transition));
            if (final1 == NULL) {
                uprv_free(final0);
                status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            if (tt0 < tt1) {
                final0->time = tt0;
                final0->from = curRule;
                final0->to = rule0;
                rule1->getNextStart(tt0, rule0->getRawOffset(), rule0->getDSTSavings(), false, final1->time);
                final1->from = rule0;
                final1->to = rule1;
            } else {
                final0->time = tt1;
                final0->from = curRule;
                final0->to = rule1;
                rule0->getNextStart(tt1, rule1->getRawOffset(), rule1->getDSTSavings(), false, final1->time);
                final1->from = rule1;
                final1->to = rule0;
            }
            fHistoricTransitions->addElement(final0, status);
            if (U_FAILURE(status)) {
                goto cleanup;
            }
            fHistoricTransitions->addElement(final1, status);
            if (U_FAILURE(status)) {
                goto cleanup;
            }
        }
    }
    fUpToDate = TRUE;
    if (done != NULL) {
        uprv_free(done);
    }
    return;

cleanup:
    deleteTransitions();
    if (done != NULL) {
        uprv_free(done);
    }
    fUpToDate = FALSE;
}

TimeZone*
RuleBasedTimeZone::clone(void) const {
    return new RuleBasedTimeZone(*this);
}

int32_t
RuleBasedTimeZone::getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                             uint8_t dayOfWeek, int32_t millis, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return 0;
    }
    if (month < UCAL_JANUARY || month > UCAL_DECEMBER) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    } else {
        return getOffset(era, year, month, day, dayOfWeek, millis,
                         Grego::monthLength(year, month), status);
    }
}

int32_t
RuleBasedTimeZone::getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                             uint8_t , int32_t millis,
                             int32_t , UErrorCode& status) const {
    
    if (U_FAILURE(status)) {
        return 0;
    }
    if (era == GregorianCalendar::BC) {
        
        year = 1 - year;
    }
    int32_t rawOffset, dstOffset;
    UDate time = (UDate)Grego::fieldsToDay(year, month, day) * U_MILLIS_PER_DAY + millis;
    getOffsetInternal(time, TRUE, kDaylight, kStandard, rawOffset, dstOffset, status);
    if (U_FAILURE(status)) {
        return 0;
    }
    return (rawOffset + dstOffset);
}

void
RuleBasedTimeZone::getOffset(UDate date, UBool local, int32_t& rawOffset,
                             int32_t& dstOffset, UErrorCode& status) const {
    getOffsetInternal(date, local, kFormer, kLatter, rawOffset, dstOffset, status);
}

void
RuleBasedTimeZone::getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
                                      int32_t& rawOffset, int32_t& dstOffset, UErrorCode& status)  {
    getOffsetInternal(date, TRUE, nonExistingTimeOpt, duplicatedTimeOpt, rawOffset, dstOffset, status);
}





void
RuleBasedTimeZone::getOffsetInternal(UDate date, UBool local,
                                     int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt,
                                     int32_t& rawOffset, int32_t& dstOffset,
                                     UErrorCode& status) const {
    rawOffset = 0;
    dstOffset = 0;

    if (U_FAILURE(status)) {
        return;
    }
    if (!fUpToDate) {
        
        
        
        status = U_INVALID_STATE_ERROR;
        return;
    }
    const TimeZoneRule *rule = NULL;
    if (fHistoricTransitions == NULL) {
        rule = fInitialRule;
    } else {
        UDate tstart = getTransitionTime((Transition*)fHistoricTransitions->elementAt(0),
            local, NonExistingTimeOpt, DuplicatedTimeOpt);
        if (date < tstart) {
            rule = fInitialRule;
        } else {
            int32_t idx = fHistoricTransitions->size() - 1;
            UDate tend = getTransitionTime((Transition*)fHistoricTransitions->elementAt(idx),
                local, NonExistingTimeOpt, DuplicatedTimeOpt);
            if (date > tend) {
                if (fFinalRules != NULL) {
                    rule = findRuleInFinal(date, local, NonExistingTimeOpt, DuplicatedTimeOpt);
                }
                if (rule == NULL) {
                    
                    
                    rule = ((Transition*)fHistoricTransitions->elementAt(idx))->to;
                }
            } else {
                
                while (idx >= 0) {
                    if (date >= getTransitionTime((Transition*)fHistoricTransitions->elementAt(idx),
                        local, NonExistingTimeOpt, DuplicatedTimeOpt)) {
                        break;
                    }
                    idx--;
                }
                rule = ((Transition*)fHistoricTransitions->elementAt(idx))->to;
            }
        }
    }
    if (rule != NULL) {
        rawOffset = rule->getRawOffset();
        dstOffset = rule->getDSTSavings();
    }
}

void
RuleBasedTimeZone::setRawOffset(int32_t ) {
    
    
}

int32_t
RuleBasedTimeZone::getRawOffset(void) const {
    
    
    UErrorCode status = U_ZERO_ERROR;
    int32_t raw, dst;
    getOffset(uprv_getUTCtime() * U_MILLIS_PER_SECOND,
        FALSE, raw, dst, status);
    return raw;
}

UBool
RuleBasedTimeZone::useDaylightTime(void) const {
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    UDate now = uprv_getUTCtime() * U_MILLIS_PER_SECOND;
    int32_t raw, dst;
    getOffset(now, FALSE, raw, dst, status);
    if (dst != 0) {
        return TRUE;
    }
    
    UDate time;
    TimeZoneRule *from, *to;
    UBool avail = findNext(now, FALSE, time, from, to);
    if (avail && to->getDSTSavings() != 0) {
        return TRUE;
    }
    return FALSE;
}

UBool
RuleBasedTimeZone::inDaylightTime(UDate date, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    int32_t raw, dst;
    getOffset(date, FALSE, raw, dst, status);
    if (dst != 0) {
        return TRUE;
    }
    return FALSE;
}

UBool
RuleBasedTimeZone::hasSameRules(const TimeZone& other) const {
    if (this == &other) {
        return TRUE;
    }
    if (typeid(*this) != typeid(other)) {
        return FALSE;
    }
    const RuleBasedTimeZone& that = (const RuleBasedTimeZone&)other;
    if (*fInitialRule != *(that.fInitialRule)) {
        return FALSE;
    }
    if (compareRules(fHistoricRules, that.fHistoricRules)
        && compareRules(fFinalRules, that.fFinalRules)) {
        return TRUE;
    }
    return FALSE;
}

UBool
RuleBasedTimeZone::getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result)  {
    UErrorCode status = U_ZERO_ERROR;
    complete(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    UDate transitionTime;
    TimeZoneRule *fromRule, *toRule;
    UBool found = findNext(base, inclusive, transitionTime, fromRule, toRule);
    if (found) {
        result.setTime(transitionTime);
        result.setFrom((const TimeZoneRule&)*fromRule);
        result.setTo((const TimeZoneRule&)*toRule);
        return TRUE;
    }
    return FALSE;
}

UBool
RuleBasedTimeZone::getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result)  {
    UErrorCode status = U_ZERO_ERROR;
    complete(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    UDate transitionTime;
    TimeZoneRule *fromRule, *toRule;
    UBool found = findPrev(base, inclusive, transitionTime, fromRule, toRule);
    if (found) {
        result.setTime(transitionTime);
        result.setFrom((const TimeZoneRule&)*fromRule);
        result.setTo((const TimeZoneRule&)*toRule);
        return TRUE;
    }
    return FALSE;
}

int32_t
RuleBasedTimeZone::countTransitionRules(UErrorCode& )  {
    int32_t count = 0;
    if (fHistoricRules != NULL) {
        count += fHistoricRules->size();
    }
    if (fFinalRules != NULL) {
        count += fFinalRules->size();
    }
    return count;
}

void
RuleBasedTimeZone::getTimeZoneRules(const InitialTimeZoneRule*& initial,
                                    const TimeZoneRule* trsrules[],
                                    int32_t& trscount,
                                    UErrorCode& status)  {
    if (U_FAILURE(status)) {
        return;
    }
    
    initial = fInitialRule;

    
    int32_t cnt = 0;
    int32_t idx;
    if (fHistoricRules != NULL && cnt < trscount) {
        int32_t historicCount = fHistoricRules->size();
        idx = 0;
        while (cnt < trscount && idx < historicCount) {
            trsrules[cnt++] = (const TimeZoneRule*)fHistoricRules->elementAt(idx++);
        }
    }
    if (fFinalRules != NULL && cnt < trscount) {
        int32_t finalCount = fFinalRules->size();
        idx = 0;
        while (cnt < trscount && idx < finalCount) {
            trsrules[cnt++] = (const TimeZoneRule*)fFinalRules->elementAt(idx++);
        }
    }
    
    trscount = cnt;
}

void
RuleBasedTimeZone::deleteRules(void) {
    delete fInitialRule;
    fInitialRule = NULL;
    if (fHistoricRules != NULL) {
        while (!fHistoricRules->isEmpty()) {
            delete (TimeZoneRule*)(fHistoricRules->orphanElementAt(0));
        }
        delete fHistoricRules;
        fHistoricRules = NULL;
    }
    if (fFinalRules != NULL) {
        while (!fFinalRules->isEmpty()) {
            delete (AnnualTimeZoneRule*)(fFinalRules->orphanElementAt(0));
        }
        delete fFinalRules;
        fFinalRules = NULL;
    }
}

void
RuleBasedTimeZone::deleteTransitions(void) {
    if (fHistoricTransitions != NULL) {
        while (!fHistoricTransitions->isEmpty()) {
            Transition *trs = (Transition*)fHistoricTransitions->orphanElementAt(0);
            uprv_free(trs);
        }
        delete fHistoricTransitions;
    }
    fHistoricTransitions = NULL;
}

UVector*
RuleBasedTimeZone::copyRules(UVector* source) {
    if (source == NULL) {
        return NULL;
    }
    UErrorCode ec = U_ZERO_ERROR;
    int32_t size = source->size();
    UVector *rules = new UVector(size, ec);
    if (U_FAILURE(ec)) {
        return NULL;
    }
    int32_t i;
    for (i = 0; i < size; i++) {
        rules->addElement(((TimeZoneRule*)source->elementAt(i))->clone(), ec);
        if (U_FAILURE(ec)) {
            break;
        }
    }
    if (U_FAILURE(ec)) {
        
        for (i = 0; i < rules->size(); i++) {
            TimeZoneRule *rule = (TimeZoneRule*)rules->orphanElementAt(i);
            delete rule;
        }
        delete rules;
        return NULL;
    }
    return rules;
}

TimeZoneRule*
RuleBasedTimeZone::findRuleInFinal(UDate date, UBool local,
                                   int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const {
    if (fFinalRules == NULL) {
        return NULL;
    }

    AnnualTimeZoneRule* fr0 = (AnnualTimeZoneRule*)fFinalRules->elementAt(0);
    AnnualTimeZoneRule* fr1 = (AnnualTimeZoneRule*)fFinalRules->elementAt(1);
    if (fr0 == NULL || fr1 == NULL) {
        return NULL;
    }

    UDate start0, start1;
    UDate base;
    int32_t localDelta;

    base = date;
    if (local) {
        localDelta = getLocalDelta(fr1->getRawOffset(), fr1->getDSTSavings(),
                                   fr0->getRawOffset(), fr0->getDSTSavings(),
                                   NonExistingTimeOpt, DuplicatedTimeOpt);
        base -= localDelta;
    }
    UBool avail0 = fr0->getPreviousStart(base, fr1->getRawOffset(), fr1->getDSTSavings(), TRUE, start0);

    base = date;
    if (local) {
        localDelta = getLocalDelta(fr0->getRawOffset(), fr0->getDSTSavings(),
                                   fr1->getRawOffset(), fr1->getDSTSavings(),
                                   NonExistingTimeOpt, DuplicatedTimeOpt);
        base -= localDelta;
    }
    UBool avail1 = fr1->getPreviousStart(base, fr0->getRawOffset(), fr0->getDSTSavings(), TRUE, start1);

    if (!avail0 || !avail1) {
        if (avail0) {
            return fr0;
        } else if (avail1) {
            return fr1;
        }
        
        return NULL;
    }

    return (start0 > start1) ? fr0 : fr1;
}

UBool
RuleBasedTimeZone::findNext(UDate base, UBool inclusive, UDate& transitionTime,
                            TimeZoneRule*& fromRule, TimeZoneRule*& toRule) const {
    if (fHistoricTransitions == NULL) {
        return FALSE;
    }
    UBool isFinal = FALSE;
    UBool found = FALSE;
    Transition result;
    Transition *tzt = (Transition*)fHistoricTransitions->elementAt(0);
    UDate tt = tzt->time;
    if (tt > base || (inclusive && tt == base)) {
        result = *tzt;
        found = TRUE;
    } else {
        int32_t idx = fHistoricTransitions->size() - 1;        
        tzt = (Transition*)fHistoricTransitions->elementAt(idx);
        tt = tzt->time;
        if (inclusive && tt == base) {
            result = *tzt;
            found = TRUE;
        } else if (tt <= base) {
            if (fFinalRules != NULL) {
                
                TimeZoneRule *r0 = (TimeZoneRule*)fFinalRules->elementAt(0);
                TimeZoneRule *r1 = (TimeZoneRule*)fFinalRules->elementAt(1);
                UDate start0, start1;
                UBool avail0 = r0->getNextStart(base, r1->getRawOffset(), r1->getDSTSavings(), inclusive, start0);
                UBool avail1 = r1->getNextStart(base, r0->getRawOffset(), r0->getDSTSavings(), inclusive, start1);
                
                if (!avail0 && !avail1) {
                    return FALSE;
                }
                if (!avail1 || start0 < start1) {
                    result.time = start0;
                    result.from = r1;
                    result.to = r0;
                } else {
                    result.time = start1;
                    result.from = r0;
                    result.to = r1;
                }
                isFinal = TRUE;
                found = TRUE;
            }
        } else {
            
            idx--;
            Transition *prev = tzt;
            while (idx > 0) {
                tzt = (Transition*)fHistoricTransitions->elementAt(idx);
                tt = tzt->time;
                if (tt < base || (!inclusive && tt == base)) {
                    break;
                }
                idx--;
                prev = tzt;
            }
            result.time = prev->time;
            result.from = prev->from;
            result.to = prev->to;
            found = TRUE;
        }
    }
    if (found) {
        
        if (result.from->getRawOffset() == result.to->getRawOffset()
            && result.from->getDSTSavings() == result.to->getDSTSavings()) {
            if (isFinal) {
                return FALSE;
            } else {
                
                return findNext(result.time, FALSE ,
                    transitionTime, fromRule, toRule);
            }
        }
        transitionTime = result.time;
        fromRule = result.from;
        toRule = result.to;
        return TRUE;
    }
    return FALSE;
}

UBool
RuleBasedTimeZone::findPrev(UDate base, UBool inclusive, UDate& transitionTime,
                            TimeZoneRule*& fromRule, TimeZoneRule*& toRule) const {
    if (fHistoricTransitions == NULL) {
        return FALSE;
    }
    UBool found = FALSE;
    Transition result;
    Transition *tzt = (Transition*)fHistoricTransitions->elementAt(0);
    UDate tt = tzt->time;
    if (inclusive && tt == base) {
        result = *tzt;
        found = TRUE;
    } else if (tt < base) {
        int32_t idx = fHistoricTransitions->size() - 1;        
        tzt = (Transition*)fHistoricTransitions->elementAt(idx);
        tt = tzt->time;
        if (inclusive && tt == base) {
            result = *tzt;
            found = TRUE;
        } else if (tt < base) {
            if (fFinalRules != NULL) {
                
                TimeZoneRule *r0 = (TimeZoneRule*)fFinalRules->elementAt(0);
                TimeZoneRule *r1 = (TimeZoneRule*)fFinalRules->elementAt(1);
                UDate start0, start1;
                UBool avail0 = r0->getPreviousStart(base, r1->getRawOffset(), r1->getDSTSavings(), inclusive, start0);
                UBool avail1 = r1->getPreviousStart(base, r0->getRawOffset(), r0->getDSTSavings(), inclusive, start1);
                
                if (!avail0 && !avail1) {
                    return FALSE;
                }
                if (!avail1 || start0 > start1) {
                    result.time = start0;
                    result.from = r1;
                    result.to = r0;
                } else {
                    result.time = start1;
                    result.from = r0;
                    result.to = r1;
                }
            } else {
                result = *tzt;
            }
            found = TRUE;
        } else {
            
            idx--;
            while (idx >= 0) {
                tzt = (Transition*)fHistoricTransitions->elementAt(idx);
                tt = tzt->time;
                if (tt < base || (inclusive && tt == base)) {
                    break;
                }
                idx--;
            }
            result = *tzt;
            found = TRUE;
        }
    }
    if (found) {
        
        if (result.from->getRawOffset() == result.to->getRawOffset()
            && result.from->getDSTSavings() == result.to->getDSTSavings()) {
            
            return findPrev(result.time, FALSE ,
                transitionTime, fromRule, toRule);
        }
        transitionTime = result.time;
        fromRule = result.from;
        toRule = result.to;
        return TRUE;
    }
    return FALSE;
}

UDate
RuleBasedTimeZone::getTransitionTime(Transition* transition, UBool local,
                                     int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const {
    UDate time = transition->time;
    if (local) {
        time += getLocalDelta(transition->from->getRawOffset(), transition->from->getDSTSavings(),
                              transition->to->getRawOffset(), transition->to->getDSTSavings(),
                              NonExistingTimeOpt, DuplicatedTimeOpt);
    }
    return time;
}

int32_t
RuleBasedTimeZone::getLocalDelta(int32_t rawBefore, int32_t dstBefore, int32_t rawAfter, int32_t dstAfter,
                             int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const {
    int32_t delta = 0;

    int32_t offsetBefore = rawBefore + dstBefore;
    int32_t offsetAfter = rawAfter + dstAfter;

    UBool dstToStd = (dstBefore != 0) && (dstAfter == 0);
    UBool stdToDst = (dstBefore == 0) && (dstAfter != 0);

    if (offsetAfter - offsetBefore >= 0) {
        
        if (((NonExistingTimeOpt & kStdDstMask) == kStandard && dstToStd)
                || ((NonExistingTimeOpt & kStdDstMask) == kDaylight && stdToDst)) {
            delta = offsetBefore;
        } else if (((NonExistingTimeOpt & kStdDstMask) == kStandard && stdToDst)
                || ((NonExistingTimeOpt & kStdDstMask) == kDaylight && dstToStd)) {
            delta = offsetAfter;
        } else if ((NonExistingTimeOpt & kFormerLatterMask) == kLatter) {
            delta = offsetBefore;
        } else {
            
            
            delta = offsetAfter;
        }
    } else {
        
        if (((DuplicatedTimeOpt & kStdDstMask) == kStandard && dstToStd)
                || ((DuplicatedTimeOpt & kStdDstMask) == kDaylight && stdToDst)) {
            delta = offsetAfter;
        } else if (((DuplicatedTimeOpt & kStdDstMask) == kStandard && stdToDst)
                || ((DuplicatedTimeOpt & kStdDstMask) == kDaylight && dstToStd)) {
            delta = offsetBefore;
        } else if ((DuplicatedTimeOpt & kFormerLatterMask) == kFormer) {
            delta = offsetBefore;
        } else {
            
            
            delta = offsetAfter;
        }
    }
    return delta;
}

U_NAMESPACE_END

#endif 



