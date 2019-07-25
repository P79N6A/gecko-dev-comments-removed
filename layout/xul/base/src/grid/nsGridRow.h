











































#ifndef nsGridRow_h___
#define nsGridRow_h___

#include "nsIFrame.h"

class nsGridLayout2;
class nsBoxLayoutState;




class nsGridRow
{
public:
   nsGridRow();
   ~nsGridRow();
   
   void Init(nsIBox* aBox, bool aIsBogus);


   nsIBox* GetBox()   { return mBox;          }
   bool IsPrefSet() { return (mPref != -1); }
   bool IsMinSet()  { return (mMin  != -1); }
   bool IsMaxSet()  { return (mMax  != -1); } 
   bool IsFlexSet() { return (mFlex != -1); }
   bool IsOffsetSet() { return (mTop != -1 && mBottom != -1); }
   bool IsCollapsed(nsBoxLayoutState& aState);

public:

   bool    mIsBogus;
   nsIBox* mBox;
   nscoord mFlex;
   nscoord mPref;
   nscoord mMin;
   nscoord mMax;
   nscoord mTop;
   nscoord mBottom;
   nscoord mTopMargin;
   nscoord mBottomMargin;

};


#endif

