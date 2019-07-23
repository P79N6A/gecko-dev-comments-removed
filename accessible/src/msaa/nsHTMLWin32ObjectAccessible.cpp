





































#include "nsHTMLWin32ObjectAccessible.h"
#include "nsAccessibleWrap.h"





nsHTMLWin32ObjectOwnerAccessible::
  nsHTMLWin32ObjectOwnerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell,
                                   void* aHwnd) :
  nsAccessibleWrap(aNode, aShell)
{
  mHwnd = aHwnd;
}




nsresult
nsHTMLWin32ObjectOwnerAccessible::Shutdown()
{
  nsAccessibleWrap::Shutdown();
  mNativeAccessible = nsnull;
  return NS_OK;
}




NS_IMETHODIMP
nsHTMLWin32ObjectOwnerAccessible::GetFirstChild(nsIAccessible **aFirstChild)
{
  NS_ENSURE_ARG_POINTER(aFirstChild);
  *aFirstChild = nsnull;

  
  if (!mNativeAccessible) {
    if (!mHwnd)
      return NS_OK;

    mNativeAccessible = new nsHTMLWin32ObjectAccessible(mHwnd);
    NS_ENSURE_TRUE(mNativeAccessible, NS_ERROR_OUT_OF_MEMORY);

    SetFirstChild(mNativeAccessible);
  }

  *aFirstChild = mNativeAccessible;
  NS_IF_ADDREF(*aFirstChild);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLWin32ObjectOwnerAccessible::GetLastChild(nsIAccessible **aLastChild)
{
  return GetFirstChild(aLastChild);
}

NS_IMETHODIMP
nsHTMLWin32ObjectOwnerAccessible::GetChildCount(PRInt32 *aChildCount)
{
  NS_ENSURE_ARG_POINTER(aChildCount);

  nsCOMPtr<nsIAccessible> onlyChild;
  GetFirstChild(getter_AddRefs(onlyChild));
  *aChildCount = onlyChild ? 1 : 0;
  return NS_OK;
}





nsresult
nsHTMLWin32ObjectOwnerAccessible::GetRoleInternal(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_EMBEDDED_OBJECT;
  return NS_OK;
}

nsresult
nsHTMLWin32ObjectOwnerAccessible::GetStateInternal(PRUint32 *aState,
                                                   PRUint32 *aExtraState)
{
  nsresult rv = nsAccessibleWrap::GetStateInternal(aState, aExtraState);
  if (rv == NS_OK_DEFUNCT_OBJECT)
    return rv;

  
  
  if (!mHwnd)
    *aState = nsIAccessibleStates::STATE_UNAVAILABLE;

  return rv;
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

