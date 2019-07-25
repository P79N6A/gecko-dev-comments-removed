






































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
  nsHTMLImageMapAccessible(nsIContent* aContent, nsDocAccessible* aDoc,
                           nsIDOMHTMLMapElement* aMapElm) :
  nsHTMLImageAccessibleWrap(aContent, aDoc), mMapElement(aMapElm)
{
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
nsHTMLImageMapAccessible::CacheChildren()
{
  if (!mMapElement)
    return;

  nsCOMPtr<nsIDOMHTMLCollection> mapAreas;
  mMapElement->GetAreas(getter_AddRefs(mapAreas));
  if (!mapAreas)
    return;

  nsDocAccessible* document = Document();

  PRUint32 areaCount = 0;
  mapAreas->GetLength(&areaCount);

  for (PRUint32 areaIdx = 0; areaIdx < areaCount; areaIdx++) {
    nsCOMPtr<nsIDOMNode> areaNode;
    mapAreas->Item(areaIdx, getter_AddRefs(areaNode));
    if (!areaNode)
      return;

    nsCOMPtr<nsIContent> areaContent(do_QueryInterface(areaNode));
    nsRefPtr<nsAccessible> area =
      new nsHTMLAreaAccessible(areaContent, mDoc);

    if (!document->BindToDocument(area, nsAccUtils::GetRoleMapEntry(areaContent)) ||
        !AppendChild(area)) {
      return;
    }
  }
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
