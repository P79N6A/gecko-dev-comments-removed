






































#ifndef nsStyleCoord_h___
#define nsStyleCoord_h___

#include "nscore.h"
#include "nsCoord.h"
#include "nsCRT.h"
#include "nsStyleConsts.h"
class nsString;

enum nsStyleUnit {
  eStyleUnit_Null         = 0,      
  eStyleUnit_Normal       = 1,      
  eStyleUnit_Auto         = 2,      
  eStyleUnit_None         = 3,      
  eStyleUnit_Percent      = 10,     
  eStyleUnit_Factor       = 11,     
  eStyleUnit_Coord        = 20,     
  eStyleUnit_Integer      = 30,     
  eStyleUnit_Enumerated   = 32      
};

typedef union {
  PRInt32     mInt;   
  float       mFloat;
} nsStyleUnion;









class nsStyleCoord {
public:
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

  nsStyleUnit GetUnit(void) const {
    NS_ASSERTION(mUnit != eStyleUnit_Null, "reading uninitialized value");
    return mUnit;
  }

  nscoord     GetCoordValue(void) const;
  PRInt32     GetIntValue(void) const;
  float       GetPercentValue(void) const;
  float       GetFactorValue(void) const;
  void        GetUnionValue(nsStyleUnion& aValue) const;

  void  Reset(void);  
  void  SetCoordValue(nscoord aValue);
  void  SetIntValue(PRInt32 aValue, nsStyleUnit aUnit);
  void  SetPercentValue(float aValue);
  void  SetFactorValue(float aValue);
  void  SetNormalValue(void);
  void  SetAutoValue(void);
  void  SetNoneValue(void);

public:
  nsStyleUnit   mUnit;
  nsStyleUnion  mValue;
};







class nsStyleSides {
public:
  nsStyleSides(void);


  PRBool         operator==(const nsStyleSides& aOther) const;
  PRBool         operator!=(const nsStyleSides& aOther) const;

  

  inline nsStyleUnit GetUnit(PRUint8 aSide) const;
  inline nsStyleUnit GetLeftUnit(void) const;
  inline nsStyleUnit GetTopUnit(void) const;
  inline nsStyleUnit GetRightUnit(void) const;
  inline nsStyleUnit GetBottomUnit(void) const;

  inline nsStyleCoord Get(PRUint8 aSide) const;
  inline nsStyleCoord GetLeft() const;
  inline nsStyleCoord GetTop() const;
  inline nsStyleCoord GetRight() const;
  inline nsStyleCoord GetBottom() const;

  void  Reset(void);

  inline void Set(PRUint8 aSide, const nsStyleCoord& aCoord);
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
  nsStyleCorners(void);

  
  
  PRBool         operator==(const nsStyleCorners& aOther) const;
  PRBool         operator!=(const nsStyleCorners& aOther) const;

  
  inline nsStyleUnit GetUnit(PRUint8 aHalfCorner) const;

  inline nsStyleCoord Get(PRUint8 aHalfCorner) const;

  void  Reset(void);

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

inline PRInt32 nsStyleCoord::GetCoordValue(void) const
{
  NS_ASSERTION((mUnit == eStyleUnit_Coord), "not a coord value");
  if (mUnit == eStyleUnit_Coord) {
    return mValue.mInt;
  }
  return 0;
}

inline PRInt32 nsStyleCoord::GetIntValue(void) const
{
  NS_ASSERTION((mUnit == eStyleUnit_Enumerated) ||
               (mUnit == eStyleUnit_Integer), "not an int value");
  if ((mUnit == eStyleUnit_Enumerated) ||
      (mUnit == eStyleUnit_Integer)) {
    return mValue.mInt;
  }
  return 0;
}

inline float nsStyleCoord::GetPercentValue(void) const
{
  NS_ASSERTION(mUnit == eStyleUnit_Percent, "not a percent value");
  if (mUnit == eStyleUnit_Percent) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline float nsStyleCoord::GetFactorValue(void) const
{
  NS_ASSERTION(mUnit == eStyleUnit_Factor, "not a factor value");
  if (mUnit == eStyleUnit_Factor) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline void nsStyleCoord::GetUnionValue(nsStyleUnion& aValue) const
{
  memcpy(&aValue, &mValue, sizeof(nsStyleUnion));
}




inline PRBool nsStyleSides::operator!=(const nsStyleSides& aOther) const
{
  return !((*this) == aOther);
}

inline nsStyleUnit nsStyleSides::GetUnit(PRUint8 aSide) const
{
  return (nsStyleUnit)mUnits[aSide];
}

inline nsStyleUnit nsStyleSides::GetLeftUnit(void) const
{
  return GetUnit(NS_SIDE_LEFT);
}

inline nsStyleUnit nsStyleSides::GetTopUnit(void) const
{
  return GetUnit(NS_SIDE_TOP);
}

inline nsStyleUnit nsStyleSides::GetRightUnit(void) const
{
  return GetUnit(NS_SIDE_RIGHT);
}

inline nsStyleUnit nsStyleSides::GetBottomUnit(void) const
{
  return GetUnit(NS_SIDE_BOTTOM);
}

inline nsStyleCoord nsStyleSides::Get(PRUint8 aSide) const
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

inline void nsStyleSides::Set(PRUint8 aSide, const nsStyleCoord& aCoord)
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
