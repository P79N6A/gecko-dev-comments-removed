







































#include "nsMaiInterfaceComponent.h"

#include "nsAccessibleWrap.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"

#include "nsIDOMDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"
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

AtkObject *
refAccessibleAtPointCB(AtkComponent *aComponent,
                       gint aAccX, gint aAccY,
                       AtkCoordType aCoordType)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aComponent));
    if (!accWrap || nsAccUtils::MustPrune(accWrap))
        return nsnull;

    
    if (aCoordType == ATK_XY_WINDOW) {
        nsIntPoint winCoords =
          nsCoreUtils::GetScreenCoordsForWindow(accWrap->GetNode());
        aAccX += winCoords.x;
        aAccY += winCoords.y;
    }

    nsCOMPtr<nsIAccessible> pointAcc;
    accWrap->GetChildAtPoint(aAccX, aAccY, getter_AddRefs(pointAcc));
    if (!pointAcc) {
        return nsnull;
    }

    AtkObject *atkObj = nsAccessibleWrap::GetAtkObject(pointAcc);
    if (atkObj) {
        g_object_ref(atkObj);
    }
    return atkObj;
}

void
getExtentsCB(AtkComponent *aComponent,
             gint *aAccX,
             gint *aAccY,
             gint *aAccWidth,
             gint *aAccHeight,
             AtkCoordType aCoordType)
{
    *aAccX = *aAccY = *aAccWidth = *aAccHeight = 0;

    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aComponent));
    if (!accWrap)
        return;

    PRInt32 nsAccX, nsAccY, nsAccWidth, nsAccHeight;
    
    nsresult rv = accWrap->GetBounds(&nsAccX, &nsAccY,
                                     &nsAccWidth, &nsAccHeight);
    if (NS_FAILED(rv))
        return;
    if (aCoordType == ATK_XY_WINDOW) {
        nsIntPoint winCoords =
          nsCoreUtils::GetScreenCoordsForWindow(accWrap->GetNode());
        nsAccX -= winCoords.x;
        nsAccY -= winCoords.y;
    }

    *aAccX = nsAccX;
    *aAccY = nsAccY;
    *aAccWidth = nsAccWidth;
    *aAccHeight = nsAccHeight;
}

gboolean
grabFocusCB(AtkComponent *aComponent)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aComponent));
    if (!accWrap)
        return FALSE;

    nsresult rv = accWrap->TakeFocus();
    return (NS_FAILED(rv)) ? FALSE : TRUE;
}
