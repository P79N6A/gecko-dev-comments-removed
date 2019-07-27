




#ifndef nsAttrValueInlines_h__
#define nsAttrValueInlines_h__

#include <stdint.h>

#include "nsAttrValue.h"
#include "mozilla/Attributes.h"

struct MiscContainer;

struct MiscContainer MOZ_FINAL
{
  typedef nsAttrValue::ValueType ValueType;

  ValueType mType;
  
  
  
  
  uintptr_t mStringBits;
  union {
    struct {
      union {
        int32_t mInteger;
        nscolor mColor;
        uint32_t mEnumValue;
        int32_t mPercent;
        mozilla::css::StyleRule* mCSSStyleRule;
        mozilla::css::URLValue* mURL;
        mozilla::css::ImageValue* mImage;
        nsAttrValue::AtomArray* mAtomArray;
        nsIntMargin* mIntMargin;
        const nsSVGAngle* mSVGAngle;
        const nsSVGIntegerPair* mSVGIntegerPair;
        const nsSVGLength2* mSVGLength;
        const mozilla::SVGLengthList* mSVGLengthList;
        const mozilla::SVGNumberList* mSVGNumberList;
        const nsSVGNumberPair* mSVGNumberPair;
        const mozilla::SVGPathData* mSVGPathData;
        const mozilla::SVGPointList* mSVGPointList;
        const mozilla::SVGAnimatedPreserveAspectRatio* mSVGPreserveAspectRatio;
        const mozilla::SVGStringList* mSVGStringList;
        const mozilla::SVGTransformList* mSVGTransformList;
        const nsSVGViewBox* mSVGViewBox;
      };
      uint32_t mRefCount : 31;
      uint32_t mCached : 1;
    } mValue;
    double mDoubleValue;
  };

  MiscContainer()
    : mType(nsAttrValue::eColor),
      mStringBits(0)
  {
    MOZ_COUNT_CTOR(MiscContainer);
    mValue.mColor = 0;
    mValue.mRefCount = 0;
    mValue.mCached = 0;
  }

protected:
  
  friend class nsAttrValue;

  ~MiscContainer()
  {
    if (IsRefCounted()) {
      MOZ_ASSERT(mValue.mRefCount == 0);
      MOZ_ASSERT(!mValue.mCached);
    }
    MOZ_COUNT_DTOR(MiscContainer);
  }

public:
  bool GetString(nsAString& aString) const;

  inline bool IsRefCounted() const
  {
    
    
    
    return mType == nsAttrValue::eCSSStyleRule;
  }

  inline int32_t AddRef() {
    MOZ_ASSERT(IsRefCounted());
    return ++mValue.mRefCount;
  }

  inline int32_t Release() {
    MOZ_ASSERT(IsRefCounted());
    return --mValue.mRefCount;
  }

  void Cache();
  void Evict();
};





inline int32_t
nsAttrValue::GetIntegerValue() const
{
  NS_PRECONDITION(Type() == eInteger, "wrong type");
  return (BaseType() == eIntegerBase)
         ? GetIntInternal()
         : GetMiscContainer()->mValue.mInteger;
}

inline int16_t
nsAttrValue::GetEnumValue() const
{
  NS_PRECONDITION(Type() == eEnum, "wrong type");
  
  
  return static_cast<int16_t>((
    (BaseType() == eIntegerBase)
    ? static_cast<uint32_t>(GetIntInternal())
    : GetMiscContainer()->mValue.mEnumValue)
      >> NS_ATTRVALUE_ENUMTABLEINDEX_BITS);
}

inline float
nsAttrValue::GetPercentValue() const
{
  NS_PRECONDITION(Type() == ePercent, "wrong type");
  return ((BaseType() == eIntegerBase)
          ? GetIntInternal()
          : GetMiscContainer()->mValue.mPercent)
            / 100.0f;
}

inline nsAttrValue::AtomArray*
nsAttrValue::GetAtomArrayValue() const
{
  NS_PRECONDITION(Type() == eAtomArray, "wrong type");
  return GetMiscContainer()->mValue.mAtomArray;
}

inline mozilla::css::StyleRule*
nsAttrValue::GetCSSStyleRuleValue() const
{
  NS_PRECONDITION(Type() == eCSSStyleRule, "wrong type");
  return GetMiscContainer()->mValue.mCSSStyleRule;
}

inline mozilla::css::URLValue*
nsAttrValue::GetURLValue() const
{
  NS_PRECONDITION(Type() == eURL, "wrong type");
  return GetMiscContainer()->mValue.mURL;
}

inline mozilla::css::ImageValue*
nsAttrValue::GetImageValue() const
{
  NS_PRECONDITION(Type() == eImage, "wrong type");
  return GetMiscContainer()->mValue.mImage;
}

inline double
nsAttrValue::GetDoubleValue() const
{
  NS_PRECONDITION(Type() == eDoubleValue, "wrong type");
  return GetMiscContainer()->mDoubleValue;
}

inline bool
nsAttrValue::GetIntMarginValue(nsIntMargin& aMargin) const
{
  NS_PRECONDITION(Type() == eIntMarginValue, "wrong type");
  nsIntMargin* m = GetMiscContainer()->mValue.mIntMargin;
  if (!m)
    return false;
  aMargin = *m;
  return true;
}

inline bool
nsAttrValue::IsSVGType(ValueType aType) const
{
  return aType >= eSVGTypesBegin && aType <= eSVGTypesEnd;
}

inline void
nsAttrValue::SetPtrValueAndType(void* aValue, ValueBaseType aType)
{
  NS_ASSERTION(!(NS_PTR_TO_INT32(aValue) & ~NS_ATTRVALUE_POINTERVALUE_MASK),
               "pointer not properly aligned, this will crash");
  mBits = reinterpret_cast<intptr_t>(aValue) | aType;
}

inline void
nsAttrValue::ResetIfSet()
{
  if (mBits) {
    Reset();
  }
}

inline MiscContainer*
nsAttrValue::GetMiscContainer() const
{
  NS_ASSERTION(BaseType() == eOtherBase, "wrong type");
  return static_cast<MiscContainer*>(GetPtr());
}

inline int32_t
nsAttrValue::GetIntInternal() const
{
  NS_ASSERTION(BaseType() == eIntegerBase,
               "getting integer from non-integer");
  
  
  
  return static_cast<int32_t>(mBits & ~NS_ATTRVALUE_INTEGERTYPE_MASK) /
         NS_ATTRVALUE_INTEGERTYPE_MULTIPLIER;
}

#endif
