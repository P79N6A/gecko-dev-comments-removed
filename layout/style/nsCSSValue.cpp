






































#include "nsCSSValue.h"
#include "nsString.h"
#include "nsCSSProps.h"
#include "nsReadableUtils.h"
#include "imgIRequest.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsIPrincipal.h"


#include "prenv.h"

nsCSSValue::nsCSSValue(PRInt32 aValue, nsCSSUnit aUnit)
  : mUnit(aUnit)
{
  NS_ASSERTION(aUnit == eCSSUnit_Integer || aUnit == eCSSUnit_Enumerated ||
               aUnit == eCSSUnit_EnumColor, "not an int value");
  if (aUnit == eCSSUnit_Integer || aUnit == eCSSUnit_Enumerated ||
      aUnit == eCSSUnit_EnumColor) {
    mValue.mInt = aValue;
  }
  else {
    mUnit = eCSSUnit_Null;
    mValue.mInt = 0;
  }
}

nsCSSValue::nsCSSValue(float aValue, nsCSSUnit aUnit)
  : mUnit(aUnit)
{
  NS_ASSERTION(eCSSUnit_Percent <= aUnit, "not a float value");
  if (eCSSUnit_Percent <= aUnit) {
    mValue.mFloat = aValue;
  }
  else {
    mUnit = eCSSUnit_Null;
    mValue.mInt = 0;
  }
}

nsCSSValue::nsCSSValue(const nsString& aValue, nsCSSUnit aUnit)
  : mUnit(aUnit)
{
  NS_ASSERTION(UnitHasStringValue(), "not a string value");
  if (UnitHasStringValue()) {
    mValue.mString = BufferFromString(aValue);
    if (NS_UNLIKELY(!mValue.mString)) {
      
      
      mUnit = eCSSUnit_Null;
    }
  }
  else {
    mUnit = eCSSUnit_Null;
    mValue.mInt = 0;
  }
}

nsCSSValue::nsCSSValue(nsCSSValue::Array* aValue, nsCSSUnit aUnit)
  : mUnit(aUnit)
{
  NS_ASSERTION(eCSSUnit_Array <= aUnit && aUnit <= eCSSUnit_Function,
               "bad unit");
  mValue.mArray = aValue;
  mValue.mArray->AddRef();
}

nsCSSValue::nsCSSValue(nsCSSValue::URL* aValue)
  : mUnit(eCSSUnit_URL)
{
  mValue.mURL = aValue;
  mValue.mURL->AddRef();
}

nsCSSValue::nsCSSValue(nsCSSValue::Image* aValue)
  : mUnit(eCSSUnit_Image)
{
  mValue.mImage = aValue;
  mValue.mImage->AddRef();
}

nsCSSValue::nsCSSValue(nsCSSValueGradient* aValue)
  : mUnit(eCSSUnit_Gradient)
{
  mValue.mGradient = aValue;
  mValue.mGradient->AddRef();
}

nsCSSValue::nsCSSValue(const nsCSSValue& aCopy)
  : mUnit(aCopy.mUnit)
{
  if (mUnit <= eCSSUnit_RectIsAuto) {
    
  }
  else if (eCSSUnit_Percent <= mUnit) {
    mValue.mFloat = aCopy.mValue.mFloat;
  }
  else if (UnitHasStringValue()) {
    mValue.mString = aCopy.mValue.mString;
    mValue.mString->AddRef();
  }
  else if (eCSSUnit_Integer <= mUnit && mUnit <= eCSSUnit_EnumColor) {
    mValue.mInt = aCopy.mValue.mInt;
  }
  else if (eCSSUnit_Color == mUnit) {
    mValue.mColor = aCopy.mValue.mColor;
  }
  else if (eCSSUnit_Array <= mUnit && mUnit <= eCSSUnit_Function) {
    mValue.mArray = aCopy.mValue.mArray;
    mValue.mArray->AddRef();
  }
  else if (eCSSUnit_URL == mUnit) {
    mValue.mURL = aCopy.mValue.mURL;
    mValue.mURL->AddRef();
  }
  else if (eCSSUnit_Image == mUnit) {
    mValue.mImage = aCopy.mValue.mImage;
    mValue.mImage->AddRef();
  }
  else if (eCSSUnit_Gradient == mUnit) {
    mValue.mGradient = aCopy.mValue.mGradient;
    mValue.mGradient->AddRef();
  }
  else {
    NS_NOTREACHED("unknown unit");
  }
}

nsCSSValue& nsCSSValue::operator=(const nsCSSValue& aCopy)
{
  if (this != &aCopy) {
    Reset();
    new (this) nsCSSValue(aCopy);
  }
  return *this;
}

PRBool nsCSSValue::operator==(const nsCSSValue& aOther) const
{
  if (mUnit == aOther.mUnit) {
    if (mUnit <= eCSSUnit_RectIsAuto) {
      return PR_TRUE;
    }
    else if (UnitHasStringValue()) {
      return (NS_strcmp(GetBufferValue(mValue.mString),
                        GetBufferValue(aOther.mValue.mString)) == 0);
    }
    else if ((eCSSUnit_Integer <= mUnit) && (mUnit <= eCSSUnit_EnumColor)) {
      return mValue.mInt == aOther.mValue.mInt;
    }
    else if (eCSSUnit_Color == mUnit) {
      return mValue.mColor == aOther.mValue.mColor;
    }
    else if (eCSSUnit_Array <= mUnit && mUnit <= eCSSUnit_Function) {
      return *mValue.mArray == *aOther.mValue.mArray;
    }
    else if (eCSSUnit_URL == mUnit) {
      return *mValue.mURL == *aOther.mValue.mURL;
    }
    else if (eCSSUnit_Image == mUnit) {
      return *mValue.mImage == *aOther.mValue.mImage;
    }
    else if (eCSSUnit_Gradient == mUnit) {
      return *mValue.mGradient == *aOther.mValue.mGradient;
    }
    else {
      return mValue.mFloat == aOther.mValue.mFloat;
    }
  }
  return PR_FALSE;
}

imgIRequest* nsCSSValue::GetImageValue() const
{
  NS_ASSERTION(mUnit == eCSSUnit_Image, "not an Image value");
  return mValue.mImage->mRequest;
}

nscoord nsCSSValue::GetLengthTwips() const
{
  NS_ASSERTION(IsFixedLengthUnit(), "not a fixed length unit");

  if (IsFixedLengthUnit()) {
    switch (mUnit) {
    case eCSSUnit_Inch:        
      return NS_INCHES_TO_TWIPS(mValue.mFloat);

    case eCSSUnit_Millimeter:
      return NS_MILLIMETERS_TO_TWIPS(mValue.mFloat);
    case eCSSUnit_Centimeter:
      return NS_CENTIMETERS_TO_TWIPS(mValue.mFloat);

    case eCSSUnit_Point:
      return NS_POINTS_TO_TWIPS(mValue.mFloat);
    case eCSSUnit_Pica:
      return NS_PICAS_TO_TWIPS(mValue.mFloat);
    default:
      NS_ERROR("should never get here");
      break;
    }
  }
  return 0;
}

void nsCSSValue::DoReset()
{
  if (UnitHasStringValue()) {
    mValue.mString->Release();
  } else if (eCSSUnit_Array <= mUnit && mUnit <= eCSSUnit_Function) {
    mValue.mArray->Release();
  } else if (eCSSUnit_URL == mUnit) {
    mValue.mURL->Release();
  } else if (eCSSUnit_Image == mUnit) {
    mValue.mImage->Release();
  } else if (eCSSUnit_Gradient == mUnit) {
    mValue.mGradient->Release();
  }
  mUnit = eCSSUnit_Null;
}

void nsCSSValue::SetIntValue(PRInt32 aValue, nsCSSUnit aUnit)
{
  NS_ASSERTION(aUnit == eCSSUnit_Integer || aUnit == eCSSUnit_Enumerated ||
               aUnit == eCSSUnit_EnumColor, "not an int value");
  Reset();
  if (aUnit == eCSSUnit_Integer || aUnit == eCSSUnit_Enumerated ||
      aUnit == eCSSUnit_EnumColor) {
    mUnit = aUnit;
    mValue.mInt = aValue;
  }
}

void nsCSSValue::SetPercentValue(float aValue)
{
  Reset();
  mUnit = eCSSUnit_Percent;
  mValue.mFloat = aValue;
}

void nsCSSValue::SetFloatValue(float aValue, nsCSSUnit aUnit)
{
  NS_ASSERTION(eCSSUnit_Number <= aUnit, "not a float value");
  Reset();
  if (eCSSUnit_Number <= aUnit) {
    mUnit = aUnit;
    mValue.mFloat = aValue;
  }
}

void nsCSSValue::SetStringValue(const nsString& aValue,
                                nsCSSUnit aUnit)
{
  Reset();
  mUnit = aUnit;
  NS_ASSERTION(UnitHasStringValue(), "not a string unit");
  if (UnitHasStringValue()) {
    mValue.mString = BufferFromString(aValue);
    if (NS_UNLIKELY(!mValue.mString)) {
      
      
      mUnit = eCSSUnit_Null;
    }
  } else
    mUnit = eCSSUnit_Null;
}

void nsCSSValue::SetColorValue(nscolor aValue)
{
  Reset();
  mUnit = eCSSUnit_Color;
  mValue.mColor = aValue;
}

void nsCSSValue::SetArrayValue(nsCSSValue::Array* aValue, nsCSSUnit aUnit)
{
  NS_ASSERTION(eCSSUnit_Array <= aUnit && aUnit <= eCSSUnit_Function,
               "bad unit");
  Reset();
  mUnit = aUnit;
  mValue.mArray = aValue;
  mValue.mArray->AddRef();
}

void nsCSSValue::SetURLValue(nsCSSValue::URL* aValue)
{
  Reset();
  mUnit = eCSSUnit_URL;
  mValue.mURL = aValue;
  mValue.mURL->AddRef();
}

void nsCSSValue::SetImageValue(nsCSSValue::Image* aValue)
{
  Reset();
  mUnit = eCSSUnit_Image;
  mValue.mImage = aValue;
  mValue.mImage->AddRef();
}

void nsCSSValue::SetGradientValue(nsCSSValueGradient* aValue)
{
  Reset();
  mUnit = eCSSUnit_Gradient;
  mValue.mGradient = aValue;
  mValue.mGradient->AddRef();
}

void nsCSSValue::SetAutoValue()
{
  Reset();
  mUnit = eCSSUnit_Auto;
}

void nsCSSValue::SetInheritValue()
{
  Reset();
  mUnit = eCSSUnit_Inherit;
}

void nsCSSValue::SetInitialValue()
{
  Reset();
  mUnit = eCSSUnit_Initial;
}

void nsCSSValue::SetNoneValue()
{
  Reset();
  mUnit = eCSSUnit_None;
}

void nsCSSValue::SetAllValue()
{
  Reset();
  mUnit = eCSSUnit_All;
}

void nsCSSValue::SetNormalValue()
{
  Reset();
  mUnit = eCSSUnit_Normal;
}

void nsCSSValue::SetSystemFontValue()
{
  Reset();
  mUnit = eCSSUnit_System_Font;
}

void nsCSSValue::SetDummyValue()
{
  Reset();
  mUnit = eCSSUnit_Dummy;
}

void nsCSSValue::SetDummyInheritValue()
{
  Reset();
  mUnit = eCSSUnit_DummyInherit;
}

void nsCSSValue::SetRectIsAutoValue()
{
  Reset();
  mUnit = eCSSUnit_RectIsAuto;
}

void nsCSSValue::StartImageLoad(nsIDocument* aDocument) const
{
  NS_PRECONDITION(eCSSUnit_URL == mUnit, "Not a URL value!");
  nsCSSValue::Image* image =
    new nsCSSValue::Image(mValue.mURL->mURI,
                          mValue.mURL->mString,
                          mValue.mURL->mReferrer,
                          mValue.mURL->mOriginPrincipal,
                          aDocument);
  if (image) {
    nsCSSValue* writable = const_cast<nsCSSValue*>(this);
    writable->SetImageValue(image);
  }
}

PRBool nsCSSValue::IsNonTransparentColor() const
{
  
  
  
  nsDependentString buf;
  return
    (mUnit == eCSSUnit_Color && NS_GET_A(GetColorValue()) > 0) ||
    (mUnit == eCSSUnit_Ident &&
     !nsGkAtoms::transparent->Equals(GetStringValue(buf))) ||
    (mUnit == eCSSUnit_EnumColor);
}

nsCSSValue::Array*
nsCSSValue::InitFunction(nsCSSKeyword aFunctionId, PRUint32 aNumArgs)
{
  nsRefPtr<nsCSSValue::Array> func = Array::Create(aNumArgs + 1);
  if (!func) {
    return nsnull;
  }

  func->Item(0).SetIntValue(aFunctionId, eCSSUnit_Enumerated);
  SetArrayValue(func, eCSSUnit_Function);

  return func;
}

PRBool
nsCSSValue::EqualsFunction(nsCSSKeyword aFunctionId) const
{
  if (mUnit != eCSSUnit_Function) {
    return PR_FALSE;
  }

  nsCSSValue::Array* func = mValue.mArray;
  NS_ABORT_IF_FALSE(func && func->Count() >= 1 &&
                    func->Item(0).GetUnit() == eCSSUnit_Enumerated,
                    "illegally structured function value");

  nsCSSKeyword thisFunctionId =
    static_cast<nsCSSKeyword>(func->Item(0).GetIntValue());
  return thisFunctionId == aFunctionId;
}


nsStringBuffer*
nsCSSValue::BufferFromString(const nsString& aValue)
{
  nsStringBuffer* buffer = nsStringBuffer::FromString(aValue);
  if (buffer) {
    buffer->AddRef();
    return buffer;
  }
  
  PRUnichar length = aValue.Length();
  buffer = nsStringBuffer::Alloc((length + 1) * sizeof(PRUnichar));
  if (NS_LIKELY(buffer != 0)) {
    PRUnichar* data = static_cast<PRUnichar*>(buffer->Data());
    nsCharTraits<PRUnichar>::copy(data, aValue.get(), length);
    
    data[length] = 0;
  }

  return buffer;
}

nsCSSValue::URL::URL(nsIURI* aURI, nsStringBuffer* aString, nsIURI* aReferrer,
                     nsIPrincipal* aOriginPrincipal)
  : mURI(aURI),
    mString(aString),
    mReferrer(aReferrer),
    mOriginPrincipal(aOriginPrincipal),
    mRefCnt(0)
{
  NS_PRECONDITION(aOriginPrincipal, "Must have an origin principal");
  mString->AddRef();
}

nsCSSValue::URL::~URL()
{
  mString->Release();
}

PRBool
nsCSSValue::URL::operator==(const URL& aOther) const
{
  PRBool eq;
  return NS_strcmp(GetBufferValue(mString),
                   GetBufferValue(aOther.mString)) == 0 &&
          (mURI == aOther.mURI || 
           (mURI && aOther.mURI &&
            NS_SUCCEEDED(mURI->Equals(aOther.mURI, &eq)) &&
            eq)) &&
          (mOriginPrincipal == aOther.mOriginPrincipal ||
           (NS_SUCCEEDED(mOriginPrincipal->Equals(aOther.mOriginPrincipal,
                                                  &eq)) && eq));
}

PRBool
nsCSSValue::URL::URIEquals(const URL& aOther) const
{
  PRBool eq;
  
  
  
  
  return (mURI == aOther.mURI ||
          (NS_SUCCEEDED(mURI->Equals(aOther.mURI, &eq)) && eq)) &&
         (mOriginPrincipal == aOther.mOriginPrincipal ||
          (NS_SUCCEEDED(mOriginPrincipal->Equals(aOther.mOriginPrincipal,
                                                 &eq)) && eq));
}

nsCSSValue::Image::Image(nsIURI* aURI, nsStringBuffer* aString,
                         nsIURI* aReferrer, nsIPrincipal* aOriginPrincipal,
                         nsIDocument* aDocument)
  : URL(aURI, aString, aReferrer, aOriginPrincipal)
{
  if (mURI &&
      nsContentUtils::CanLoadImage(mURI, aDocument, aDocument,
                                   aOriginPrincipal)) {
    nsContentUtils::LoadImage(mURI, aDocument, aOriginPrincipal, aReferrer,
                              nsnull, nsIRequest::LOAD_NORMAL,
                              getter_AddRefs(mRequest));
  }
}

nsCSSValue::Image::~Image()
{
}

nsCSSValueGradientStop::nsCSSValueGradientStop(const nsCSSValue& aLocation,
                                               const nsCSSValue& aColor)
  : mLocation(aLocation),
    mColor(aColor)
{ 
  MOZ_COUNT_CTOR(nsCSSValueGradientStop);
}

nsCSSValueGradientStop::nsCSSValueGradientStop(const nsCSSValueGradientStop& aOther)
  : mLocation(aOther.mLocation),
    mColor(aOther.mColor)
{ 
  MOZ_COUNT_CTOR(nsCSSValueGradientStop);
}

nsCSSValueGradientStop::~nsCSSValueGradientStop()
{
  MOZ_COUNT_DTOR(nsCSSValueGradientStop);
}

nsCSSValueGradient::nsCSSValueGradient(PRBool aIsRadial, const nsCSSValue& aStartX,
           const nsCSSValue& aStartY, const nsCSSValue& aStartRadius, const nsCSSValue& aEndX,
           const nsCSSValue& aEndY, const nsCSSValue& aEndRadius)
  : mIsRadial(aIsRadial),
    mStartX(aStartX),
    mStartY(aStartY),
    mEndX(aEndX),
    mEndY(aEndY),
    mStartRadius(aStartRadius),
    mEndRadius(aEndRadius),
    mRefCnt(0)
{ 
}
