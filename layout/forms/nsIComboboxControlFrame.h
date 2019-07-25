




































#ifndef nsIComboboxControlFrame_h___
#define nsIComboboxControlFrame_h___

#include "nsQueryFrame.h"
#include "nsFont.h"

class nsString;
class nsIContent;
class nsCSSFrameConstructor;






class nsIComboboxControlFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIComboboxControlFrame)

  


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

#endif

