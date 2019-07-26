




#ifndef nsISelectControlFrame_h___
#define nsISelectControlFrame_h___

#include "nsQueryFrame.h"

class nsIDOMHTMLOptionElement;




class nsISelectControlFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsISelectControlFrame)

  



  NS_IMETHOD AddOption(int32_t index) = 0;

  



  NS_IMETHOD RemoveOption(int32_t index) = 0; 

  



  NS_IMETHOD DoneAddingChildren(bool aIsDone) = 0;

  


  NS_IMETHOD OnOptionSelected(int32_t aIndex, bool aSelected) = 0;

  



  NS_IMETHOD OnSetSelectedIndex(int32_t aOldIndex, int32_t aNewIndex) = 0;

};

#endif
