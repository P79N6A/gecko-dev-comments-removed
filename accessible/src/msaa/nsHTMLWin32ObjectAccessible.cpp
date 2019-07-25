





































#include "nsHTMLWin32ObjectAccessible.h"





nsHTMLWin32ObjectOwnerAccessible::
  nsHTMLWin32ObjectOwnerAccessible(nsIContent *aContent,
                                   nsIWeakReference *aShell, void *aHwnd) :
  nsAccessibleWrap(aContent, aShell), mHwnd(aHwnd)
{
  
  mNativeAccessible = new nsHTMLWin32ObjectAccessible(mHwnd);
}




void
nsHTMLWin32ObjectOwnerAccessible::Shutdown()
{
  nsAccessibleWrap::Shutdown();
  mNativeAccessible = nsnull;
}




PRUint32
nsHTMLWin32ObjectOwnerAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_EMBEDDED_OBJECT;
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
  if (mNativeAccessible)
    AppendChild(mNativeAccessible);
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

