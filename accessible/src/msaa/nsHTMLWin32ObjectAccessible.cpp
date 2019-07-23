





































#include "nsHTMLWin32ObjectAccessible.h"
#include "nsAccessibleWrap.h"


nsHTMLWin32ObjectOwnerAccessible::nsHTMLWin32ObjectOwnerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell, void* aHwnd):
nsAccessibleWrap(aNode, aShell)
{
  mHwnd = aHwnd;
}

NS_IMETHODIMP nsHTMLWin32ObjectOwnerAccessible::Shutdown()
{
  nsAccessibleWrap::Shutdown();
  mNativeAccessible = nsnull;
  return NS_OK;
}




NS_IMETHODIMP nsHTMLWin32ObjectOwnerAccessible::GetFirstChild(nsIAccessible **aFirstChild)
{
  *aFirstChild = mNativeAccessible;
  if (!mNativeAccessible) {
    if (!mHwnd) {
      return NS_OK;
    }
    mNativeAccessible = new nsHTMLWin32ObjectAccessible(mHwnd) ;
    SetFirstChild(mNativeAccessible);
    *aFirstChild = mNativeAccessible;
  }
  NS_IF_ADDREF(*aFirstChild);
  return NS_OK;
}

NS_IMETHODIMP nsHTMLWin32ObjectOwnerAccessible::GetLastChild(nsIAccessible **aLastChild)
{
  return GetFirstChild(aLastChild);
}

NS_IMETHODIMP nsHTMLWin32ObjectOwnerAccessible::GetChildCount(PRInt32 *aChildCount)
{
  nsCOMPtr<nsIAccessible> onlyChild;
  GetFirstChild(getter_AddRefs(onlyChild));
  *aChildCount = onlyChild ? 1 : 0;
  return NS_OK;
}

nsHTMLWin32ObjectAccessible::nsHTMLWin32ObjectAccessible(void* aHwnd):
nsLeafAccessible(nsnull, nsnull)
{
  mHwnd = aHwnd;
  if (mHwnd) {
    
    
    
    
    HWND childWnd = ::GetWindow((HWND)aHwnd, GW_CHILD);
    if (childWnd) {
      mHwnd = childWnd;
    }
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(nsHTMLWin32ObjectAccessible, nsAccessible, nsIAccessibleWin32Object)

NS_IMETHODIMP nsHTMLWin32ObjectAccessible::GetHwnd(void **aHwnd) 
{
  *aHwnd = mHwnd;
  return NS_OK;
}

