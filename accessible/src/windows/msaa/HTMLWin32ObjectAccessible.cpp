




#include "HTMLWin32ObjectAccessible.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





HTMLWin32ObjectOwnerAccessible::
  HTMLWin32ObjectOwnerAccessible(nsIContent* aContent,
                                 DocAccessible* aDoc, void* aHwnd) :
  AccessibleWrap(aContent, aDoc), mHwnd(aHwnd)
{
  
  if (mHwnd)
    mNativeAccessible = new HTMLWin32ObjectAccessible(mHwnd);
}




void
HTMLWin32ObjectOwnerAccessible::Shutdown()
{
  AccessibleWrap::Shutdown();
  mNativeAccessible = nullptr;
}




role
HTMLWin32ObjectOwnerAccessible::NativeRole()
{
  return roles::EMBEDDED_OBJECT;
}

bool
HTMLWin32ObjectOwnerAccessible::NativelyUnavailable() const
{
  
  
  return !mHwnd;
}




void
HTMLWin32ObjectOwnerAccessible::CacheChildren()
{
  if (mNativeAccessible)
    AppendChild(mNativeAccessible);
}






HTMLWin32ObjectAccessible::HTMLWin32ObjectAccessible(void* aHwnd) :
  DummyAccessible()
{
  mHwnd = aHwnd;
  if (mHwnd) {
    
    
    
    
    HWND childWnd = ::GetWindow((HWND)aHwnd, GW_CHILD);
    if (childWnd) {
      mHwnd = childWnd;
    }
  }
}

NS_IMETHODIMP 
HTMLWin32ObjectAccessible::GetNativeInterface(void** aNativeAccessible)
{
  if (mHwnd) {
    ::AccessibleObjectFromWindow(static_cast<HWND>(mHwnd),
                                 OBJID_WINDOW, IID_IAccessible,
                                 aNativeAccessible);
  }
  return NS_OK;
}

