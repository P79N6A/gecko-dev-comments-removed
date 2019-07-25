












































#include "nscore.h"
#include "nsCSSStruct.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsCSSProps.h"
#include "nsFont.h"

#include "nsStyleConsts.h"

#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsPrintfCString.h"
#include "prlog.h"



nsCSSFont::nsCSSFont(void)
{
  MOZ_COUNT_CTOR(nsCSSFont);
}

nsCSSFont::~nsCSSFont(void)
{
  MOZ_COUNT_DTOR(nsCSSFont);
}



nsCSSValueList::~nsCSSValueList()
{
  MOZ_COUNT_DTOR(nsCSSValueList);
  NS_CSS_DELETE_LIST_MEMBER(nsCSSValueList, this, mNext);
}

nsCSSValueList*
nsCSSValueList::Clone() const
{
  nsCSSValueList* result = new nsCSSValueList(*this);
  nsCSSValueList* dest = result;
  const nsCSSValueList* src = this->mNext;
  while (src) {
    dest->mNext = new nsCSSValueList(*src);
    dest = dest->mNext;
    src = src->mNext;
  }
  return result;
}

void
nsCSSValueList::AppendToString(nsCSSProperty aProperty, nsAString& aResult) const
{
  const nsCSSValueList* val = this;
  for (;;) {
    val->mValue.AppendToString(aProperty, aResult);
    val = val->mNext;
    if (!val)
      break;

    if (nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_VALUE_LIST_USES_COMMAS))
      aResult.Append(PRUnichar(','));
    aResult.Append(PRUnichar(' '));
  }
}

bool
nsCSSValueList::operator==(const nsCSSValueList& aOther) const
{
  if (this == &aOther)
    return true;

  const nsCSSValueList *p1 = this, *p2 = &aOther;
  for ( ; p1 && p2; p1 = p1->mNext, p2 = p2->mNext) {
    if (p1->mValue != p2->mValue)
      return false;
  }
  return !p1 && !p2; 
}



nsCSSColor::nsCSSColor(void)
  : mBackImage(nsnull)
  , mBackRepeat(nsnull)
  , mBackAttachment(nsnull)
  , mBackPosition(nsnull)
  , mBackSize(nsnull)
  , mBackClip(nsnull)
  , mBackOrigin(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSColor);
}

nsCSSColor::~nsCSSColor(void)
{
  MOZ_COUNT_DTOR(nsCSSColor);

  delete mBackImage;
  delete mBackRepeat;
  delete mBackAttachment;
  delete mBackPosition;
  delete mBackSize;
  delete mBackClip;
  delete mBackOrigin;
}



nsCSSText::nsCSSText(void)
  : mTextShadow(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSText);
}

nsCSSText::~nsCSSText(void)
{
  MOZ_COUNT_DTOR(nsCSSText);
  delete mTextShadow;
}



nsCSSRect::nsCSSRect(void)
{
  MOZ_COUNT_CTOR(nsCSSRect);
}

nsCSSRect::nsCSSRect(const nsCSSRect& aCopy)
  : mTop(aCopy.mTop),
    mRight(aCopy.mRight),
    mBottom(aCopy.mBottom),
    mLeft(aCopy.mLeft)
{
  MOZ_COUNT_CTOR(nsCSSRect);
}

nsCSSRect::~nsCSSRect()
{
  MOZ_COUNT_DTOR(nsCSSRect);
}

void
nsCSSRect::AppendToString(nsCSSProperty aProperty, nsAString& aResult) const
{
  const nsCSSUnit topUnit = mTop.GetUnit();
  if (topUnit == eCSSUnit_Inherit ||
      topUnit == eCSSUnit_Initial ||
      topUnit == eCSSUnit_RectIsAuto) {
    NS_ASSERTION(mRight.GetUnit() == topUnit &&
                 mBottom.GetUnit() == topUnit &&
                 mLeft.GetUnit() == topUnit,
                 "parser should make all sides have the same unit");
    if (topUnit == eCSSUnit_RectIsAuto)
      aResult.AppendLiteral("auto");
    else
      mTop.AppendToString(aProperty, aResult);
  } else {
    aResult.AppendLiteral("rect(");
    mTop.AppendToString(aProperty, aResult);
    NS_NAMED_LITERAL_STRING(comma, ", ");
    aResult.Append(comma);
    mRight.AppendToString(aProperty, aResult);
    aResult.Append(comma);
    mBottom.AppendToString(aProperty, aResult);
    aResult.Append(comma);
    mLeft.AppendToString(aProperty, aResult);
    aResult.Append(PRUnichar(')'));
  }
}

void nsCSSRect::SetAllSidesTo(const nsCSSValue& aValue)
{
  mTop = aValue;
  mRight = aValue;
  mBottom = aValue;
  mLeft = aValue;
}

PR_STATIC_ASSERT((NS_SIDE_TOP == 0) && (NS_SIDE_RIGHT == 1) && (NS_SIDE_BOTTOM == 2) && (NS_SIDE_LEFT == 3));

 const nsCSSRect::side_type nsCSSRect::sides[4] = {
  &nsCSSRect::mTop,
  &nsCSSRect::mRight,
  &nsCSSRect::mBottom,
  &nsCSSRect::mLeft,
};



nsCSSCornerSizes::nsCSSCornerSizes(void)
{
  MOZ_COUNT_CTOR(nsCSSCornerSizes);
}

nsCSSCornerSizes::nsCSSCornerSizes(const nsCSSCornerSizes& aCopy)
  : mTopLeft(aCopy.mTopLeft),
    mTopRight(aCopy.mTopRight),
    mBottomRight(aCopy.mBottomRight),
    mBottomLeft(aCopy.mBottomLeft)
{
  MOZ_COUNT_CTOR(nsCSSCornerSizes);
}

nsCSSCornerSizes::~nsCSSCornerSizes()
{
  MOZ_COUNT_DTOR(nsCSSCornerSizes);
}

void
nsCSSCornerSizes::Reset()
{
  NS_FOR_CSS_FULL_CORNERS(corner) {
    this->GetFullCorner(corner).Reset();
  }
}

PR_STATIC_ASSERT(NS_CORNER_TOP_LEFT == 0 && NS_CORNER_TOP_RIGHT == 1 && \
    NS_CORNER_BOTTOM_RIGHT == 2 && NS_CORNER_BOTTOM_LEFT == 3);

 const nsCSSCornerSizes::corner_type
nsCSSCornerSizes::corners[4] = {
  &nsCSSCornerSizes::mTopLeft,
  &nsCSSCornerSizes::mTopRight,
  &nsCSSCornerSizes::mBottomRight,
  &nsCSSCornerSizes::mBottomLeft,
};



nsCSSValueListRect::nsCSSValueListRect(void)
  : mTop(nsnull),
    mRight(nsnull),
    mBottom(nsnull),
    mLeft(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSValueListRect);
}

nsCSSValueListRect::nsCSSValueListRect(const nsCSSValueListRect& aCopy)
  : mTop(aCopy.mTop),
    mRight(aCopy.mRight),
    mBottom(aCopy.mBottom),
    mLeft(aCopy.mLeft)
{
  MOZ_COUNT_CTOR(nsCSSValueListRect);
}

nsCSSValueListRect::~nsCSSValueListRect()
{
  MOZ_COUNT_DTOR(nsCSSValueListRect);
}

 const nsCSSValueListRect::side_type
nsCSSValueListRect::sides[4] = {
  &nsCSSValueListRect::mTop,
  &nsCSSValueListRect::mRight,
  &nsCSSValueListRect::mBottom,
  &nsCSSValueListRect::mLeft,
};




nsCSSDisplay::nsCSSDisplay(void) : mTransform(nsnull)
  , mTransitionProperty(nsnull)
  , mTransitionDuration(nsnull)
  , mTransitionTimingFunction(nsnull)
  , mTransitionDelay(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSDisplay);
}

nsCSSDisplay::~nsCSSDisplay(void)
{
  MOZ_COUNT_DTOR(nsCSSDisplay);
}



nsCSSMargin::nsCSSMargin(void)
  : mBoxShadow(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSMargin);
}

nsCSSMargin::~nsCSSMargin(void)
{
  MOZ_COUNT_DTOR(nsCSSMargin);
  delete mBoxShadow;
}



nsCSSPosition::nsCSSPosition(void)
{
  MOZ_COUNT_CTOR(nsCSSPosition);
}

nsCSSPosition::~nsCSSPosition(void)
{
  MOZ_COUNT_DTOR(nsCSSPosition);
}



nsCSSList::nsCSSList(void)
{
  MOZ_COUNT_CTOR(nsCSSList);
}

nsCSSList::~nsCSSList(void)
{
  MOZ_COUNT_DTOR(nsCSSList);
}



nsCSSTable::nsCSSTable(void)
{
  MOZ_COUNT_CTOR(nsCSSTable);
}

nsCSSTable::~nsCSSTable(void)
{
  MOZ_COUNT_DTOR(nsCSSTable);
}



nsCSSBreaks::nsCSSBreaks(void)
{
  MOZ_COUNT_CTOR(nsCSSBreaks);
}

nsCSSBreaks::~nsCSSBreaks(void)
{
  MOZ_COUNT_DTOR(nsCSSBreaks);
}



nsCSSPage::nsCSSPage(void)
{
  MOZ_COUNT_CTOR(nsCSSPage);
}

nsCSSPage::~nsCSSPage(void)
{
  MOZ_COUNT_DTOR(nsCSSPage);
}



void
nsCSSValuePair::AppendToString(nsCSSProperty aProperty, nsAString& aResult) const
{
  mXValue.AppendToString(aProperty, aResult);
  if (mYValue != mXValue ||
      ((aProperty == eCSSProperty_background_position ||
        aProperty == eCSSProperty__moz_transform_origin) &&
       mXValue.GetUnit() != eCSSUnit_Inherit &&
       mXValue.GetUnit() != eCSSUnit_Initial) ||
      (aProperty == eCSSProperty_background_size &&
       mXValue.GetUnit() != eCSSUnit_Inherit &&
       mXValue.GetUnit() != eCSSUnit_Initial &&
       mXValue.GetUnit() != eCSSUnit_Enumerated)) {
    
    
    
    
    
    aResult.Append(PRUnichar(' '));
    mYValue.AppendToString(aProperty, aResult);
  }
}



nsCSSValuePairList::~nsCSSValuePairList()
{
  MOZ_COUNT_DTOR(nsCSSValuePairList);
  NS_CSS_DELETE_LIST_MEMBER(nsCSSValuePairList, this, mNext);
}

nsCSSValuePairList*
nsCSSValuePairList::Clone() const
{
  nsCSSValuePairList* result = new nsCSSValuePairList(*this);
  nsCSSValuePairList* dest = result;
  const nsCSSValuePairList* src = this->mNext;
  while (src) {
    dest->mNext = new nsCSSValuePairList(*src);
    dest = dest->mNext;
    src = src->mNext;
  }
  return result;
}

void
nsCSSValuePairList::AppendToString(nsCSSProperty aProperty,
                                   nsAString& aResult) const
{
  const nsCSSValuePairList* val = this;
  for (;;) {
    NS_ABORT_IF_FALSE(val->mXValue.GetUnit() != eCSSUnit_Null,
                      "unexpected null unit");
    val->mXValue.AppendToString(aProperty, aResult);
    if (val->mXValue.GetUnit() != eCSSUnit_Inherit &&
        val->mXValue.GetUnit() != eCSSUnit_Initial &&
        val->mYValue.GetUnit() != eCSSUnit_Null) {
      aResult.Append(PRUnichar(' '));
      val->mYValue.AppendToString(aProperty, aResult);
    }
    val = val->mNext;
    if (!val)
      break;

    if (nsCSSProps::PropHasFlags(aProperty, CSS_PROPERTY_VALUE_LIST_USES_COMMAS))
      aResult.Append(PRUnichar(','));
    aResult.Append(PRUnichar(' '));
  }
}

bool
nsCSSValuePairList::operator==(const nsCSSValuePairList& aOther) const
{
  if (this == &aOther)
    return true;

  const nsCSSValuePairList *p1 = this, *p2 = &aOther;
  for ( ; p1 && p2; p1 = p1->mNext, p2 = p2->mNext) {
    if (p1->mXValue != p2->mXValue ||
        p1->mYValue != p2->mYValue)
      return false;
  }
  return !p1 && !p2; 
}



nsCSSContent::nsCSSContent(void)
  : mContent(nsnull),
    mCounterIncrement(nsnull),
    mCounterReset(nsnull),
    mQuotes(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSContent);
}

nsCSSContent::~nsCSSContent(void)
{
  MOZ_COUNT_DTOR(nsCSSContent);
  delete mContent;
  delete mCounterIncrement;
  delete mCounterReset;
  delete mQuotes;
}



nsCSSUserInterface::nsCSSUserInterface(void)
  : mCursor(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSUserInterface);
}

nsCSSUserInterface::~nsCSSUserInterface(void)
{
  MOZ_COUNT_DTOR(nsCSSUserInterface);
  delete mCursor;
}



nsCSSAural::nsCSSAural(void)
{
  MOZ_COUNT_CTOR(nsCSSAural);
}

nsCSSAural::~nsCSSAural(void)
{
  MOZ_COUNT_DTOR(nsCSSAural);
}



nsCSSXUL::nsCSSXUL(void)
{
  MOZ_COUNT_CTOR(nsCSSXUL);
}

nsCSSXUL::~nsCSSXUL(void)
{
  MOZ_COUNT_DTOR(nsCSSXUL);
}



nsCSSColumn::nsCSSColumn(void)
{
  MOZ_COUNT_CTOR(nsCSSColumn);
}

nsCSSColumn::~nsCSSColumn(void)
{
  MOZ_COUNT_DTOR(nsCSSColumn);
}



nsCSSSVG::nsCSSSVG(void) : mStrokeDasharray(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSSVG);
}

nsCSSSVG::~nsCSSSVG(void)
{
  MOZ_COUNT_DTOR(nsCSSSVG);
  delete mStrokeDasharray;
}
