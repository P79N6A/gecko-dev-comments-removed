






































#ifndef nsIRootBox_h___
#define nsIRootBox_h___

#include "nsQueryFrame.h"
class nsPopupSetFrame;
class nsIContent;
class nsIPresShell;

class nsIRootBox
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIRootBox)

  virtual nsPopupSetFrame* GetPopupSetFrame() = 0;
  virtual void SetPopupSetFrame(nsPopupSetFrame* aPopupSet) = 0;

  virtual nsIContent* GetDefaultTooltip() = 0;
  virtual void SetDefaultTooltip(nsIContent* aTooltip) = 0;

  virtual nsresult AddTooltipSupport(nsIContent* aNode) = 0;
  virtual nsresult RemoveTooltipSupport(nsIContent* aNode) = 0;

  static nsIRootBox* GetRootBox(nsIPresShell* aShell);
};

#endif

