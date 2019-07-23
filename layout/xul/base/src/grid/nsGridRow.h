











































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
   
   void Init(nsIBox* aBox, PRBool aIsBogus);
   void MarkDirty(nsBoxLayoutState& aState);


   nsIBox* GetBox()   { return mBox;          }
   PRBool IsPrefSet() { return (mPref != -1); }
   PRBool IsMinSet()  { return (mMin  != -1); }
   PRBool IsMaxSet()  { return (mMax  != -1); } 
   PRBool IsFlexSet() { return (mFlex != -1); }
   PRBool IsOffsetSet() { return (mTop != -1 && mBottom != -1); }
   PRBool IsCollapsed(nsBoxLayoutState& aState);

public:

   PRBool  mIsBogus;
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

