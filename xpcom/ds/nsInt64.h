




































#ifndef nsInt64_h__
#define nsInt64_h__

#include "prlong.h"
#include "nscore.h"










template<class T>
class nsTInt64
{
public: 
    T mValue;

public:
    


    nsTInt64(void) : mValue(LL_ZERO) {
    }

    


    nsTInt64(const PRInt32 aInt32) {
        LL_I2L(mValue, aInt32);
    }

    


    nsTInt64(const PRUint32 aUint32) {
        LL_UI2L(mValue, aUint32);
    }

    


    nsTInt64(const PRFloat64 aFloat64) {
        LL_D2L(mValue, aFloat64);
    }

    


    nsTInt64(const T aInt64) : mValue(aInt64) {
    }

    


    nsTInt64(const nsTInt64& aObject) : mValue(aObject.mValue) {
    }

    

    


    const nsTInt64& operator =(const nsTInt64& aObject) {
        mValue = aObject.mValue;
        return *this;
    }

    


    operator PRInt32(void) const {
        PRInt32 result;
        LL_L2I(result, mValue);
        return result;
    }

    


    operator PRUint32(void) const {
        PRUint32 result;
        LL_L2UI(result, mValue);
        return result;
    }

    


    operator PRFloat64(void) const {
        PRFloat64 result;
        LL_L2D(result, mValue);
        return result;
    }

    


    operator T() const {
        return mValue;
    }

    


    const nsTInt64 operator -(void) {
        nsTInt64 result;
        LL_NEG(result.mValue, mValue);
        return result;
    }

    

    


    nsTInt64& operator +=(const nsTInt64& aObject) {
        LL_ADD(mValue, mValue, aObject.mValue);
        return *this;
    }

    


    nsTInt64& operator -=(const nsTInt64& aObject) {
        LL_SUB(mValue, mValue, aObject.mValue);
        return *this;
    }

    


    nsTInt64& operator *=(const nsTInt64& aObject) {
        LL_MUL(mValue, mValue, aObject.mValue);
        return *this;
    }

    


    nsTInt64& operator /=(const nsTInt64& aObject) {
        LL_DIV(mValue, mValue, aObject.mValue);
        return *this;
    }

    


    nsTInt64& operator %=(const nsTInt64& aObject) {
        LL_MOD(mValue, mValue, aObject.mValue);
        return *this;
    }

    


    nsTInt64& operator <<=(int aCount) {
        LL_SHL(mValue, mValue, aCount);
        return *this;
    }

    


    nsTInt64& operator >>=(int aCount) {
        LL_SHR(mValue, mValue, aCount);
        return *this;
    }

    
    


    inline const nsTInt64
    operator +(const nsTInt64& aObject2) const {
        return nsTInt64(*this) += aObject2;
    }

    


    inline const nsTInt64
    operator -(const nsTInt64& aObject2) const {
        return nsTInt64(*this) -= aObject2;
    }

    


    inline const nsTInt64
    operator *(const nsTInt64& aObject2) const {
        return nsTInt64(*this) *= aObject2;
    }

    


    inline const nsTInt64
    operator /(const nsTInt64& aObject2) const {
        return nsTInt64(*this) /= aObject2;
    }

    


    inline const nsTInt64
    operator %(const nsTInt64& aObject2) const {
        return nsTInt64(*this) %= aObject2;
    }

    


    inline const nsTInt64
    operator <<(int aCount) const {
        return nsTInt64(*this) <<= aCount;
    }

    


    inline const nsTInt64
    operator >>(int aCount) const {
        return nsTInt64(*this) >>= aCount;
    }

    


    inline PRBool
    operator ==(const nsTInt64& aObject2) const {
        return LL_EQ(mValue, aObject2.mValue);
    }

    inline PRBool
    operator ==(T aValue) const {
        return LL_EQ(mValue, aValue);
    }

    


    inline PRBool
    operator !=(const nsTInt64& aObject2) const {
        return LL_NE(mValue, aObject2.mValue);
    }

    inline PRBool
    operator !=(T aValue) const {
        return LL_NE(mValue, aValue);
    }

    


    inline const nsTInt64
    operator &(const nsTInt64& aObject2) const {
        return nsTInt64(*this) &= aObject2;
    }

    


    inline const nsTInt64
    operator |(const nsTInt64& aObject2) const {
        return nsTInt64(*this) |= aObject2;
    }

    


    inline const nsTInt64
    operator ^(const nsTInt64& aObject2) const {
        return nsTInt64(*this) ^= aObject2;
    }

    

    


    const nsTInt64 operator ~(void) const {
        nsTInt64 result;
        LL_NOT(result.mValue, mValue);
        return result;
    }

    


    nsTInt64& operator &=(const nsTInt64& aObject) {
        LL_AND(mValue, mValue, aObject.mValue);
        return *this;
    }

    


    nsTInt64& operator |=(const nsTInt64& aObject) {
        LL_OR(mValue, mValue, aObject.mValue);
        return *this;
    }

    


    nsTInt64& operator ^=(const nsTInt64& aObject) {
        LL_XOR(mValue, mValue, aObject.mValue);
        return *this;
    }


    


    PRBool operator!() const {
        return LL_IS_ZERO(mValue);
    }
    
};

typedef nsTInt64<PRInt64> nsInt64;
typedef nsTInt64<PRUint64> nsUint64;




inline PRBool
operator >(const nsInt64& aObject1, const nsInt64& aObject2) {
    return LL_CMP(aObject1.mValue, >, aObject2.mValue);
}

inline PRBool
operator >(const nsUint64& aObject1, const nsUint64& aObject2) {
    return LL_UCMP(aObject1.mValue, >, aObject2.mValue);
}




inline PRBool
operator >=(const nsInt64& aObject1, const nsInt64& aObject2) {
    return ! LL_CMP(aObject1.mValue, <, aObject2.mValue);
}

inline PRBool
operator >=(const nsUint64& aObject1, const nsUint64& aObject2) {
    return ! LL_UCMP(aObject1.mValue, <, aObject2.mValue);
}




inline PRBool 
operator <(const nsInt64& aObject1, const nsInt64& aObject2) {
    return LL_CMP(aObject1.mValue, <, aObject2.mValue);
}

inline PRBool 
operator <(const nsUint64& aObject1, const nsUint64& aObject2) {
    return LL_UCMP(aObject1.mValue, <, aObject2.mValue);
}




inline PRBool
operator <=(const nsInt64& aObject1, const nsInt64& aObject2) {
    return ! LL_CMP(aObject1.mValue, >, aObject2.mValue);
}

inline PRBool
operator <=(const nsUint64& aObject1, const nsUint64& aObject2) {
    return ! LL_UCMP(aObject1.mValue, >, aObject2.mValue);
}

#endif 
