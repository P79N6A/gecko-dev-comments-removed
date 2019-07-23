





































#ifndef nsMenuBarX_h_
#define nsMenuBarX_h_

#include "nsIMenuBar.h"
#include "nsObjCExceptions.h"
#include "nsIMutationObserver.h"
#include "nsCOMArray.h"
#include "nsHashtable.h"
#include "nsTHashtable.h"
#include "nsWeakReference.h"
#include "nsIContent.h"

#import  <Carbon/Carbon.h>
#import  <Cocoa/Cocoa.h>

class nsIWidget;
class nsIDocument;
class nsIDOMNode;
class nsChangeObserver;

extern "C" MenuRef _NSGetCarbonMenu(NSMenu* aMenu);

PRBool NodeIsHiddenOrCollapsed(nsIContent* inContent);

namespace MenuHelpersX
{
  nsEventStatus DispatchCommandTo(nsIContent* aTargetContent);
  NSString* CreateTruncatedCocoaLabel(const nsString& itemLabel);
  PRUint8 GeckoModifiersForNodeAttribute(const nsString& modifiersAttribute);
  unsigned int MacModifiersForGeckoModifiers(PRUint8 geckoModifiers);
  nsIMenuBar* GetHiddenWindowMenuBar();
  NSMenuItem* GetStandardEditMenuItem();
}



@interface NativeMenuItemTarget : NSObject
{
}
-(IBAction)menuItemHit:(id)sender;
@end


struct CocoaKeyEquivContainer {
  CocoaKeyEquivContainer(const unsigned int modifiers, const NSString* string)
  {
    NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

    mModifiers = modifiers;
    mString = [string retain];

    NS_OBJC_END_TRY_ABORT_BLOCK;
  }
  
  ~CocoaKeyEquivContainer()
  {
    NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

    [mString release];

    NS_OBJC_END_TRY_ABORT_BLOCK;
  }
  
  CocoaKeyEquivContainer(const CocoaKeyEquivContainer& other)
  {
    NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

    mModifiers = other.mModifiers;
    mString = [other.mString retain];

    NS_OBJC_END_TRY_ABORT_BLOCK;
  }
  
  CocoaKeyEquivContainer& operator=(CocoaKeyEquivContainer& other)
  {
    NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

    mModifiers = other.mModifiers;
    if (mString)
      [mString release];
    mString = [other.mString retain];
    return *this;

    NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(*this);
  }
  
  unsigned int mModifiers;
  NSString* mString;
};


struct CocoaKeyEquivKey : public PLDHashEntryHdr {
  typedef const CocoaKeyEquivContainer& KeyType;
  typedef const CocoaKeyEquivContainer* KeyTypePointer;
  
  CocoaKeyEquivKey(KeyTypePointer aObj) : mObj(*aObj) { }
  CocoaKeyEquivKey(const CocoaKeyEquivKey& other) : mObj(other.mObj) { }
  ~CocoaKeyEquivKey() { }
  
  KeyType GetKey() const { return mObj; }
  
  PRBool KeyEquals(KeyTypePointer aKey) const {
    NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

    return aKey->mModifiers == mObj.mModifiers &&
    [aKey->mString isEqualToString:mObj.mString];

    NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(PR_FALSE);
  }
  
  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey) {
    return [aKey->mString hash] ^ aKey->mModifiers;
  }
  enum { ALLOW_MEMMOVE = PR_FALSE };
private:
  const CocoaKeyEquivContainer mObj;
};







class nsMenuBarX : public nsIMenuBar,
                   public nsIMutationObserver,
                   public nsSupportsWeakReference
{
public:
    nsMenuBarX();
    virtual ~nsMenuBarX();

    
    static NativeMenuItemTarget* sNativeEventTarget;
    
    static nsMenuBarX* sLastGeckoMenuBarPainted;
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIMUTATIONOBSERVER

    
    NS_IMETHOD Create(nsIWidget * aParent);
    NS_IMETHOD GetParent(nsIWidget *&aParent);
    NS_IMETHOD SetParent(nsIWidget * aParent);
    NS_IMETHOD AddMenu(nsIMenu * aMenu);
    NS_IMETHOD GetMenuCount(PRUint32 &aCount);
    NS_IMETHOD GetMenuAt(const PRUint32 aCount, nsIMenu *& aMenu);
    NS_IMETHOD InsertMenuAt(const PRUint32 aCount, nsIMenu *& aMenu);
    NS_IMETHOD RemoveMenu(const PRUint32 aCount);
    NS_IMETHOD RemoveAll();
    NS_IMETHOD GetNativeData(void*& aData);
    NS_IMETHOD Paint();
    NS_IMETHOD SetNativeData(void* aData);
    NS_IMETHOD MenuConstruct(const nsMenuEvent & aMenuEvent, nsIWidget * aParentWindow, void * aMenuNode);
    PRBool ContainsKeyEquiv(unsigned int modifiers, NSString* string);

    PRUint32 RegisterForCommand(nsIMenuItem* aItem);
    void UnregisterCommand(PRUint32 aCommandID);
    nsIMenuItem* GetMenuItemForCommandID(PRUint32 inCommandID);

    void RegisterForContentChanges(nsIContent* aContent, nsChangeObserver* aMenuObject);
    void UnregisterForContentChanges(nsIContent* aContent);
    nsChangeObserver* LookupContentChangeObserver(nsIContent* aContent);

    void RegisterKeyEquivalent(unsigned int modifiers, NSString* string);
    void UnregisterKeyEquivalent(unsigned int modifiers, NSString* string);

    nsCOMPtr<nsIContent>    mAboutItemContent;    
                                                  
    nsCOMPtr<nsIContent>    mPrefItemContent;     
    nsCOMPtr<nsIContent>    mQuitItemContent;     
protected:
    
    void AquifyMenuBar();
    void HideItem(nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode);

    
    NSMenuItem* CreateNativeAppMenuItem(nsIMenu* inMenu, const nsAString& nodeID, SEL action,
                                                    int tag, NativeMenuItemTarget* target);
    nsresult CreateApplicationMenu(nsIMenu* inMenu);

    nsCOMArray<nsIMenu>     mMenusArray;          
    nsCOMPtr<nsIContent>    mMenuBarContent;      
    nsIWidget*              mParent;              
    PRBool                  mIsMenuBarAdded;
    PRUint32                mCurrentCommandID;    
    nsIDocument*            mDocument;            
    NSMenu*                 mRootMenu;            
    nsHashtable             mObserverTable;       
    nsTHashtable<CocoaKeyEquivKey> mKeyEquivTable;
};

#endif 
