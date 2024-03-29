





#include "InterfaceInitFuncs.h"

#include "AccessibleWrap.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsMai.h"
#include "mozilla/Likely.h"

using namespace mozilla::a11y;

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

  accWrap->TakeFocus();
  return TRUE;
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

  nsIntRect screenRect = aAccWrap->Bounds();
  if (screenRect.IsEmpty())
    return;

  if (aCoordType == ATK_XY_WINDOW) {
    nsIntPoint winCoords =
      nsCoreUtils::GetScreenCoordsForWindow(aAccWrap->GetNode());
    screenRect.x -= winCoords.x;
    screenRect.y -= winCoords.y;
  }

  *aX = screenRect.x;
  *aY = screenRect.y;
  *aWidth = screenRect.width;
  *aHeight = screenRect.height;
}

void
componentInterfaceInitCB(AtkComponentIface* aIface)
{
  NS_ASSERTION(aIface, "Invalid Interface");
  if(MOZ_UNLIKELY(!aIface))
    return;

  



  aIface->ref_accessible_at_point = refAccessibleAtPointCB;
  aIface->get_extents = getExtentsCB;
  aIface->grab_focus = grabFocusCB;
}
