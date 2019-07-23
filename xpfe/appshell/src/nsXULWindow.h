






































#ifndef nsXULWindow_h__
#define nsXULWindow_h__


#include "nsChromeTreeOwner.h"
#include "nsContentTreeOwner.h"


#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsCOMArray.h"


#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindowInternal.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIXULWindow.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsGUIEvent.h"
#include "nsIXULBrowserWindow.h"
#include "nsIWeakReference.h"



#define NS_XULWINDOW_IMPL_CID                         \
{ /* 2a38ef7e-3174-44ad-a785-b5a863cf5588 */          \
     0x2a38ef7e,                                      \
     0x3174,                                          \
     0x44ad,                                          \
   { 0xa7, 0x85, 0xb5, 0xa8, 0x63, 0xcf, 0x55, 0x88 } \
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
   NS_DECL_ISUPPORTS

   NS_DECL_NSIINTERFACEREQUESTOR
   NS_DECL_NSIXULWINDOW
   NS_DECL_NSIBASEWINDOW

   NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULWINDOW_IMPL_CID)

   void LockUntilChromeLoad() { mLockedUntilChromeLoad = PR_TRUE; }
   PRBool IsLocked() const { return mLockedUntilChromeLoad; }

protected:
   enum persistentAttributes {
     PAD_MISC =         0x1,
     PAD_POSITION =     0x2,
     PAD_SIZE =         0x4
   };

   nsXULWindow();
   virtual ~nsXULWindow();

   NS_IMETHOD EnsureChromeTreeOwner();
   NS_IMETHOD EnsureContentTreeOwner();
   NS_IMETHOD EnsurePrimaryContentTreeOwner();
   NS_IMETHOD EnsurePrompter();
   NS_IMETHOD EnsureAuthPrompter();
   
   void OnChromeLoaded();
   void StaggerPosition(PRInt32 &aRequestedX, PRInt32 &aRequestedY,
                        PRInt32 aSpecWidth, PRInt32 aSpecHeight);
   PRBool     LoadPositionFromXUL();
   PRBool     LoadSizeFromXUL();
   PRBool     LoadMiscPersistentAttributesFromXUL();
   nsresult   LoadChromeHidingFromXUL();
   NS_IMETHOD LoadWindowClassFromXUL();
   NS_IMETHOD LoadIconFromXUL();
   NS_IMETHOD LoadToolbarButtonPresenceFromXUL();
   NS_IMETHOD SavePersistentAttributes();

   NS_IMETHOD GetWindowDOMWindow(nsIDOMWindowInternal** aDOMWindow);
   NS_IMETHOD GetWindowDOMElement(nsIDOMElement** aDOMElement);

   
   NS_HIDDEN_(nsresult) ContentShellAdded(nsIDocShellTreeItem* aContentShell,
                                          PRBool aPrimary, PRBool aTargetable,
                                          const nsAString& aID);
   NS_HIDDEN_(nsresult) ContentShellRemoved(nsIDocShellTreeItem* aContentShell);
   NS_IMETHOD SizeShellTo(nsIDocShellTreeItem* aShellItem, PRInt32 aCX, 
      PRInt32 aCY);
   NS_IMETHOD ExitModalLoop(nsresult aStatus);
   NS_IMETHOD CreateNewChromeWindow(PRInt32 aChromeFlags,
      nsIAppShell* aAppShell, nsIXULWindow **_retval);
   NS_IMETHOD CreateNewContentWindow(PRInt32 aChromeFlags,
      nsIAppShell* aAppShell, nsIXULWindow **_retval);

   void       EnableParent(PRBool aEnable);
   PRBool     ConstrainToZLevel(PRBool aImmediate, nsWindowZ *aPlacement,
                                nsIWidget *aReqBelow, nsIWidget **aActualBelow);
   void       PlaceWindowLayersBehind(PRUint32 aLowLevel, PRUint32 aHighLevel,
                                      nsIXULWindow *aBehind);
   void       SetContentScrollbarVisibility(PRBool aVisible);
   PRBool     GetContentScrollbarVisibility();
   void       PersistentAttributesDirty(PRUint32 aDirtyFlags);
   PRInt32    AppUnitsPerDevPixel();

   nsChromeTreeOwner*      mChromeTreeOwner;
   nsContentTreeOwner*     mContentTreeOwner;
   nsContentTreeOwner*     mPrimaryContentTreeOwner;
   nsCOMPtr<nsIWidget>     mWindow;
   nsCOMPtr<nsIDocShell>   mDocShell;
   nsCOMPtr<nsIDOMWindowInternal>  mDOMWindow;
   nsCOMPtr<nsIWeakReference> mParentWindow;
   nsCOMPtr<nsIPrompt>     mPrompter;
   nsCOMPtr<nsIAuthPrompt> mAuthPrompter;
   nsCOMPtr<nsIXULBrowserWindow> mXULBrowserWindow;
   nsCOMPtr<nsIDocShellTreeItem> mPrimaryContentShell;
   nsTArray<nsContentShellInfo*> mContentShells; 
   nsresult                mModalStatus;
   PRPackedBool            mContinueModalLoop;
   PRPackedBool            mDebuting;       
   PRPackedBool            mChromeLoaded; 
   PRPackedBool            mShowAfterLoad;
   PRPackedBool            mIntrinsicallySized; 
   PRPackedBool            mCenterAfterLoad;
   PRPackedBool            mIsHiddenWindow;
   PRPackedBool            mLockedUntilChromeLoad;
   PRUint32                mContextFlags;
   PRUint32                mBlurSuppressionLevel;
   PRUint32                mPersistentAttributesDirty; 
   PRUint32                mPersistentAttributesMask;
   PRUint32                mChromeFlags;
   PRUint32                mAppPerDev; 
                                       
   nsString                mTitle;

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
