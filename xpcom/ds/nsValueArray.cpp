
















































#include "nsValueArray.h"
#include "nsCRT.h"
#include "prmem.h"
#include "prbit.h"

#define NSVALUEARRAY_LINEAR_GROWBY 8
#define NSVALUEARRAY_LINEAR_THRESHOLD  128

nsValueArray::nsValueArray(nsValueArrayValue aMaxValue, nsValueArrayCount aInitialCapacity) {
    mCount = 0;
    mCapacity = 0;
    mValueArray = nsnull;

    PRUint8 test8 = (PRUint8)aMaxValue;
    PRUint16 test16 = (PRUint16)aMaxValue;
    PRUint32 test32 = (PRUint32)aMaxValue;
    if ((nsValueArrayValue)test8 == aMaxValue) {
        mBytesPerValue = sizeof(test8);
    }
    else if ((nsValueArrayValue)test16 == aMaxValue) {
        mBytesPerValue = sizeof(test16);
    }
    else if ((nsValueArrayValue)test32 == aMaxValue) {
        mBytesPerValue = sizeof(test32);
    }
    else {
        NS_ASSERTION(0, "not supported yet, add it yourself...");
        mBytesPerValue = 0;
    }

    if (aInitialCapacity) {
        mValueArray = (PRUint8*)PR_Malloc(aInitialCapacity * mBytesPerValue);
        if (nsnull != mValueArray) {
            mCapacity = aInitialCapacity;
        }
    }
}

nsValueArray::~nsValueArray() {
    if (nsnull != mValueArray) {
        PR_Free(mValueArray);
        mValueArray = nsnull;
    }
}




nsValueArray& nsValueArray::operator=(const nsValueArray& aOther) {
    
    
    
    if ((mBytesPerValue != aOther.mBytesPerValue || mCapacity < aOther.mCount) && nsnull != mValueArray) {
        PR_Free(mValueArray);
        mValueArray = nsnull;
        mCount = mCapacity = 0;
    }

    
    
    
    mBytesPerValue = aOther.mBytesPerValue;
    mCount = aOther.mCount;

    
    
    
    if (0 != mCount) {
        
        
        
        if (0 == mCapacity) {
            mValueArray = (PRUint8*)PR_Malloc(mCount * mBytesPerValue);
            mCapacity = mCount;
        }

        NS_ASSERTION(nsnull != mValueArray, "loss of value array assignment and original data.");
        if (nsnull != mValueArray) {
            memcpy(mValueArray, aOther.mValueArray, mCount * mBytesPerValue);
        }
        else {
            mCount = mCapacity = 0;
        }
    }
    
    return *this;
}





PRBool nsValueArray::InsertValueAt(nsValueArrayValue aValue, nsValueArrayIndex aIndex) {
    PRBool retval = PR_FALSE;

    nsValueArrayCount count = Count();
    if (aIndex <= count) {
        
        
        
        if (Capacity() == count) {
            PRUint8* reallocRes = nsnull;
            nsValueArrayCount growBy = NSVALUEARRAY_LINEAR_GROWBY;

            
            
            
            
            if (count >= NSVALUEARRAY_LINEAR_THRESHOLD) {
                growBy = PR_BIT(PR_CeilingLog2(count + 1)) - count;
            }

            if (nsnull == mValueArray) {
                reallocRes = (PRUint8*)PR_Malloc((count + growBy) * mBytesPerValue);
            }
            else {
                reallocRes = (PRUint8*)PR_Realloc(mValueArray, (count + growBy) * mBytesPerValue);
            }
            if (nsnull != reallocRes) {
                mValueArray = reallocRes;
                mCapacity += growBy;
            }
        }

        
        
        
        if (Capacity() > count) {
            
            
            
            if (aIndex < count) {
                memmove(&mValueArray[(aIndex + 1) * mBytesPerValue], &mValueArray[aIndex * mBytesPerValue], (count - aIndex) * mBytesPerValue);
            }

            
            
            
            switch (mBytesPerValue) {
                case sizeof(PRUint8):
                    *((PRUint8*)&mValueArray[aIndex * mBytesPerValue]) = (PRUint8)aValue;
                    NS_ASSERTION(*((PRUint8*)&mValueArray[aIndex * mBytesPerValue]) == aValue, "Lossy value array detected.  Report a higher maximum upon construction!");
                    break;
                case sizeof(PRUint16):
                    *((PRUint16*)&mValueArray[aIndex * mBytesPerValue]) = (PRUint16)aValue;
                    NS_ASSERTION(*((PRUint16*)&mValueArray[aIndex * mBytesPerValue]) == aValue, "Lossy value array detected.  Report a higher maximum upon construction!");
                    break;
                case sizeof(PRUint32):
                    *((PRUint32*)&mValueArray[aIndex * mBytesPerValue]) = (PRUint32)aValue;
                    NS_ASSERTION(*((PRUint32*)&mValueArray[aIndex * mBytesPerValue]) == aValue, "Lossy value array detected.  Report a higher maximum upon construction!");
                    break;
                default:
                    NS_ERROR("surely you've been warned prior to this!");
                    break;
            }

            
            
            
            mCount++;
        }
    }

    return retval;
}





PRBool nsValueArray::ReplaceValueAt(nsValueArrayValue aValue, nsValueArrayIndex aIndex) {
    NS_ENSURE_TRUE(aIndex < Count(), PR_FALSE);
       
    
    
    
    switch (mBytesPerValue) {
    case sizeof(PRUint8):
        *((PRUint8*)&mValueArray[aIndex * mBytesPerValue]) = (PRUint8)aValue;
        NS_ASSERTION(*((PRUint8*)&mValueArray[aIndex * mBytesPerValue]) == aValue, "Lossy value array detected.  Report a higher maximum upon construction!");
        break;
    case sizeof(PRUint16):
        *((PRUint16*)&mValueArray[aIndex * mBytesPerValue]) = (PRUint16)aValue;
        NS_ASSERTION(*((PRUint16*)&mValueArray[aIndex * mBytesPerValue]) == aValue, "Lossy value array detected.  Report a higher maximum upon construction!");
        break;
    case sizeof(PRUint32):
        *((PRUint32*)&mValueArray[aIndex * mBytesPerValue]) = (PRUint32)aValue;
        NS_ASSERTION(*((PRUint32*)&mValueArray[aIndex * mBytesPerValue]) == aValue, "Lossy value array detected.  Report a higher maximum upon construction!");
        break;
    default:
        NS_ERROR("surely you've been warned prior to this!");
        break;
    }
    
    return PR_TRUE;
}





PRBool nsValueArray::RemoveValueAt(nsValueArrayIndex aIndex) {
    PRBool retval = PR_FALSE;

    nsValueArrayCount count = Count();
    if (aIndex < count) {
        
        
        
        if (aIndex != (count - 1)) {
            memmove(&mValueArray[aIndex * mBytesPerValue], &mValueArray[(aIndex + 1) * mBytesPerValue], (count - aIndex - 1) * mBytesPerValue);
        }

        
        
        
        mCount--;
    }

    return retval;
}




void nsValueArray::Compact() {
    nsValueArrayCount count = Count();
    if (Capacity() != count)
    {
        if (0 == count) {
            PR_Free(mValueArray);
            mValueArray = nsnull;
            mCapacity = 0;
        }
        else {
            PRUint8* reallocRes = (PRUint8*)PR_Realloc(mValueArray, count * mBytesPerValue);
            if (nsnull != reallocRes) {
                mValueArray = reallocRes;
                mCapacity = count;
            }
        }
    }
}




nsValueArrayValue nsValueArray::ValueAt(nsValueArrayIndex aIndex) const {
    nsValueArrayValue retval = NSVALUEARRAY_INVALID;
    
    if (aIndex < Count()) {
        switch (mBytesPerValue) {
            case sizeof(PRUint8):
                retval = (nsValueArrayIndex)*((PRUint8*)&mValueArray[aIndex * mBytesPerValue]);
                break;
            case sizeof(PRUint16):
                retval = (nsValueArrayIndex)*((PRUint16*)&mValueArray[aIndex * mBytesPerValue]);
                break;
            case sizeof(PRUint32):
                retval = (nsValueArrayIndex)*((PRUint32*)&mValueArray[aIndex * mBytesPerValue]);
                break;
            default:
                NS_ASSERTION(0, "unexpected for sure.");
                break;
        }
    }
    
    return retval;
}




nsValueArrayIndex nsValueArray::IndexOf(nsValueArrayValue aPossibleValue) const {
    nsValueArrayIndex retval = NSVALUEARRAY_INVALID;
    nsValueArrayIndex traverse;
    
    for (traverse = 0; traverse < Count(); traverse++) {
        if (aPossibleValue == ValueAt(traverse)) {
            retval = traverse;
            break;
        }
    }
    
    return retval;
}
