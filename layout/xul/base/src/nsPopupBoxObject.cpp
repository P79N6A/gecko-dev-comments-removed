





































#include "nsCOMPtr.h"
#include "nsIPopupBoxObject.h"
#include "nsIPopupSetFrame.h"
#include "nsIRootBox.h"
#include "nsBoxObject.h"
#include "nsIPresShell.h"
#include "nsFrameManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsMenuPopupFrame.h"


class nsPopupBoxObject : public nsBoxObject,
                         public nsIPopupBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPOPUPBOXOBJECT

  nsPopupBoxObject() {}
protected:
  virtual ~nsPopupBoxObject() {}

  nsIPopupSetFrame* GetPopupSetFrame();
  nsMenuPopupFrame* GetMenuPopupFrame()
  { return NS_STATIC_CAST(nsMenuPopupFrame*, GetFrame(PR_FALSE)); }
};

NS_IMPL_ISUPPORTS_INHERITED1(nsPopupBoxObject, nsBoxObject, nsIPopupBoxObject)

nsIPopupSetFrame*
nsPopupBoxObject::GetPopupSetFrame()
{
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(GetPresShell(PR_FALSE));
  if (!rootBox)
    return nsnull;

  nsIFrame* popupSetFrame = rootBox->GetPopupSetFrame();
  if (!popupSetFrame)
    return nsnull;

  nsIPopupSetFrame *popupSet = nsnull;
  CallQueryInterface(popupSetFrame, &popupSet);
  return popupSet;
}

NS_IMETHODIMP
nsPopupBoxObject::HidePopup()
{
  nsIPopupSetFrame *popupSet = GetPopupSetFrame();
  nsIFrame *ourFrame = GetFrame(PR_FALSE);
  if (ourFrame && popupSet) {
    nsWeakFrame weakFrame(ourFrame);
    popupSet->HidePopup(ourFrame);
    if (weakFrame.IsAlive()) {
      popupSet->DestroyPopup(ourFrame, PR_TRUE);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::ShowPopup(nsIDOMElement* aSrcContent, 
                            nsIDOMElement* aPopupContent, 
                            PRInt32 aXPos, PRInt32 aYPos, 
                            const PRUnichar *aPopupType,
                            const PRUnichar *anAnchorAlignment, 
                            const PRUnichar *aPopupAlignment)
{
  nsIPopupSetFrame *popupSet = GetPopupSetFrame();
  if (!popupSet) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> srcContent(do_QueryInterface(aSrcContent));
  nsCOMPtr<nsIContent> popupContent(do_QueryInterface(aPopupContent));
  NS_ENSURE_TRUE(popupContent, NS_ERROR_INVALID_ARG);
  

  nsAutoString popupType(aPopupType);
  nsAutoString anchorAlign(anAnchorAlignment);
  nsAutoString popupAlign(aPopupAlignment);

  
  
  nsAutoString left, top;
  popupContent->GetAttr(kNameSpaceID_None, nsGkAtoms::left, left);
  popupContent->GetAttr(kNameSpaceID_None, nsGkAtoms::top, top);
  
  PRInt32 err;
  if (!left.IsEmpty()) {
    aXPos = left.ToInteger(&err);
    if (NS_FAILED(err))
      return err;
  }
  if (!top.IsEmpty()) {
    aYPos = top.ToInteger(&err);
    if (NS_FAILED(err))
      return err;
  }

  return popupSet->ShowPopup(srcContent, popupContent, aXPos, aYPos, 
                             popupType, anchorAlign, popupAlign);
}

NS_IMETHODIMP
nsPopupBoxObject::MoveTo(PRInt32 aLeft, PRInt32 aTop)
{
  nsMenuPopupFrame *menuPopupFrame = GetMenuPopupFrame();
  if (menuPopupFrame) {
    menuPopupFrame->MoveTo(aLeft, aTop);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::SizeTo(PRInt32 aWidth, PRInt32 aHeight)
{
  nsAutoString width, height;
  width.AppendInt(aWidth);
  height.AppendInt(aHeight);

  nsCOMPtr<nsIContent> content = mContent;
  content->SetAttr(kNameSpaceID_None, nsGkAtoms::width, width, PR_FALSE);
  content->SetAttr(kNameSpaceID_None, nsGkAtoms::height, height, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::GetAutoPosition(PRBool* aShouldAutoPosition)
{
  nsMenuPopupFrame *menuPopupFrame = GetMenuPopupFrame();
  if (menuPopupFrame) {
    menuPopupFrame->GetAutoPosition(aShouldAutoPosition);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::SetAutoPosition(PRBool aShouldAutoPosition)
{
  nsMenuPopupFrame *menuPopupFrame = GetMenuPopupFrame();
  if (menuPopupFrame) {
    menuPopupFrame->SetAutoPosition(aShouldAutoPosition);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::EnableRollup(PRBool aShouldRollup)
{
  nsMenuPopupFrame *menuPopupFrame = GetMenuPopupFrame();
  if (menuPopupFrame) {
    menuPopupFrame->EnableRollup(aShouldRollup);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::SetConsumeRollupEvent(PRUint32 aConsume)
{
  nsMenuPopupFrame *menuPopupFrame = GetMenuPopupFrame();
  if (menuPopupFrame) {
    menuPopupFrame->SetConsumeRollupEvent(aConsume);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::EnableKeyboardNavigator(PRBool aEnableKeyboardNavigator)
{
  nsMenuPopupFrame *menuPopupFrame = GetMenuPopupFrame();
  if (menuPopupFrame) {
    if (aEnableKeyboardNavigator) {
      menuPopupFrame->InstallKeyboardNavigator();
    } else {
      menuPopupFrame->RemoveKeyboardNavigator();
    }
  }

  return NS_OK;
}



nsresult
NS_NewPopupBoxObject(nsIBoxObject** aResult)
{
  *aResult = new nsPopupBoxObject;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

