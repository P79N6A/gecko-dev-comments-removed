





































#include "nsHTMLWin32ObjectAccessible.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





nsHTMLWin32ObjectOwnerAccessible::
  nsHTMLWin32ObjectOwnerAccessible(nsIContent* aContent,
                                   nsDocAccessible* aDoc, void* aHwnd) :
  nsAccessibleWrap(aContent, aDoc), mHwnd(aHwnd)
{
  
  mNativeAccessible = new nsHTMLWin32ObjectAccessible(mHwnd);
}




void
nsHTMLWin32ObjectOwnerAccessible::Shutdown()
{
  nsAccessibleWrap::Shutdown();
  mNativeAccessible = nsnull;
}




role
nsHTMLWin32ObjectOwnerAccessible::NativeRole()
{
  return roles::EMBEDDED_OBJECT;
}

PRUint64
nsHTMLWin32ObjectOwnerAccessible::NativeState()
{
  
  
  return mHwnd ? nsAccessibleWrap::NativeState() : states::UNAVAILABLE;
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

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLWin32ObjectAccessible, nsAccessible)

NS_IMETHODIMP 
nsHTMLWin32ObjectAccessible::GetNativeInterface(void** aNativeAccessible)
{
  if (mHwnd) {
    ::AccessibleObjectFromWindow(static_cast<HWND>(mHwnd),
                                 OBJID_WINDOW, IID_IAccessible,
                                 aNativeAccessible);
  }
  return NS_OK;
}

