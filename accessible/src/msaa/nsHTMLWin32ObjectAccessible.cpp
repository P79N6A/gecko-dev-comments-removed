





































#include "nsHTMLWin32ObjectAccessible.h"
#include "nsAccessibleWrap.h"





nsHTMLWin32ObjectOwnerAccessible::
  nsHTMLWin32ObjectOwnerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell,
                                   void* aHwnd) :
  nsAccessibleWrap(aNode, aShell)
{
  mHwnd = aHwnd;

  
  mNativeAccessible = new nsHTMLWin32ObjectAccessible(mHwnd);
}




nsresult
nsHTMLWin32ObjectOwnerAccessible::Shutdown()
{
  nsAccessibleWrap::Shutdown();
  mNativeAccessible = nsnull;
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




void
nsHTMLWin32ObjectOwnerAccessible::CacheChildren()
{
  if (mNativeAccessible) {
    mChildren.AppendObject(mNativeAccessible);

    nsRefPtr<nsAccessible> nativeAcc =
      nsAccUtils::QueryObject<nsAccessible>(mNativeAccessible);
    nativeAcc->SetParent(this);
  }
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

