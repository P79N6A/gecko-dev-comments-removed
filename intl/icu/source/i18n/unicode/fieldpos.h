



















 
#ifndef FIELDPOS_H
#define FIELDPOS_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"

U_NAMESPACE_BEGIN





































































class U_I18N_API FieldPosition : public UObject {
public:
    



    enum { DONT_CARE = -1 };

    



    FieldPosition() 
        : UObject(), fField(DONT_CARE), fBeginIndex(0), fEndIndex(0) {}

    










    FieldPosition(int32_t field) 
        : UObject(), fField(field), fBeginIndex(0), fEndIndex(0) {}

    




    FieldPosition(const FieldPosition& copy) 
        : UObject(copy), fField(copy.fField), fBeginIndex(copy.fBeginIndex), fEndIndex(copy.fEndIndex) {}

    



    virtual ~FieldPosition();

    




    FieldPosition&      operator=(const FieldPosition& copy);

    





    UBool              operator==(const FieldPosition& that) const;

    





    UBool              operator!=(const FieldPosition& that) const;

    










    FieldPosition *clone() const;

    




    int32_t getField(void) const { return fField; }

    




    int32_t getBeginIndex(void) const { return fBeginIndex; }

    






    int32_t getEndIndex(void) const { return fEndIndex; }
 
    




    void setField(int32_t f) { fField = f; }

    




    void setBeginIndex(int32_t bi) { fBeginIndex = bi; }

    




    void setEndIndex(int32_t ei) { fEndIndex = ei; }
    
    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

private:
    



    int32_t fField;

    



    int32_t fBeginIndex;

    



    int32_t fEndIndex;
};

inline FieldPosition&
FieldPosition::operator=(const FieldPosition& copy)
{
    fField         = copy.fField;
    fEndIndex     = copy.fEndIndex;
    fBeginIndex = copy.fBeginIndex;
    return *this;
}

inline UBool
FieldPosition::operator==(const FieldPosition& copy) const
{
    return (fField == copy.fField &&
        fEndIndex == copy.fEndIndex &&
        fBeginIndex == copy.fBeginIndex);
}

inline UBool
FieldPosition::operator!=(const FieldPosition& copy) const
{
    return !operator==(copy);
}

U_NAMESPACE_END

#endif 

#endif 

