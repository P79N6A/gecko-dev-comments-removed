






































#ifndef nsCSSValue_h___
#define nsCSSValue_h___

#include "nsColor.h"
#include "nsString.h"
#include "nsCoord.h"
#include "nsCSSProperty.h"
#include "nsCSSKeywords.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCRTGlue.h"
#include "nsStringBuffer.h"
#include "nsTArray.h"
#include "nsISupportsImpl.h"

class imgIRequest;
class nsIDocument;
class nsIPrincipal;


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
                                  
  eCSSUnit_RectIsAuto   = 10,     
  eCSSUnit_String       = 11,     
  eCSSUnit_Ident        = 12,     
  eCSSUnit_Families     = 13,     
  eCSSUnit_Attr         = 14,     
  eCSSUnit_Local_Font   = 15,     
  eCSSUnit_Font_Format  = 16,     
  eCSSUnit_Array        = 20,     
  eCSSUnit_Counter      = 21,     
  eCSSUnit_Counters     = 22,     
  eCSSUnit_Cubic_Bezier = 23,     
  eCSSUnit_Function     = 24,     
                                  

  
  
  
  

  
  
  
  
  eCSSUnit_Calc         = 25,     
  
  
  eCSSUnit_Calc_Plus    = 26,     
  eCSSUnit_Calc_Minus   = 27,     
  eCSSUnit_Calc_Times_L = 28,     
  eCSSUnit_Calc_Times_R = 29,     
  eCSSUnit_Calc_Divided = 30,     
  
  eCSSUnit_Calc_Minimum = 31,     
  eCSSUnit_Calc_Maximum = 32,     

  eCSSUnit_URL          = 40,     
  eCSSUnit_Image        = 41,     
  eCSSUnit_Gradient     = 42,     
  eCSSUnit_Integer      = 50,     
  eCSSUnit_Enumerated   = 51,     
  eCSSUnit_EnumColor    = 80,     
  eCSSUnit_Color        = 81,     
  eCSSUnit_Percent      = 90,     
  eCSSUnit_Number       = 91,     

  
  
  eCSSUnit_Inch         = 100,    

  
  eCSSUnit_Millimeter   = 207,    
  eCSSUnit_Centimeter   = 208,    

  
  eCSSUnit_Point        = 300,    
  eCSSUnit_Pica         = 301,    

  
  
  eCSSUnit_EM           = 800,    
  eCSSUnit_XHeight      = 801,    
  eCSSUnit_Char         = 802,    
  eCSSUnit_RootEM       = 803,    

  
  eCSSUnit_Pixel        = 900,    

  
  eCSSUnit_Degree       = 1000,    
  eCSSUnit_Grad         = 1001,    
  eCSSUnit_Radian       = 1002,    

  
  eCSSUnit_Hertz        = 2000,    
  eCSSUnit_Kilohertz    = 2001,    

  
  eCSSUnit_Seconds      = 3000,    
  eCSSUnit_Milliseconds = 3001     
};

struct nsCSSValueGradient;

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
    NS_ASSERTION(aUnit <= eCSSUnit_RectIsAuto, "not a valueless unit");
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
  PRBool      operator==(const nsCSSValue& aOther) const;

  PRBool operator!=(const nsCSSValue& aOther) const
  {
    return !(*this == aOther);
  }

  nsCSSUnit GetUnit() const { return mUnit; }
  PRBool    IsLengthUnit() const
    { return eCSSUnit_Inch <= mUnit && mUnit <= eCSSUnit_Pixel; }
  PRBool    IsFixedLengthUnit() const  
    { return eCSSUnit_Inch <= mUnit && mUnit <= eCSSUnit_Pica; }
  PRBool    IsRelativeLengthUnit() const  
    { return eCSSUnit_EM <= mUnit && mUnit <= eCSSUnit_Pixel; }
  PRBool    IsAngularUnit() const  
    { return eCSSUnit_Degree <= mUnit && mUnit <= eCSSUnit_Radian; }
  PRBool    IsFrequencyUnit() const  
    { return eCSSUnit_Hertz <= mUnit && mUnit <= eCSSUnit_Kilohertz; }
  PRBool    IsTimeUnit() const  
    { return eCSSUnit_Seconds <= mUnit && mUnit <= eCSSUnit_Milliseconds; }
  PRBool    IsCalcUnit() const
    { return eCSSUnit_Calc <= mUnit && mUnit <= eCSSUnit_Calc_Maximum; }

  PRBool    UnitHasStringValue() const
    { return eCSSUnit_String <= mUnit && mUnit <= eCSSUnit_Font_Format; }
  PRBool    UnitHasArrayValue() const
    { return eCSSUnit_Array <= mUnit && mUnit <= eCSSUnit_Calc_Maximum; }

  PRInt32 GetIntValue() const
  {
    NS_ASSERTION(mUnit == eCSSUnit_Integer || mUnit == eCSSUnit_Enumerated ||
                 mUnit == eCSSUnit_EnumColor,
                 "not an int value");
    return mValue.mInt;
  }

  float GetPercentValue() const
  {
    NS_ASSERTION(mUnit == eCSSUnit_Percent, "not a percent value");
    return mValue.mFloat;
  }

  float GetFloatValue() const
  {
    NS_ASSERTION(eCSSUnit_Number <= mUnit, "not a float value");
    return mValue.mFloat;
  }

  float GetAngleValue() const
  {
    NS_ASSERTION(eCSSUnit_Degree <= mUnit &&
                 mUnit <= eCSSUnit_Radian, "not an angle value");
    return mValue.mFloat;
  }

  
  double GetAngleValueInRadians() const;

  nsAString& GetStringValue(nsAString& aBuffer) const
  {
    NS_ASSERTION(UnitHasStringValue(), "not a string value");
    aBuffer.Truncate();
    PRUint32 len = NS_strlen(GetBufferValue(mValue.mString));
    mValue.mString->ToString(len, aBuffer);
    return aBuffer;
  }

  const PRUnichar* GetStringBufferValue() const
  {
    NS_ASSERTION(UnitHasStringValue(), "not a string value");
    return GetBufferValue(mValue.mString);
  }

  nscolor GetColorValue() const
  {
    NS_ASSERTION((mUnit == eCSSUnit_Color), "not a color value");
    return mValue.mColor;
  }

  PRBool IsNonTransparentColor() const;

  Array* GetArrayValue() const
  {
    NS_ASSERTION(UnitHasArrayValue(), "not an array value");
    return mValue.mArray;
  }

  nsIURI* GetURLValue() const
  {
    NS_ASSERTION(mUnit == eCSSUnit_URL || mUnit == eCSSUnit_Image,
                 "not a URL value");
    return mUnit == eCSSUnit_URL ?
      mValue.mURL->mURI : mValue.mImage->mURI;
  }

  nsCSSValueGradient* GetGradientValue() const
  {
    NS_ASSERTION(mUnit == eCSSUnit_Gradient, "not a gradient value");
    return mValue.mGradient;
  }

  URL* GetURLStructValue() const
  {
    
    
    NS_ASSERTION(mUnit == eCSSUnit_URL, "not a URL value");
    return mValue.mURL;
  }

  const PRUnichar* GetOriginalURLValue() const
  {
    NS_ASSERTION(mUnit == eCSSUnit_URL || mUnit == eCSSUnit_Image,
                 "not a URL value");
    return GetBufferValue(mUnit == eCSSUnit_URL ?
                            mValue.mURL->mString :
                            mValue.mImage->mString);
  }

  
  
  
  imgIRequest* GetImageValue() const;

  nscoord GetLengthTwips() const;

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
  void SetAutoValue();
  void SetInheritValue();
  void SetInitialValue();
  void SetNoneValue();
  void SetAllValue();
  void SetNormalValue();
  void SetSystemFontValue();
  void SetDummyValue();
  void SetDummyInheritValue();
  void StartImageLoad(nsIDocument* aDocument) const;  

  
  Array* InitFunction(nsCSSKeyword aFunctionId, PRUint32 aNumArgs);
  
  PRBool EqualsFunction(nsCSSKeyword aFunctionId) const;

  
  
  static nsStringBuffer* BufferFromString(const nsString& aValue);
  
  struct URL {
    
    
    

    
    
    URL(nsIURI* aURI, nsStringBuffer* aString, nsIURI* aReferrer,
        nsIPrincipal* aOriginPrincipal);

    ~URL();

    PRBool operator==(const URL& aOther) const;

    
    
    
    
    PRBool URIEquals(const URL& aOther) const;

    nsCOMPtr<nsIURI> mURI; 
    nsStringBuffer* mString; 
                             
    nsCOMPtr<nsIURI> mReferrer;
    nsCOMPtr<nsIPrincipal> mOriginPrincipal;

    NS_INLINE_DECL_REFCOUNTING(nsCSSValue::URL)

  protected:

    
    URL(const URL& aOther);
    URL& operator=(const URL& aOther);
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
  }         mValue;
};

struct nsCSSValueGradientStop {
public:
  nsCSSValueGradientStop();
  
  nsCSSValueGradientStop(const nsCSSValueGradientStop& aOther);
  ~nsCSSValueGradientStop();

  nsCSSValue mLocation;
  nsCSSValue mColor;

  PRBool operator==(const nsCSSValueGradientStop& aOther) const
  {
    return (mLocation == aOther.mLocation &&
            mColor == aOther.mColor);
  }

  PRBool operator!=(const nsCSSValueGradientStop& aOther) const
  {
    return !(*this == aOther);
  }
};

struct nsCSSValueGradient {
  nsCSSValueGradient(PRBool aIsRadial, PRBool aIsRepeating);

  
  PRPackedBool mIsRadial;
  PRPackedBool mIsRepeating;
  
  nsCSSValue mBgPosX;
  nsCSSValue mBgPosY;
  nsCSSValue mAngle;

  
  nsCSSValue mRadialShape;
  nsCSSValue mRadialSize;

  nsTArray<nsCSSValueGradientStop> mStops;

  PRBool operator==(const nsCSSValueGradient& aOther) const
  {
    if (mIsRadial != aOther.mIsRadial ||
        mIsRepeating != aOther.mIsRepeating ||
        mBgPosX != aOther.mBgPosX ||
        mBgPosY != aOther.mBgPosY ||
        mAngle != aOther.mAngle ||
        mRadialShape != aOther.mRadialShape ||
        mRadialSize != aOther.mRadialSize)
      return PR_FALSE;

    if (mStops.Length() != aOther.mStops.Length())
      return PR_FALSE;

    for (PRUint32 i = 0; i < mStops.Length(); i++) {
      if (mStops[i] != aOther.mStops[i])
        return PR_FALSE;
    }

    return PR_TRUE;
  }

  PRBool operator!=(const nsCSSValueGradient& aOther) const
  {
    return !(*this == aOther);
  }

  NS_INLINE_DECL_REFCOUNTING(nsCSSValueGradient)

private:
  
  nsCSSValueGradient(const nsCSSValueGradient& aOther);
  nsCSSValueGradient& operator=(const nsCSSValueGradient& aOther);
};

struct nsCSSValue::Array {

  
  static Array* Create(size_t aItemCount) {
    return new (aItemCount) Array(aItemCount);
  }

  nsCSSValue& operator[](size_t aIndex) {
    NS_ASSERTION(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  const nsCSSValue& operator[](size_t aIndex) const {
    NS_ASSERTION(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  nsCSSValue& Item(size_t aIndex) { return (*this)[aIndex]; }
  const nsCSSValue& Item(size_t aIndex) const { return (*this)[aIndex]; }

  size_t Count() const { return mCount; }

  PRBool operator==(const Array& aOther) const
  {
    if (mCount != aOther.mCount)
      return PR_FALSE;
    for (size_t i = 0; i < mCount; ++i)
      if ((*this)[i] != aOther[i])
        return PR_FALSE;
    return PR_TRUE;
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

#undef CSSVALUE_LIST_FOR_EXTRA_VALUES

private:
  
  Array(const Array& aOther);
  Array& operator=(const Array& aOther);
};

#endif 

