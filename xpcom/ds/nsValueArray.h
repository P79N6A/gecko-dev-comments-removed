






































#ifndef nsValueArray_h___
#define nsValueArray_h___










#include "nscore.h"

typedef PRUint32 nsValueArrayCount;
typedef PRUint32 nsValueArrayIndex;
typedef PRUint32 nsValueArrayValue;
#define NSVALUEARRAY_INVALID ((nsValueArrayValue)-1)

class NS_COM nsValueArray {
  public:
    nsValueArray(nsValueArrayValue aMaxValue,
                 nsValueArrayCount aInitialCapacity = 0);
    ~nsValueArray();

    
    
    
  public:
    nsValueArray& operator=(const nsValueArray& other);

    
    
    
    
  public:
    inline nsValueArrayCount Count() const {
        return mCount;
    }

    inline nsValueArrayCount Capacity() const {
        return mCapacity;
    }

    void Compact();

    
    inline void Clear() {
        mCount = 0;
    }

    
    
    
  public:
    nsValueArrayValue ValueAt(nsValueArrayIndex aIndex) const;

    inline nsValueArrayValue operator[](nsValueArrayIndex aIndex) const {
        return ValueAt(aIndex);
    }

    nsValueArrayIndex IndexOf(nsValueArrayValue aPossibleValue) const;

    inline PRBool AppendValue(nsValueArrayValue aValue) {
        return InsertValueAt(aValue, Count());
    }

    inline PRBool RemoveValue(nsValueArrayValue aValue) {
        return  RemoveValueAt(IndexOf(aValue));
    }

    PRBool InsertValueAt(nsValueArrayValue aValue, nsValueArrayIndex aIndex);

    PRBool ReplaceValueAt(nsValueArrayValue aValue, nsValueArrayIndex aIndex);

    PRBool RemoveValueAt(nsValueArrayIndex aIndex);

    
    
    
  private:
    nsValueArrayCount mCount;
    nsValueArrayCount mCapacity;
    PRUint8* mValueArray;
    PRUint8 mBytesPerValue;
};

#endif 
