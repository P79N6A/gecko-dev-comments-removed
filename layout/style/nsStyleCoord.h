






#ifndef nsStyleCoord_h___
#define nsStyleCoord_h___

#include "nsCoord.h"
#include "nsStyleConsts.h"

enum nsStyleUnit : uint8_t {
  eStyleUnit_Null         = 0,      
  eStyleUnit_Normal       = 1,      
  eStyleUnit_Auto         = 2,      
  eStyleUnit_None         = 3,      
  eStyleUnit_Percent      = 10,     
  eStyleUnit_Factor       = 11,     
  eStyleUnit_Degree       = 12,     
  eStyleUnit_Grad         = 13,     
  eStyleUnit_Radian       = 14,     
  eStyleUnit_Turn         = 15,     
  eStyleUnit_FlexFraction = 16,     
  eStyleUnit_Coord        = 20,     
  eStyleUnit_Integer      = 30,     
  eStyleUnit_Enumerated   = 32,     

  
  eStyleUnit_Calc         = 40,     
                                    

  eStyleUnit_MAX          = 40      
};

typedef union {
  int32_t     mInt;   
  float       mFloat;
  
  
  void*       mPointer;
} nsStyleUnion;









class nsStyleCoord {
public:
  
  
  struct CalcValue {
    
    nscoord mLength;
    float mPercent;
    bool mHasPercent; 

    bool operator==(const CalcValue& aOther) const {
      return mLength == aOther.mLength &&
             mPercent == aOther.mPercent &&
             mHasPercent == aOther.mHasPercent;
    }
    bool operator!=(const CalcValue& aOther) const {
      return !(*this == aOther);
    }
  };

  
  
  struct Calc MOZ_FINAL : public CalcValue {
    NS_INLINE_DECL_REFCOUNTING(Calc)
    Calc() {}

  private:
    Calc(const Calc&) MOZ_DELETE;
    ~Calc() {}
    Calc& operator=(const Calc&) MOZ_DELETE;
  };

  explicit nsStyleCoord(nsStyleUnit aUnit = eStyleUnit_Null);
  enum CoordConstructorType { CoordConstructor };
  inline nsStyleCoord(nscoord aValue, CoordConstructorType);
  nsStyleCoord(int32_t aValue, nsStyleUnit aUnit);
  nsStyleCoord(float aValue, nsStyleUnit aUnit);
  inline nsStyleCoord(const nsStyleCoord& aCopy);
  inline nsStyleCoord(const nsStyleUnion& aValue, nsStyleUnit aUnit);
  ~nsStyleCoord() { Reset(); }

  nsStyleCoord&  operator=(const nsStyleCoord& aOther)
  {
    if (this != &aOther) {
      SetValue(mUnit, mValue, aOther);
    }
    return *this;
  }
  bool           operator==(const nsStyleCoord& aOther) const;
  bool           operator!=(const nsStyleCoord& aOther) const;

  nsStyleUnit GetUnit() const {
    NS_ASSERTION(mUnit != eStyleUnit_Null, "reading uninitialized value");
    return mUnit;
  }

  bool IsAngleValue() const {
    return eStyleUnit_Degree <= mUnit && mUnit <= eStyleUnit_Turn;
  }

  static bool IsCalcUnit(nsStyleUnit aUnit) {
    return aUnit == eStyleUnit_Calc;
  }

  static bool IsPointerUnit(nsStyleUnit aUnit) {
    return IsCalcUnit(aUnit);
  }

  bool IsCalcUnit() const {
    return IsCalcUnit(mUnit);
  }

  bool IsPointerValue() const {
    return IsPointerUnit(mUnit);
  }

  bool IsCoordPercentCalcUnit() const {
    return mUnit == eStyleUnit_Coord ||
           mUnit == eStyleUnit_Percent ||
           IsCalcUnit();
  }

  
  
  bool CalcHasPercent() const {
    return GetCalcValue()->mHasPercent;
  }

  bool HasPercent() const {
    return mUnit == eStyleUnit_Percent ||
           (IsCalcUnit() && CalcHasPercent());
  }

  bool ConvertsToLength() const {
    return mUnit == eStyleUnit_Coord ||
           (IsCalcUnit() && !CalcHasPercent());
  }

  nscoord     GetCoordValue() const;
  int32_t     GetIntValue() const;
  float       GetPercentValue() const;
  float       GetFactorValue() const;
  float       GetFactorOrPercentValue() const;
  float       GetAngleValue() const;
  double      GetAngleValueInDegrees() const;
  double      GetAngleValueInRadians() const;
  float       GetFlexFractionValue() const;
  Calc*       GetCalcValue() const;
  uint32_t    HashValue(uint32_t aHash) const;

  
  
  void Reset();

  void  SetCoordValue(nscoord aValue);
  void  SetIntValue(int32_t aValue, nsStyleUnit aUnit);
  void  SetPercentValue(float aValue);
  void  SetFactorValue(float aValue);
  void  SetAngleValue(float aValue, nsStyleUnit aUnit);
  void  SetFlexFractionValue(float aValue);
  void  SetNormalValue();
  void  SetAutoValue();
  void  SetNoneValue();
  void  SetCalcValue(Calc* aValue);

  
  static inline void Reset(nsStyleUnit& aUnit, nsStyleUnion& aValue);

  
  
  static inline void SetValue(nsStyleUnit& aUnit,
                              nsStyleUnion& aValue,
                              nsStyleUnit aOtherUnit,
                              const nsStyleUnion& aOtherValue);

  
  static inline void SetValue(nsStyleUnit& aUnit, nsStyleUnion& aValue,
                              const nsStyleCoord& aOther);

private:
  nsStyleUnit   mUnit;
  nsStyleUnion  mValue;
};






class nsStyleSides {
public:
  nsStyleSides();
  nsStyleSides(const nsStyleSides&);
  ~nsStyleSides();

  nsStyleSides&  operator=(const nsStyleSides& aCopy);
  bool           operator==(const nsStyleSides& aOther) const;
  bool           operator!=(const nsStyleSides& aOther) const;

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

  
  
  
  void Reset();

  inline void Set(mozilla::css::Side aSide, const nsStyleCoord& aCoord);
  inline void SetLeft(const nsStyleCoord& aCoord);
  inline void SetTop(const nsStyleCoord& aCoord);
  inline void SetRight(const nsStyleCoord& aCoord);
  inline void SetBottom(const nsStyleCoord& aCoord);

protected:
  nsStyleUnit   mUnits[4];
  nsStyleUnion  mValues[4];
};






class nsStyleCorners {
public:
  nsStyleCorners();
  nsStyleCorners(const nsStyleCorners&);
  ~nsStyleCorners();

  
  nsStyleCorners& operator=(const nsStyleCorners& aCopy);
  bool           operator==(const nsStyleCorners& aOther) const;
  bool           operator!=(const nsStyleCorners& aOther) const;

  
  inline nsStyleUnit GetUnit(uint8_t aHalfCorner) const;

  inline nsStyleCoord Get(uint8_t aHalfCorner) const;

  
  
  
  void Reset();

  inline void Set(uint8_t aHalfCorner, const nsStyleCoord& aCoord);

protected:
  nsStyleUnit   mUnits[8];
  nsStyleUnion  mValues[8];
};





inline nsStyleCoord::nsStyleCoord(nscoord aValue, CoordConstructorType)
  : mUnit(eStyleUnit_Coord)
{
  mValue.mInt = aValue;
}

inline nsStyleCoord::nsStyleCoord(const nsStyleCoord& aCopy)
  : mUnit(eStyleUnit_Null)
{
  SetValue(mUnit, mValue, aCopy);
}

inline nsStyleCoord::nsStyleCoord(const nsStyleUnion& aValue, nsStyleUnit aUnit)
  : mUnit(eStyleUnit_Null)
{
  SetValue(mUnit, mValue, aUnit, aValue);
}

inline bool nsStyleCoord::operator!=(const nsStyleCoord& aOther) const
{
  return !((*this) == aOther);
}

inline nscoord nsStyleCoord::GetCoordValue() const
{
  NS_ASSERTION((mUnit == eStyleUnit_Coord), "not a coord value");
  if (mUnit == eStyleUnit_Coord) {
    return mValue.mInt;
  }
  return 0;
}

inline int32_t nsStyleCoord::GetIntValue() const
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

inline float nsStyleCoord::GetFactorOrPercentValue() const
{
  NS_ASSERTION(mUnit == eStyleUnit_Factor || mUnit == eStyleUnit_Percent,
               "not a percent or factor value");
  if (mUnit == eStyleUnit_Factor || mUnit == eStyleUnit_Percent) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline float nsStyleCoord::GetAngleValue() const
{
  NS_ASSERTION(mUnit >= eStyleUnit_Degree &&
               mUnit <= eStyleUnit_Turn, "not an angle value");
  if (mUnit >= eStyleUnit_Degree && mUnit <= eStyleUnit_Turn) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline float nsStyleCoord::GetFlexFractionValue() const
{
  NS_ASSERTION(mUnit == eStyleUnit_FlexFraction, "not a fr value");
  if (mUnit == eStyleUnit_FlexFraction) {
    return mValue.mFloat;
  }
  return 0.0f;
}

inline nsStyleCoord::Calc* nsStyleCoord::GetCalcValue() const
{
  NS_ASSERTION(IsCalcUnit(), "not a pointer value");
  if (IsCalcUnit()) {
    return static_cast<Calc*>(mValue.mPointer);
  }
  return nullptr;
}

 inline void
nsStyleCoord::Reset(nsStyleUnit& aUnit, nsStyleUnion& aValue)
{
  MOZ_ASSERT(aUnit <= eStyleUnit_MAX,
             "calling Reset on uninitialized nsStyleCoord?");

  switch (aUnit) {
    case eStyleUnit_Calc:
      static_cast<Calc*>(aValue.mPointer)->Release();
      break;
    default:
      MOZ_ASSERT(!IsPointerUnit(aUnit), "check pointer refcounting logic");
  }

  aUnit = eStyleUnit_Null;
  aValue.mInt = 0;
}

 inline void
nsStyleCoord::SetValue(nsStyleUnit& aUnit,
                       nsStyleUnion& aValue,
                       nsStyleUnit aOtherUnit,
                       const nsStyleUnion& aOtherValue)
{
  Reset(aUnit, aValue);

  aUnit = aOtherUnit;
  aValue = aOtherValue;

  switch (aUnit) {
    case eStyleUnit_Calc:
      static_cast<Calc*>(aValue.mPointer)->AddRef();
      break;
    default:
      MOZ_ASSERT(!IsPointerUnit(aUnit), "check pointer refcounting logic");
  }
}

 inline void
nsStyleCoord::SetValue(nsStyleUnit& aUnit, nsStyleUnion& aValue,
                       const nsStyleCoord& aOther)
{
  SetValue(aUnit, aValue, aOther.mUnit, aOther.mValue);
}





inline bool nsStyleSides::operator!=(const nsStyleSides& aOther) const
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
  nsStyleCoord::SetValue(mUnits[aSide], mValues[aSide], aCoord);
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




inline bool nsStyleCorners::operator!=(const nsStyleCorners& aOther) const
{
  return !((*this) == aOther);
}

inline nsStyleUnit nsStyleCorners::GetUnit(uint8_t aCorner) const
{
  return (nsStyleUnit)mUnits[aCorner];
}

inline nsStyleCoord nsStyleCorners::Get(uint8_t aCorner) const
{
  return nsStyleCoord(mValues[aCorner], nsStyleUnit(mUnits[aCorner]));
}

inline void nsStyleCorners::Set(uint8_t aCorner, const nsStyleCoord& aCoord)
{
  nsStyleCoord::SetValue(mUnits[aCorner], mValues[aCorner], aCoord);
}

#endif 
