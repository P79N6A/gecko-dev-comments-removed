









#ifndef MEASUREFORMAT_H
#define MEASUREFORMAT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"






U_NAMESPACE_BEGIN















class U_I18N_API MeasureFormat : public Format {
 public:
    



    virtual ~MeasureFormat();

    







    static MeasureFormat* U_EXPORT2 createCurrencyFormat(const Locale& locale,
                                               UErrorCode& ec);

    






    static MeasureFormat* U_EXPORT2 createCurrencyFormat(UErrorCode& ec);

 protected:

    



    MeasureFormat();
};

U_NAMESPACE_END

#endif 
#endif 
