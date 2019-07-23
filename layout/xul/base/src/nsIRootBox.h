






































#ifndef nsIRootBox_h___
#define nsIRootBox_h___

#include "nsISupports.h"
class nsPopupSetFrame;
class nsIContent;
class nsIPresShell;


#define NS_IROOTBOX_IID \
{ 0x9777EC2A, 0x9A46, 0x4D01, \
  { 0x8C, 0xEB, 0xB9, 0xCE, 0xB2, 0xC2, 0x62, 0xA5 } }


class nsIRootBox : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IROOTBOX_IID)

  virtual nsPopupSetFrame* GetPopupSetFrame() = 0;
  virtual void SetPopupSetFrame(nsPopupSetFrame* aPopupSet) = 0;

  virtual nsIContent* GetDefaultTooltip() = 0;
  virtual void SetDefaultTooltip(nsIContent* aTooltip) = 0;

  virtual nsresult AddTooltipSupport(nsIContent* aNode) = 0;
  virtual nsresult RemoveTooltipSupport(nsIContent* aNode) = 0;

  static nsIRootBox* GetRootBox(nsIPresShell* aShell);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRootBox, NS_IROOTBOX_IID)

#endif

