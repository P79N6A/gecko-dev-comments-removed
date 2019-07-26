




#include "HTMLImageMapAccessible.h"

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





HTMLImageMapAccessible::
  HTMLImageMapAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  ImageAccessibleWrap(aContent, aDoc)
{
  mFlags |= eImageMapAccessible;
}




NS_IMPL_ISUPPORTS_INHERITED0(HTMLImageMapAccessible, ImageAccessible)




role
HTMLImageMapAccessible::NativeRole()
{
  return roles::IMAGE_MAP;
}




uint32_t
HTMLImageMapAccessible::AnchorCount()
{
  return ChildCount();
}

Accessible*
HTMLImageMapAccessible::AnchorAt(uint32_t aAnchorIndex)
{
  return GetChildAt(aAnchorIndex);
}

already_AddRefed<nsIURI>
HTMLImageMapAccessible::AnchorURIAt(uint32_t aAnchorIndex)
{
  Accessible* area = GetChildAt(aAnchorIndex);
  if (!area)
    return nullptr;

  nsIContent* linkContent = area->GetContent();
  return linkContent ? linkContent->GetHrefURI() : nullptr;
}




void
HTMLImageMapAccessible::UpdateChildAreas(bool aDoFireEvents)
{
  nsImageFrame* imageFrame = do_QueryFrame(mContent->GetPrimaryFrame());

  
  nsImageMap* imageMapObj = imageFrame->GetExistingImageMap();
  if (!imageMapObj)
    return;

  bool doReorderEvent = false;

  
  for (int32_t childIdx = mChildren.Length() - 1; childIdx >= 0; childIdx--) {
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

  
  uint32_t areaElmCount = imageMapObj->AreaCount();
  for (uint32_t idx = 0; idx < areaElmCount; idx++) {
    nsIContent* areaContent = imageMapObj->GetAreaAt(idx);

    Accessible* area = mChildren.SafeElementAt(idx);
    if (!area || area->GetContent() != areaContent) {
      nsRefPtr<Accessible> area = new HTMLAreaAccessible(areaContent, mDoc);
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
HTMLImageMapAccessible::CacheChildren()
{
  UpdateChildAreas(false);
}






HTMLAreaAccessible::
  HTMLAreaAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HTMLLinkAccessible(aContent, aDoc)
{
}




nsresult
HTMLAreaAccessible::GetNameInternal(nsAString& aName)
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
HTMLAreaAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();

  
  nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(mContent));
  if (area)
    area->GetShape(aDescription);
}




bool
HTMLAreaAccessible::IsPrimaryForNode() const
{
  
  
  return false;
}




Accessible*
HTMLAreaAccessible::ChildAtPoint(int32_t aX, int32_t aY,
                                 EWhichChildAtPoint aWhichChild)
{
  
  return this;
}




uint32_t
HTMLAreaAccessible::StartOffset()
{
  
  
  
  
  
  return IndexInParent();
}

uint32_t
HTMLAreaAccessible::EndOffset()
{
  return IndexInParent() + 1;
}




void
HTMLAreaAccessible::CacheChildren()
{
  
}

void
HTMLAreaAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame)
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
