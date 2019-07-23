







































#include "nsMaiInterfaceComponent.h"
#include "nsAccessibleWrap.h"
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
    NS_ENSURE_TRUE(accWrap, nsnull);

    
    if (aCoordType == ATK_XY_WINDOW) {
        
    }

    nsCOMPtr<nsIAccessible> pointAcc;
    nsresult rv = accWrap->GetChildAtPoint(aAccX, aAccY, getter_AddRefs(pointAcc));
    if (NS_FAILED(rv))
        return nsnull;

    nsIAccessible *tmpAcc = pointAcc;
    nsAccessibleWrap *tmpAccWrap =
        NS_STATIC_CAST(nsAccessibleWrap *, tmpAcc);
    AtkObject *atkObj = tmpAccWrap->GetAtkObject();
    if (!atkObj)
        return nsnull;
    g_object_ref(atkObj);
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
      
      nsCOMPtr<nsIDOMNode> domNode;
      accWrap->GetDOMNode(getter_AddRefs(domNode));
      nsCOMPtr<nsIDocShellTreeItem> treeItem = nsAccessNode::GetDocShellTreeItemFor(domNode);
      nsCOMPtr<nsIDocShellTreeItem> rootTreeItem;
      treeItem->GetRootTreeItem(getter_AddRefs(rootTreeItem));
      nsCOMPtr<nsIDOMDocument> domDoc = do_GetInterface(rootTreeItem);
      nsCOMPtr<nsIDOMDocumentView> docView(do_QueryInterface(domDoc));
      if (!docView) {
        return;
      }

      nsCOMPtr<nsIDOMAbstractView> abstractView;
      docView->GetDefaultView(getter_AddRefs(abstractView));
      nsCOMPtr<nsIDOMWindowInternal> windowInter(do_QueryInterface(abstractView));
      if (!windowInter) {
        return;
      }

      PRInt32 screenX, screenY;
      if (NS_FAILED(windowInter->GetScreenX(&screenX)) ||
          NS_FAILED(windowInter->GetScreenY(&screenY))) {
        return;
      }
      nsAccX -= screenX;
      nsAccY -= screenY;
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
    NS_ENSURE_TRUE(accWrap, FALSE);

    nsresult rv = accWrap->TakeFocus();
    return (NS_FAILED(rv)) ? FALSE : TRUE;
}
