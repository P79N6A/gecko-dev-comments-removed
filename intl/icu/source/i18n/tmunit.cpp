






#include "unicode/tmunit.h"
#include "uassert.h"

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TimeUnit)








































TimeUnit* U_EXPORT2 
TimeUnit::createInstance(TimeUnit::UTimeUnitFields timeUnitField, 
                         UErrorCode& status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    if (timeUnitField < 0 || timeUnitField >= UTIMEUNIT_FIELD_COUNT) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    return new TimeUnit(timeUnitField);
}


TimeUnit::TimeUnit(TimeUnit::UTimeUnitFields timeUnitField) {
    fTimeUnitField = timeUnitField;
    switch (fTimeUnitField) {
    case UTIMEUNIT_YEAR:
        initTime("year");
        break;
    case UTIMEUNIT_MONTH:
        initTime("month");
        break;
    case UTIMEUNIT_DAY:
        initTime("day");
        break;
    case UTIMEUNIT_WEEK:
        initTime("week");
        break;
    case UTIMEUNIT_HOUR:
        initTime("hour");
        break;
    case UTIMEUNIT_MINUTE:
        initTime("minute");
        break;
    case UTIMEUNIT_SECOND:
        initTime("second");
        break;
    default:
        U_ASSERT(false);
        break;
    }
}

TimeUnit::TimeUnit(const TimeUnit& other) 
:   MeasureUnit(other), fTimeUnitField(other.fTimeUnitField) {
}

UObject* 
TimeUnit::clone() const {
    return new TimeUnit(*this);
}

TimeUnit&
TimeUnit::operator=(const TimeUnit& other) {
    if (this == &other) {
        return *this;
    }
    MeasureUnit::operator=(other);
    fTimeUnitField = other.fTimeUnitField;
    return *this;
}

TimeUnit::UTimeUnitFields
TimeUnit::getTimeUnitField() const {
    return fTimeUnitField;
}

TimeUnit::~TimeUnit() {
}


U_NAMESPACE_END

#endif
