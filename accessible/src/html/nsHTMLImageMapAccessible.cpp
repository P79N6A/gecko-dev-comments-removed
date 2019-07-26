




#include "nsHTMLImageMapAccessible.h"

#include "nsAccUtils.h"
#include "nsARIAMap.h"
#include "DocAccessible.h"
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
  nsHTMLImageMapAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  ImageAccessibleWrap(aContent, aDoc)
{
  mFlags |= eImageMapAccessible;
}




NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLImageMapAccessible, ImageAccessible)




role
nsHTMLImageMapAccessible::NativeRole()
{
  return roles::IMAGE_MAP;
}




PRUint32
nsHTMLImageMapAccessible::AnchorCount()
{
  return ChildCount();
}

Accessible*
nsHTMLImageMapAccessible::AnchorAt(PRUint32 aAnchorIndex)
{
  return GetChildAt(aAnchorIndex);
}

already_AddRefed<nsIURI>
nsHTMLImageMapAccessible::AnchorURIAt(PRUint32 aAnchorIndex)
{
  Accessible* area = GetChildAt(aAnchorIndex);
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
    Accessible* area = mChildren.ElementAt(childIdx);
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

    Accessible* area = mChildren.SafeElementAt(idx);
    if (!area || area->GetContent() != areaContent) {
      nsRefPtr<Accessible> area = new nsHTMLAreaAccessible(areaContent, mDoc);
      if (!mDoc->BindToDocument(area, aria::GetRoleMap(areaContent)))
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
  nsHTMLAreaAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  nsHTMLLinkAccessible(aContent, aDoc)
{
}




nsresult
nsHTMLAreaAccessible::GetNameInternal(nsAString & aName)
{
  nsresult rv = Accessible::GetNameInternal(aName);
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




bool
nsHTMLAreaAccessible::IsPrimaryForNode() const
{
  
  
  return false;
}




Accessible*
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

void
nsHTMLAreaAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame)
{
  nsIFrame* frame = GetFrame();
  if (!frame)
    return;

  nsImageFrame* imageFrame = do_QueryFrame(frame);
  nsImageMap* map = imageFrame->GetImageMap();

  nsresult rv = map->GetBoundsForAreaContent(mContent, aBounds);
  if (NS_FAILED(rv))
    return;

  
  
  aBounds.width -= aBounds.x;
  aBounds.height -= aBounds.y;

  *aBoundingFrame = frame;
}
