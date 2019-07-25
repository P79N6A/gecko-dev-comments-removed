





































#ifndef nsISelectControlFrame_h___
#define nsISelectControlFrame_h___

#include "nsQueryFrame.h"

class nsIDOMHTMLOptionElement;




class nsISelectControlFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsISelectControlFrame)

  



  NS_IMETHOD AddOption(PRInt32 index) = 0;

  



  NS_IMETHOD RemoveOption(PRInt32 index) = 0; 

  



  NS_IMETHOD DoneAddingChildren(bool aIsDone) = 0;

  


  NS_IMETHOD OnOptionSelected(PRInt32 aIndex, bool aSelected) = 0;

  



  NS_IMETHOD OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex) = 0;

};

#endif
