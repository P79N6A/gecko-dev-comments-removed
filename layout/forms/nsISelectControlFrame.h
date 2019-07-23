





































#ifndef nsISelectControlFrame_h___
#define nsISelectControlFrame_h___

#include "nsISupports.h"



#define NS_ISELECTCONTROLFRAME_IID \
{ 0xf8a1b329, 0xd0d8, 0x4bd5, \
 { 0xa9, 0xab, 0x08, 0xc3, 0xc0, 0xf2, 0xf1, 0x66 } }

class nsIDOMHTMLOptionElement;




class nsISelectControlFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISELECTCONTROLFRAME_IID)

  



  NS_IMETHOD AddOption(nsPresContext* aPresContext, PRInt32 index) = 0;

  



  NS_IMETHOD RemoveOption(nsPresContext* aPresContext, PRInt32 index) = 0; 

  


  NS_IMETHOD GetOptionSelected(PRInt32 index, PRBool* value) = 0;

  



  NS_IMETHOD DoneAddingChildren(PRBool aIsDone) = 0;

  


  NS_IMETHOD OnOptionSelected(nsPresContext* aPresContext,
                              PRInt32 aIndex,
                              PRBool aSelected) = 0;

  


  NS_IMETHOD OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISelectControlFrame,
                              NS_ISELECTCONTROLFRAME_IID)

#endif
