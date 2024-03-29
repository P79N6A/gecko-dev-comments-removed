




#ifndef nsIComboboxControlFrame_h___
#define nsIComboboxControlFrame_h___

#include "nsQueryFrame.h"




class nsIComboboxControlFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIComboboxControlFrame)

  


  virtual bool IsDroppedDown() = 0;

  


  virtual void ShowDropDown(bool aDoDropDown) = 0;

  


  virtual nsIFrame* GetDropDown() = 0;

  


  virtual void SetDropDown(nsIFrame* aDropDownFrame) = 0;

  


  virtual void RollupFromList() = 0;

  




  NS_IMETHOD RedisplaySelectedText() = 0;

  


  virtual int32_t UpdateRecentIndex(int32_t aIndex) = 0;

  


  virtual void OnContentReset() = 0;
  
  















  virtual int32_t GetIndexOfDisplayArea() = 0;
};

#endif

