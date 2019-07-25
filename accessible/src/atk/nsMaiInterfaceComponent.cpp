






































#include "nsMaiInterfaceComponent.h"

#include "nsAccessibleWrap.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"

#include "nsIDOMDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDocShellTreeItem.h"
#include "nsIInterfaceRequestorUtils.h"

void
componentInterfaceInitCB(AtkComponentIface *aIface)
{
    NS_ASSERTION(aIface, "Invalid Interface");
    if(!aIface)
        return;

    



    aIface->ref_accessible_at_point = refAccessibleAtPointCB;
    aIface->get_extents = getExtentsCB;
    aIface->grab_focus = grabFocusCB;
}

AtkObject*
refAccessibleAtPointCB(AtkComponent* aComponent, gint aAccX, gint aAccY,
                       AtkCoordType aCoordType)
{
  return refAccessibleAtPointHelper(GetAccessibleWrap(ATK_OBJECT(aComponent)),
                                    aAccX, aAccY, aCoordType);
}

void
getExtentsCB(AtkComponent* aComponent, gint* aX, gint* aY,
             gint* aWidth, gint* aHeight, AtkCoordType aCoordType)
{
  getExtentsHelper(GetAccessibleWrap(ATK_OBJECT(aComponent)),
                   aX, aY, aWidth, aHeight, aCoordType);
}

gboolean
grabFocusCB(AtkComponent* aComponent)
{
  nsAccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aComponent));
  if (!accWrap)
    return FALSE;

  nsresult rv = accWrap->TakeFocus();
  return (NS_FAILED(rv)) ? FALSE : TRUE;
}

AtkObject*
refAccessibleAtPointHelper(nsAccessibleWrap* aAccWrap, gint aX, gint aY,
                           AtkCoordType aCoordType)
{
  if (!aAccWrap || aAccWrap->IsDefunct() || nsAccUtils::MustPrune(aAccWrap))
    return nsnull;

  
  if (aCoordType == ATK_XY_WINDOW) {
    nsIntPoint winCoords =
      nsCoreUtils::GetScreenCoordsForWindow(aAccWrap->GetNode());
    aX += winCoords.x;
    aY += winCoords.y;
  }

  nsAccessible* accAtPoint = aAccWrap->ChildAtPoint(aX, aY,
                                                    nsAccessible::eDirectChild);
  if (!accAtPoint)
    return nsnull;

  AtkObject* atkObj = nsAccessibleWrap::GetAtkObject(accAtPoint);
  if (atkObj)
    g_object_ref(atkObj);
  return atkObj;
}

void
getExtentsHelper(nsAccessibleWrap* aAccWrap,
                 gint* aX, gint* aY, gint* aWidth, gint* aHeight,
                 AtkCoordType aCoordType)
{
  *aX = *aY = *aWidth = *aHeight = 0;

  if (!aAccWrap || aAccWrap->IsDefunct())
    return;

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  
  nsresult rv = aAccWrap->GetBounds(&x, &y, &width, &height);
  if (NS_FAILED(rv))
    return;

  if (aCoordType == ATK_XY_WINDOW) {
    nsIntPoint winCoords =
      nsCoreUtils::GetScreenCoordsForWindow(aAccWrap->GetNode());
    x -= winCoords.x;
    y -= winCoords.y;
  }

  *aX = x;
  *aY = y;
  *aWidth = width;
  *aHeight = height;
}
