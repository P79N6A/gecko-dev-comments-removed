





#include "InterfaceInitFuncs.h"

#include "AccessibleWrap.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsMai.h"

extern "C" {

static AtkObject*
refAccessibleAtPointCB(AtkComponent* aComponent, gint aAccX, gint aAccY,
                       AtkCoordType aCoordType)
{
  return refAccessibleAtPointHelper(GetAccessibleWrap(ATK_OBJECT(aComponent)),
                                    aAccX, aAccY, aCoordType);
}

static void
getExtentsCB(AtkComponent* aComponent, gint* aX, gint* aY,
             gint* aWidth, gint* aHeight, AtkCoordType aCoordType)
{
  getExtentsHelper(GetAccessibleWrap(ATK_OBJECT(aComponent)),
                   aX, aY, aWidth, aHeight, aCoordType);
}

static gboolean
grabFocusCB(AtkComponent* aComponent)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aComponent));
  if (!accWrap)
    return FALSE;

  nsresult rv = accWrap->TakeFocus();
  return (NS_FAILED(rv)) ? FALSE : TRUE;
}
}

AtkObject*
refAccessibleAtPointHelper(AccessibleWrap* aAccWrap, gint aX, gint aY,
                           AtkCoordType aCoordType)
{
  if (!aAccWrap || aAccWrap->IsDefunct() || nsAccUtils::MustPrune(aAccWrap))
    return nullptr;

  
  if (aCoordType == ATK_XY_WINDOW) {
    nsIntPoint winCoords =
      nsCoreUtils::GetScreenCoordsForWindow(aAccWrap->GetNode());
    aX += winCoords.x;
    aY += winCoords.y;
  }

  Accessible* accAtPoint = aAccWrap->ChildAtPoint(aX, aY,
                                                  Accessible::eDirectChild);
  if (!accAtPoint)
    return nullptr;

  AtkObject* atkObj = AccessibleWrap::GetAtkObject(accAtPoint);
  if (atkObj)
    g_object_ref(atkObj);
  return atkObj;
}

void
getExtentsHelper(AccessibleWrap* aAccWrap,
                 gint* aX, gint* aY, gint* aWidth, gint* aHeight,
                 AtkCoordType aCoordType)
{
  *aX = *aY = *aWidth = *aHeight = 0;

  if (!aAccWrap || aAccWrap->IsDefunct())
    return;

  int32_t x = 0, y = 0, width = 0, height = 0;
  
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

void
componentInterfaceInitCB(AtkComponentIface* aIface)
{
  NS_ASSERTION(aIface, "Invalid Interface");
  if(NS_UNLIKELY(!aIface))
    return;

  



  aIface->ref_accessible_at_point = refAccessibleAtPointCB;
  aIface->get_extents = getExtentsCB;
  aIface->grab_focus = grabFocusCB;
}
