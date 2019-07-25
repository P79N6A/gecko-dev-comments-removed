






































#include "nsHTMLImageMapAccessible.h"

#include "nsAccUtils.h"
#include "nsDocAccessible.h"
#include "Role.h"

#include "nsIDOMHTMLCollection.h"
#include "nsIServiceManager.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIFrame.h"
#include "nsImageFrame.h"
#include "nsImageMap.h"

using namespace mozilla::a11y;





nsHTMLImageMapAccessible::
  nsHTMLImageMapAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsHTMLImageAccessibleWrap(aContent, aDoc)
{
  mFlags |= eImageMapAccessible;
}




NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLImageMapAccessible, nsHTMLImageAccessible)




role
nsHTMLImageMapAccessible::NativeRole()
{
  return roles::IMAGE_MAP;
}




PRUint32
nsHTMLImageMapAccessible::AnchorCount()
{
  return GetChildCount();
}

nsAccessible*
nsHTMLImageMapAccessible::AnchorAt(PRUint32 aAnchorIndex)
{
  return GetChildAt(aAnchorIndex);
}

already_AddRefed<nsIURI>
nsHTMLImageMapAccessible::AnchorURIAt(PRUint32 aAnchorIndex)
{
  nsAccessible* area = GetChildAt(aAnchorIndex);
  if (!area)
    return nsnull;

  nsIContent* linkContent = area->GetContent();
  return linkContent ? linkContent->GetHrefURI() : nsnull;
}




void
nsHTMLImageMapAccessible::UpdateChildAreas(bool aDoFireEvents)
{
  nsImageFrame* imageFrame = do_QueryFrame(mContent->GetPrimaryFrame());

  
  nsImageMap* imageMapObj = imageFrame->GetExistingImageMap();
  if (!imageMapObj)
    return;

  bool doReorderEvent = false;

  
  for (PRInt32 childIdx = mChildren.Length() - 1; childIdx >= 0; childIdx--) {
    nsAccessible* area = mChildren.ElementAt(childIdx);
    if (area->GetContent()->GetPrimaryFrame())
      continue;

    if (aDoFireEvents) {
      nsRefPtr<AccEvent> event = new AccHideEvent(area, area->GetContent());
      mDoc->FireDelayedAccessibleEvent(event);
      doReorderEvent = true;
    }

    RemoveChild(area);
  }

  
  PRUint32 areaElmCount = imageMapObj->AreaCount();
  for (PRUint32 idx = 0; idx < areaElmCount; idx++) {
    nsIContent* areaContent = imageMapObj->GetAreaAt(idx);

    nsAccessible* area = mChildren.SafeElementAt(idx);
    if (!area || area->GetContent() != areaContent) {
      nsRefPtr<nsAccessible> area = new nsHTMLAreaAccessible(areaContent, mDoc);
      if (!mDoc->BindToDocument(area, nsAccUtils::GetRoleMapEntry(areaContent)))
        break;

      if (!InsertChildAt(idx, area)) {
        mDoc->UnbindFromDocument(area);
        break;
      }

      if (aDoFireEvents) {
        nsRefPtr<AccEvent> event = new AccShowEvent(area, areaContent);
        mDoc->FireDelayedAccessibleEvent(event);
        doReorderEvent = true;
      }
    }
  }

  
  if (doReorderEvent) {
    nsRefPtr<AccEvent> reorderEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_REORDER, mContent,
                   eAutoDetect, AccEvent::eCoalesceFromSameSubtree);
    mDoc->FireDelayedAccessibleEvent(reorderEvent);
  }
}




void
nsHTMLImageMapAccessible::CacheChildren()
{
  UpdateChildAreas(false);
}






nsHTMLAreaAccessible::
  nsHTMLAreaAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsHTMLLinkAccessible(aContent, aDoc)
{
}




nsresult
nsHTMLAreaAccessible::GetNameInternal(nsAString & aName)
{
  nsresult rv = nsAccessible::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  if (!mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::alt, aName))
    return GetValue(aName);

  return NS_OK;
}

void
nsHTMLAreaAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();

  
  nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(mContent));
  if (area) 
    area->GetShape(aDescription);
}

NS_IMETHODIMP
nsHTMLAreaAccessible::GetBounds(PRInt32 *aX, PRInt32 *aY,
                                PRInt32 *aWidth, PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER(aX);
  *aX = 0;
  NS_ENSURE_ARG_POINTER(aY);
  *aY = 0;
  NS_ENSURE_ARG_POINTER(aWidth);
  *aWidth = 0;
  NS_ENSURE_ARG_POINTER(aHeight);
  *aHeight = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsPresContext *presContext = GetPresContext();
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);
  nsImageFrame *imageFrame = do_QueryFrame(frame);

  nsImageMap* map = imageFrame->GetImageMap();
  NS_ENSURE_TRUE(map, NS_ERROR_FAILURE);

  nsRect rect;
  nsresult rv = map->GetBoundsForAreaContent(mContent, rect);
  NS_ENSURE_SUCCESS(rv, rv);

  *aX = presContext->AppUnitsToDevPixels(rect.x);
  *aY = presContext->AppUnitsToDevPixels(rect.y);

  
  
  *aWidth  = presContext->AppUnitsToDevPixels(rect.width - rect.x);
  *aHeight = presContext->AppUnitsToDevPixels(rect.height - rect.y);

  
  nsIntRect orgRectPixels = frame->GetScreenRectExternal();
  *aX += orgRectPixels.x;
  *aY += orgRectPixels.y;

  return NS_OK;
}




bool
nsHTMLAreaAccessible::IsPrimaryForNode() const
{
  
  
  return false;
}




PRUint64
nsHTMLAreaAccessible::NativeState()
{
  
  if (mRoleMapEntry &&
      mRoleMapEntry->role != roles::NOTHING &&
      mRoleMapEntry->role != roles::LINK) {
    return nsAccessible::NativeState();
  }

  return nsHTMLLinkAccessible::NativeState();
}

nsAccessible*
nsHTMLAreaAccessible::ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   EWhichChildAtPoint aWhichChild)
{
  
  return this;
}




PRUint32
nsHTMLAreaAccessible::StartOffset()
{
  
  
  
  
  
  return IndexInParent();
}

PRUint32
nsHTMLAreaAccessible::EndOffset()
{
  return IndexInParent() + 1;
}




void
nsHTMLAreaAccessible::CacheChildren()
{
  
}
