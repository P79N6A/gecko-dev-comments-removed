




































#ifndef nsIListControlFrame_h___
#define nsIListControlFrame_h___

#include "nsISupports.h"
#include "nsFont.h"
class nsPresContext;
class nsAString;
class nsIContent;


#define NS_ILISTCONTROLFRAME_IID    \
{ 0x4de9ab73, 0x31b5, 0x4d92, \
 { 0xb7, 0xe4, 0x73, 0xb4, 0x4d, 0xcb, 0xfc, 0xda } }




class nsIListControlFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILISTCONTROLFRAME_IID)

  



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

NS_DEFINE_STATIC_IID_ACCESSOR(nsIListControlFrame, NS_ILISTCONTROLFRAME_IID)

#endif

