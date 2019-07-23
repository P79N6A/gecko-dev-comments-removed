











































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
nsGridRow::Init(nsIBox* aBox, PRBool aIsBogus)
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

void 
nsGridRow::MarkDirty(nsBoxLayoutState& aState) 
{ 
  mPref = -1;
  mMin = -1;
  mMax = -1;
  mFlex = -1;
  mTop = -1;
  mBottom = -1;

  if (mBox) {
    mBox->AddStateBits(NS_FRAME_IS_DIRTY);
    aState.PresShell()->FrameNeedsReflow(mBox, nsIPresShell::eTreeChange);
  }
}

PRBool 
nsGridRow::IsCollapsed(nsBoxLayoutState& aState)
{
  return mBox && mBox->IsCollapsed(aState);
}

