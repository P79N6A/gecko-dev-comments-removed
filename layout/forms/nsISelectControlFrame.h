





































#ifndef nsISelectControlFrame_h___
#define nsISelectControlFrame_h___

#include "nsQueryFrame.h"

class nsIDOMHTMLOptionElement;




class nsISelectControlFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsISelectControlFrame)

  



  NS_IMETHOD AddOption(nsPresContext* aPresContext, PRInt32 index) = 0;

  



  NS_IMETHOD RemoveOption(nsPresContext* aPresContext, PRInt32 index) = 0; 

  


  NS_IMETHOD GetOptionSelected(PRInt32 index, PRBool* value) = 0;

  



  NS_IMETHOD DoneAddingChildren(PRBool aIsDone) = 0;

  


  NS_IMETHOD OnOptionSelected(nsPresContext* aPresContext,
                              PRInt32 aIndex,
                              PRBool aSelected) = 0;

  


  NS_IMETHOD OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex) = 0;

};

#endif
