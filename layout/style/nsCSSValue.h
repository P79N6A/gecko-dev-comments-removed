






































#ifndef nsCSSValue_h___
#define nsCSSValue_h___

#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"
#include "nsCRTGlue.h"
#include "nsCSSKeywords.h"
#include "nsCSSProperty.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsString.h"
#include "nsStringBuffer.h"
#include "nsTArray.h"
#include "nsStyleConsts.h"

class imgIRequest;
class nsIDocument;
class nsIPrincipal;
class nsPresContext;
class nsIURI;


#define NS_CSS_DELETE_LIST_MEMBER(type_, ptr_, member_)                        \
  {                                                                            \
    type_ *cur = (ptr_)->member_;                                              \
    (ptr_)->member_ = nsnull;                                                  \
    while (cur) {                                                              \
      type_ *next = cur->member_;                                              \
      cur->member_ = nsnull;                                                   \
      delete cur;                                                              \
      cur = next;                                                              \
    }                                                                          \
  }




#define NS_CSS_CLONE_LIST_MEMBER(type_, from_, member_, to_, args_)            \
  {                                                                            \
    type_ *dest = (to_);                                                       \
    (to_)->member_ = nsnull;                                                   \
    for (const type_ *src = (from_)->member_; src; src = src->member_) {       \
      type_ *clone = src->Clone args_;                                         \
      if (!clone) {                                                            \
        delete (to_);                                                          \
        return nsnull;                                                         \
      }                                                                        \
      dest->member_ = clone;                                                   \
      dest = clone;                                                            \
    }                                                                          \
  }

enum nsCSSUnit {
  eCSSUnit_Null         = 0,      
  eCSSUnit_Auto         = 1,      
  eCSSUnit_Inherit      = 2,      
  eCSSUnit_Initial      = 3,      
  eCSSUnit_None         = 4,      
  eCSSUnit_Normal       = 5,      
  eCSSUnit_System_Font  = 6,      
  eCSSUnit_All          = 7,      
  eCSSUnit_Dummy        = 8,      
                                  
  eCSSUnit_DummyInherit = 9,      
                                  

  eCSSUnit_String       = 11,     
  eCSSUnit_Ident        = 12,     
  eCSSUnit_Families     = 13,     
  eCSSUnit_Attr         = 14,     
  eCSSUnit_Local_Font   = 15,     
  eCSSUnit_Font_Format  = 16,     
  eCSSUnit_Element      = 17,     

  eCSSUnit_Array        = 20,     
  eCSSUnit_Counter      = 21,     
  eCSSUnit_Counters     = 22,     
  eCSSUnit_Cubic_Bezier = 23,     
  eCSSUnit_Steps        = 24,     
  eCSSUnit_Function     = 25,     
                                  
                                  

  
  
  

  
  
  
  
  eCSSUnit_Calc         = 30,     
  
  
  eCSSUnit_Calc_Plus    = 31,     
  eCSSUnit_Calc_Minus   = 32,     
  eCSSUnit_Calc_Times_L = 33,     
  eCSSUnit_Calc_Times_R = 34,     
  eCSSUnit_Calc_Divided = 35,     

  eCSSUnit_URL          = 40,     
  eCSSUnit_Image        = 41,     
  eCSSUnit_Gradient     = 42,     

  eCSSUnit_Pair         = 50,     
  eCSSUnit_Triplet      = 51,     
  eCSSUnit_Rect         = 52,     
  eCSSUnit_List         = 53,     
  eCSSUnit_ListDep      = 54,     
                                  
  eCSSUnit_PairList     = 55,     
  eCSSUnit_PairListDep  = 56,     
                                  

  eCSSUnit_Integer      = 70,     
  eCSSUnit_Enumerated   = 71,     

  eCSSUnit_EnumColor    = 80,     
  eCSSUnit_Color        = 81,     

  eCSSUnit_Percent      = 90,     
  eCSSUnit_Number       = 91,     

  
  eCSSUnit_PhysicalMillimeter = 200,   

  
  
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
  eCSSUnit_Milliseconds = 3001     
};

struct nsCSSValueGradient;
struct nsCSSValuePair;
struct nsCSSValuePair_heap;
struct nsCSSRect;
struct nsCSSRect_heap;
struct nsCSSValueList;
struct nsCSSValueList_heap;
struct nsCSSValuePairList;
struct nsCSSValuePairList_heap;
struct nsCSSValueTriplet;
struct nsCSSValueTriplet_heap;

class nsCSSValue {
public:
  struct Array;
  friend struct Array;

  struct URL;
  friend struct URL;

  struct Image;
  friend struct Image;

  
  explicit nsCSSValue(nsCSSUnit aUnit = eCSSUnit_Null)
    : mUnit(aUnit)
  {
    NS_ABORT_IF_FALSE(aUnit <= eCSSUnit_DummyInherit, "not a valueless unit");
  }

  nsCSSValue(PRInt32 aValue, nsCSSUnit aUnit);
  nsCSSValue(float aValue, nsCSSUnit aUnit);
  nsCSSValue(const nsString& aValue, nsCSSUnit aUnit);
  nsCSSValue(Array* aArray, nsCSSUnit aUnit);
  explicit nsCSSValue(URL* aValue);
  explicit nsCSSValue(Image* aValue);
  explicit nsCSSValue(nsCSSValueGradient* aValue);
  nsCSSValue(const nsCSSValue& aCopy);
  ~nsCSSValue() { Reset(); }

  nsCSSValue&  operator=(const nsCSSValue& aCopy);
  bool        operator==(const nsCSSValue& aOther) const;

  bool operator!=(const nsCSSValue& aOther) const
  {
    return !(*this == aOther);
  }

  



  void AppendToString(nsCSSProperty aProperty, nsAString& aResult) const;

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

  PRInt32 GetIntValue() const
  {
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Integer ||
                      mUnit == eCSSUnit_Enumerated ||
                      mUnit == eCSSUnit_EnumColor,
                      "not an int value");
    return mValue.mInt;
  }

  float GetPercentValue() const
  {
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Percent, "not a percent value");
    return mValue.mFloat;
  }

  float GetFloatValue() const
  {
    NS_ABORT_IF_FALSE(eCSSUnit_Number <= mUnit, "not a float value");
    return mValue.mFloat;
  }

  float GetAngleValue() const
  {
    NS_ABORT_IF_FALSE(eCSSUnit_Degree <= mUnit &&
                 mUnit <= eCSSUnit_Turn, "not an angle value");
    return mValue.mFloat;
  }

  
  double GetAngleValueInRadians() const;

  nsAString& GetStringValue(nsAString& aBuffer) const
  {
    NS_ABORT_IF_FALSE(UnitHasStringValue(), "not a string value");
    aBuffer.Truncate();
    PRUint32 len = NS_strlen(GetBufferValue(mValue.mString));
    mValue.mString->ToString(len, aBuffer);
    return aBuffer;
  }

  const PRUnichar* GetStringBufferValue() const
  {
    NS_ABORT_IF_FALSE(UnitHasStringValue(), "not a string value");
    return GetBufferValue(mValue.mString);
  }

  nscolor GetColorValue() const
  {
    NS_ABORT_IF_FALSE((mUnit == eCSSUnit_Color), "not a color value");
    return mValue.mColor;
  }

  bool IsNonTransparentColor() const;

  Array* GetArrayValue() const
  {
    NS_ABORT_IF_FALSE(UnitHasArrayValue(), "not an array value");
    return mValue.mArray;
  }

  nsIURI* GetURLValue() const
  {
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_URL || mUnit == eCSSUnit_Image,
                 "not a URL value");
    return mUnit == eCSSUnit_URL ?
      mValue.mURL->GetURI() : mValue.mImage->GetURI();
  }

  nsCSSValueGradient* GetGradientValue() const
  {
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Gradient, "not a gradient value");
    return mValue.mGradient;
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

  URL* GetURLStructValue() const
  {
    
    
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_URL, "not a URL value");
    return mValue.mURL;
  }

  const PRUnichar* GetOriginalURLValue() const
  {
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_URL || mUnit == eCSSUnit_Image,
                      "not a URL value");
    return GetBufferValue(mUnit == eCSSUnit_URL ?
                            mValue.mURL->mString :
                            mValue.mImage->mString);
  }

  
  
  
  imgIRequest* GetImageValue() const;

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
  void SetIntValue(PRInt32 aValue, nsCSSUnit aUnit);
  void SetPercentValue(float aValue);
  void SetFloatValue(float aValue, nsCSSUnit aUnit);
  void SetStringValue(const nsString& aValue, nsCSSUnit aUnit);
  void SetColorValue(nscolor aValue);
  void SetArrayValue(nsCSSValue::Array* aArray, nsCSSUnit aUnit);
  void SetURLValue(nsCSSValue::URL* aURI);
  void SetImageValue(nsCSSValue::Image* aImage);
  void SetGradientValue(nsCSSValueGradient* aGradient);
  void SetPairValue(const nsCSSValuePair* aPair);
  void SetPairValue(const nsCSSValue& xValue, const nsCSSValue& yValue);
  void SetDependentListValue(nsCSSValueList* aList);
  void SetDependentPairListValue(nsCSSValuePairList* aList);
  void SetTripletValue(const nsCSSValueTriplet* aTriplet);
  void SetTripletValue(const nsCSSValue& xValue, const nsCSSValue& yValue, const nsCSSValue& zValue);
  void SetAutoValue();
  void SetInheritValue();
  void SetInitialValue();
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

  
  Array* InitFunction(nsCSSKeyword aFunctionId, PRUint32 aNumArgs);
  
  bool EqualsFunction(nsCSSKeyword aFunctionId) const;

  
  
  static already_AddRefed<nsStringBuffer>
    BufferFromString(const nsString& aValue);

  size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  struct URL {
    
    
    

    
    
    
    
    URL(nsStringBuffer* aString, nsIURI* aBaseURI, nsIURI* aReferrer,
        nsIPrincipal* aOriginPrincipal);
    
    URL(nsIURI* aURI, nsStringBuffer* aString, nsIURI* aReferrer,
        nsIPrincipal* aOriginPrincipal);

    ~URL();

    bool operator==(const URL& aOther) const;

    
    
    
    
    bool URIEquals(const URL& aOther) const;

    nsIURI* GetURI() const;

    size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  private:
    
    
    
    mutable nsCOMPtr<nsIURI> mURI;
  public:
    nsStringBuffer* mString; 
                             
    nsCOMPtr<nsIURI> mReferrer;
    nsCOMPtr<nsIPrincipal> mOriginPrincipal;

    NS_INLINE_DECL_REFCOUNTING(nsCSSValue::URL)

  private:
    mutable bool mURIResolved;

    URL(const URL& aOther) MOZ_DELETE;
    URL& operator=(const URL& aOther) MOZ_DELETE;
  };

  struct Image : public URL {
    
    
    
    
    Image(nsIURI* aURI, nsStringBuffer* aString, nsIURI* aReferrer,
          nsIPrincipal* aOriginPrincipal, nsIDocument* aDocument);
    ~Image();

    

    nsCOMPtr<imgIRequest> mRequest; 

    
    
    NS_INLINE_DECL_REFCOUNTING(nsCSSValue::Image)
  };

private:
  static const PRUnichar* GetBufferValue(nsStringBuffer* aBuffer) {
    return static_cast<PRUnichar*>(aBuffer->Data());
  }

protected:
  nsCSSUnit mUnit;
  union {
    PRInt32    mInt;
    float      mFloat;
    
    
    nsStringBuffer* mString;
    nscolor    mColor;
    Array*     mArray;
    URL*       mURL;
    Image*     mImage;
    nsCSSValueGradient* mGradient;
    nsCSSValuePair_heap* mPair;
    nsCSSRect_heap* mRect;
    nsCSSValueTriplet_heap* mTriplet;
    nsCSSValueList_heap* mList;
    nsCSSValueList* mListDependent;
    nsCSSValuePairList_heap* mPairList;
    nsCSSValuePairList* mPairListDependent;
  } mValue;
};

struct nsCSSValue::Array {

  
  static Array* Create(size_t aItemCount) {
    return new (aItemCount) Array(aItemCount);
  }

  nsCSSValue& operator[](size_t aIndex) {
    NS_ABORT_IF_FALSE(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  const nsCSSValue& operator[](size_t aIndex) const {
    NS_ABORT_IF_FALSE(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  nsCSSValue& Item(size_t aIndex) { return (*this)[aIndex]; }
  const nsCSSValue& Item(size_t aIndex) const { return (*this)[aIndex]; }

  size_t Count() const { return mCount; }

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
    NS_ABORT_IF_FALSE(aItemCount > 0, "cannot have a 0 item count");
    return ::operator new(aSelfSize + sizeof(nsCSSValue) * (aItemCount - 1));
  }

  void operator delete(void* aPtr) { ::operator delete(aPtr); }

  nsCSSValue* First() { return mArray; }

  const nsCSSValue* First() const { return mArray; }

#define CSSVALUE_LIST_FOR_EXTRA_VALUES(var)                                   \
  for (nsCSSValue *var = First() + 1, *var##_end = First() + mCount;          \
       var != var##_end; ++var)

  Array(size_t aItemCount)
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

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

#undef CSSVALUE_LIST_FOR_EXTRA_VALUES

private:
  Array(const Array& aOther) MOZ_DELETE;
  Array& operator=(const Array& aOther) MOZ_DELETE;
};


struct nsCSSValueList {
  nsCSSValueList() : mNext(nsnull) { MOZ_COUNT_CTOR(nsCSSValueList); }
  ~nsCSSValueList();

  nsCSSValueList* Clone() const;  
  void CloneInto(nsCSSValueList* aList) const; 
  void AppendToString(nsCSSProperty aProperty, nsAString& aResult) const;

  bool operator==(nsCSSValueList const& aOther) const;
  bool operator!=(const nsCSSValueList& aOther) const
  { return !(*this == aOther); }

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  nsCSSValue      mValue;
  nsCSSValueList* mNext;

private:
  nsCSSValueList(const nsCSSValueList& aCopy) 
    : mValue(aCopy.mValue), mNext(nsnull)
  {
    MOZ_COUNT_CTOR(nsCSSValueList);
  }
};




struct nsCSSValueList_heap : public nsCSSValueList {
  NS_INLINE_DECL_REFCOUNTING(nsCSSValueList_heap)

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
};



inline nsCSSValueList*
nsCSSValue::GetListValue()
{
  if (mUnit == eCSSUnit_List)
    return mValue.mList;
  else {
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_ListDep, "not a pairlist value");
    return mValue.mListDependent;
  }
}

inline const nsCSSValueList*
nsCSSValue::GetListValue() const
{
  if (mUnit == eCSSUnit_List)
    return mValue.mList;
  else {
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_ListDep, "not a pairlist value");
    return mValue.mListDependent;
  }
}

struct nsCSSRect {
  nsCSSRect(void);
  nsCSSRect(const nsCSSRect& aCopy);
  ~nsCSSRect();

  void AppendToString(nsCSSProperty aProperty, nsAString& aResult) const;

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




struct nsCSSRect_heap : public nsCSSRect {
  NS_INLINE_DECL_REFCOUNTING(nsCSSRect_heap)

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
};



inline nsCSSRect&
nsCSSValue::GetRectValue()
{
  NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Rect, "not a rect value");
  return *mValue.mRect;
}

inline const nsCSSRect&
nsCSSValue::GetRectValue() const
{
  NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Rect, "not a rect value");
  return *mValue.mRect;
}

struct nsCSSValuePair {
  nsCSSValuePair()
  {
    MOZ_COUNT_CTOR(nsCSSValuePair);
  }
  nsCSSValuePair(nsCSSUnit aUnit)
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

  void AppendToString(nsCSSProperty aProperty, nsAString& aResult) const;

  size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  nsCSSValue mXValue;
  nsCSSValue mYValue;
};




struct nsCSSValuePair_heap : public nsCSSValuePair {
  
  nsCSSValuePair_heap(const nsCSSValue& aXValue, const nsCSSValue& aYValue)
      : nsCSSValuePair(aXValue, aYValue)
  {}

  NS_INLINE_DECL_REFCOUNTING(nsCSSValuePair_heap)

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
};

struct nsCSSValueTriplet {
    nsCSSValueTriplet()
    {
        MOZ_COUNT_CTOR(nsCSSValueTriplet);
    }
    nsCSSValueTriplet(nsCSSUnit aUnit)
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

    void AppendToString(nsCSSProperty aProperty, nsAString& aResult) const;

    nsCSSValue mXValue;
    nsCSSValue mYValue;
    nsCSSValue mZValue;
};




struct nsCSSValueTriplet_heap : public nsCSSValueTriplet {
  
  nsCSSValueTriplet_heap(const nsCSSValue& aXValue, const nsCSSValue& aYValue, const nsCSSValue& aZValue)
    : nsCSSValueTriplet(aXValue, aYValue, aZValue)
  {}

  NS_INLINE_DECL_REFCOUNTING(nsCSSValueTriplet_heap)

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
};



inline nsCSSValuePair&
nsCSSValue::GetPairValue()
{
  NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Pair, "not a pair value");
  return *mValue.mPair;
}

inline const nsCSSValuePair&
nsCSSValue::GetPairValue() const
{
  NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Pair, "not a pair value");
  return *mValue.mPair;
}

inline nsCSSValueTriplet&
nsCSSValue::GetTripletValue()
{
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Triplet, "not a triplet value");
    return *mValue.mTriplet;
}

inline const nsCSSValueTriplet&
nsCSSValue::GetTripletValue() const
{
    NS_ABORT_IF_FALSE(mUnit == eCSSUnit_Triplet, "not a triplet value");
    return *mValue.mTriplet;
}


struct nsCSSValuePairList {
  nsCSSValuePairList() : mNext(nsnull) { MOZ_COUNT_CTOR(nsCSSValuePairList); }
  ~nsCSSValuePairList();

  nsCSSValuePairList* Clone() const; 
  void AppendToString(nsCSSProperty aProperty, nsAString& aResult) const;

  bool operator==(const nsCSSValuePairList& aOther) const;
  bool operator!=(const nsCSSValuePairList& aOther) const
  { return !(*this == aOther); }

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  nsCSSValue          mXValue;
  nsCSSValue          mYValue;
  nsCSSValuePairList* mNext;

private:
  nsCSSValuePairList(const nsCSSValuePairList& aCopy) 
    : mXValue(aCopy.mXValue), mYValue(aCopy.mYValue), mNext(nsnull)
  {
    MOZ_COUNT_CTOR(nsCSSValuePairList);
  }
};




struct nsCSSValuePairList_heap : public nsCSSValuePairList {
  NS_INLINE_DECL_REFCOUNTING(nsCSSValuePairList_heap)

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
};



inline nsCSSValuePairList*
nsCSSValue::GetPairListValue()
{
  if (mUnit == eCSSUnit_PairList)
    return mValue.mPairList;
  else {
    NS_ABORT_IF_FALSE (mUnit == eCSSUnit_PairListDep, "not a pairlist value");
    return mValue.mPairListDependent;
  }
}

inline const nsCSSValuePairList*
nsCSSValue::GetPairListValue() const
{
  if (mUnit == eCSSUnit_PairList)
    return mValue.mPairList;
  else {
    NS_ABORT_IF_FALSE (mUnit == eCSSUnit_PairListDep, "not a pairlist value");
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

  bool operator==(const nsCSSValueGradientStop& aOther) const
  {
    return (mLocation == aOther.mLocation &&
            mColor == aOther.mColor);
  }

  bool operator!=(const nsCSSValueGradientStop& aOther) const
  {
    return !(*this == aOther);
  }

  size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
};

struct nsCSSValueGradient {
  nsCSSValueGradient(bool aIsRadial, bool aIsRepeating);

  
  bool mIsRadial;
  bool mIsRepeating;
  bool mIsToCorner;
  
  nsCSSValuePair mBgPos;
  nsCSSValue mAngle;

  
  nsCSSValue mRadialShape;
  nsCSSValue mRadialSize;

  InfallibleTArray<nsCSSValueGradientStop> mStops;

  bool operator==(const nsCSSValueGradient& aOther) const
  {
    if (mIsRadial != aOther.mIsRadial ||
        mIsRepeating != aOther.mIsRepeating ||
        mIsToCorner != aOther.mIsToCorner ||
        mBgPos != aOther.mBgPos ||
        mAngle != aOther.mAngle ||
        mRadialShape != aOther.mRadialShape ||
        mRadialSize != aOther.mRadialSize)
      return false;

    if (mStops.Length() != aOther.mStops.Length())
      return false;

    for (PRUint32 i = 0; i < mStops.Length(); i++) {
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

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
  nsCSSValueGradient(const nsCSSValueGradient& aOther) MOZ_DELETE;
  nsCSSValueGradient& operator=(const nsCSSValueGradient& aOther) MOZ_DELETE;
};

struct nsCSSCornerSizes {
  nsCSSCornerSizes(void);
  nsCSSCornerSizes(const nsCSSCornerSizes& aCopy);
  ~nsCSSCornerSizes();

  
  nsCSSValue const & GetCorner(PRUint32 aCorner) const {
    return this->*corners[aCorner];
  }
  nsCSSValue & GetCorner(PRUint32 aCorner) {
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

