





































#ifndef nsMenuBarX_h_
#define nsMenuBarX_h_

#include "nsIMenuBar.h"
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
}



@interface NativeMenuItemTarget : NSObject
{
}
-(IBAction)menuItemHit:(id)sender;
@end


struct CocoaKeyEquivContainer {
  CocoaKeyEquivContainer(const unsigned int modifiers, const NSString* string)
  {
    mModifiers = modifiers;
    mString = [string retain];
  }
  
  ~CocoaKeyEquivContainer()
  {
    [mString release];
  }
  
  CocoaKeyEquivContainer(const CocoaKeyEquivContainer& other)
  {
    mModifiers = other.mModifiers;
    mString = [other.mString retain];
  }
  
  CocoaKeyEquivContainer& operator=(CocoaKeyEquivContainer& other)
  {
    mModifiers = other.mModifiers;
    if (mString)
      [mString release];
    mString = [other.mString retain];
    return *this;
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
    return aKey->mModifiers == mObj.mModifiers &&
    [aKey->mString isEqualToString:mObj.mString];
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
    
    static NSWindow* sEventTargetWindow;
    
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

    void RegisterForContentChanges(nsIContent* aContent, nsChangeObserver* aMenuObject);
    void UnregisterForContentChanges(nsIContent* aContent);
    nsChangeObserver* LookupContentChangeObserver(nsIContent* aContent);

    void RegisterKeyEquivalent(unsigned int modifiers, NSString* string);
    void UnregisterKeyEquivalent(unsigned int modifiers, NSString* string);
protected:
    
    void AquifyMenuBar();
    void HideItem(nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode);
    OSStatus InstallCommandEventHandler();

    
    pascal static OSStatus CommandEventHandler(EventHandlerCallRef inHandlerChain, 
                                               EventRef inEvent, void* userData);
    nsEventStatus ExecuteCommand(nsIContent* inDispatchTo);
    
    
    NSMenuItem* CreateNativeAppMenuItem(nsIMenu* inMenu, const nsAString& nodeID, SEL action,
                                                    int tag, NativeMenuItemTarget* target);
    nsresult CreateApplicationMenu(nsIMenu* inMenu);

    nsHashtable             mObserverTable;       

    nsCOMArray<nsIMenu>     mMenusArray;          
    nsCOMPtr<nsIContent>    mMenuBarContent;      
    nsCOMPtr<nsIContent>    mAboutItemContent;    
                                                  
    nsCOMPtr<nsIContent>    mPrefItemContent;     
    nsCOMPtr<nsIContent>    mQuitItemContent;     
    nsIWidget*              mParent;              
    PRBool                  mIsMenuBarAdded;
    PRUint32                mCurrentCommandID;    

    nsIDocument*            mDocument;            

    NSMenu*                 mRootMenu;            

    nsTHashtable<CocoaKeyEquivKey> mKeyEquivTable;

    static EventHandlerUPP  sCommandEventHandler; 
};

#endif 
