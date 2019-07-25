






































#ifndef nsStyleCoord_h___
#define nsStyleCoord_h___

#include "nscore.h"
#include "nsCoord.h"
#include "nsCRT.h"
#include "nsStyleConsts.h"
class nsString;
class nsStyleContext;

enum nsStyleUnit {
  eStyleUnit_Null         = 0,      
  eStyleUnit_Normal       = 1,      
  eStyleUnit_Auto         = 2,      
  eStyleUnit_None         = 3,      
  eStyleUnit_Percent      = 10,     
  eStyleUnit_Factor       = 11,     
  eStyleUnit_Degree       = 12,     
  eStyleUnit_Grad         = 13,     
  eStyleUnit_Radian       = 14,     
  eStyleUnit_Coord        = 20,     
  eStyleUnit_Integer      = 30,     
  eStyleUnit_Enumerated   = 32,     
  
  
  
  
  eStyleUnit_Calc         = 39,     
                                    
  eStyleUnit_Calc_Plus    = 40,     
  eStyleUnit_Calc_Minus   = 41,     
  eStyleUnit_Calc_Times_L = 42,     
  eStyleUnit_Calc_Times_R = 43,     
  eStyleUnit_Calc_Divided = 44,     
  eStyleUnit_Calc_Minimum = 45,     
  eStyleUnit_Calc_Maximum = 46      
};

typedef union {
  PRInt32     mInt;   
  float       mFloat;
  
  
  
  
  
  void*       mPointer;
} nsStyleUnion;









class nsStyleCoord {
public:
  struct Array;
  friend struct Array;

  nsStyleCoord(nsStyleUnit aUnit = eStyleUnit_Null);
  enum CoordConstructorType { CoordConstructor };
  inline nsStyleCoord(nscoord aValue, CoordConstructorType);
  nsStyleCoord(PRInt32 aValue, nsStyleUnit aUnit);
  nsStyleCoord(float aValue, nsStyleUnit aUnit);
  inline nsStyleCoord(const nsStyleCoord& aCopy);
  inline nsStyleCoord(const nsStyleUnion& aValue, nsStyleUnit aUnit);

  nsStyleCoord&  operator=(const nsStyleCoord& aCopy);
  PRBool         operator==(const nsStyleCoord& aOther) const;
  PRBool         operator!=(const nsStyleCoord& aOther) const;

  nsStyleUnit GetUnit() const {
    NS_ASSERTION(mUnit != eStyleUnit_Null, "reading uninitialized value");
    return mUnit;
  }

  PRBool IsAngleValue() const {
    return eStyleUnit_Degree <= mUnit && mUnit <= eStyleUnit_Radian;
  }

  PRBool IsCalcUnit() const {
    return eStyleUnit_Calc <= mUnit && mUnit <= eStyleUnit_Calc_Maximum;
  }

  PRBool IsArrayValue() const {
    return IsCalcUnit();
  }

  nscoord     GetCoordValue() const;
  PRInt32     GetIntValue() const;
  float       GetPercentValue() const;
  float       GetFactorValue() const;
  float       GetAngleValue() const;
  double      GetAngleValueInRadians() const;
  Array*      GetArrayValue() const;
  void        GetUnionValue(nsStyleUnion& aValue) const;

  void  Reset();  
  void  SetCoordValue(nscoord aValue);
  void  SetIntValue(PRInt32 aValue, nsStyleUnit aUnit);
  void  SetPercentValue(float aValue);
  void  SetFactorValue(float aValue);
  void  SetAngleValue(float aValue, nsStyleUnit aUnit);
  void  SetNormalValue();
  void  SetAutoValue();
  void  SetNoneValue();
  void  SetArrayValue(Array* aValue, nsStyleUnit aUnit);

public: 
  nsStyleUnit   mUnit;
  nsStyleUnion  mValue;
};



struct nsStyleCoord::Array {
  static Array* Create(nsStyleContext *aAllocationContext,
                       PRBool& aCanStoreInRuleTree,
                       size_t aCount);

  size_t Count() const { return mCount; }

  nsStyleCoord& operator[](size_t aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  const nsStyleCoord& operator[](size_t aIndex) const {
    NS_ABORT_IF_FALSE(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  
  nsStyleCoord& Item(size_t aIndex) { return (*this)[aIndex]; }
  const nsStyleCoord& Item(size_t aIndex) const { return (*this)[aIndex]; }

  bool operator==(const Array& aOther) const;

  bool operator!=(const Array& aOther) const {
    return !(*this == aOther);
  }

private:
  inline void* operator new(size_t aSelfSize,
                            nsStyleContext *aAllocationContext,
                            size_t aItemCount) CPP_THROW_NEW;

  Array(size_t aCount)
    : mCount(aCount)
  {
    
    for (size_t i = 1; i < aCount; ++i) {
      new (mArray + i) nsStyleCoord();
    }
  }

  size_t mCount;
  nsStyleCoord mArray[1]; 

  
  Array(const Array& aOther);
  Array& operator=(const Array& aOther);
  ~Array();
};






class nsStyleSides {
public:
  nsStyleSides();


  PRBool         operator==(const nsStyleSides& aOther) const;
  PRBool         operator!=(const nsStyleSides& aOther) const;

  inline nsStyleUnit GetUnit(mozilla::css::Side aSide) const;
  inline nsStyleUnit GetLeftUnit() const;
  inline nsStyleUnit GetTopUnit() const;
  inline nsStyleUnit GetRightUnit() const;
  inline nsStyleUnit GetBottomUnit() const;

  inline nsStyleCoord Get(mozilla::css::Side aSide) const;
  inline nsStyleCoord GetLeft() const;
  inline nsStyleCoord GetTop() const;
  inline nsStyleCoord GetRight() const;
  inline nsStyleCoord GetBottom() const;

  void  Reset();

  inline void Set(mozilla::css::Side aSide, const nsStyleCoord& aCoord);
  inline void SetLeft(const nsStyleCoord& aCoord);
  inline void SetTop(const nsStyleCoord& aCoord);
  inline void SetRight(const nsStyleCoord& aCoord);
  inline void SetBottom(const nsStyleCoord& aCoord);

protected:
  PRUint8       mUnits[4];
  nsStyleUnion  mValues[4];
};






class nsStyleCorners {
public:
  nsStyleCorners();

  
  
  PRBool         operator==(const nsStyleCorners& aOther) const;
  PRBool         operator!=(const nsStyleCorners& aOther) const;

  
  inline nsStyleUnit GetUnit(PRUint8 aHalfCorner) const;

  inline nsStyleCoord Get(PRUint8 aHalfCorner) const;

  void  Reset();

  inline void Set(PRUint8 aHalfCorner, const nsStyleCoord& aCoord);

protected:
  PRUint8       mUnits[8];
  nsStyleUnion  mValues[8];
};





inline nsStyleCoord::nsStyleCoord(nscoord aValue, CoordConstructorType)
  : mUnit(eStyleUnit_Coord)
{
  mValue.mInt = aValue;
}




inline nsStyleCoord::nsStyleCoord(const nsStyleCoord& aCopy)
  : mUnit(aCopy.mUnit)
{
  if ((eStyleUnit_Percent <= mUnit) && (mUnit < eStyleUnit_Coord)) {
    mValue.mFloat = aCopy.mValue.mFloat;
  }
  else if (IsArrayValue()) {
    mValue.mPointer = aCopy.mValue.mPointer;
  }
  else {
    mValue.mInt = aCopy.mValue.mInt;
  }
}

inline nsStyleCoord::nsStyleCoord(const nsStyleUnion& aValue, nsStyleUnit aUnit)
  : mUnit(aUnit)
{
#if PR_BYTES_PER_INT == PR_BYTES_PER_FLOAT
  mValue.mInt = aValue.mInt;
#else
  memcpy(&mValue, &aValue, sizeof(nsStyleUnion));
#endif
}

inline PRBool nsStyleCoord::operator!=(const nsStyleCoord& aOther) const
{
  return !((*this) == aOther);
}

inline PRInt32 nsStyleCoord::GetCoordValue() const
{
  NS_ASSERTION((mUnit == eStyleUnit_Coord), "not a coord value");
  if (mUnit == eStyleUnit_Coord) {
    return mValue.mInt;
  }
  return 0;
}

inline PRInt32 nsStyleCoord::GetIntValue() const
{
  NS_ASSERTION((mUnit == eStyleUnit_Enumerated) ||
               (mUnit == eStyleUnit_Integer), "not an int value");
  if ((mUnit == eStyleUnit_Enumerated) ||
      (mUnit == eStyleUnit_Integer)) {
    return mValue.mInt;
  }
  return 0;
}

inline float nsStyleCoord::GetPercentValue() const
{
  NS_ASSERTION(mUnit == eStyleUnit_Percent, "not a percent value");
  if (mUnit == eStyleUnit_Percent) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline float nsStyleCoord::GetFactorValue() const
{
  NS_ASSERTION(mUnit == eStyleUnit_Factor, "not a factor value");
  if (mUnit == eStyleUnit_Factor) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline float nsStyleCoord::GetAngleValue() const
{
  NS_ASSERTION(mUnit >= eStyleUnit_Degree &&
               mUnit <= eStyleUnit_Radian, "not an angle value");
  if (mUnit >= eStyleUnit_Degree && mUnit <= eStyleUnit_Radian) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline nsStyleCoord::Array* nsStyleCoord::GetArrayValue() const
{
  NS_ASSERTION(IsArrayValue(), "not a pointer value");
  if (IsArrayValue()) {
    return static_cast<Array*>(mValue.mPointer);
  }
  return nsnull;
}


inline void nsStyleCoord::GetUnionValue(nsStyleUnion& aValue) const
{
  memcpy(&aValue, &mValue, sizeof(nsStyleUnion));
}




inline PRBool nsStyleSides::operator!=(const nsStyleSides& aOther) const
{
  return !((*this) == aOther);
}

inline nsStyleUnit nsStyleSides::GetUnit(mozilla::css::Side aSide) const
{
  return (nsStyleUnit)mUnits[aSide];
}

inline nsStyleUnit nsStyleSides::GetLeftUnit() const
{
  return GetUnit(NS_SIDE_LEFT);
}

inline nsStyleUnit nsStyleSides::GetTopUnit() const
{
  return GetUnit(NS_SIDE_TOP);
}

inline nsStyleUnit nsStyleSides::GetRightUnit() const
{
  return GetUnit(NS_SIDE_RIGHT);
}

inline nsStyleUnit nsStyleSides::GetBottomUnit() const
{
  return GetUnit(NS_SIDE_BOTTOM);
}

inline nsStyleCoord nsStyleSides::Get(mozilla::css::Side aSide) const
{
  return nsStyleCoord(mValues[aSide], nsStyleUnit(mUnits[aSide]));
}

inline nsStyleCoord nsStyleSides::GetLeft() const
{
  return Get(NS_SIDE_LEFT);
}

inline nsStyleCoord nsStyleSides::GetTop() const
{
  return Get(NS_SIDE_TOP);
}

inline nsStyleCoord nsStyleSides::GetRight() const
{
  return Get(NS_SIDE_RIGHT);
}

inline nsStyleCoord nsStyleSides::GetBottom() const
{
  return Get(NS_SIDE_BOTTOM);
}

inline void nsStyleSides::Set(mozilla::css::Side aSide, const nsStyleCoord& aCoord)
{
  mUnits[aSide] = aCoord.GetUnit();
  aCoord.GetUnionValue(mValues[aSide]);
}

inline void nsStyleSides::SetLeft(const nsStyleCoord& aCoord)
{
  Set(NS_SIDE_LEFT, aCoord);
}

inline void nsStyleSides::SetTop(const nsStyleCoord& aCoord)
{
  Set(NS_SIDE_TOP, aCoord);
}

inline void nsStyleSides::SetRight(const nsStyleCoord& aCoord)
{
  Set(NS_SIDE_RIGHT, aCoord);
}

inline void nsStyleSides::SetBottom(const nsStyleCoord& aCoord)
{
  Set(NS_SIDE_BOTTOM, aCoord);
}




inline PRBool nsStyleCorners::operator!=(const nsStyleCorners& aOther) const
{
  return !((*this) == aOther);
}

inline nsStyleUnit nsStyleCorners::GetUnit(PRUint8 aCorner) const
{
  return (nsStyleUnit)mUnits[aCorner];
}

inline nsStyleCoord nsStyleCorners::Get(PRUint8 aCorner) const
{
  return nsStyleCoord(mValues[aCorner], nsStyleUnit(mUnits[aCorner]));
}

inline void nsStyleCorners::Set(PRUint8 aCorner, const nsStyleCoord& aCoord)
{
  mUnits[aCorner] = aCoord.GetUnit();
  aCoord.GetUnionValue(mValues[aCorner]);
}

#endif 
