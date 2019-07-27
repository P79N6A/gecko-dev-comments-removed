




#ifndef nsIListControlFrame_h___
#define nsIListControlFrame_h___

#include "nsQueryFrame.h"

class nsAString;

namespace mozilla {
namespace dom {
class HTMLOptionElement;
} 
} 




class nsIListControlFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIListControlFrame)

  



  virtual void SetComboboxFrame(nsIFrame* aComboboxFrame) = 0;

  


  virtual void GetOptionText(uint32_t aIndex, nsAString& aStr) = 0;

  



  virtual int32_t GetSelectedIndex() = 0;

  



  virtual mozilla::dom::HTMLOptionElement* GetCurrentOption() = 0;

  



  virtual void CaptureMouseEvents(bool aGrabMouseEvents) = 0;

  



  virtual nscoord GetBSizeOfARow() = 0;

  



  virtual uint32_t GetNumberOfOptions() = 0;

  


  virtual void AboutToDropDown() = 0;

  


  virtual void AboutToRollup() = 0;

  


  virtual void FireOnChange() = 0;

  





  virtual void ComboboxFinish(int32_t aIndex) = 0;

  


  virtual void OnContentReset() = 0;
};

#endif

