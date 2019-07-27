





#ifndef nsXULWindow_h__
#define nsXULWindow_h__


#include "nsChromeTreeOwner.h"
#include "nsContentTreeOwner.h"


#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsCOMArray.h"
#include "nsRect.h"


#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIXULWindow.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIXULBrowserWindow.h"
#include "nsIWeakReference.h"
#include "nsIWidgetListener.h"
#include "nsITabParent.h"

namespace mozilla {
namespace dom {
class Element;
}
}



#define NS_XULWINDOW_IMPL_CID                         \
{ /* 8eaec2f3-ed02-4be2-8e0f-342798477298 */          \
     0x8eaec2f3,                                      \
     0xed02,                                          \
     0x4be2,                                          \
   { 0x8e, 0x0f, 0x34, 0x27, 0x98, 0x47, 0x72, 0x98 } \
}

class nsContentShellInfo;

class nsXULWindow : public nsIBaseWindow,
                    public nsIInterfaceRequestor,
                    public nsIXULWindow,
                    public nsSupportsWeakReference
{
friend class nsChromeTreeOwner;
friend class nsContentTreeOwner;

public:
   NS_DECL_THREADSAFE_ISUPPORTS

   NS_DECL_NSIINTERFACEREQUESTOR
   NS_DECL_NSIXULWINDOW
   NS_DECL_NSIBASEWINDOW

   NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULWINDOW_IMPL_CID)

   void LockUntilChromeLoad() { mLockedUntilChromeLoad = true; }
   bool IsLocked() const { return mLockedUntilChromeLoad; }
   void IgnoreXULSizeMode(bool aEnable) { mIgnoreXULSizeMode = aEnable; }

protected:
   enum persistentAttributes {
     PAD_MISC =         0x1,
     PAD_POSITION =     0x2,
     PAD_SIZE =         0x4
   };

   explicit nsXULWindow(uint32_t aChromeFlags);
   virtual ~nsXULWindow();

   NS_IMETHOD EnsureChromeTreeOwner();
   NS_IMETHOD EnsureContentTreeOwner();
   NS_IMETHOD EnsurePrimaryContentTreeOwner();
   NS_IMETHOD EnsurePrompter();
   NS_IMETHOD EnsureAuthPrompter();

   void OnChromeLoaded();
   void StaggerPosition(int32_t &aRequestedX, int32_t &aRequestedY,
                        int32_t aSpecWidth, int32_t aSpecHeight);
   bool       LoadPositionFromXUL();
   bool       LoadSizeFromXUL();
   bool       LoadMiscPersistentAttributesFromXUL();
   void       SyncAttributesToWidget();
   NS_IMETHOD SavePersistentAttributes();

   NS_IMETHOD GetWindowDOMWindow(nsIDOMWindow** aDOMWindow);
   mozilla::dom::Element* GetWindowDOMElement() const;

   
   nsresult ContentShellAdded(nsIDocShellTreeItem* aContentShell,
                                          bool aPrimary, bool aTargetable,
                                          const nsAString& aID);
   nsresult ContentShellRemoved(nsIDocShellTreeItem* aContentShell);
   NS_IMETHOD SizeShellTo(nsIDocShellTreeItem* aShellItem, int32_t aCX, 
      int32_t aCY);
   NS_IMETHOD ExitModalLoop(nsresult aStatus);
   NS_IMETHOD CreateNewChromeWindow(int32_t aChromeFlags, nsITabParent* aOpeningTab, nsIXULWindow **_retval);
   NS_IMETHOD CreateNewContentWindow(int32_t aChromeFlags, nsITabParent* aOpeningTab, nsIXULWindow **_retval);

   void       EnableParent(bool aEnable);
   bool       ConstrainToZLevel(bool aImmediate, nsWindowZ *aPlacement,
                                nsIWidget *aReqBelow, nsIWidget **aActualBelow);
   void       PlaceWindowLayersBehind(uint32_t aLowLevel, uint32_t aHighLevel,
                                      nsIXULWindow *aBehind);
   void       SetContentScrollbarVisibility(bool aVisible);
   bool       GetContentScrollbarVisibility();
   void       PersistentAttributesDirty(uint32_t aDirtyFlags);

   nsChromeTreeOwner*      mChromeTreeOwner;
   nsContentTreeOwner*     mContentTreeOwner;
   nsContentTreeOwner*     mPrimaryContentTreeOwner;
   nsCOMPtr<nsIWidget>     mWindow;
   nsCOMPtr<nsIDocShell>   mDocShell;
   nsCOMPtr<nsIDOMWindow>  mDOMWindow;
   nsCOMPtr<nsIWeakReference> mParentWindow;
   nsCOMPtr<nsIPrompt>     mPrompter;
   nsCOMPtr<nsIAuthPrompt> mAuthPrompter;
   nsCOMPtr<nsIXULBrowserWindow> mXULBrowserWindow;
   nsCOMPtr<nsIDocShellTreeItem> mPrimaryContentShell;
   nsTArray<nsContentShellInfo*> mContentShells; 
   nsresult                mModalStatus;
   bool                    mContinueModalLoop;
   bool                    mDebuting;       
   bool                    mChromeLoaded; 
   bool                    mShowAfterLoad;
   bool                    mIntrinsicallySized; 
   bool                    mCenterAfterLoad;
   bool                    mIsHiddenWindow;
   bool                    mLockedUntilChromeLoad;
   bool                    mIgnoreXULSize;
   bool                    mIgnoreXULPosition;
   bool                    mChromeFlagsFrozen;
   bool                    mIgnoreXULSizeMode;
   
   
   bool                    mDestroying;
   uint32_t                mContextFlags;
   uint32_t                mPersistentAttributesDirty; 
   uint32_t                mPersistentAttributesMask;
   uint32_t                mChromeFlags;
   nsString                mTitle;
   nsIntRect               mOpenerScreenRect; 

   nsCOMArray<nsIWeakReference> mTargetableShells; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXULWindow, NS_XULWINDOW_IMPL_CID)




class nsContentShellInfo
{
public:
   nsContentShellInfo(const nsAString& aID,
                      nsIWeakReference* aContentShell);
   ~nsContentShellInfo();

public:
   nsAutoString id; 
   nsWeakPtr child; 
};

#endif
