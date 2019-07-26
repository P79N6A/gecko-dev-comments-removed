














#ifndef FPOSITER_H
#define FPOSITER_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"






#if UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN





class FieldPositionIterator;

U_NAMESPACE_END

#else

#include "unicode/fieldpos.h"
#include "unicode/umisc.h"

U_NAMESPACE_BEGIN

class UVector32;






class U_I18N_API FieldPositionIterator : public UObject {
public:
    



    ~FieldPositionIterator();

    



    FieldPositionIterator(void);

    




    FieldPositionIterator(const FieldPositionIterator&);

    







    UBool operator==(const FieldPositionIterator&) const;

    





    UBool operator!=(const FieldPositionIterator& rhs) const { return !operator==(rhs); }

    




    UBool next(FieldPosition& fp);

private:
    friend class FieldPositionIteratorHandler;

    




    void setData(UVector32 *adopt, UErrorCode& status);

    UVector32 *data;
    int32_t pos;

    
    virtual UClassID getDynamicClassID() const;
};

U_NAMESPACE_END

#endif 

#endif 
