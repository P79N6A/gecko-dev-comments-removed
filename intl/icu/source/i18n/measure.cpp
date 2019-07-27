









#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/measure.h"
#include "unicode/measunit.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(Measure)

Measure::Measure() {}

Measure::Measure(const Formattable& _number, MeasureUnit* adoptedUnit,
                 UErrorCode& ec) :
    number(_number), unit(adoptedUnit) {
    if (U_SUCCESS(ec) &&
        (!number.isNumeric() || adoptedUnit == 0)) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
    }
}

Measure::Measure(const Measure& other) :
    UObject(other), unit(0) {
    *this = other;
}

Measure& Measure::operator=(const Measure& other) {
    if (this != &other) {
        delete unit;
        number = other.number;
        unit = (MeasureUnit*) other.unit->clone();
    }
    return *this;
}

UObject *Measure::clone() const {
    return new Measure(*this);
}

Measure::~Measure() {
    delete unit;
}

UBool Measure::operator==(const UObject& other) const {
    if (this == &other) {  
        return TRUE;
    }
    if (typeid(*this) != typeid(other)) { 
        return FALSE;
    }
    const Measure &m = static_cast<const Measure&>(other);
    return number == m.number &&
        ((unit == NULL) == (m.unit == NULL)) &&
        (unit == NULL || *unit == *m.unit);
}

U_NAMESPACE_END

#endif 
