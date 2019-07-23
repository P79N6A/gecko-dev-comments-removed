






































#ifndef nsCSSValue_h___
#define nsCSSValue_h___

#include "nsColor.h"
#include "nsString.h"
#include "nsCoord.h"
#include "nsCSSProperty.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCRTGlue.h"
#include "nsStringBuffer.h"

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
  eCSSUnit_Dummy        = 7,      
                                  
  eCSSUnit_DummyInherit = 8,      
                                  
  eCSSUnit_RectIsAuto   = 9,      
  eCSSUnit_String       = 10,     
  eCSSUnit_Ident        = 11,     
  eCSSUnit_Families     = 12,     
  eCSSUnit_Attr         = 13,     
  eCSSUnit_Local_Font   = 14,     
  eCSSUnit_Font_Format  = 15,     
  eCSSUnit_Array        = 20,     
  eCSSUnit_Counter      = 21,     
  eCSSUnit_Counters     = 22,     
  eCSSUnit_Function     = 23,     
                                  

  eCSSUnit_URL          = 30,     
  eCSSUnit_Image        = 31,     
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

  nsCSSValue(PRInt32 aValue, nsCSSUnit aUnit) NS_HIDDEN;
  nsCSSValue(float aValue, nsCSSUnit aUnit) NS_HIDDEN;
  nsCSSValue(const nsString& aValue, nsCSSUnit aUnit) NS_HIDDEN;
  explicit nsCSSValue(nscolor aValue) NS_HIDDEN;
  nsCSSValue(Array* aArray, nsCSSUnit aUnit) NS_HIDDEN;
  explicit nsCSSValue(URL* aValue) NS_HIDDEN;
  explicit nsCSSValue(Image* aValue) NS_HIDDEN;
  nsCSSValue(const nsCSSValue& aCopy) NS_HIDDEN;
  ~nsCSSValue() { Reset(); }

  NS_HIDDEN_(nsCSSValue&)  operator=(const nsCSSValue& aCopy);
  NS_HIDDEN_(PRBool)      operator==(const nsCSSValue& aOther) const;

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

  PRBool    UnitHasStringValue() const
    { return eCSSUnit_String <= mUnit && mUnit <= eCSSUnit_Font_Format; }

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
    NS_ASSERTION(eCSSUnit_Array <= mUnit && mUnit <= eCSSUnit_Function,
                 "not an array value");
    return mValue.mArray;
  }

  nsIURI* GetURLValue() const
  {
    NS_ASSERTION(mUnit == eCSSUnit_URL || mUnit == eCSSUnit_Image,
                 "not a URL value");
    return mUnit == eCSSUnit_URL ?
      mValue.mURL->mURI : mValue.mImage->mURI;
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

  
  
  
  NS_HIDDEN_(imgIRequest*) GetImageValue() const;

  NS_HIDDEN_(nscoord)   GetLengthTwips() const;

  NS_HIDDEN_(void)  Reset()  
  {
    if (mUnit != eCSSUnit_Null)
      DoReset();
  }
private:
  NS_HIDDEN_(void)  DoReset();

public:
  NS_HIDDEN_(void)  SetIntValue(PRInt32 aValue, nsCSSUnit aUnit);
  NS_HIDDEN_(void)  SetPercentValue(float aValue);
  NS_HIDDEN_(void)  SetFloatValue(float aValue, nsCSSUnit aUnit);
  NS_HIDDEN_(void)  SetStringValue(const nsString& aValue, nsCSSUnit aUnit);
  NS_HIDDEN_(void)  SetColorValue(nscolor aValue);
  NS_HIDDEN_(void)  SetArrayValue(nsCSSValue::Array* aArray, nsCSSUnit aUnit);
  NS_HIDDEN_(void)  SetURLValue(nsCSSValue::URL* aURI);
  NS_HIDDEN_(void)  SetImageValue(nsCSSValue::Image* aImage);
  NS_HIDDEN_(void)  SetAutoValue();
  NS_HIDDEN_(void)  SetInheritValue();
  NS_HIDDEN_(void)  SetInitialValue();
  NS_HIDDEN_(void)  SetNoneValue();
  NS_HIDDEN_(void)  SetNormalValue();
  NS_HIDDEN_(void)  SetSystemFontValue();
  NS_HIDDEN_(void)  SetDummyValue();
  NS_HIDDEN_(void)  SetDummyInheritValue();
  NS_HIDDEN_(void)  SetRectIsAutoValue();
  NS_HIDDEN_(void)  StartImageLoad(nsIDocument* aDocument)
                                   const;  

  
  
  static nsStringBuffer* BufferFromString(const nsString& aValue);
  
  struct URL {
    
    
    

    
    
    URL(nsIURI* aURI, nsStringBuffer* aString, nsIURI* aReferrer,
        nsIPrincipal* aOriginPrincipal) NS_HIDDEN;

    ~URL() NS_HIDDEN;

    NS_HIDDEN_(PRBool) operator==(const URL& aOther) const;

    
    
    
    
    NS_HIDDEN_(PRBool) URIEquals(const URL& aOther) const;

    nsCOMPtr<nsIURI> mURI; 
    nsStringBuffer* mString; 
                             
    nsCOMPtr<nsIURI> mReferrer;
    nsCOMPtr<nsIPrincipal> mOriginPrincipal;

    void AddRef() {
      if (mRefCnt == PR_UINT32_MAX) {
        NS_WARNING("refcount overflow, leaking nsCSSValue::URL");
        return;
      }
      ++mRefCnt;
    }
    void Release() {
      if (mRefCnt == PR_UINT32_MAX) {
        NS_WARNING("refcount overflow, leaking nsCSSValue::URL");
        return;
      }
      if (--mRefCnt == 0)
        delete this;
    }
  protected:
    nsrefcnt mRefCnt;
  };

  struct Image : public URL {
    
    
    
    
    Image(nsIURI* aURI, nsStringBuffer* aString, nsIURI* aReferrer,
          nsIPrincipal* aOriginPrincipal, nsIDocument* aDocument) NS_HIDDEN;
    ~Image() NS_HIDDEN;

    

    nsCOMPtr<imgIRequest> mRequest; 

    
    void Release() {
      if (mRefCnt == PR_UINT32_MAX) {
        NS_WARNING("refcount overflow, leaking nsCSSValue::Image");
        return;
      }
      if (--mRefCnt == 0)
        delete this;
    }
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
  }         mValue;
};

struct nsCSSValue::Array {

  
  static Array* Create(PRUint16 aItemCount) {
    return new (aItemCount) Array(aItemCount);
  }

  nsCSSValue& operator[](PRUint16 aIndex) {
    NS_ASSERTION(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  const nsCSSValue& operator[](PRUint16 aIndex) const {
    NS_ASSERTION(aIndex < mCount, "out of range");
    return mArray[aIndex];
  }

  nsCSSValue& Item(PRUint16 aIndex) { return (*this)[aIndex]; }
  const nsCSSValue& Item(PRUint16 aIndex) const { return (*this)[aIndex]; }

  PRUint16 Count() const { return mCount; }

  PRBool operator==(const Array& aOther) const
  {
    if (mCount != aOther.mCount)
      return PR_FALSE;
    for (PRUint16 i = 0; i < mCount; ++i)
      if ((*this)[i] != aOther[i])
        return PR_FALSE;
    return PR_TRUE;
  }

  void AddRef() {
    if (mRefCnt == PR_UINT16_MAX) {
      NS_WARNING("refcount overflow, leaking nsCSSValue::Array");
      return;
    }
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsCSSValue::Array", sizeof(*this));
  }
  void Release() {
    if (mRefCnt == PR_UINT16_MAX) {
      NS_WARNING("refcount overflow, leaking nsCSSValue::Array");
      return;
    }
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsCSSValue::Array");
    if (mRefCnt == 0)
      delete this;
  }

private:

  PRUint16 mRefCnt;
  const PRUint16 mCount;
  
  
  
  nsCSSValue mArray[1];

  void* operator new(size_t aSelfSize, PRUint16 aItemCount) CPP_THROW_NEW {
    NS_ABORT_IF_FALSE(aItemCount > 0, "cannot have a 0 item count");
    return ::operator new(aSelfSize + sizeof(nsCSSValue) * (aItemCount - 1));
  }

  void operator delete(void* aPtr) { ::operator delete(aPtr); }

  nsCSSValue* First() { return mArray; }

  const nsCSSValue* First() const { return mArray; }

#define CSSVALUE_LIST_FOR_EXTRA_VALUES(var)                                   \
  for (nsCSSValue *var = First() + 1, *var##_end = First() + mCount;          \
       var != var##_end; ++var)

  Array(PRUint16 aItemCount)
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
};

#endif 

