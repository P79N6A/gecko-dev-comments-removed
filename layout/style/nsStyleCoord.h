






































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
  eStyleUnit_Percent      = 10,     
  eStyleUnit_Factor       = 11,     
  eStyleUnit_Coord        = 20,     
  eStyleUnit_Integer      = 30,     
  eStyleUnit_Proportional = 31,     
  eStyleUnit_Enumerated   = 32,     
  eStyleUnit_Chars        = 33      
};

typedef union {
  PRInt32     mInt;   
  float       mFloat;
} nsStyleUnion;









class nsStyleCoord {
public:
  nsStyleCoord(nsStyleUnit aUnit = eStyleUnit_Null);
  nsStyleCoord(nscoord aValue);
  nsStyleCoord(PRInt32 aValue, nsStyleUnit aUnit);
  nsStyleCoord(float aValue, nsStyleUnit aUnit);
  nsStyleCoord(const nsStyleCoord& aCopy);

  nsStyleCoord&  operator=(const nsStyleCoord& aCopy);
  PRBool         operator==(const nsStyleCoord& aOther) const;
  PRBool         operator!=(const nsStyleCoord& aOther) const;

  nsStyleUnit GetUnit(void) const { return mUnit; }
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
  void  SetUnionValue(const nsStyleUnion& aValue, nsStyleUnit aUnit);

  void  AppendToString(nsString& aBuffer) const;
  void  ToString(nsString& aBuffer) const;

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

  inline nsStyleCoord& Get(PRUint8 aSide, nsStyleCoord& aCoord) const;
  inline nsStyleCoord& GetLeft(nsStyleCoord& aCoord) const;
  inline nsStyleCoord& GetTop(nsStyleCoord& aCoord) const;
  inline nsStyleCoord& GetRight(nsStyleCoord& aCoord) const;
  inline nsStyleCoord& GetBottom(nsStyleCoord& aCoord) const;

  void  Reset(void);

  inline void Set(PRUint8 aSide, const nsStyleCoord& aCoord);
  inline void SetLeft(const nsStyleCoord& aCoord);
  inline void SetTop(const nsStyleCoord& aCoord);
  inline void SetRight(const nsStyleCoord& aCoord);
  inline void SetBottom(const nsStyleCoord& aCoord);

  void  AppendToString(nsString& aBuffer) const;
  void  ToString(nsString& aBuffer) const;

protected:
  PRUint8       mUnits[4];
  nsStyleUnion  mValues[4];
};




inline PRBool nsStyleCoord::operator!=(const nsStyleCoord& aOther) const
{
  return PRBool(! ((*this) == aOther));
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
  NS_ASSERTION((mUnit == eStyleUnit_Proportional) ||
               (mUnit == eStyleUnit_Enumerated) ||
               (mUnit == eStyleUnit_Chars) ||
               (mUnit == eStyleUnit_Integer), "not an int value");
  if ((mUnit == eStyleUnit_Proportional) ||
      (mUnit == eStyleUnit_Enumerated) ||
      (mUnit == eStyleUnit_Chars) ||
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
  return PRBool(! ((*this) == aOther));
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

inline nsStyleCoord& nsStyleSides::Get(PRUint8 aSide, nsStyleCoord& aCoord) const
{
  aCoord.SetUnionValue(mValues[aSide], (nsStyleUnit)mUnits[aSide]);
  return aCoord;
}

inline nsStyleCoord& nsStyleSides::GetLeft(nsStyleCoord& aCoord) const
{
  return Get(NS_SIDE_LEFT, aCoord);
}

inline nsStyleCoord& nsStyleSides::GetTop(nsStyleCoord& aCoord) const
{
  return Get(NS_SIDE_TOP, aCoord);
}

inline nsStyleCoord& nsStyleSides::GetRight(nsStyleCoord& aCoord) const
{
  return Get(NS_SIDE_RIGHT, aCoord);
}

inline nsStyleCoord& nsStyleSides::GetBottom(nsStyleCoord& aCoord) const
{
  return Get(NS_SIDE_BOTTOM, aCoord);
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

#endif 
