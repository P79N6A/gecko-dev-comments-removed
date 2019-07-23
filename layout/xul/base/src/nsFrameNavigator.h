











































#ifndef nsGrippyFrame_h___
#define nsGrippyFrame_h___

#include "nsIFrame.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

class nsFrameNavigator 
{
public:

  static nsIBox* GetChildBeforeAfter(nsPresContext* aPresContext, nsIBox* start, PRBool before);
  static nsIBox* GetChildAt(nsPresContext* aPresContext, nsIBox* parent, PRInt32 index);
  static PRInt32 IndexOf(nsPresContext* aPresContext, nsIBox* parent, nsIBox* child);
  static PRInt32 CountFrames(nsPresContext* aPresContext, nsIBox* aFrame);
}; 



#endif

