












































#include "nscore.h"
#include "nsCSSStruct.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsCSSProps.h"
#include "nsUnitConversion.h"
#include "nsFont.h"

#include "nsStyleConsts.h"

#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsPrintfCString.h"

#define CSS_IF_DELETE(ptr)  if (nsnull != ptr)  { delete ptr; ptr = nsnull; }



nsCSSFont::nsCSSFont(void)
{
  MOZ_COUNT_CTOR(nsCSSFont);
}

nsCSSFont::~nsCSSFont(void)
{
  MOZ_COUNT_DTOR(nsCSSFont);
}



#define CSS_IF_COPY(val, type) \
  if (aCopy.val) (val) = new type(*(aCopy.val));

nsCSSValueList::nsCSSValueList(void)
  : mValue(),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSValueList);
}

nsCSSValueList::nsCSSValueList(const nsCSSValueList& aCopy)
  : mValue(aCopy.mValue),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSValueList);
  CSS_IF_COPY(mNext, nsCSSValueList);
}

nsCSSValueList::~nsCSSValueList(void)
{
  MOZ_COUNT_DTOR(nsCSSValueList);
  CSS_IF_DELETE(mNext);
}

 PRBool
nsCSSValueList::Equal(nsCSSValueList* aList1, nsCSSValueList* aList2)
{
  if (aList1 == aList2)
    return PR_TRUE;
    
  nsCSSValueList *p1 = aList1, *p2 = aList2;
  for ( ; p1 && p2; p1 = p1->mNext, p2 = p2->mNext) {
    if (p1->mValue != p2->mValue)
      return PR_FALSE;
  }
  return !p1 && !p2; 
}



nsCSSColor::nsCSSColor(void)
{
  MOZ_COUNT_CTOR(nsCSSColor);
}

nsCSSColor::~nsCSSColor(void)
{
  MOZ_COUNT_DTOR(nsCSSColor);
}



nsCSSText::nsCSSText(void)
  : mTextShadow(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSText);
}

nsCSSText::~nsCSSText(void)
{
  MOZ_COUNT_DTOR(nsCSSText);
  CSS_IF_DELETE(mTextShadow);
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

void nsCSSRect::SetAllSidesTo(const nsCSSValue& aValue)
{
  mTop = aValue;
  mRight = aValue;
  mBottom = aValue;
  mLeft = aValue;
}

#if (NS_SIDE_TOP != 0) || (NS_SIDE_RIGHT != 1) || (NS_SIDE_BOTTOM != 2) || (NS_SIDE_LEFT != 3)
#error "Somebody changed the side constants."
#endif

 const nsCSSRect::side_type nsCSSRect::sides[4] = {
  &nsCSSRect::mTop,
  &nsCSSRect::mRight,
  &nsCSSRect::mBottom,
  &nsCSSRect::mLeft,
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



nsCSSDisplay::nsCSSDisplay(void)
{
  MOZ_COUNT_CTOR(nsCSSDisplay);
}

nsCSSDisplay::~nsCSSDisplay(void)
{
  MOZ_COUNT_DTOR(nsCSSDisplay);
}



nsCSSMargin::nsCSSMargin(void)
{
  MOZ_COUNT_CTOR(nsCSSMargin);
}

nsCSSMargin::~nsCSSMargin(void)
{
  MOZ_COUNT_DTOR(nsCSSMargin);
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



nsCSSCounterData::nsCSSCounterData(void)
  : mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSCounterData);
}

nsCSSCounterData::nsCSSCounterData(const nsCSSCounterData& aCopy)
  : mCounter(aCopy.mCounter),
    mValue(aCopy.mValue),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSCounterData);
  CSS_IF_COPY(mNext, nsCSSCounterData);
}

nsCSSCounterData::~nsCSSCounterData(void)
{
  MOZ_COUNT_DTOR(nsCSSCounterData);
  CSS_IF_DELETE(mNext);
}

 PRBool
nsCSSCounterData::Equal(nsCSSCounterData* aList1, nsCSSCounterData* aList2)
{
  if (aList1 == aList2)
    return PR_TRUE;

  nsCSSCounterData *p1 = aList1, *p2 = aList2;
  for ( ; p1 && p2; p1 = p1->mNext, p2 = p2->mNext) {
    if (p1->mCounter != p2->mCounter ||
        p1->mValue != p2->mValue)
      return PR_FALSE;
  }
  return !p1 && !p2; 
}

nsCSSQuotes::nsCSSQuotes(void)
  : mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSQuotes);
}

nsCSSQuotes::nsCSSQuotes(const nsCSSQuotes& aCopy)
  : mOpen(aCopy.mOpen),
    mClose(aCopy.mClose),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSQuotes);
  CSS_IF_COPY(mNext, nsCSSQuotes);
}

nsCSSQuotes::~nsCSSQuotes(void)
{
  MOZ_COUNT_DTOR(nsCSSQuotes);
  CSS_IF_DELETE(mNext);
}

 PRBool
nsCSSQuotes::Equal(nsCSSQuotes* aList1, nsCSSQuotes* aList2)
{
  if (aList1 == aList2)
    return PR_TRUE;

  nsCSSQuotes *p1 = aList1, *p2 = aList2;
  for ( ; p1 && p2; p1 = p1->mNext, p2 = p2->mNext) {
    if (p1->mOpen != p2->mOpen ||
        p1->mClose != p2->mClose)
      return PR_FALSE;
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
  CSS_IF_DELETE(mContent);
  CSS_IF_DELETE(mCounterIncrement);
  CSS_IF_DELETE(mCounterReset);
  CSS_IF_DELETE(mQuotes);
}



nsCSSUserInterface::nsCSSUserInterface(void)
  : mCursor(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSUserInterface);
}

nsCSSUserInterface::~nsCSSUserInterface(void)
{
  MOZ_COUNT_DTOR(nsCSSUserInterface);
  CSS_IF_DELETE(mCursor);
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

#ifdef MOZ_SVG


nsCSSSVG::nsCSSSVG(void) : mStrokeDasharray(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSSVG);
}

nsCSSSVG::~nsCSSSVG(void)
{
  MOZ_COUNT_DTOR(nsCSSSVG);
  CSS_IF_DELETE(mStrokeDasharray);
}

#endif 
