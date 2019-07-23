






































#ifndef nsIRootBox_h___
#define nsIRootBox_h___

#include "nsISupports.h"
class nsIFrame;
class nsIContent;
class nsIPresShell;


#define NS_IROOTBOX_IID \
{ 0x2256d568, 0x3f5a, 0x42ec, \
  { 0xb9, 0x32, 0x3d, 0x0f, 0x78, 0x55, 0x1a, 0x1a } }


class nsIRootBox : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IROOTBOX_IID)

  virtual nsIFrame* GetPopupSetFrame() = 0;
  virtual void SetPopupSetFrame(nsIFrame* aPopupSet)=0;

  virtual nsIContent* GetDefaultTooltip() = 0;
  virtual void SetDefaultTooltip(nsIContent* aTooltip) = 0;

  virtual nsresult AddTooltipSupport(nsIContent* aNode) = 0;
  virtual nsresult RemoveTooltipSupport(nsIContent* aNode) = 0;

  static nsIRootBox* GetRootBox(nsIPresShell* aShell);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRootBox, NS_IROOTBOX_IID)

#endif

