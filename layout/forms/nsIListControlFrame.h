




































#ifndef nsIListControlFrame_h___
#define nsIListControlFrame_h___

#include "nsQueryFrame.h"
#include "nsFont.h"
class nsPresContext;
class nsAString;
class nsIContent;




class nsIListControlFrame : public nsQueryFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIListControlFrame)

  



  virtual void SetComboboxFrame(nsIFrame* aComboboxFrame) = 0;

  


  virtual void GetOptionText(PRInt32 aIndex, nsAString & aStr) = 0;

  



  virtual PRInt32 GetSelectedIndex() = 0;

  



  virtual void CaptureMouseEvents(PRBool aGrabMouseEvents) = 0;

  



  virtual nscoord GetHeightOfARow() = 0;

  



  virtual PRInt32 GetNumberOfOptions() = 0; 

  


  virtual void SyncViewWithFrame() = 0;

  


  virtual void AboutToDropDown() = 0;

  


  virtual void AboutToRollup() = 0;

  


  virtual void FireOnChange() = 0;

  





  virtual void ComboboxFinish(PRInt32 aIndex) = 0;

  


  virtual void OnContentReset() = 0;
};

#endif

