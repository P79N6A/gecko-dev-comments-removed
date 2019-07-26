







#ifndef mozilla_EnumSet_h
#define mozilla_EnumSet_h

#include "mozilla/Assertions.h"

#include <stdint.h>

namespace mozilla {






template<typename T>
class EnumSet
{
  public:
    EnumSet()
      : mBitField(0)
    { }

    EnumSet(T aEnum)
      : mBitField(bitFor(aEnum))
    { }

    EnumSet(T aEnum1, T aEnum2)
      : mBitField(bitFor(aEnum1) |
                  bitFor(aEnum2))
    { }

    EnumSet(T aEnum1, T aEnum2, T aEnum3)
      : mBitField(bitFor(aEnum1) |
                  bitFor(aEnum2) |
                  bitFor(aEnum3))
    { }

    EnumSet(T aEnum1, T aEnum2, T aEnum3, T aEnum4)
     : mBitField(bitFor(aEnum1) |
                 bitFor(aEnum2) |
                 bitFor(aEnum3) |
                 bitFor(aEnum4))
    { }

    EnumSet(const EnumSet& aEnumSet)
     : mBitField(aEnumSet.mBitField)
    { }

    


    void operator+=(T aEnum) {
      mBitField |= bitFor(aEnum);
    }

    


    EnumSet<T> operator+(T aEnum) const {
      EnumSet<T> result(*this);
      result += aEnum;
      return result;
    }

    


    void operator+=(const EnumSet<T> aEnumSet) {
      mBitField |= aEnumSet.mBitField;
    }

    


    EnumSet<T> operator+(const EnumSet<T> aEnumSet) const {
      EnumSet<T> result(*this);
      result += aEnumSet;
      return result;
    }

    


    void operator-=(T aEnum) {
      mBitField &= ~(bitFor(aEnum));
    }

    


    EnumSet<T> operator-(T aEnum) const {
      EnumSet<T> result(*this);
      result -= aEnum;
      return result;
    }

    


    void operator-=(const EnumSet<T> aEnumSet) {
      mBitField &= ~(aEnumSet.mBitField);
    }

    


    EnumSet<T> operator-(const EnumSet<T> aEnumSet) const {
      EnumSet<T> result(*this);
      result -= aEnumSet;
      return result;
    }

    


    void operator&=(const EnumSet<T> aEnumSet) {
      mBitField &= aEnumSet.mBitField;
    }

    


    EnumSet<T> operator&(const EnumSet<T> aEnumSet) const {
      EnumSet<T> result(*this);
      result &= aEnumSet;
      return result;
    }

    



    bool operator==(const EnumSet<T> aEnumSet) const {
      return mBitField == aEnumSet.mBitField;
    }

    


    bool contains(T aEnum) const {
      return mBitField & bitFor(aEnum);
    }

    



    uint8_t size() {
      uint8_t count = 0;
      for (uint32_t bitField = mBitField; bitField; bitField >>= 1) {
        if (bitField & 1)
          count++;
      }
      return count;
    }

    bool isEmpty() const {
      return mBitField == 0;
    }

    uint32_t serialize() const {
      return mBitField;
    }

    void deserialize(uint32_t aValue) {
      mBitField = aValue;
    }

  private:
    static uint32_t bitFor(T aEnum) {
      uint32_t bitNumber = (uint32_t)aEnum;
      MOZ_ASSERT(bitNumber < 32);
      return 1U << bitNumber;
    }

    uint32_t mBitField;
};

} 

#endif 
