




































#ifndef nsIComboboxControlFrame_h___
#define nsIComboboxControlFrame_h___

#include "nsISupports.h"
#include "nsFont.h"

class nsPresContext;
class nsString;
class nsIContent;
class nsVoidArray;
class nsCSSFrameConstructor;



#define NS_ICOMBOBOXCONTROLFRAME_IID    \
  { 0x23f75e9c, 0x6850, 0x11da, \
      { 0x95, 0x2c, 0x0, 0xe0, 0x81, 0x61, 0x16, 0x5f } }






class nsIComboboxControlFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICOMBOBOXCONTROLFRAME_IID)

  


  virtual PRBool IsDroppedDown() = 0;

  


  virtual void ShowDropDown(PRBool aDoDropDown) = 0;

  


  virtual nsIFrame* GetDropDown() = 0;

  


  virtual void SetDropDown(nsIFrame* aDropDownFrame) = 0;

  


  virtual void RollupFromList() = 0;

  


  NS_IMETHOD RedisplaySelectedText() = 0;

  


  virtual PRInt32 UpdateRecentIndex(PRInt32 aIndex) = 0;

  


  virtual void AbsolutelyPositionDropDown() = 0;

  


  virtual void OnContentReset() = 0;
  
  















  virtual PRInt32 GetIndexOfDisplayArea() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIComboboxControlFrame,
                              NS_ICOMBOBOXCONTROLFRAME_IID)

#endif

