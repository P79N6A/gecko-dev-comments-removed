




































#ifndef nsIPopupSetFrame_h___
#define nsIPopupSetFrame_h___


#define NS_IPOPUPSETFRAME_IID \
{ 0x043ecc8e, 0x469f, 0x40e1, \
 { 0x95, 0x69, 0x05, 0x29, 0xac, 0x0c, 0x30, 0x39 } }

class nsIFrame;
class nsIContent;
class nsIDOMElement;

#include "nsString.h"

class nsIPopupSetFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPOPUPSETFRAME_IID)

  NS_IMETHOD ShowPopup(nsIContent* aElementContent, nsIContent* aPopupContent, 
                       PRInt32 aXPos, PRInt32 aYPos, 
                       const nsString& aPopupType, const nsString& anAnchorAlignment,
                       const nsString& aPopupAlignment) = 0;
  NS_IMETHOD HidePopup(nsIFrame* aPopup) = 0;
  NS_IMETHOD DestroyPopup(nsIFrame* aPopup, PRBool aDestroyEntireChain) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPopupSetFrame, NS_IPOPUPSETFRAME_IID)

#endif

