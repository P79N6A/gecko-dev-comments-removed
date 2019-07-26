









#ifndef __MEASUREUNIT_H__
#define __MEASUREUNIT_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/fmtable.h"





 
U_NAMESPACE_BEGIN










class U_I18N_API MeasureUnit: public UObject {
 public:
    




    virtual UObject* clone() const = 0;

    



    virtual ~MeasureUnit();
    
    




    virtual UBool operator==(const UObject& other) const = 0;

 protected:
    



    MeasureUnit();
};

U_NAMESPACE_END



#endif 
#endif 
