





#include "nsDOMWindowList.h"


#include "nsCOMPtr.h"


#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWebNavigation.h"

nsDOMWindowList::nsDOMWindowList(nsIDocShell *aDocShell)
{
  SetDocShell(aDocShell);
}

nsDOMWindowList::~nsDOMWindowList()
{
}

NS_IMPL_ADDREF(nsDOMWindowList)
NS_IMPL_RELEASE(nsDOMWindowList)

NS_INTERFACE_MAP_BEGIN(nsDOMWindowList)
   NS_INTERFACE_MAP_ENTRY(nsIDOMWindowCollection)
   NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsDOMWindowList::SetDocShell(nsIDocShell* aDocShell)
{
  nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
  mDocShellNode = docShellAsNode; 

  return NS_OK;
}

void
nsDOMWindowList::EnsureFresh()
{
  nsCOMPtr<nsIWebNavigation> shellAsNav = do_QueryInterface(mDocShellNode);

  if (shellAsNav) {
    nsCOMPtr<nsIDOMDocument> domdoc;
    shellAsNav->GetDocument(getter_AddRefs(domdoc));

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domdoc);

    if (doc) {
      doc->FlushPendingNotifications(Flush_ContentAndNotify);
    }
  }
}

uint32_t
nsDOMWindowList::GetLength()
{
  EnsureFresh();

  NS_ENSURE_TRUE(mDocShellNode, 0);

  int32_t length;
  nsresult rv = mDocShellNode->GetChildCount(&length);
  NS_ENSURE_SUCCESS(rv, 0);

  return uint32_t(length);
}

NS_IMETHODIMP 
nsDOMWindowList::GetLength(uint32_t* aLength)
{
  *aLength = GetLength();
  return NS_OK;
}

NS_IMETHODIMP 
nsDOMWindowList::Item(uint32_t aIndex, nsIDOMWindow** aReturn)
{
  nsCOMPtr<nsIDocShellTreeItem> item;

  *aReturn = nullptr;

  EnsureFresh();

  if (mDocShellNode) {
    mDocShellNode->GetChildAt(aIndex, getter_AddRefs(item));

    nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(item));
    NS_ASSERTION(!item || (item && globalObject),
                 "Couldn't get to the globalObject");

    if (globalObject) {
      CallQueryInterface(globalObject, aReturn);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsDOMWindowList::NamedItem(const nsAString& aName, nsIDOMWindow** aReturn)
{
  nsCOMPtr<nsIDocShellTreeItem> item;

  *aReturn = nullptr;

  EnsureFresh();

  if (mDocShellNode) {
    mDocShellNode->FindChildWithName(PromiseFlatString(aName).get(),
                                     false, false, nullptr,
                                     nullptr, getter_AddRefs(item));

    nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(item));
    if (globalObject) {
      CallQueryInterface(globalObject.get(), aReturn);
    }
  }

  return NS_OK;
}
