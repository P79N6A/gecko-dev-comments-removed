











#ifndef nsGridRow_h___
#define nsGridRow_h___

#include "nsCoord.h"

class nsIFrame;




class nsGridRow
{
public:
   nsGridRow();
   ~nsGridRow();

   void Init(nsIFrame* aBox, bool aIsBogus);


   nsIFrame* GetBox()   { return mBox;          }
   bool IsPrefSet() { return (mPref != -1); }
   bool IsMinSet()  { return (mMin  != -1); }
   bool IsMaxSet()  { return (mMax  != -1); } 
   bool IsFlexSet() { return (mFlex != -1); }
   bool IsOffsetSet() { return (mTop != -1 && mBottom != -1); }
   bool IsCollapsed();

public:

   bool    mIsBogus;
   nsIFrame* mBox;
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

