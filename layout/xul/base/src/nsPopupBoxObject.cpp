



#include "nsCOMPtr.h"
#include "nsIPopupBoxObject.h"
#include "nsIRootBox.h"
#include "nsBoxObject.h"
#include "nsIPresShell.h"
#include "nsFrameManager.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsMenuPopupFrame.h"
#include "nsClientRect.h"

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
};

NS_IMPL_ISUPPORTS_INHERITED1(nsPopupBoxObject, nsBoxObject, nsIPopupBoxObject)

nsPopupSetFrame*
nsPopupBoxObject::GetPopupSetFrame()
{
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(GetPresShell(false));
  if (!rootBox)
    return nullptr;

  return rootBox->GetPopupSetFrame();
}

NS_IMETHODIMP
nsPopupBoxObject::HidePopup()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mContent)
    pm->HidePopup(mContent, false, true, false);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::ShowPopup(nsIDOMElement* aAnchorElement,
                            nsIDOMElement* aPopupElement,
                            int32_t aXPos, int32_t aYPos,
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
                            int32_t aXPos, int32_t aYPos,
                            bool aIsContextMenu,
                            bool aAttributesOverride,
                            nsIDOMEvent* aTriggerEvent)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mContent) {
    nsCOMPtr<nsIContent> anchorContent(do_QueryInterface(aAnchorElement));
    pm->ShowPopup(mContent, anchorContent, aPosition, aXPos, aYPos,
                  aIsContextMenu, aAttributesOverride, false, aTriggerEvent);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::OpenPopupAtScreen(int32_t aXPos, int32_t aYPos,
                                    bool aIsContextMenu,
                                    nsIDOMEvent* aTriggerEvent)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mContent)
    pm->ShowPopupAtScreen(mContent, aXPos, aYPos, aIsContextMenu, aTriggerEvent);
  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::MoveTo(int32_t aLeft, int32_t aTop)
{
  nsMenuPopupFrame *menuPopupFrame = mContent ? do_QueryFrame(mContent->GetPrimaryFrame()) : nullptr;
  if (menuPopupFrame) {
    menuPopupFrame->MoveTo(aLeft, aTop, true);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::SizeTo(int32_t aWidth, int32_t aHeight)
{
  if (!mContent)
    return NS_OK;

  nsAutoString width, height;
  width.AppendInt(aWidth);
  height.AppendInt(aHeight);

  nsCOMPtr<nsIContent> content = mContent;
  content->SetAttr(kNameSpaceID_None, nsGkAtoms::width, width, false);
  content->SetAttr(kNameSpaceID_None, nsGkAtoms::height, height, true);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::GetAutoPosition(bool* aShouldAutoPosition)
{
  *aShouldAutoPosition = true;

  nsMenuPopupFrame *menuPopupFrame = mContent ? do_QueryFrame(mContent->GetPrimaryFrame()) : nullptr;
  if (menuPopupFrame) {
    *aShouldAutoPosition = menuPopupFrame->GetAutoPosition();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::SetAutoPosition(bool aShouldAutoPosition)
{
  nsMenuPopupFrame *menuPopupFrame = mContent ? do_QueryFrame(mContent->GetPrimaryFrame()) : nullptr;
  if (menuPopupFrame) {
    menuPopupFrame->SetAutoPosition(aShouldAutoPosition);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::EnableRollup(bool aShouldRollup)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::SetConsumeRollupEvent(uint32_t aConsume)
{
  nsMenuPopupFrame *menuPopupFrame = do_QueryFrame(GetFrame(false));
  if (menuPopupFrame) {
    menuPopupFrame->SetConsumeRollupEvent(aConsume);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::EnableKeyboardNavigator(bool aEnableKeyboardNavigator)
{
  if (!mContent)
    return NS_OK;

  
  if (aEnableKeyboardNavigator)
    mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::ignorekeys, true);
  else
    mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::ignorekeys,
                      NS_LITERAL_STRING("true"), true);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::GetPopupState(nsAString& aState)
{
  
  aState.AssignLiteral("closed");

  nsMenuPopupFrame *menuPopupFrame = mContent ? do_QueryFrame(mContent->GetPrimaryFrame()) : nullptr;
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

NS_IMETHODIMP
nsPopupBoxObject::GetTriggerNode(nsIDOMNode** aTriggerNode)
{
  *aTriggerNode = nullptr;

  nsMenuPopupFrame *menuPopupFrame = mContent ? do_QueryFrame(mContent->GetPrimaryFrame()) : nullptr;
  nsIContent* triggerContent = nsMenuPopupFrame::GetTriggerContent(menuPopupFrame);
  if (triggerContent)
    CallQueryInterface(triggerContent, aTriggerNode);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::GetAnchorNode(nsIDOMElement** aAnchor)
{
  *aAnchor = nullptr;

  nsMenuPopupFrame *menuPopupFrame = mContent ? do_QueryFrame(mContent->GetPrimaryFrame()) : nullptr;
  if (!menuPopupFrame)
    return NS_OK;

  nsIContent* anchor = menuPopupFrame->GetAnchor();
  if (anchor)
    CallQueryInterface(anchor, aAnchor);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupBoxObject::GetOuterScreenRect(nsIDOMClientRect** aRect)
{
  nsClientRect* rect = new nsClientRect();
  if (!rect)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aRect = rect);

  nsMenuPopupFrame *menuPopupFrame = do_QueryFrame(GetFrame(false));
  if (!menuPopupFrame)
    return NS_OK;

  
  nsPopupState state = menuPopupFrame->PopupState();
  if (state != ePopupOpen && state != ePopupOpenAndVisible)
    return NS_OK;

  nsIView* view = menuPopupFrame->GetView();
  if (view) {
    nsIWidget* widget = view->GetWidget();
    if (widget) {
      nsIntRect screenRect;
      widget->GetScreenBounds(screenRect);

      int32_t pp = menuPopupFrame->PresContext()->AppUnitsPerDevPixel();
      rect->SetLayoutRect(screenRect.ToAppUnits(pp));
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
