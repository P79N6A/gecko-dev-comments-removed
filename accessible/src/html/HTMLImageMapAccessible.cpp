




#include "HTMLImageMapAccessible.h"

#include "ARIAMap.h"
#include "nsAccUtils.h"
#include "DocAccessible-inl.h"
#include "Role.h"

#include "nsIDOMHTMLCollection.h"
#include "nsIServiceManager.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIFrame.h"
#include "nsImageFrame.h"
#include "nsImageMap.h"
#include "nsIURI.h"

using namespace mozilla::a11y;





HTMLImageMapAccessible::
  HTMLImageMapAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  ImageAccessibleWrap(aContent, aDoc)
{
  mType = eImageMapType;
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
  nsRefPtr<AccReorderEvent> reorderEvent = new AccReorderEvent(this);

  
  for (int32_t childIdx = mChildren.Length() - 1; childIdx >= 0; childIdx--) {
    Accessible* area = mChildren.ElementAt(childIdx);
    if (area->GetContent()->GetPrimaryFrame())
      continue;

    if (aDoFireEvents) {
      nsRefPtr<AccHideEvent> event = new AccHideEvent(area, area->GetContent());
      mDoc->FireDelayedEvent(event);
      reorderEvent->AddSubMutationEvent(event);
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
      mDoc->BindToDocument(area, aria::GetRoleMap(areaContent));

      if (!InsertChildAt(idx, area)) {
        mDoc->UnbindFromDocument(area);
        break;
      }

      if (aDoFireEvents) {
        nsRefPtr<AccShowEvent> event = new AccShowEvent(area, areaContent);
        mDoc->FireDelayedEvent(event);
        reorderEvent->AddSubMutationEvent(event);
        doReorderEvent = true;
      }
    }
  }

  
  if (doReorderEvent)
    mDoc->FireDelayedEvent(reorderEvent);
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
  
  
  mStateFlags |= eNotNodeMapEntry;
}




ENameValueFlag
HTMLAreaAccessible::NativeName(nsString& aName)
{
  ENameValueFlag nameFlag = Accessible::NativeName(aName);
  if (!aName.IsEmpty())
    return nameFlag;

  if (!mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::alt, aName))
    GetValue(aName);

  return eNameOK;
}

void
HTMLAreaAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();

  
  nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(mContent));
  if (area)
    area->GetShape(aDescription);
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
