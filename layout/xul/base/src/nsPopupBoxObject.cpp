





































#include "nsCOMPtr.h"
#include "nsIPopupBoxObject.h"
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

  nsPopupSetFrame* GetPopupSetFrame();
  nsMenuPopupFrame* GetMenuPopupFrame()
  {
    nsIFrame* frame = GetFrame(PR_FALSE);
    if (frame && frame->GetType() == nsGkAtoms::menuPopupFrame)
      return static_cast<nsMenuPopupFrame*>(frame);
    return nsnull;
  }
};

NS_IMPL_ISUPPORTS_INHERITED1(nsPopupBoxObject, nsBoxObject, nsIPopupBoxObject)

nsPopupSetFrame*
nsPopupBoxObject::GetPopupSetFrame()
{
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(GetPresShell(PR_FALSE));
  if (!rootBox)
    return nsnull;

  return rootBox->GetPopupSetFrame();
}

NS_IMETHODIMP
nsPopupBoxObject::HidePopup()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mContent)
    pm->HidePopup(mContent, PR_FALSE, PR_TRUE, PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::ShowPopup(nsIDOMElement* aAnchorElement,
                            nsIDOMElement* aPopupElement,
                            PRInt32 aXPos, PRInt32 aYPos,
                            const PRUnichar *aPopupType,
                            const PRUnichar *aAnchorAlignment,
                            const PRUnichar *aPopupAlignment)
{
  NS_ENSURE_TRUE(aPopupElement, NS_ERROR_INVALID_ARG);
  

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mContent) {
    nsCOMPtr<nsIContent> anchorContent(do_QueryInterface(aAnchorElement));
    nsAutoString popupType(aPopupType);
    nsAutoString anchor(aAnchorAlignment);
    nsAutoString align(aPopupAlignment);
    pm->ShowPopupWithAnchorAlign(mContent, anchorContent, anchor, align,
                                 aXPos, aYPos, popupType.EqualsLiteral("context"));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::OpenPopup(nsIDOMElement* aAnchorElement,
                            const nsAString& aPosition,
                            PRInt32 aXPos, PRInt32 aYPos,
                            PRBool aIsContextMenu,
                            PRBool aAttributesOverride)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mContent) {
    nsCOMPtr<nsIContent> anchorContent(do_QueryInterface(aAnchorElement));
    pm->ShowPopup(mContent, anchorContent, aPosition, aXPos, aYPos,
                  aIsContextMenu, aAttributesOverride, PR_FALSE, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::OpenPopupAtScreen(PRInt32 aXPos, PRInt32 aYPos, PRBool aIsContextMenu)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mContent)
    pm->ShowPopupAtScreen(mContent, aXPos, aYPos, aIsContextMenu, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::MoveTo(PRInt32 aLeft, PRInt32 aTop)
{
  nsMenuPopupFrame *menuPopupFrame = GetMenuPopupFrame();
  if (menuPopupFrame) {
    menuPopupFrame->MoveTo(aLeft, aTop, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::SizeTo(PRInt32 aWidth, PRInt32 aHeight)
{
  if (!mContent)
    return NS_OK;

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
    *aShouldAutoPosition = menuPopupFrame->GetAutoPosition();
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
  if (!mContent)
    return NS_OK;

  
  if (aEnableKeyboardNavigator)
    mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::ignorekeys, PR_TRUE);
  else
    mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::ignorekeys,
                      NS_LITERAL_STRING("true"), PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::GetPopupState(nsAString& aState)
{
  
  aState.AssignLiteral("closed");

  nsMenuPopupFrame *menuPopupFrame = GetMenuPopupFrame();
  if (menuPopupFrame) {
    switch (menuPopupFrame->PopupState()) {
      case ePopupShowing:
      case ePopupOpen:
        aState.AssignLiteral("showing");
        break;
      case ePopupOpenAndVisible:
        aState.AssignLiteral("open");
        break;
      case ePopupHiding:
      case ePopupInvisible:
        aState.AssignLiteral("hiding");
        break;
      case ePopupClosed:
        break;
      default:
        NS_NOTREACHED("Bad popup state");
        break;
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
