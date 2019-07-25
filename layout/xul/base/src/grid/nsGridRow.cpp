











































#include "nsGridRow.h"
#include "nsIFrame.h"
#include "nsBoxLayoutState.h"

nsGridRow::nsGridRow():mIsBogus(PR_FALSE),
                       mBox(nsnull), 
                       mFlex(-1),
                       mPref(-1),
                       mMin(-1),
                       mMax(-1),
                       mTop(-1),
                       mBottom(-1), 
                       mTopMargin(0),
                       mBottomMargin(0)

{
    MOZ_COUNT_CTOR(nsGridRow);
}

void
nsGridRow::Init(nsIBox* aBox, bool aIsBogus)
{
  mBox = aBox;
  mIsBogus = aIsBogus;
  mFlex = -1;
  mPref = -1;
  mMin = -1;
  mMax = -1;
  mTop = -1;
  mBottom = -1;
  mTopMargin = 0;
  mBottomMargin = 0;
}

nsGridRow::~nsGridRow()
{
   MOZ_COUNT_DTOR(nsGridRow);
}

bool 
nsGridRow::IsCollapsed(nsBoxLayoutState& aState)
{
  return mBox && mBox->IsCollapsed(aState);
}

