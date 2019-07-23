








































#include "nsMai.h"
#include "nsRootAccessibleWrap.h"
#include "nsAppRootAccessible.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIFocusController.h"

#ifdef MOZ_XUL
#include "nsIAccessibleTreeCache.h"
#endif

nsRootAccessibleWrap::nsRootAccessibleWrap(nsIDOMNode *aDOMNode,
                                           nsIWeakReference* aShell):
    nsRootAccessible(aDOMNode, aShell)
{
    MAI_LOG_DEBUG(("New Root Acc=%p\n", (void*)this));
}

NS_IMETHODIMP nsRootAccessibleWrap::Init()
{
    nsresult rv = nsRootAccessible::Init();
    nsAppRootAccessible *root = nsAppRootAccessible::Create();
    if (root) {
        root->AddRootAccessible(this);
    }
    return rv;
}

nsRootAccessibleWrap::~nsRootAccessibleWrap()
{
    MAI_LOG_DEBUG(("Delete Root Acc=%p\n", (void*)this));
}

NS_IMETHODIMP nsRootAccessibleWrap::Shutdown()
{
    nsAppRootAccessible *root = nsAppRootAccessible::Create();
    if (root) {
        root->RemoveRootAccessible(this);
    }
    return nsRootAccessible::Shutdown();
}

NS_IMETHODIMP nsRootAccessibleWrap::GetParent(nsIAccessible **  aParent)
{
    nsAppRootAccessible *root = nsAppRootAccessible::Create();
    nsresult rv = NS_OK;
    if (root) {
        NS_IF_ADDREF(*aParent = root);
    }
    else {
        *aParent = nsnull;
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}


nsresult nsRootAccessibleWrap::HandleEventWithTarget(nsIDOMEvent *aEvent,
                                                     nsIDOMNode  *aTargetNode)
{
    nsAutoString eventType;
    aEvent->GetType(eventType);
    nsAutoString localName;
    aTargetNode->GetLocalName(localName);

    if (eventType.EqualsLiteral("pagehide")) {
        nsRootAccessible::HandleEventWithTarget(aEvent, aTargetNode);
        return NS_OK;
    }

    nsCOMPtr<nsIAccessible> accessible;
    nsCOMPtr<nsIAccessibilityService> accService = GetAccService();
    accService->GetAccessibleFor(aTargetNode, getter_AddRefs(accessible));
    if (!accessible)
        return NS_OK;

    if (eventType.EqualsLiteral("popupshown")) {
        nsRootAccessible::HandleEventWithTarget(aEvent, aTargetNode);
        nsCOMPtr<nsIContent> content(do_QueryInterface(aTargetNode));

        
        
        
        

        
        
        
        
        if (!content->NodeInfo()->Equals(nsAccessibilityAtoms::tooltip,
                                        kNameSpaceID_XUL) &&
            !content->AttrValueIs(kNameSpaceID_None, nsAccessibilityAtoms::type,
                                 NS_LITERAL_STRING("autocomplete"), eIgnoreCase)) {
            FireAccessibleFocusEvent(accessible, aTargetNode, aEvent);
        }
        return NS_OK;
    }

    nsCOMPtr<nsPIAccessible> privAcc(do_QueryInterface(accessible));

#ifdef MOZ_XUL
  
    nsCOMPtr<nsIAccessible> treeItemAccessible;
    if (localName.EqualsLiteral("tree")) {
        nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
            do_QueryInterface(aTargetNode);
        if (multiSelect) {
            PRInt32 treeIndex = -1;
            multiSelect->GetCurrentIndex(&treeIndex);
            if (treeIndex >= 0) {
                nsCOMPtr<nsIAccessibleTreeCache> treeCache(do_QueryInterface(accessible));
                if (!treeCache || 
                    NS_FAILED(treeCache->GetCachedTreeitemAccessible(
                    treeIndex,
                    nsnull,
                    getter_AddRefs(treeItemAccessible))) ||
                    !treeItemAccessible) {
                        return NS_ERROR_OUT_OF_MEMORY;
                }
                accessible = treeItemAccessible;
            }
        }
    }
#endif

    if (eventType.EqualsLiteral("focus")) {
#ifdef MOZ_XUL
        if (treeItemAccessible) { 
            privAcc = do_QueryInterface(treeItemAccessible);
            privAcc->FireToolkitEvent(nsIAccessibleEvent::EVENT_FOCUS, 
                                      treeItemAccessible, nsnull);
            accessible = treeItemAccessible;
        }
        else
#endif 
        if (localName.EqualsIgnoreCase("radiogroup")) {
            
            PRInt32 childCount = 0;
            accessible->GetChildCount(&childCount);
            nsCOMPtr<nsIAccessible> radioAcc;
            for (PRInt32 index = 0; index < childCount; index++) {
                accessible->GetChildAt(index, getter_AddRefs(radioAcc));
                if (radioAcc) {
                    PRUint32 state = State(radioAcc);
                    if (state & (nsIAccessibleStates::STATE_CHECKED |
                        nsIAccessibleStates::STATE_SELECTED)) {
                        break;
                    }
                }
            }
            accessible = radioAcc;
            if (radioAcc) {
                privAcc->FireToolkitEvent(nsIAccessibleEvent::EVENT_FOCUS, radioAcc, nsnull);
            }
        }
        else if (!FireAccessibleFocusEvent(accessible, aTargetNode, aEvent)) {
            accessible = nsnull;  
        }
        if (accessible) {
            
            nsCOMPtr<nsIAccessibleStateChangeEvent> accEvent =
                new nsAccStateChangeEvent(accessible,
                                          nsIAccessibleStates::STATE_FOCUSED,
                                          PR_FALSE, PR_TRUE);
            return FireAccessibleEvent(accEvent);
        }
    }
    else if (eventType.EqualsLiteral("select")) {
#ifdef MOZ_XUL
        if (treeItemAccessible) { 
            
            privAcc = do_QueryInterface(treeItemAccessible);
            privAcc->FireToolkitEvent(nsIAccessibleEvent::EVENT_FOCUS, 
                                    treeItemAccessible, nsnull);
        }
        else 
#endif
        if (localName.LowerCaseEqualsLiteral("tabpanels")) {
            
            privAcc->FireToolkitEvent(nsIAccessibleEvent::EVENT_REORDER, accessible, nsnull);
        }
    } else {
      nsRootAccessible::HandleEventWithTarget(aEvent, aTargetNode);
    }

    return NS_OK;
}

nsNativeRootAccessibleWrap::nsNativeRootAccessibleWrap(AtkObject *aAccessible):
    nsRootAccessible(nsnull, nsnull)
{
    g_object_ref(aAccessible);
    nsAccessibleWrap::mAtkObject = aAccessible;
}
