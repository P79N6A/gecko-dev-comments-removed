











































#include "nsFrameNavigator.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIFrame.h"

nsIBox*
nsFrameNavigator::GetChildBeforeAfter(nsPresContext* aPresContext,
                                      nsIBox* start, PRBool before)
{
   nsIBox* parent = start->GetParentBox();
   PRInt32 index = IndexOf(aPresContext, parent,start);
   PRInt32 count = CountFrames(aPresContext, parent);

   if (index == -1) 
     return nsnull;

   if (before) {
     if (index == 0) {
         return nsnull;
     }

     return GetChildAt(aPresContext, parent, index-1);
   }


   if (index == count-1)
       return nsnull;

   return GetChildAt(aPresContext, parent, index+1);
}

PRInt32
nsFrameNavigator::IndexOf(nsPresContext* aPresContext, nsIBox* parent, nsIBox* child)
{
  PRInt32 count = 0;

  nsIBox* box = parent->GetChildBox();
  while (box)
  {    
    if (box == child)
       return count;

    box = box->GetNextBox();
    count++;
  }

  return -1;
}

PRInt32
nsFrameNavigator::CountFrames(nsPresContext* aPresContext, nsIBox* aBox)
{
  PRInt32 count = 0;

  nsIBox* box = aBox->GetChildBox();
  while (box)
  {    
    box = box->GetNextBox();
    count++;
  }

  return count;
}

nsIBox*
nsFrameNavigator::GetChildAt(nsPresContext* aPresContext, nsIBox* parent, PRInt32 index)
{
  PRInt32 count = 0;

  nsIBox* box = parent->GetChildBox();
  while (box)
  {    
    if (count == index)
       return box;

    box = box->GetNextBox();
    count++;
  }

  return nsnull;
}

