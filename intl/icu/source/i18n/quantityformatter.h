







#ifndef __QUANTITY_FORMATTER_H__
#define __QUANTITY_FORMATTER_H__

#include "unicode/utypes.h"
#include "unicode/uobject.h"

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

class SimplePatternFormatter;
class UnicodeString;
class PluralRules;
class NumberFormat;
class Formattable;
class FieldPosition;














class U_I18N_API QuantityFormatter : public UMemory {
public:
    


    QuantityFormatter();

    


    QuantityFormatter(const QuantityFormatter& other);

    


    QuantityFormatter &operator=(const QuantityFormatter& other);

    


    ~QuantityFormatter();

    


    void reset();

    







    UBool add(
            const char *variant,
            const UnicodeString &rawPattern,
            UErrorCode &status);

    


    UBool isValid() const;

    




    const SimplePatternFormatter *getByVariant(const char *variant) const;

    











    UnicodeString &format(
            const Formattable &quantity,
            const NumberFormat &fmt,
            const PluralRules &rules,
            UnicodeString &appendTo,
            FieldPosition &pos,
            UErrorCode &status) const;

private:
    SimplePatternFormatter *formatters[6];
};

U_NAMESPACE_END

#endif

#endif
