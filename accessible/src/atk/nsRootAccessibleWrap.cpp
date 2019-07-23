








































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
        root = nsnull;
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

nsNativeRootAccessibleWrap::nsNativeRootAccessibleWrap(AtkObject *aAccessible):
    nsRootAccessible(nsnull, nsnull)
{
    g_object_ref(aAccessible);
    nsAccessibleWrap::mAtkObject = aAccessible;
}
