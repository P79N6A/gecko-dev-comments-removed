






#ifndef nsCSSValue_h___
#define nsCSSValue_h___

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsCSSKeywords.h"
#include "nsCSSProperty.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsRefPtrHashtable.h"
#include "nsString.h"
#include "nsStringBuffer.h"
#include "nsTArray.h"
#include "nsStyleConsts.h"
#include "gfxFontFamilyList.h"

class imgRequestProxy;
class nsIDocument;
class nsIPrincipal;
class nsIURI;
class nsPresContext;
template <class T>
class nsPtrHashKey;

namespace mozilla {
class CSSStyleSheet;
} 


#define NS_CSS_DELETE_LIST_MEMBER(type_, ptr_, member_)                        \
  {                                                                            \
    type_ *cur = (ptr_)->member_;                                              \
    (ptr_)->member_ = nullptr;                                                 \
    while (cur) {                                                              \
      type_ *dlm_next = cur->member_;                                          \
      cur->member_ = nullptr;                                                  \
      delete cur;                                                              \
      cur = dlm_next;                                                          \
    }                                                                          \
  }




#define NS_CSS_CLONE_LIST_MEMBER(type_, from_, member_, to_, args_)            \
  {                                                                            \
    type_ *dest = (to_);                                                       \
    (to_)->member_ = nullptr;                                                  \
    for (const type_ *src = (from_)->member_; src; src = src->member_) {       \
      type_ *clm_clone = src->Clone args_;                                     \
      if (!clm_clone) {                                                        \
        delete (to_);                                                          \
        return nullptr;                                                        \
      }                                                                        \
      dest->member_ = clm_clone;                                               \
      dest = clm_clone;                                                        \
    }                                                                          \
  }

namespace mozilla {
namespace css {

struct URLValue {
  
  
  

  
  
  
  
  URLValue(nsStringBuffer* aString, nsIURI* aBaseURI, nsIURI* aReferrer,
           nsIPrincipal* aOriginPrincipal);
  
  URLValue(nsIURI* aURI, nsStringBuffer* aString, nsIURI* aReferrer,
           nsIPrincipal* aOriginPrincipal);

protected:
  ~URLValue();

public:
  bool operator==(const URLValue& aOther) const;

  
  
  
  
  bool URIEquals(const URLValue& aOther) const;

  nsIURI* GetURI() const;

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  
  
  mutable nsCOMPtr<nsIURI> mURI;
public:
  nsStringBuffer* mString; 
                           
  nsCOMPtr<nsIURI> mReferrer;
  nsCOMPtr<nsIPrincipal> mOriginPrincipal;

  NS_INLINE_DECL_REFCOUNTING(URLValue)

private:
  mutable bool mURIResolved;

  URLValue(const URLValue& aOther) = delete;
  URLValue& operator=(const URLValue& aOther) = delete;
};

struct ImageValue : public URLValue {
  
  
  
  
  ImageValue(nsIURI* aURI, nsStringBuffer* aString, nsIURI* aReferrer,
             nsIPrincipal* aOriginPrincipal, nsIDocument* aDocument);
private:
  ~ImageValue();

public:
  

  nsRefPtrHashtable<nsPtrHashKey<nsISupports>, imgRequestProxy> mRequests; 

  
  
  
  NS_METHOD_(MozExternalRefCountType) AddRef();
  NS_METHOD_(MozExternalRefCountType) Release();
};

struct GridNamedArea {
  nsString mName;
  uint32_t mColumnStart;
  uint32_t mColumnEnd;
  uint32_t mRowStart;
  uint32_t mRowEnd;
};

struct GridTemplateAreasValue final {
  
  nsTArray<GridNamedArea> mNamedAreas;

  
  
  nsTArray<nsString> mTemplates;

  
  
  uint32_t mNColumns;

  
  
  uint32_t NRows() const {
    return mTemplates.Length();
  }

  GridTemplateAreasValue()
    : mNColumns(0)
    
  {
  }

  bool operator==(const GridTemplateAreasValue& aOther) const
  {
    return mTemplates == aOther.mTemplates;
  }

  bool operator!=(const GridTemplateAreasValue& aOther) const
  {
    return !(*this == aOther);
  }

  NS_INLINE_DECL_REFCOUNTING(GridTemplateAreasValue)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  
  ~GridTemplateAreasValue()
  {
  }

  GridTemplateAreasValue(const GridTemplateAreasValue& aOther) = delete;
  GridTemplateAreasValue&
  operator=(const GridTemplateAreasValue& aOther) = delete;
};

class FontFamilyListRefCnt final : public FontFamilyList {
public:
    FontFamilyListRefCnt()
        : FontFamilyList()
    {
        MOZ_COUNT_CTOR(FontFamilyListRefCnt);
    }

    explicit FontFamilyListRefCnt(FontFamilyType aGenericType)
        : FontFamilyList(aGenericType)
    {
        MOZ_COUNT_CTOR(FontFamilyListRefCnt);
    }

    FontFamilyListRefCnt(const nsAString& aFamilyName,
                         QuotedName aQuoted)
        : FontFamilyList(aFamilyName, aQuoted)
    {
        MOZ_COUNT_CTOR(FontFamilyListRefCnt);
    }

    FontFamilyListRefCnt(const FontFamilyListRefCnt& aOther)
        : FontFamilyList(aOther)
    {
        MOZ_COUNT_CTOR(FontFamilyListRefCnt);
    }

    NS_INLINE_DECL_REFCOUNTING(FontFamilyListRefCnt);

private:
    ~FontFamilyListRefCnt() {
        MOZ_COUNT_DTOR(FontFamilyListRefCnt);
    }
};

}
}

enum nsCSSUnit {
  eCSSUnit_Null         = 0,      
  eCSSUnit_Auto         = 1,      
  eCSSUnit_Inherit      = 2,      
  eCSSUnit_Initial      = 3,      
  eCSSUnit_Unset        = 4,      
  eCSSUnit_None         = 5,      
  eCSSUnit_Normal       = 6,      
  eCSSUnit_System_Font  = 7,      
  eCSSUnit_All          = 8,      
  eCSSUnit_Dummy        = 9,      
                                  
  eCSSUnit_DummyInherit = 10,     
                                  

  eCSSUnit_String       = 11,     
  eCSSUnit_Ident        = 12,     
  eCSSUnit_Attr         = 14,     
  eCSSUnit_Local_Font   = 15,     
  eCSSUnit_Font_Format  = 16,     
  eCSSUnit_Element      = 17,     

  eCSSUnit_Array        = 20,     
  eCSSUnit_Counter      = 21,     
  eCSSUnit_Counters     = 22,     
  eCSSUnit_Cubic_Bezier = 23,     
  eCSSUnit_Steps        = 24,     
  eCSSUnit_Symbols      = 25,     
  eCSSUnit_Function     = 26,     
                                  
                                  
                                  

  
  
  

  
  
  
  
  eCSSUnit_Calc         = 30,     
  
  
  eCSSUnit_Calc_Plus    = 31,     
  eCSSUnit_Calc_Minus   = 32,     
  eCSSUnit_Calc_Times_L = 33,     
  eCSSUnit_Calc_Times_R = 34,     
  eCSSUnit_Calc_Divided = 35,     

  eCSSUnit_URL          = 40,     
  eCSSUnit_Image        = 41,     
  eCSSUnit_Gradient     = 42,     
  eCSSUnit_TokenStream  = 43,     
  eCSSUnit_GridTemplateAreas   = 44,   
                                       

  eCSSUnit_Pair         = 50,     
  eCSSUnit_Triplet      = 51,     
  eCSSUnit_Rect         = 52,     
  eCSSUnit_List         = 53,     
  eCSSUnit_ListDep      = 54,     
                                  
  eCSSUnit_SharedList   = 55,     
                                  
  eCSSUnit_PairList     = 56,     
  eCSSUnit_PairListDep  = 57,     
                                  

  eCSSUnit_FontFamilyList = 58,   

  eCSSUnit_Integer      = 70,     
  eCSSUnit_Enumerated   = 71,     

  eCSSUnit_EnumColor           = 80,   
  eCSSUnit_RGBColor            = 81,   
  eCSSUnit_RGBAColor           = 82,   
  eCSSUnit_HexColor            = 83,   
  eCSSUnit_ShortHexColor       = 84,   
  eCSSUnit_PercentageRGBColor  = 85,   
  eCSSUnit_PercentageRGBAColor = 86,   
  eCSSUnit_HSLColor            = 87,   
  eCSSUnit_HSLAColor           = 88,   

  eCSSUnit_Percent      = 90,     
  eCSSUnit_Number       = 91,     

  
  eCSSUnit_PhysicalMillimeter = 200,   

  
  
  eCSSUnit_ViewportWidth  = 700,    
  eCSSUnit_ViewportHeight = 701,    
  eCSSUnit_ViewportMin    = 702,    
  eCSSUnit_ViewportMax    = 703,    

  
  eCSSUnit_EM           = 800,    
  eCSSUnit_XHeight      = 801,    
  eCSSUnit_Char         = 802,    
  eCSSUnit_RootEM       = 803,    

  
  eCSSUnit_Point        = 900,    
  eCSSUnit_Inch         = 901,    
  eCSSUnit_Millimeter   = 902,    
  eCSSUnit_Centimeter   = 903,    
  eCSSUnit_Pica         = 904,    
  eCSSUnit_Pixel        = 905,    

  
  eCSSUnit_Degree       = 1000,    
  eCSSUnit_Grad         = 1001,    
  eCSSUnit_Radian       = 1002,    
  eCSSUnit_Turn         = 1003,    

  
  eCSSUnit_Hertz        = 2000,    
  eCSSUnit_Kilohertz    = 2001,    

  
  eCSSUnit_Seconds      = 3000,    
  eCSSUnit_Milliseconds = 3001,    

  
  eCSSUnit_FlexFraction = 4000     
};

struct nsCSSValueGradient;
struct nsCSSValuePair;
struct nsCSSValuePair_heap;
struct nsCSSValueTokenStream;
struct nsCSSRect;
struct nsCSSRect_heap;
struct nsCSSValueList;
struct nsCSSValueList_heap;
struct nsCSSValueSharedList;
struct nsCSSValuePairList;
struct nsCSSValuePairList_heap;
struct nsCSSValueTriplet;
struct nsCSSValueTriplet_heap;
class nsCSSValueFloatColor;

class nsCSSValue {
public:
  struct Array;
  friend struct Array;

  friend struct mozilla::css::URLValue;

  friend struct mozilla::css::ImageValue;

  
  explicit nsCSSValue(nsCSSUnit aUnit = eCSSUnit_Null)
    : mUnit(aUnit)
  {
    MOZ_ASSERT(aUnit <= eCSSUnit_DummyInherit, "not a valueless unit");
  }

  nsCSSValue(int32_t aValue, nsCSSUnit aUnit);
  nsCSSValue(float aValue, nsCSSUnit aUnit);
  nsCSSValue(const nsString& aValue, nsCSSUnit aUnit);
  nsCSSValue(Array* aArray, nsCSSUnit aUnit);
  explicit nsCSSValue(mozilla::css::URLValue* aValue);
  explicit nsCSSValue(mozilla::css::ImageValue* aValue);
  explicit nsCSSValue(nsCSSValueGradient* aValue);
  explicit nsCSSValue(nsCSSValueTokenStream* aValue);
  explicit nsCSSValue(mozilla::css::GridTemplateAreasValue* aValue);
  explicit nsCSSValue(mozilla::css::FontFamilyListRefCnt* aValue);
  nsCSSValue(const nsCSSValue& aCopy);
  ~nsCSSValue() { Reset(); }

  nsCSSValue&  operator=(const nsCSSValue& aCopy);
  bool        operator==(const nsCSSValue& aOther) const;

  bool operator!=(const nsCSSValue& aOther) const
  {
    return !(*this == aOther);
  }

  
  enum Serialization { eNormalized, eAuthorSpecified };

  



  void AppendToString(nsCSSProperty aProperty, nsAString& aResult,
                      Serialization aValueSerialization) const;

  nsCSSUnit GetUnit() const { return mUnit; }
  bool      IsLengthUnit() const
    { return eCSSUnit_PhysicalMillimeter <= mUnit && mUnit <= eCSSUnit_Pixel; }
  




  bool      IsFixedLengthUnit() const  
    { return mUnit == eCSSUnit_PhysicalMillimeter; }
  








  bool      IsRelativeLengthUnit() const  
    { return eCSSUnit_EM <= mUnit && mUnit <= eCSSUnit_RootEM; }
  


  bool      IsPixelLengthUnit() const
    { return eCSSUnit_Point <= mUnit && mUnit <= eCSSUnit_Pixel; }
  bool      IsAngularUnit() const  
    { return eCSSUnit_Degree <= mUnit && mUnit <= eCSSUnit_Turn; }
  bool      IsFrequencyUnit() const  
    { return eCSSUnit_Hertz <= mUnit && mUnit <= eCSSUnit_Kilohertz; }
  bool      IsTimeUnit() const  
    { return eCSSUnit_Seconds <= mUnit && mUnit <= eCSSUnit_Milliseconds; }
  bool      IsCalcUnit() const
    { return eCSSUnit_Calc <= mUnit && mUnit <= eCSSUnit_Calc_Divided; }

  bool      UnitHasStringValue() const
    { return eCSSUnit_String <= mUnit && mUnit <= eCSSUnit_Element; }
  bool      UnitHasArrayValue() const
    { return eCSSUnit_Array <= mUnit && mUnit <= eCSSUnit_Calc_Divided; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool IsIntegerColorUnit() const { return IsIntegerColorUnit(mUnit); }
  bool IsFloatColorUnit() const { return IsFloatColorUnit(mUnit); }
  bool IsNumericColorUnit() const { return IsNumericColorUnit(mUnit); }
  static bool IsIntegerColorUnit(nsCSSUnit aUnit)
  { return eCSSUnit_RGBColor <= aUnit && aUnit <= eCSSUnit_ShortHexColor; }
  static bool IsFloatColorUnit(nsCSSUnit aUnit)
  { return eCSSUnit_PercentageRGBColor <= aUnit &&
           aUnit <= eCSSUnit_HSLAColor; }
  static bool IsNumericColorUnit(nsCSSUnit aUnit)
  { return IsIntegerColorUnit(aUnit) || IsFloatColorUnit(aUnit); }

  int32_t GetIntValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_Integer ||
               mUnit == eCSSUnit_Enumerated ||
               mUnit == eCSSUnit_EnumColor,
               "not an int value");
    return mValue.mInt;
  }

  nsCSSKeyword GetKeywordValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_Enumerated, "not a keyword value");
    return static_cast<nsCSSKeyword>(mValue.mInt);
  }

  float GetPercentValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_Percent, "not a percent value");
    return mValue.mFloat;
  }

  float GetFloatValue() const
  {
    MOZ_ASSERT(eCSSUnit_Number <= mUnit, "not a float value");
    MOZ_ASSERT(!mozilla::IsNaN(mValue.mFloat));
    return mValue.mFloat;
  }

  float GetAngleValue() const
  {
    MOZ_ASSERT(eCSSUnit_Degree <= mUnit && mUnit <= eCSSUnit_Turn,
               "not an angle value");
    return mValue.mFloat;
  }

  
  double GetAngleValueInRadians() const;

  nsAString& GetStringValue(nsAString& aBuffer) const
  {
    MOZ_ASSERT(UnitHasStringValue(), "not a string value");
    aBuffer.Truncate();
    uint32_t len = NS_strlen(GetBufferValue(mValue.mString));
    mValue.mString->ToString(len, aBuffer);
    return aBuffer;
  }

  const char16_t* GetStringBufferValue() const
  {
    MOZ_ASSERT(UnitHasStringValue(), "not a string value");
    return GetBufferValue(mValue.mString);
  }

  nscolor GetColorValue() const;
  bool IsNonTransparentColor() const;

  Array* GetArrayValue() const
  {
    MOZ_ASSERT(UnitHasArrayValue(), "not an array value");
    return mValue.mArray;
  }

  nsIURI* GetURLValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_URL || mUnit == eCSSUnit_Image,
               "not a URL value");
    return mUnit == eCSSUnit_URL ?
      mValue.mURL->GetURI() : mValue.mImage->GetURI();
  }

  nsCSSValueGradient* GetGradientValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_Gradient, "not a gradient value");
    return mValue.mGradient;
  }

  nsCSSValueTokenStream* GetTokenStreamValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_TokenStream, "not a token stream value");
    return mValue.mTokenStream;
  }

  nsCSSValueSharedList* GetSharedListValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_SharedList, "not a shared list value");
    return mValue.mSharedList;
  }

  mozilla::FontFamilyList* GetFontFamilyListValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_FontFamilyList,
               "not a font family list value");
    NS_ASSERTION(mValue.mFontFamilyList != nullptr,
                 "font family list value should never be null");
    return mValue.mFontFamilyList;
  }

  
  inline nsCSSValuePair& GetPairValue();
  inline const nsCSSValuePair& GetPairValue() const;

  inline nsCSSRect& GetRectValue();
  inline const nsCSSRect& GetRectValue() const;

  inline nsCSSValueList* GetListValue();
  inline const nsCSSValueList* GetListValue() const;

  inline nsCSSValuePairList* GetPairListValue();
  inline const nsCSSValuePairList* GetPairListValue() const;

  inline nsCSSValueTriplet& GetTripletValue();
  inline const nsCSSValueTriplet& GetTripletValue() const;


  mozilla::css::URLValue* GetURLStructValue() const
  {
    
    
    MOZ_ASSERT(mUnit == eCSSUnit_URL, "not a URL value");
    return mValue.mURL;
  }

  mozilla::css::ImageValue* GetImageStructValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_Image, "not an Image value");
    return mValue.mImage;
  }

  mozilla::css::GridTemplateAreasValue* GetGridTemplateAreas() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_GridTemplateAreas,
               "not a grid-template-areas value");
    return mValue.mGridTemplateAreas;
  }

  const char16_t* GetOriginalURLValue() const
  {
    MOZ_ASSERT(mUnit == eCSSUnit_URL || mUnit == eCSSUnit_Image,
               "not a URL value");
    return GetBufferValue(mUnit == eCSSUnit_URL ?
                            mValue.mURL->mString :
                            mValue.mImage->mString);
  }

  
  
  
  imgRequestProxy* GetImageValue(nsIDocument* aDocument) const;

  nscoord GetFixedLength(nsPresContext* aPresContext) const;
  nscoord GetPixelLength() const;

  void Reset()  
  {
    if (mUnit != eCSSUnit_Null)
      DoReset();
  }
private:
  void DoReset();

public:
  void SetIntValue(int32_t aValue, nsCSSUnit aUnit);
  void SetPercentValue(float aValue);
  void SetFloatValue(float aValue, nsCSSUnit aUnit);
  void SetStringValue(const nsString& aValue, nsCSSUnit aUnit);
  void SetColorValue(nscolor aValue);
  void SetIntegerColorValue(nscolor aValue, nsCSSUnit aUnit);
  void SetFloatColorValue(float aComponent1,
                          float aComponent2,
                          float aComponent3,
                          float aAlpha, nsCSSUnit aUnit);
  void SetArrayValue(nsCSSValue::Array* aArray, nsCSSUnit aUnit);
  void SetURLValue(mozilla::css::URLValue* aURI);
  void SetImageValue(mozilla::css::ImageValue* aImage);
  void SetGradientValue(nsCSSValueGradient* aGradient);
  void SetTokenStreamValue(nsCSSValueTokenStream* aTokenStream);
  void SetGridTemplateAreas(mozilla::css::GridTemplateAreasValue* aValue);
  void SetFontFamilyListValue(mozilla::css::FontFamilyListRefCnt* aFontListValue);
  void SetPairValue(const nsCSSValuePair* aPair);
  void SetPairValue(const nsCSSValue& xValue, const nsCSSValue& yValue);
  void SetSharedListValue(nsCSSValueSharedList* aList);
  void SetDependentListValue(nsCSSValueList* aList);
  void SetDependentPairListValue(nsCSSValuePairList* aList);
  void SetTripletValue(const nsCSSValueTriplet* aTriplet);
  void SetTripletValue(const nsCSSValue& xValue, const nsCSSValue& yValue, const nsCSSValue& zValue);
  void SetAutoValue();
  void SetInheritValue();
  void SetInitialValue();
  void SetUnsetValue();
  void SetNoneValue();
  void SetAllValue();
  void SetNormalValue();
  void SetSystemFontValue();
  void SetDummyValue();
  void SetDummyInheritValue();

  
  
  nsCSSRect& SetRectValue();
  nsCSSValueList* SetListValue();
  nsCSSValuePairList* SetPairListValue();

  void StartImageLoad(nsIDocument* aDocument) const;  

  
  Array* InitFunction(nsCSSKeyword aFunctionId, uint32_t aNumArgs);
  
  bool EqualsFunction(nsCSSKeyword aFunctionId) const;

  
  
  static already_AddRefed<nsStringBuffer>
    BufferFromString(const nsString& aValue);

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  static void
  AppendSidesShorthandToString(const nsCSSProperty aProperties[],
                               const nsCSSValue* aValues[],
                               nsAString& aString,
                               Serialization aSerialization);
  static void
  AppendBasicShapeRadiusToString(const nsCSSProperty aProperties[],
                                 const nsCSSValue* aValues[],
                                 nsAString& aResult,
                                 Serialization aValueSerialization);
private:
  static const char16_t* GetBufferValue(nsStringBuffer* aBuffer) {
    return static_cast<char16_t*>(aBuffer->Data());
  }

  void AppendPolygonToString(nsCSSProperty aProperty, nsAString& aResult,
                             Serialization aValueSerialization) const;
  void AppendPositionCoordinateToString(const nsCSSValue& aValue,
                                        nsCSSProperty aProperty,
                                        nsAString& aResult,
                                        Serialization aSerialization) const;
  void AppendCircleOrEllipseToString(
           nsCSSKeyword aFunctionId,
           nsCSSProperty aProperty, nsAString& aResult,
           Serialization aValueSerialization) const;
  void AppendInsetToString(nsCSSProperty aProperty, nsAString& aResult,
                           Serialization aValueSerialization) const;
protected:
  nsCSSUnit mUnit;
  union {
    int32_t    mInt;
    float      mFloat;
    
    
    nsStringBuffer* mString;
    nscolor    mColor;
    Array*     mArray;
    mozilla::css::URLValue* mURL;
    mozilla::css::ImageValue* mImage;
    mozilla::css::GridTemplateAreasValue* mGridTemplateAreas;
    nsCSSValueGradient* mGradient;
    nsCSSValueTokenStream* mTokenStream;
    nsCSSValuePair_heap* mPair;
    nsCSSRect_heap* mRect;
    nsCSSValueTriplet_heap* mTriplet;
    nsCSSValueList_heap* mList;
    nsCSSValueList* mListDependent;
    nsCSSValueSharedList* mSharedList;
    nsCSSValuePairList_heap* mPairList;
    nsCSSValuePairList* mPairListDependent;
    nsCSSValueFloatColor* mFloatColor;
    mozilla::css::FontFamilyListRefCnt* mFontFamilyList;
  } mValue;
};

struct nsCSSValue::Array final {

  
  static Array* Create(size_t aItemCount) {
    return new (aItemCount) Array(aItemCount);
  }

  nsCSSValue& operator[](size_t aIndex) {
    MOZ_ASSERT(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  const nsCSSValue& operator[](size_t aIndex) const {
    MOZ_ASSERT(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  nsCSSValue& Item(size_t aIndex) { return (*this)[aIndex]; }
  const nsCSSValue& Item(size_t aIndex) const { return (*this)[aIndex]; }

  size_t Count() const { return mCount; }

  
  nsCSSValue* ItemStorage() {
    return this->First();
  }

  bool operator==(const Array& aOther) const
  {
    if (mCount != aOther.mCount)
      return false;
    for (size_t i = 0; i < mCount; ++i)
      if ((*this)[i] != aOther[i])
        return false;
    return true;
  }

  
  
  void AddRef() {
    if (mRefCnt == size_t(-1)) { 
      NS_WARNING("refcount overflow, leaking nsCSSValue::Array");
      return;
    }
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsCSSValue::Array", sizeof(*this));
  }
  void Release() {
    if (mRefCnt == size_t(-1)) { 
      NS_WARNING("refcount overflow, leaking nsCSSValue::Array");
      return;
    }
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsCSSValue::Array");
    if (mRefCnt == 0)
      delete this;
  }

private:

  size_t mRefCnt;
  const size_t mCount;
  
  
  
  nsCSSValue mArray[1];

  void* operator new(size_t aSelfSize, size_t aItemCount) CPP_THROW_NEW {
    MOZ_ASSERT(aItemCount > 0, "cannot have a 0 item count");
    return ::operator new(aSelfSize + sizeof(nsCSSValue) * (aItemCount - 1));
  }

  void operator delete(void* aPtr) { ::operator delete(aPtr); }

  nsCSSValue* First() { return mArray; }

  const nsCSSValue* First() const { return mArray; }

#define CSSVALUE_LIST_FOR_EXTRA_VALUES(var)                                   \
  for (nsCSSValue *var = First() + 1, *var##_end = First() + mCount;          \
       var != var##_end; ++var)

  explicit Array(size_t aItemCount)
    : mRefCnt(0)
    , mCount(aItemCount)
  {
    MOZ_COUNT_CTOR(nsCSSValue::Array);
    CSSVALUE_LIST_FOR_EXTRA_VALUES(val) {
      new (val) nsCSSValue();
    }
  }

  ~Array()
  {
    MOZ_COUNT_DTOR(nsCSSValue::Array);
    CSSVALUE_LIST_FOR_EXTRA_VALUES(val) {
      val->~nsCSSValue();
    }
  }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

#undef CSSVALUE_LIST_FOR_EXTRA_VALUES

private:
  Array(const Array& aOther) = delete;
  Array& operator=(const Array& aOther) = delete;
};


struct nsCSSValueList {
  nsCSSValueList() : mNext(nullptr) { MOZ_COUNT_CTOR(nsCSSValueList); }
  ~nsCSSValueList();

  nsCSSValueList* Clone() const;  
  void CloneInto(nsCSSValueList* aList) const; 
  void AppendToString(nsCSSProperty aProperty, nsAString& aResult,
                      nsCSSValue::Serialization aValueSerialization) const;

  static bool Equal(const nsCSSValueList* aList1,
                    const nsCSSValueList* aList2);

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  nsCSSValue      mValue;
  nsCSSValueList* mNext;

private:
  nsCSSValueList(const nsCSSValueList& aCopy) 
    : mValue(aCopy.mValue), mNext(nullptr)
  {
    MOZ_COUNT_CTOR(nsCSSValueList);
  }

  
  
  
  bool operator==(nsCSSValueList const& aOther) const = delete;
  bool operator!=(const nsCSSValueList& aOther) const = delete;
};




struct nsCSSValueList_heap final : public nsCSSValueList {
  NS_INLINE_DECL_REFCOUNTING(nsCSSValueList_heap)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  ~nsCSSValueList_heap()
  {
  }
};





struct nsCSSValueSharedList final {
  nsCSSValueSharedList()
    : mHead(nullptr)
  {
    MOZ_COUNT_CTOR(nsCSSValueSharedList);
  }

  
  explicit nsCSSValueSharedList(nsCSSValueList* aList)
    : mHead(aList)
  {
    MOZ_COUNT_CTOR(nsCSSValueSharedList);
  }

private:
  
  ~nsCSSValueSharedList();

public:
  NS_INLINE_DECL_REFCOUNTING(nsCSSValueSharedList)

  void AppendToString(nsCSSProperty aProperty, nsAString& aResult,
                      nsCSSValue::Serialization aValueSerialization) const;

  bool operator==(nsCSSValueSharedList const& aOther) const;
  bool operator!=(const nsCSSValueSharedList& aOther) const
  { return !(*this == aOther); }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  nsCSSValueList* mHead;
};



inline nsCSSValueList*
nsCSSValue::GetListValue()
{
  if (mUnit == eCSSUnit_List)
    return mValue.mList;
  else {
    MOZ_ASSERT(mUnit == eCSSUnit_ListDep, "not a pairlist value");
    return mValue.mListDependent;
  }
}

inline const nsCSSValueList*
nsCSSValue::GetListValue() const
{
  if (mUnit == eCSSUnit_List)
    return mValue.mList;
  else {
    MOZ_ASSERT(mUnit == eCSSUnit_ListDep, "not a pairlist value");
    return mValue.mListDependent;
  }
}

struct nsCSSRect {
  nsCSSRect(void);
  nsCSSRect(const nsCSSRect& aCopy);
  ~nsCSSRect();

  void AppendToString(nsCSSProperty aProperty, nsAString& aResult,
                      nsCSSValue::Serialization aValueSerialization) const;

  bool operator==(const nsCSSRect& aOther) const {
    return mTop == aOther.mTop &&
           mRight == aOther.mRight &&
           mBottom == aOther.mBottom &&
           mLeft == aOther.mLeft;
  }

  bool operator!=(const nsCSSRect& aOther) const {
    return mTop != aOther.mTop ||
           mRight != aOther.mRight ||
           mBottom != aOther.mBottom ||
           mLeft != aOther.mLeft;
  }

  void SetAllSidesTo(const nsCSSValue& aValue);

  bool AllSidesEqualTo(const nsCSSValue& aValue) const {
    return mTop == aValue &&
           mRight == aValue &&
           mBottom == aValue &&
           mLeft == aValue;
  }

  void Reset() {
    mTop.Reset();
    mRight.Reset();
    mBottom.Reset();
    mLeft.Reset();
  }

  bool HasValue() const {
    return
      mTop.GetUnit() != eCSSUnit_Null ||
      mRight.GetUnit() != eCSSUnit_Null ||
      mBottom.GetUnit() != eCSSUnit_Null ||
      mLeft.GetUnit() != eCSSUnit_Null;
  }

  nsCSSValue mTop;
  nsCSSValue mRight;
  nsCSSValue mBottom;
  nsCSSValue mLeft;

  typedef nsCSSValue nsCSSRect::*side_type;
  static const side_type sides[4];
};




struct nsCSSRect_heap final : public nsCSSRect {
  NS_INLINE_DECL_REFCOUNTING(nsCSSRect_heap)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  ~nsCSSRect_heap()
  {
  }
};



inline nsCSSRect&
nsCSSValue::GetRectValue()
{
  MOZ_ASSERT(mUnit == eCSSUnit_Rect, "not a rect value");
  return *mValue.mRect;
}

inline const nsCSSRect&
nsCSSValue::GetRectValue() const
{
  MOZ_ASSERT(mUnit == eCSSUnit_Rect, "not a rect value");
  return *mValue.mRect;
}

struct nsCSSValuePair {
  nsCSSValuePair()
  {
    MOZ_COUNT_CTOR(nsCSSValuePair);
  }
  explicit nsCSSValuePair(nsCSSUnit aUnit)
    : mXValue(aUnit), mYValue(aUnit)
  {
    MOZ_COUNT_CTOR(nsCSSValuePair);
  }
  nsCSSValuePair(const nsCSSValue& aXValue, const nsCSSValue& aYValue)
    : mXValue(aXValue), mYValue(aYValue)
  {
    MOZ_COUNT_CTOR(nsCSSValuePair);
  }
  nsCSSValuePair(const nsCSSValuePair& aCopy)
    : mXValue(aCopy.mXValue), mYValue(aCopy.mYValue)
  {
    MOZ_COUNT_CTOR(nsCSSValuePair);
  }
  ~nsCSSValuePair()
  {
    MOZ_COUNT_DTOR(nsCSSValuePair);
  }

  bool operator==(const nsCSSValuePair& aOther) const {
    return mXValue == aOther.mXValue &&
           mYValue == aOther.mYValue;
  }

  bool operator!=(const nsCSSValuePair& aOther) const {
    return mXValue != aOther.mXValue ||
           mYValue != aOther.mYValue;
  }

  bool BothValuesEqualTo(const nsCSSValue& aValue) const {
    return mXValue == aValue &&
           mYValue == aValue;
  }

  void SetBothValuesTo(const nsCSSValue& aValue) {
    mXValue = aValue;
    mYValue = aValue;
  }

  void Reset() {
    mXValue.Reset();
    mYValue.Reset();
  }

  bool HasValue() const {
    return mXValue.GetUnit() != eCSSUnit_Null ||
           mYValue.GetUnit() != eCSSUnit_Null;
  }

  void AppendToString(nsCSSProperty aProperty, nsAString& aResult,
                      nsCSSValue::Serialization aValueSerialization) const;

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  nsCSSValue mXValue;
  nsCSSValue mYValue;
};




struct nsCSSValuePair_heap final : public nsCSSValuePair {
  
  nsCSSValuePair_heap(const nsCSSValue& aXValue, const nsCSSValue& aYValue)
      : nsCSSValuePair(aXValue, aYValue)
  {}

  NS_INLINE_DECL_REFCOUNTING(nsCSSValuePair_heap)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  ~nsCSSValuePair_heap()
  {
  }
};

struct nsCSSValueTriplet {
    nsCSSValueTriplet()
    {
        MOZ_COUNT_CTOR(nsCSSValueTriplet);
    }
    explicit nsCSSValueTriplet(nsCSSUnit aUnit)
        : mXValue(aUnit), mYValue(aUnit), mZValue(aUnit)
    {
        MOZ_COUNT_CTOR(nsCSSValueTriplet);
    }
    nsCSSValueTriplet(const nsCSSValue& aXValue, 
                      const nsCSSValue& aYValue, 
                      const nsCSSValue& aZValue)
        : mXValue(aXValue), mYValue(aYValue), mZValue(aZValue)
    {
        MOZ_COUNT_CTOR(nsCSSValueTriplet);
    }
    nsCSSValueTriplet(const nsCSSValueTriplet& aCopy)
        : mXValue(aCopy.mXValue), mYValue(aCopy.mYValue), mZValue(aCopy.mZValue)
    {
        MOZ_COUNT_CTOR(nsCSSValueTriplet);
    }
    ~nsCSSValueTriplet()
    {
        MOZ_COUNT_DTOR(nsCSSValueTriplet);
    }

    bool operator==(const nsCSSValueTriplet& aOther) const {
        return mXValue == aOther.mXValue &&
               mYValue == aOther.mYValue &&
               mZValue == aOther.mZValue;
    }

    bool operator!=(const nsCSSValueTriplet& aOther) const {
        return mXValue != aOther.mXValue ||
               mYValue != aOther.mYValue ||
               mZValue != aOther.mZValue;
    }

    bool AllValuesEqualTo(const nsCSSValue& aValue) const {
        return mXValue == aValue &&
               mYValue == aValue &&
               mZValue == aValue;
    }

    void SetAllValuesTo(const nsCSSValue& aValue) {
        mXValue = aValue;
        mYValue = aValue;
        mZValue = aValue;
    }

    void Reset() {
        mXValue.Reset();
        mYValue.Reset();
        mZValue.Reset();
    }

    bool HasValue() const {
        return mXValue.GetUnit() != eCSSUnit_Null ||
               mYValue.GetUnit() != eCSSUnit_Null ||
               mZValue.GetUnit() != eCSSUnit_Null;
    }

    void AppendToString(nsCSSProperty aProperty, nsAString& aResult,
                        nsCSSValue::Serialization aValueSerialization) const;

    nsCSSValue mXValue;
    nsCSSValue mYValue;
    nsCSSValue mZValue;
};




struct nsCSSValueTriplet_heap final : public nsCSSValueTriplet {
  
  nsCSSValueTriplet_heap(const nsCSSValue& aXValue, const nsCSSValue& aYValue, const nsCSSValue& aZValue)
    : nsCSSValueTriplet(aXValue, aYValue, aZValue)
  {}

  NS_INLINE_DECL_REFCOUNTING(nsCSSValueTriplet_heap)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  ~nsCSSValueTriplet_heap()
  {
  }
};



inline nsCSSValuePair&
nsCSSValue::GetPairValue()
{
  MOZ_ASSERT(mUnit == eCSSUnit_Pair, "not a pair value");
  return *mValue.mPair;
}

inline const nsCSSValuePair&
nsCSSValue::GetPairValue() const
{
  MOZ_ASSERT(mUnit == eCSSUnit_Pair, "not a pair value");
  return *mValue.mPair;
}

inline nsCSSValueTriplet&
nsCSSValue::GetTripletValue()
{
    MOZ_ASSERT(mUnit == eCSSUnit_Triplet, "not a triplet value");
    return *mValue.mTriplet;
}

inline const nsCSSValueTriplet&
nsCSSValue::GetTripletValue() const
{
    MOZ_ASSERT(mUnit == eCSSUnit_Triplet, "not a triplet value");
    return *mValue.mTriplet;
}


struct nsCSSValuePairList {
  nsCSSValuePairList() : mNext(nullptr) { MOZ_COUNT_CTOR(nsCSSValuePairList); }
  ~nsCSSValuePairList();

  nsCSSValuePairList* Clone() const; 
  void AppendToString(nsCSSProperty aProperty, nsAString& aResult,
                      nsCSSValue::Serialization aValueSerialization) const;

  static bool Equal(const nsCSSValuePairList* aList1,
                    const nsCSSValuePairList* aList2);

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  nsCSSValue          mXValue;
  nsCSSValue          mYValue;
  nsCSSValuePairList* mNext;

private:
  nsCSSValuePairList(const nsCSSValuePairList& aCopy) 
    : mXValue(aCopy.mXValue), mYValue(aCopy.mYValue), mNext(nullptr)
  {
    MOZ_COUNT_CTOR(nsCSSValuePairList);
  }

  
  
  
  bool operator==(const nsCSSValuePairList& aOther) const = delete;
  bool operator!=(const nsCSSValuePairList& aOther) const = delete;
};




struct nsCSSValuePairList_heap final : public nsCSSValuePairList {
  NS_INLINE_DECL_REFCOUNTING(nsCSSValuePairList_heap)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  ~nsCSSValuePairList_heap()
  {
  }
};



inline nsCSSValuePairList*
nsCSSValue::GetPairListValue()
{
  if (mUnit == eCSSUnit_PairList)
    return mValue.mPairList;
  else {
    MOZ_ASSERT (mUnit == eCSSUnit_PairListDep, "not a pairlist value");
    return mValue.mPairListDependent;
  }
}

inline const nsCSSValuePairList*
nsCSSValue::GetPairListValue() const
{
  if (mUnit == eCSSUnit_PairList)
    return mValue.mPairList;
  else {
    MOZ_ASSERT (mUnit == eCSSUnit_PairListDep, "not a pairlist value");
    return mValue.mPairListDependent;
  }
}

struct nsCSSValueGradientStop {
public:
  nsCSSValueGradientStop();
  
  
  nsCSSValueGradientStop(const nsCSSValueGradientStop& aOther);
  ~nsCSSValueGradientStop();

  nsCSSValue mLocation;
  nsCSSValue mColor;
  
  
  bool mIsInterpolationHint;

  bool operator==(const nsCSSValueGradientStop& aOther) const
  {
    return (mLocation == aOther.mLocation &&
            mIsInterpolationHint == aOther.mIsInterpolationHint &&
            (mIsInterpolationHint || mColor == aOther.mColor));
  }

  bool operator!=(const nsCSSValueGradientStop& aOther) const
  {
    return !(*this == aOther);
  }

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
};

struct nsCSSValueGradient final {
  nsCSSValueGradient(bool aIsRadial, bool aIsRepeating);

  
  bool mIsRadial;
  bool mIsRepeating;
  bool mIsLegacySyntax;
  bool mIsExplicitSize;
  
  nsCSSValuePair mBgPos;
  nsCSSValue mAngle;

  
private:
  nsCSSValue mRadialValues[2];
public:
  nsCSSValue& GetRadialShape()
  {
    MOZ_ASSERT(!mIsExplicitSize);
    return mRadialValues[0];
  }
  const nsCSSValue& GetRadialShape() const
  {
    MOZ_ASSERT(!mIsExplicitSize);
    return mRadialValues[0];
  }
  nsCSSValue& GetRadialSize()
  {
    MOZ_ASSERT(!mIsExplicitSize);
    return mRadialValues[1];
  }
  const nsCSSValue& GetRadialSize() const
  {
    MOZ_ASSERT(!mIsExplicitSize);
    return mRadialValues[1];
  }
  nsCSSValue& GetRadiusX()
  {
    MOZ_ASSERT(mIsExplicitSize);
    return mRadialValues[0];
  }
  const nsCSSValue& GetRadiusX() const
  {
    MOZ_ASSERT(mIsExplicitSize);
    return mRadialValues[0];
  }
  nsCSSValue& GetRadiusY()
  {
    MOZ_ASSERT(mIsExplicitSize);
    return mRadialValues[1];
  }
  const nsCSSValue& GetRadiusY() const
  {
    MOZ_ASSERT(mIsExplicitSize);
    return mRadialValues[1];
  }

  InfallibleTArray<nsCSSValueGradientStop> mStops;

  bool operator==(const nsCSSValueGradient& aOther) const
  {
    if (mIsRadial != aOther.mIsRadial ||
        mIsRepeating != aOther.mIsRepeating ||
        mIsLegacySyntax != aOther.mIsLegacySyntax ||
        mIsExplicitSize != aOther.mIsExplicitSize ||
        mBgPos != aOther.mBgPos ||
        mAngle != aOther.mAngle ||
        mRadialValues[0] != aOther.mRadialValues[0] ||
        mRadialValues[1] != aOther.mRadialValues[1])
      return false;

    if (mStops.Length() != aOther.mStops.Length())
      return false;

    for (uint32_t i = 0; i < mStops.Length(); i++) {
      if (mStops[i] != aOther.mStops[i])
        return false;
    }

    return true;
  }

  bool operator!=(const nsCSSValueGradient& aOther) const
  {
    return !(*this == aOther);
  }

  NS_INLINE_DECL_REFCOUNTING(nsCSSValueGradient)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  ~nsCSSValueGradient()
  {
  }

  nsCSSValueGradient(const nsCSSValueGradient& aOther) = delete;
  nsCSSValueGradient& operator=(const nsCSSValueGradient& aOther) = delete;
};

struct nsCSSValueTokenStream final {
  nsCSSValueTokenStream();

private:
  
  ~nsCSSValueTokenStream();

public:
  bool operator==(const nsCSSValueTokenStream& aOther) const
  {
    bool eq;
    return mPropertyID == aOther.mPropertyID &&
           mShorthandPropertyID == aOther.mShorthandPropertyID &&
           mTokenStream.Equals(aOther.mTokenStream) &&
           (mBaseURI == aOther.mBaseURI ||
            (mBaseURI && aOther.mBaseURI &&
             NS_SUCCEEDED(mBaseURI->Equals(aOther.mBaseURI, &eq)) &&
             eq)) &&
           (mSheetURI == aOther.mSheetURI ||
            (mSheetURI && aOther.mSheetURI &&
             NS_SUCCEEDED(mSheetURI->Equals(aOther.mSheetURI, &eq)) &&
             eq)) &&
           (mSheetPrincipal == aOther.mSheetPrincipal ||
            (mSheetPrincipal && aOther.mSheetPrincipal &&
             NS_SUCCEEDED(mSheetPrincipal->Equals(aOther.mSheetPrincipal,
                                                  &eq)) &&
             eq));
  }

  bool operator!=(const nsCSSValueTokenStream& aOther) const
  {
    return !(*this == aOther);
  }

  NS_INLINE_DECL_REFCOUNTING(nsCSSValueTokenStream)

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  
  
  
  
  nsCSSProperty mPropertyID;

  
  
  
  nsCSSProperty mShorthandPropertyID;

  
  
  
  
  nsString mTokenStream;

  nsCOMPtr<nsIURI> mBaseURI;
  nsCOMPtr<nsIURI> mSheetURI;
  nsCOMPtr<nsIPrincipal> mSheetPrincipal;
  mozilla::CSSStyleSheet* mSheet;
  uint32_t mLineNumber;
  uint32_t mLineOffset;

  
  
  
  
  
  
  
  
  
  
  nsTHashtable<nsRefPtrHashKey<mozilla::css::ImageValue> > mImageValues;

private:
  nsCSSValueTokenStream(const nsCSSValueTokenStream& aOther) = delete;
  nsCSSValueTokenStream& operator=(const nsCSSValueTokenStream& aOther) = delete;
};

class nsCSSValueFloatColor final {
public:
  nsCSSValueFloatColor(float aComponent1, float aComponent2, float aComponent3,
                       float aAlpha)
    : mComponent1(aComponent1)
    , mComponent2(aComponent2)
    , mComponent3(aComponent3)
    , mAlpha(aAlpha)
  {
    MOZ_COUNT_CTOR(nsCSSValueFloatColor);
  }

private:
  
  ~nsCSSValueFloatColor()
  {
    MOZ_COUNT_DTOR(nsCSSValueFloatColor);
  }

public:
  bool operator==(nsCSSValueFloatColor& aOther) const;

  nscolor GetColorValue(nsCSSUnit aUnit) const;
  bool IsNonTransparentColor() const;

  void AppendToString(nsCSSUnit aUnit, nsAString& aResult) const;

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  NS_INLINE_DECL_REFCOUNTING(nsCSSValueFloatColor)

private:
  
  float mComponent1;  
  float mComponent2;  
  float mComponent3;  
  float mAlpha;       

  nsCSSValueFloatColor(const nsCSSValueFloatColor& aOther) = delete;
  nsCSSValueFloatColor& operator=(const nsCSSValueFloatColor& aOther)
                                                                   = delete;
};

struct nsCSSCornerSizes {
  nsCSSCornerSizes(void);
  nsCSSCornerSizes(const nsCSSCornerSizes& aCopy);
  ~nsCSSCornerSizes();

  
  nsCSSValue const & GetCorner(uint32_t aCorner) const {
    return this->*corners[aCorner];
  }
  nsCSSValue & GetCorner(uint32_t aCorner) {
    return this->*corners[aCorner];
  }

  bool operator==(const nsCSSCornerSizes& aOther) const {
    NS_FOR_CSS_FULL_CORNERS(corner) {
      if (this->GetCorner(corner) != aOther.GetCorner(corner))
        return false;
    }
    return true;
  }

  bool operator!=(const nsCSSCornerSizes& aOther) const {
    NS_FOR_CSS_FULL_CORNERS(corner) {
      if (this->GetCorner(corner) != aOther.GetCorner(corner))
        return true;
    }
    return false;
  }

  bool HasValue() const {
    NS_FOR_CSS_FULL_CORNERS(corner) {
      if (this->GetCorner(corner).GetUnit() != eCSSUnit_Null)
        return true;
    }
    return false;
  }

  void Reset();

  nsCSSValue mTopLeft;
  nsCSSValue mTopRight;
  nsCSSValue mBottomRight;
  nsCSSValue mBottomLeft;

protected:
  typedef nsCSSValue nsCSSCornerSizes::*corner_type;
  static const corner_type corners[4];
};

#endif 

