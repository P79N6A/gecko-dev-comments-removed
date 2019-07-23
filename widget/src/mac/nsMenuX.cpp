




































#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIDOMDocument.h"
#include "nsIDocumentViewer.h"
#include "nsIDocumentObserver.h"
#include "nsIComponentManager.h"
#include "nsIDocShell.h"
#include "prinrval.h"
#include "nsIRollupListener.h"

#include "nsMenuX.h"
#include "nsMenuBarX.h"
#include "nsIMenu.h"
#include "nsIMenuBar.h"
#include "nsIMenuItem.h"
#include "nsIMenuListener.h"
#include "nsPresContext.h"
#include "nsIMenuCommandDispatcher.h"
#include "nsMenuItemIcon.h"

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "plstr.h"

#include "nsINameSpaceManager.h"
#include "nsWidgetAtoms.h"
#include "nsIXBLService.h"
#include "nsIServiceManager.h"

#include "nsGUIEvent.h"

#include "nsCRT.h"
#include "nsToolkit.h"


extern nsIRollupListener * gRollupListener;
extern nsIWidget         * gRollupWidget;

static OSStatus InstallMyMenuEventHandler(MenuRef menuRef, void* userData, EventHandlerRef* outHandler) ;

static PRInt16 gMacMenuIDCountX = nsMenuBarX::kAppleMenuID + 1;
static PRBool gConstructingMenu = PR_FALSE;
  
#if DEBUG
nsInstanceCounter   gMenuCounterX("nsMenuX");
#endif


#include "nsWidgetsCID.h"
static NS_DEFINE_CID(kMenuCID,     NS_MENU_CID);
static NS_DEFINE_CID(kMenuItemCID, NS_MENUITEM_CID);


class nsDummyMenuItemX : public nsISupports {
public:
    NS_DECL_ISUPPORTS

    nsDummyMenuItemX()
    {
    }
};

NS_IMETHODIMP_(nsrefcnt) nsDummyMenuItemX::AddRef() { return ++mRefCnt; }
NS_METHOD nsDummyMenuItemX::Release() { return --mRefCnt; }
NS_IMPL_QUERY_INTERFACE0(nsDummyMenuItemX)
static nsDummyMenuItemX gDummyMenuItemX;


NS_IMPL_ISUPPORTS4(nsMenuX, nsIMenu, nsIMenuListener, nsIChangeObserver, nsISupportsWeakReference)




nsMenuX::nsMenuX()
    :   mNumMenuItems(0), mParent(nsnull), mManager(nsnull), mMacMenuID(0),
        mMacMenuHandle(nsnull), mIsEnabled(PR_TRUE), mDestroyHandlerCalled(PR_FALSE),
        mNeedsRebuild(PR_TRUE), mConstructed(PR_FALSE), mVisible(PR_TRUE), mHandler(nsnull)
{
#if DEBUG
  ++gMenuCounterX;
#endif 
}





nsMenuX::~nsMenuX()
{
  RemoveAll();

  if ( mMacMenuHandle ) {
    if ( mHandler )
      ::RemoveEventHandler(mHandler);
    ::ReleaseMenu(mMacMenuHandle);
  }
  
  
  mManager->Unregister(mMenuContent);

#if DEBUG
  --gMenuCounterX;
#endif
}





NS_METHOD 
nsMenuX::Create(nsISupports * aParent, const nsAString &aLabel, const nsAString &aAccessKey, 
                nsIChangeManager* aManager, nsIDocShell* aShell, nsIContent* aNode )
{
  mDocShellWeakRef = do_GetWeakReference(aShell);
  mMenuContent = aNode;

  
  mManager = aManager;			
  nsCOMPtr<nsIChangeObserver> changeObs ( do_QueryInterface(static_cast<nsIChangeObserver*>(this)) );
  mManager->Register(mMenuContent, changeObs);

  NS_ASSERTION ( mMenuContent, "Menu not given a dom node at creation time" );
  NS_ASSERTION ( mManager, "No change manager given, can't tell content model updates" );

  mParent = aParent;
  
  nsCOMPtr<nsIMenuBar> menubar = do_QueryInterface(aParent);
  nsCOMPtr<nsIMenu> menu = do_QueryInterface(aParent);
  NS_ASSERTION(menubar || menu, "Menu parent not a menu bar or menu!" );

  SetLabel(aLabel);
  SetAccessKey(aAccessKey);

  if (mMenuContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::hidden,
                                nsWidgetAtoms::_true, eCaseMatters) ||
      mMenuContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::collapsed,
                                nsWidgetAtoms::_true, eCaseMatters))
    mVisible = PR_FALSE;

  if (menubar && mMenuContent->GetChildCount() == 0)
    mVisible = PR_FALSE;

  
  
  
  
  nsMenuEvent fake(PR_TRUE, 0, nsnull);
  MenuConstruct(fake, nsnull, nsnull, nsnull);

  if (menu)
    mIcon = new nsMenuItemIcon(static_cast<nsIMenu*>(this),
                               menu, mMenuContent);

  return NS_OK;
}


NS_METHOD nsMenuX::GetParent(nsISupports*& aParent)
{
  aParent = mParent;
  NS_IF_ADDREF(aParent);
  return NS_OK;
}


NS_METHOD nsMenuX::GetLabel(nsString &aText)
{
  aText = mLabel;
  return NS_OK;
}



NS_METHOD nsMenuX::SetLabel(const nsAString &aText)
{
  mLabel = aText;

  
  if (mMacMenuHandle == nsnull) {
    mMacMenuID = gMacMenuIDCountX++;
    mMacMenuHandle = NSStringNewMenu(mMacMenuID, mLabel);
  }
  
  return NS_OK;
}


NS_METHOD nsMenuX::GetAccessKey(nsString &aText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD nsMenuX::SetAccessKey(const nsAString &aText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD nsMenuX::AddItem(nsISupports* aItem)
{
    nsresult rv = NS_ERROR_FAILURE;
    if (aItem) {
        
        nsCOMPtr<nsIMenuItem> menuItem(do_QueryInterface(aItem));
        if (menuItem) {
            rv = AddMenuItem(menuItem);
        } else {
            nsCOMPtr<nsIMenu> menu(do_QueryInterface(aItem));
            if (menu)
                rv = AddMenu(menu);
        }
    }
    return rv;
}


NS_METHOD nsMenuX::AddMenuItem(nsIMenuItem * aMenuItem)
{
  if(!aMenuItem) return NS_ERROR_NULL_POINTER;

  mMenuItemsArray.AppendElement(aMenuItem);    
  PRUint32 currItemIndex;
  mMenuItemsArray.Count(&currItemIndex);

  mNumMenuItems++;

  nsAutoString label;
  aMenuItem->GetLabel(label);
  InsertMenuItemWithTruncation ( label, currItemIndex );
  
  
  nsAutoString keyEquivalent(NS_LITERAL_STRING(" "));
  aMenuItem->GetShortcutChar(keyEquivalent);
  if (!keyEquivalent.IsEmpty() && !keyEquivalent.EqualsLiteral(" ")) {
    ToUpperCase(keyEquivalent);
    short inKey = NS_LossyConvertUTF16toASCII(keyEquivalent).First();
    ::SetItemCmd(mMacMenuHandle, currItemIndex, inKey);
    
  }

  PRUint8 modifiers;
  aMenuItem->GetModifiers(&modifiers);
  PRUint8 macModifiers = kMenuNoModifiers;
  if (knsMenuItemShiftModifier & modifiers)
    macModifiers |= kMenuShiftModifier;

  if (knsMenuItemAltModifier & modifiers)
    macModifiers |= kMenuOptionModifier;

  if (knsMenuItemControlModifier & modifiers)
    macModifiers |= kMenuControlModifier;

  if (!(knsMenuItemCommandModifier & modifiers))
    macModifiers |= kMenuNoCommandModifier;

  ::SetMenuItemModifiers(mMacMenuHandle, currItemIndex, macModifiers);

  
  nsCOMPtr<nsIMenuCommandDispatcher> dispatcher ( do_QueryInterface(mManager) );
  if ( dispatcher ) {
    PRUint32 commandID = 0L;
    dispatcher->Register(aMenuItem, &commandID);
    if ( commandID )
      ::SetMenuItemCommandID(mMacMenuHandle, currItemIndex, commandID);
  }
  
  PRBool isEnabled;
  aMenuItem->GetEnabled(&isEnabled);
  if(isEnabled)
    ::EnableMenuItem(mMacMenuHandle, currItemIndex);
  else
    ::DisableMenuItem(mMacMenuHandle, currItemIndex);

  PRBool isChecked;
  aMenuItem->GetChecked(&isChecked);
  if(isChecked)
    ::CheckMenuItem(mMacMenuHandle, currItemIndex, true);
  else
    ::CheckMenuItem(mMacMenuHandle, currItemIndex, false);

  return NS_OK;
}


NS_METHOD nsMenuX::AddMenu(nsIMenu * aMenu)
{
  
  if (!aMenu) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsISupports>  supports = do_QueryInterface(aMenu);
  if (!supports) return NS_ERROR_NO_INTERFACE;

  mMenuItemsArray.AppendElement(supports);   
  PRUint32 currItemIndex;
  mMenuItemsArray.Count(&currItemIndex);

  mNumMenuItems++;

  
  nsAutoString label;
  aMenu->GetLabel(label);
  InsertMenuItemWithTruncation ( label, currItemIndex );

  PRBool isEnabled;
  aMenu->GetEnabled(&isEnabled);
  if (isEnabled)
    ::EnableMenuItem(mMacMenuHandle, currItemIndex);
  else
    ::DisableMenuItem(mMacMenuHandle, currItemIndex);	    

  MenuHandle childMenu;
  if (aMenu->GetNativeData((void**)&childMenu) == NS_OK)
    ::SetMenuItemHierarchicalMenu((MenuHandle) mMacMenuHandle, currItemIndex, childMenu);
  
  return NS_OK;
}








void
nsMenuX :: InsertMenuItemWithTruncation ( nsAutoString & inItemLabel, PRUint32 inItemIndex )
{
  
  
  
  
  const short kMaxItemPixelWidth = 300;

  CFMutableStringRef labelRef = ::CFStringCreateMutable ( kCFAllocatorDefault, inItemLabel.Length() );
  ::CFStringAppendCharacters ( labelRef, (UniChar*)inItemLabel.get(), inItemLabel.Length() );
  ::TruncateThemeText(labelRef, kThemeMenuItemFont, kThemeStateActive, kMaxItemPixelWidth, truncMiddle, NULL);
  ::InsertMenuItemTextWithCFString(mMacMenuHandle, labelRef, inItemIndex, 0, 0);
  ::CFRelease(labelRef);

} 



NS_METHOD nsMenuX::AddSeparator()
{
  
  
  mMenuItemsArray.AppendElement(&gDummyMenuItemX);   
  PRUint32  numItems;
  mMenuItemsArray.Count(&numItems);
  ::InsertMenuItem(mMacMenuHandle, "\p(-", numItems);
  mNumMenuItems++;
  return NS_OK;
}


NS_METHOD nsMenuX::GetItemCount(PRUint32 &aCount)
{
  return mMenuItemsArray.Count(&aCount);
}


NS_METHOD nsMenuX::GetItemAt(const PRUint32 aPos, nsISupports *& aMenuItem)
{
  mMenuItemsArray.GetElementAt(aPos, &aMenuItem);
  return NS_OK;
}


NS_METHOD nsMenuX::InsertItemAt(const PRUint32 aPos, nsISupports * aMenuItem)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD nsMenuX::RemoveItem(const PRUint32 aPos)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD nsMenuX::RemoveAll()
{
  if (mMacMenuHandle != NULL) {    
    
    nsCOMPtr<nsIMenuCommandDispatcher> dispatcher ( do_QueryInterface(mManager) );
    if ( dispatcher ) {
      for ( unsigned int i = 1; i <= mNumMenuItems; ++i ) {
        PRUint32 commandID = 0L;
        OSErr err = ::GetMenuItemCommandID(mMacMenuHandle, i, (unsigned long*)&commandID);
        if ( !err )
          dispatcher->Unregister(commandID);
      }
    }
    ::DeleteMenuItems(mMacMenuHandle, 1, ::CountMenuItems(mMacMenuHandle));
  }
  
  mMenuItemsArray.Clear();    
  return NS_OK;
}


NS_METHOD nsMenuX::GetNativeData(void ** aData)
{
  *aData = mMacMenuHandle;
  return NS_OK;
}


NS_METHOD nsMenuX::SetNativeData(void * aData)
{
  mMacMenuHandle = (MenuHandle) aData;
  return NS_OK;
}


NS_METHOD nsMenuX::AddMenuListener(nsIMenuListener * aMenuListener)
{
  mListener = aMenuListener;    
  return NS_OK;
}


NS_METHOD nsMenuX::RemoveMenuListener(nsIMenuListener * aMenuListener)
{
  if (aMenuListener == mListener)
    mListener = nsnull;
  return NS_OK;
}







nsEventStatus nsMenuX::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  
  return nsEventStatus_eConsumeNoDefault;
}


nsEventStatus nsMenuX::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

  
  MenuHandle selectedMenuHandle = (MenuHandle) aMenuEvent.mCommand;

  if (mMacMenuHandle == selectedMenuHandle) {
    
    mMenuContent->SetAttr(kNameSpaceID_None, nsWidgetAtoms::open, NS_LITERAL_STRING("true"), PR_TRUE);
  

    
    PRBool keepProcessing = OnCreate();

    if (!mNeedsRebuild || !keepProcessing)
      return nsEventStatus_eConsumeNoDefault;

    if (!mConstructed || mNeedsRebuild) {
      if (mNeedsRebuild)
        RemoveAll();

      nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocShellWeakRef);
      if (!docShell) {
        NS_ERROR("No doc shell");
        return nsEventStatus_eConsumeNoDefault;
      }

      MenuConstruct(aMenuEvent, nsnull , nsnull, docShell);
      mConstructed = true;
    } 

    OnCreated();  

    eventStatus = nsEventStatus_eConsumeNoDefault;  
  } 
  else {
    
    PRUint32    numItems;
    mMenuItemsArray.Count(&numItems);
    for (PRUint32 i = numItems; i > 0; i--) {
      nsCOMPtr<nsISupports>     menuSupports = getter_AddRefs(mMenuItemsArray.ElementAt(i - 1));    
      nsCOMPtr<nsIMenu>         submenu = do_QueryInterface(menuSupports);
      nsCOMPtr<nsIMenuListener> menuListener = do_QueryInterface(submenu);
      if (menuListener) {
        eventStatus = menuListener->MenuSelected(aMenuEvent);
        if (nsEventStatus_eIgnore != eventStatus)
          return eventStatus;
      }  
    }
  }

  return eventStatus;
}


nsEventStatus nsMenuX::MenuDeselected(const nsMenuEvent & aMenuEvent)
{
  
  if (mConstructed) {
    MenuDestruct(aMenuEvent);
    mConstructed = false;
  }
  return nsEventStatus_eIgnore;
}


nsEventStatus nsMenuX::MenuConstruct(
    const nsMenuEvent & aMenuEvent,
    nsIWidget         * aParentWindow, 
    void              * ,
	  void              * aDocShell)
{
  mConstructed = false;
  gConstructingMenu = PR_TRUE;
  
  
  mDestroyHandlerCalled = PR_FALSE;
  
  
  
  
  
  nsCOMPtr<nsIContent> menuPopup;
  GetMenuPopupContent(getter_AddRefs(menuPopup));
  if (!menuPopup)
    return nsEventStatus_eIgnore;
      
  
  PRUint32 count = menuPopup->GetChildCount();
  for ( PRUint32 i = 0; i < count; ++i ) {
    nsIContent *child = menuPopup->GetChildAt(i);
    if ( child ) {
      
      nsIAtom *tag = child->Tag();
      if ( tag == nsWidgetAtoms::menuitem )
        LoadMenuItem(this, child);
      else if ( tag == nsWidgetAtoms::menuseparator )
        LoadSeparator(child);
      else if ( tag == nsWidgetAtoms::menu )
        LoadSubMenu(this, child);
    }
  } 
  
  gConstructingMenu = PR_FALSE;
  mNeedsRebuild = PR_FALSE;
  
  
  return nsEventStatus_eIgnore;
}



nsEventStatus nsMenuX::MenuDestruct(const nsMenuEvent & aMenuEvent)
{
  
  
  
  PRBool keepProcessing = OnDestroy();
  if ( keepProcessing ) {
    if(mNeedsRebuild) {
        mConstructed = false;
        
    } 
    
    mMenuContent->UnsetAttr(kNameSpaceID_None, nsWidgetAtoms::open, PR_TRUE);

    OnDestroyed();
  }
  
  return nsEventStatus_eIgnore;
}


nsEventStatus nsMenuX::CheckRebuild(PRBool & aNeedsRebuild)
{
  aNeedsRebuild = PR_TRUE; 
  return nsEventStatus_eIgnore;
}


nsEventStatus nsMenuX::SetRebuild(PRBool aNeedsRebuild)
{
  if (!gConstructingMenu)
    mNeedsRebuild = aNeedsRebuild;

  return nsEventStatus_eIgnore;
}






NS_METHOD nsMenuX::SetEnabled(PRBool aIsEnabled)
{
  mIsEnabled = aIsEnabled;

  if ( aIsEnabled )
    ::EnableMenuItem(mMacMenuHandle, 0);
  else
    ::DisableMenuItem(mMacMenuHandle, 0);

  return NS_OK;
}






NS_METHOD nsMenuX::GetEnabled(PRBool* aIsEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  *aIsEnabled = mIsEnabled;
  return NS_OK;
}






NS_METHOD nsMenuX::GetMenuContent(nsIContent ** aMenuContent)
{
  NS_ENSURE_ARG_POINTER(aMenuContent);
  NS_IF_ADDREF(*aMenuContent = mMenuContent);
	return NS_OK;
}






static pascal OSStatus MyMenuEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData)
{
  OSStatus result = eventNotHandledErr;

  UInt32 kind = ::GetEventKind(event);
  if (kind == kEventMenuTargetItem) {
    
    nsIMenu* targetMenu = reinterpret_cast<nsIMenu*>(userData);
    PRUint16 aPos;
    ::GetEventParameter(event, kEventParamMenuItemIndex, typeMenuItemIndex, NULL, sizeof(MenuItemIndex), NULL, &aPos);
    aPos--; 
    
    nsISupports* aTargetMenuItem;
    targetMenu->GetItemAt((PRUint32)aPos, aTargetMenuItem);
    
    
    
    nsCOMPtr<nsIMenuItem> bTargetMenuItem(do_QueryInterface(aTargetMenuItem));
    if (bTargetMenuItem) {
      PRBool handlerCalledPreventDefault; 
      bTargetMenuItem->DispatchDOMEvent(NS_LITERAL_STRING("DOMMenuItemActive"), &handlerCalledPreventDefault);
    }
  }
  else if (kind == kEventMenuOpening || kind == kEventMenuClosed) {
    if (kind == kEventMenuOpening && gRollupListener != nsnull && gRollupWidget != nsnull) {
      gRollupListener->Rollup();
      
      
      if (nsToolkit::OSXVersion() >= MAC_OS_X_VERSION_10_4_HEX)
        return userCanceledErr;
    }

    nsISupports* supports = reinterpret_cast<nsISupports*>(userData);
    nsCOMPtr<nsIMenuListener> listener(do_QueryInterface(supports));
    if (listener) {
      MenuRef menuRef;
      ::GetEventParameter(event, kEventParamDirectObject, typeMenuRef, NULL, sizeof(menuRef), NULL, &menuRef);
      nsMenuEvent menuEvent(PR_TRUE, NS_MENU_SELECTED, nsnull);
      menuEvent.time = PR_IntervalNow();
      menuEvent.mCommand = (PRUint32) menuRef;
      if (kind == kEventMenuOpening)
        listener->MenuSelected(menuEvent);
      else
        listener->MenuDeselected(menuEvent);
    }
  }
  
  return result;
}

static OSStatus InstallMyMenuEventHandler(MenuRef menuRef, void* userData, EventHandlerRef* outHandler)
{
  
  static EventTypeSpec eventList[] = {
      { kEventClassMenu, kEventMenuOpening },
      { kEventClassMenu, kEventMenuClosed },
      { kEventClassMenu, kEventMenuTargetItem }
  };
  static EventHandlerUPP gMyMenuEventHandlerUPP = NewEventHandlerUPP(&MyMenuEventHandler);
  return ::InstallMenuEventHandler(menuRef, gMyMenuEventHandlerUPP,
                                   sizeof(eventList) / sizeof(EventTypeSpec), eventList,
                                   userData, outHandler);
}


MenuHandle nsMenuX::NSStringNewMenu(short menuID, nsString& menuTitle)
{
  MenuRef menuRef;
  OSStatus status = ::CreateNewMenu(menuID, 0, &menuRef);
  NS_ASSERTION(status == noErr,"nsMenuX::NSStringNewMenu: NewMenu failed.");
  CFStringRef titleRef = ::CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar*)menuTitle.get(), menuTitle.Length());
  NS_ASSERTION(titleRef,"nsMenuX::NSStringNewMenu: CFStringCreateWithCharacters failed.");
  if (titleRef) {
    ::SetMenuTitleWithCFString(menuRef, titleRef);
    ::CFRelease(titleRef);
  }
  
  status = InstallMyMenuEventHandler(menuRef, this, &mHandler);
  NS_ASSERTION(status == noErr,"nsMenuX::NSStringNewMenu: InstallMyMenuEventHandler failed.");

  return menuRef;
}



void nsMenuX::LoadMenuItem( nsIMenu* inParentMenu, nsIContent* inMenuItemContent )
{
  if ( !inMenuItemContent )
    return;

  
  if (inMenuItemContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::hidden,
                                     nsWidgetAtoms::_true, eCaseMatters))
    return;
  
  nsCOMPtr<nsIDOMDocument> domDocument = do_QueryInterface(inMenuItemContent->GetDocument());
  if (!domDocument)
    return;
  
  
  
  
  
  nsAutoString ourCommand;
  inMenuItemContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::command, ourCommand);
  if (!ourCommand.IsEmpty()) {
    
    nsCOMPtr<nsIDOMElement> commandElt;
    domDocument->GetElementById(ourCommand, getter_AddRefs(commandElt));
    if (commandElt) {
      nsCOMPtr<nsIContent> commandContent = do_QueryInterface(commandElt);
      nsAutoString menuItemDisabled;
      nsAutoString commandDisabled;
      inMenuItemContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, menuItemDisabled);
      commandContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, commandDisabled);
      if (!commandDisabled.Equals(menuItemDisabled)) {
        
        if (commandDisabled.IsEmpty()) 
          inMenuItemContent->UnsetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, PR_TRUE);
        else
          inMenuItemContent->SetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, commandDisabled, PR_TRUE);
      }
    }
  }

  
  nsCOMPtr<nsIMenuItem> pnsMenuItem = do_CreateInstance ( kMenuItemCID ) ;
  if (pnsMenuItem) {
    nsAutoString menuitemName;
    
    inMenuItemContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::label, menuitemName);

    
    
    
    if ( menuitemName[0] == '-' )
      menuitemName.Assign( NS_LITERAL_STRING("\u200c") + menuitemName );

    
    
    static nsIContent::AttrValuesArray strings[] =
      {&nsWidgetAtoms::checkbox, &nsWidgetAtoms::radio, nsnull};
    nsIMenuItem::EMenuItemType itemType = nsIMenuItem::eRegular;
    switch (inMenuItemContent->FindAttrValueIn(kNameSpaceID_None, nsWidgetAtoms::type,
                                               strings, eCaseMatters)) {
      case 0: itemType = nsIMenuItem::eCheckbox; break;
      case 1: itemType = nsIMenuItem::eRadio; break;
    }
      
    nsCOMPtr<nsIDocShell>  docShell = do_QueryReferent(mDocShellWeakRef);
    if (!docShell)
      return;

    
    pnsMenuItem->Create(inParentMenu, menuitemName, PR_FALSE, itemType, 
                        mManager, docShell, inMenuItemContent);   

    
    
    
    
    nsAutoString keyValue;
    inMenuItemContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::key, keyValue);

    
    nsCOMPtr<nsIDOMElement> keyElement;
    if (!keyValue.IsEmpty())
      domDocument->GetElementById(keyValue, getter_AddRefs(keyElement));
    if ( keyElement ) {
      nsCOMPtr<nsIContent> keyContent ( do_QueryInterface(keyElement) );
      nsAutoString keyChar(NS_LITERAL_STRING(" "));
      keyContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::key, keyChar);
	    if(!keyChar.EqualsLiteral(" ")) 
        pnsMenuItem->SetShortcutChar(keyChar);
        
      PRUint8 modifiers = knsMenuItemNoModifier;
	    nsAutoString modifiersStr;
      keyContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::modifiers, modifiersStr);
		  char* str = ToNewCString(modifiersStr);
		  char* newStr;
		  char* token = nsCRT::strtok( str, ", \t", &newStr );
		  while( token != NULL ) {
		    if (PL_strcmp(token, "shift") == 0)
		      modifiers |= knsMenuItemShiftModifier;
		    else if (PL_strcmp(token, "alt") == 0) 
		      modifiers |= knsMenuItemAltModifier;
		    else if (PL_strcmp(token, "control") == 0) 
		      modifiers |= knsMenuItemControlModifier;
		    else if ((PL_strcmp(token, "accel") == 0) ||
		             (PL_strcmp(token, "meta") == 0)) {
          modifiers |= knsMenuItemCommandModifier;
		    }
		    
		    token = nsCRT::strtok( newStr, ", \t", &newStr );
		  }
		  nsMemory::Free(str);

	    pnsMenuItem->SetModifiers ( modifiers );
    }

    if (inMenuItemContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::checked,
                                       nsWidgetAtoms::_true, eCaseMatters))
      pnsMenuItem->SetChecked(PR_TRUE);
    else
      pnsMenuItem->SetChecked(PR_FALSE);
      
    nsCOMPtr<nsISupports> supports ( do_QueryInterface(pnsMenuItem) );
    inParentMenu->AddItem(supports);         

    pnsMenuItem->SetupIcon();
  }
}


void 
nsMenuX::LoadSubMenu( nsIMenu * pParentMenu, nsIContent* inMenuItemContent )
{
  
  if (inMenuItemContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::hidden,
                                     nsWidgetAtoms::_true, eCaseMatters))
    return;
  
  nsAutoString menuName; 
  inMenuItemContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::label, menuName);
  

  
  nsCOMPtr<nsIMenu> pnsMenu ( do_CreateInstance(kMenuCID) );
  if (pnsMenu) {
    
    nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocShellWeakRef);
    if (!docShell)
        return;
    nsCOMPtr<nsISupports> supports(do_QueryInterface(pParentMenu));
    pnsMenu->Create(supports, menuName, EmptyString(), mManager, docShell, inMenuItemContent);

    
    if (inMenuItemContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::disabled,
                                       nsWidgetAtoms::_true, eCaseMatters))
      pnsMenu->SetEnabled ( PR_FALSE );
    else
      pnsMenu->SetEnabled ( PR_TRUE );

    
    nsCOMPtr<nsISupports> supports2 ( do_QueryInterface(pnsMenu) );
	  pParentMenu->AddItem(supports2);

    pnsMenu->SetupIcon();
  }     
}


void
nsMenuX::LoadSeparator ( nsIContent* inMenuItemContent ) 
{
  
  if (inMenuItemContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::hidden,
                                     nsWidgetAtoms::_true, eCaseMatters))
    return;

  AddSeparator();
}









PRBool
nsMenuX::OnCreate()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWING, nsnull,
                     nsMouseEvent::eReal);
  
  nsCOMPtr<nsIContent> popupContent;
  GetMenuPopupContent(getter_AddRefs(popupContent));

  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocShellWeakRef);
  if (!docShell) {
    NS_ERROR("No doc shell");
    return PR_FALSE;
  }
  nsCOMPtr<nsPresContext> presContext;
  MenuHelpersX::DocShellToPresContext(docShell, getter_AddRefs(presContext) );
  if ( presContext ) {
    nsresult rv = NS_OK;
    nsIContent* dispatchTo = popupContent ? popupContent : mMenuContent;
    rv = dispatchTo->DispatchDOMEvent(&event, nsnull, presContext, &status);
    if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
      return PR_FALSE;
 }

  
  
  
  
  if (popupContent) {
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(popupContent->GetDocument()));

    PRUint32 count = popupContent->GetChildCount();
    for (PRUint32 i = 0; i < count; i++) {
      nsIContent *grandChild = popupContent->GetChildAt(i);
      if (grandChild->Tag() == nsWidgetAtoms::menuitem) {
        
        nsAutoString command;
        grandChild->GetAttr(kNameSpaceID_None, nsWidgetAtoms::command, command);
        if (!command.IsEmpty()) {
          
          nsCOMPtr<nsIDOMElement> commandElt;
          domDoc->GetElementById(command, getter_AddRefs(commandElt));
          nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));

          if ( commandContent ) {
            nsAutoString commandDisabled, menuDisabled;
            commandContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, commandDisabled);
            grandChild->GetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, menuDisabled);
            if (!commandDisabled.Equals(menuDisabled)) {
              
              if (commandDisabled.IsEmpty()) 
                grandChild->UnsetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, PR_TRUE);
              else grandChild->SetAttr(kNameSpaceID_None, nsWidgetAtoms::disabled, commandDisabled, PR_TRUE);
            }

            
            
            
            nsAutoString commandChecked, menuChecked;
            commandContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::checked, commandChecked);
            grandChild->GetAttr(kNameSpaceID_None, nsWidgetAtoms::checked, menuChecked);
            if (!commandChecked.Equals(menuChecked)) {
              if (!commandChecked.IsEmpty()) 
                grandChild->SetAttr(kNameSpaceID_None, nsWidgetAtoms::checked, commandChecked, PR_TRUE);
            }

            nsAutoString commandValue, menuValue;
            commandContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::label, commandValue);
            grandChild->GetAttr(kNameSpaceID_None, nsWidgetAtoms::label, menuValue);
            if (!commandValue.Equals(menuValue)) {
              if (!commandValue.IsEmpty()) 
                grandChild->SetAttr(kNameSpaceID_None, nsWidgetAtoms::label, commandValue, PR_TRUE);
            }
          }
        }
      }
    }
  }
  
  return PR_TRUE;
}

PRBool
nsMenuX::OnCreated()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWN, nsnull, nsMouseEvent::eReal);
  
  nsCOMPtr<nsIContent> popupContent;
  GetMenuPopupContent(getter_AddRefs(popupContent));

  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocShellWeakRef);
  if (!docShell) {
    NS_ERROR("No doc shell");
    return PR_FALSE;
  }
  nsCOMPtr<nsPresContext> presContext;
  MenuHelpersX::DocShellToPresContext(docShell, getter_AddRefs(presContext) );
  if ( presContext ) {
    nsresult rv = NS_OK;
    nsIContent* dispatchTo = popupContent ? popupContent : mMenuContent;
    rv = dispatchTo->DispatchDOMEvent(&event, nsnull, presContext, &status);
    if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
      return PR_FALSE;
 }
  
  return PR_TRUE;
}







PRBool
nsMenuX::OnDestroy()
{
  if ( mDestroyHandlerCalled )
    return PR_TRUE;

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDING, nsnull,
                     nsMouseEvent::eReal);
  
  nsCOMPtr<nsIDocShell>  docShell = do_QueryReferent(mDocShellWeakRef);
  if (!docShell) {
    NS_WARNING("No doc shell so can't run the OnDestroy");
    return PR_FALSE;
  }

  nsCOMPtr<nsIContent> popupContent;
  GetMenuPopupContent(getter_AddRefs(popupContent));

  nsCOMPtr<nsPresContext> presContext;
  MenuHelpersX::DocShellToPresContext (docShell, getter_AddRefs(presContext) );
  if (presContext )  {
    nsresult rv = NS_OK;
    nsIContent* dispatchTo = popupContent ? popupContent : mMenuContent;
    rv = dispatchTo->DispatchDOMEvent(&event, nsnull, presContext, &status);

    mDestroyHandlerCalled = PR_TRUE;
    
    if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsMenuX::OnDestroyed()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDDEN, nsnull,
                     nsMouseEvent::eReal);
  
  nsCOMPtr<nsIDocShell>  docShell = do_QueryReferent(mDocShellWeakRef);
  if (!docShell) {
    NS_WARNING("No doc shell so can't run the OnDestroy");
    return PR_FALSE;
  }

  nsCOMPtr<nsIContent> popupContent;
  GetMenuPopupContent(getter_AddRefs(popupContent));

  nsCOMPtr<nsPresContext> presContext;
  MenuHelpersX::DocShellToPresContext (docShell, getter_AddRefs(presContext) );
  if (presContext )  {
    nsresult rv = NS_OK;
    nsIContent* dispatchTo = popupContent ? popupContent : mMenuContent;
    rv = dispatchTo->DispatchDOMEvent(&event, nsnull, presContext, &status);

    mDestroyHandlerCalled = PR_TRUE;
    
    if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
      return PR_FALSE;
  }
  return PR_TRUE;
}







void
nsMenuX::GetMenuPopupContent(nsIContent** aResult)
{
  if (!aResult )
    return;
  *aResult = nsnull;
  
  nsresult rv;
  nsCOMPtr<nsIXBLService> xblService = 
           do_GetService("@mozilla.org/xbl;1", &rv);
  if ( !xblService )
    return;
  
  PRUint32 count = mMenuContent->GetChildCount();

  for (PRUint32 i = 0; i < count; i++) {
    PRInt32 dummy;
    nsIContent *child = mMenuContent->GetChildAt(i);
    nsCOMPtr<nsIAtom> tag;
    xblService->ResolveTag(child, &dummy, getter_AddRefs(tag));
    if (tag == nsWidgetAtoms::menupopup) {
      *aResult = child;
      NS_ADDREF(*aResult);
      return;
    }
  }

} 









nsresult 
nsMenuX :: CountVisibleBefore ( PRUint32* outVisibleBefore )
{
  NS_ASSERTION ( outVisibleBefore, "bad index param" );
  
  nsCOMPtr<nsIMenuBar> menubarParent = do_QueryInterface(mParent);
  if (!menubarParent) return NS_ERROR_FAILURE;

  PRUint32 numMenus = 0;
  menubarParent->GetMenuCount(numMenus);
  
  
  PRBool gotThisMenu = PR_FALSE;
  *outVisibleBefore = 1;                            
  for ( PRUint32 i = 0; i < numMenus; ++i ) {
    nsCOMPtr<nsIMenu> currMenu;
    menubarParent->GetMenuAt(i, *getter_AddRefs(currMenu));
    
    
    if ( currMenu == static_cast<nsIMenu*>(this) ) {
      gotThisMenu = PR_TRUE;
      break;
    }
      
    
    
    if (currMenu) {
      nsCOMPtr<nsIContent> menuContent;
      currMenu->GetMenuContent(getter_AddRefs(menuContent));
      if ( menuContent ) {
        if ( menuContent->GetChildCount() > 0 ||
             !menuContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::hidden,
                                       nsWidgetAtoms::_true, eCaseMatters) &&
             !menuContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::collapsed,
                                       nsWidgetAtoms::_true, eCaseMatters))
          ++(*outVisibleBefore);
      }
    }
    
  } 

  return gotThisMenu ? NS_OK : NS_ERROR_FAILURE;

} 


NS_IMETHODIMP
nsMenuX::ChangeNativeEnabledStatusForMenuItem(nsIMenuItem* aMenuItem,
                                              PRBool aEnabled)
{
  MenuRef menuRef;
  PRUint16 menuItemIndex;
  nsresult rv = GetMenuRefAndItemIndexForMenuItem(aMenuItem,
                                                  (void**)&menuRef,
                                                  &menuItemIndex);
  if (NS_FAILED(rv)) return rv;

  if (aEnabled)
    ::EnableMenuItem(menuRef, menuItemIndex);
  else
    ::DisableMenuItem(menuRef, menuItemIndex);

  return NS_OK;
}


NS_IMETHODIMP
nsMenuX::GetMenuRefAndItemIndexForMenuItem(nsISupports* aMenuItem,
                                           void**       aMenuRef,
                                           PRUint16*    aMenuItemIndex)
{
  
  PRUint32 menuItemCount;
  mMenuItemsArray.Count(&menuItemCount);

  for (PRUint32 i = 0; i < menuItemCount; i++) {
    nsCOMPtr<nsISupports> currItem; 
    mMenuItemsArray.GetElementAt(i, getter_AddRefs(currItem));
    if (currItem == aMenuItem) {
      *aMenuRef = (void*)mMacMenuHandle;
      *aMenuItemIndex = i + 1;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}


#pragma mark -






NS_IMETHODIMP
nsMenuX::AttributeChanged(nsIDocument *aDocument, PRInt32 aNameSpaceID, nsIContent *aContent, nsIAtom *aAttribute)
{
  if (gConstructingMenu)
    return NS_OK;

  
  if ( aAttribute == nsWidgetAtoms::open )
    return NS_OK;
    
  nsCOMPtr<nsIMenuBar> menubarParent = do_QueryInterface(mParent);

  if (aAttribute == nsWidgetAtoms::disabled) {
    SetRebuild(PR_TRUE);
   
    if (mMenuContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::disabled,
                                  nsWidgetAtoms::_true, eCaseMatters))
      SetEnabled(PR_FALSE);
    else
      SetEnabled(PR_TRUE);
      
    ::DrawMenuBar();
  } 
  else if (aAttribute == nsWidgetAtoms::label) {
    SetRebuild(PR_TRUE);
    
    mMenuContent->GetAttr(kNameSpaceID_None, nsWidgetAtoms::label, mLabel);

    
    
    
    if ( !menubarParent ) {
      nsCOMPtr<nsIMenuListener> parentListener ( do_QueryInterface(mParent) );
      parentListener->SetRebuild(PR_TRUE);
    }
    else {
      
      NS_ASSERTION(mMacMenuHandle != NULL, "nsMenuX::AttributeChanged: invalid menu handle.");
      RemoveAll();
      CFStringRef titleRef = ::CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar*)mLabel.get(), mLabel.Length());
      NS_ASSERTION(titleRef, "nsMenuX::AttributeChanged: CFStringCreateWithCharacters failed.");
      ::SetMenuTitleWithCFString(mMacMenuHandle, titleRef);
      ::CFRelease(titleRef);
      
      ::DrawMenuBar();
    }
  }
  else if (aAttribute == nsWidgetAtoms::hidden || aAttribute == nsWidgetAtoms::collapsed) {
    SetRebuild(PR_TRUE);
      
      if (mMenuContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::hidden,
                                    nsWidgetAtoms::_true, eCaseMatters) ||
          mMenuContent->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::collapsed,
                                    nsWidgetAtoms::_true, eCaseMatters)) {
        if ( mVisible ) {
          if ( menubarParent ) {
            PRUint32 indexToRemove = 0;
            if ( NS_SUCCEEDED(CountVisibleBefore(&indexToRemove)) ) {
              ++indexToRemove;                
              void *clientData = nsnull;
              menubarParent->GetNativeData ( clientData );
              if ( clientData ) {
                MenuRef menubar = reinterpret_cast<MenuRef>(clientData);
                ::SetMenuItemHierarchicalMenu(menubar, indexToRemove, nsnull);
                ::DeleteMenuItem(menubar, indexToRemove);
                mVisible = PR_FALSE;
              }
            }
          } 
          else {
            
            NS_ASSERTION(PR_FALSE, "nsMenuX::AttributeChanged: WRITE HIDE CODE FOR SUBMENU.");
          }
        } 
        else
          NS_WARNING("You're hiding the menu twice, please stop");
      } 
      else {
        if ( !mVisible ) {
          if ( menubarParent ) {
            PRUint32 insertAfter = 0;
            if ( NS_SUCCEEDED(CountVisibleBefore(&insertAfter)) ) {
              void *clientData = nsnull;
              menubarParent->GetNativeData ( clientData );
              if ( clientData ) {
                MenuRef menubar = reinterpret_cast<MenuRef>(clientData);
                
                
                
                ::InsertMenuItem(menubar, "\pPlaceholder", insertAfter);
                ::SetMenuItemHierarchicalMenu(menubar, insertAfter + 1, mMacMenuHandle);  
                mVisible = PR_TRUE;
              }
            }
          } 
          else {
            
            NS_ASSERTION(PR_FALSE, "nsMenuX::AttributeChanged: WRITE SHOW CODE FOR SUBMENU.");
          }
        } 
      } 

      if (menubarParent) {
        ::DrawMenuBar();
      }
  }
  else if (aAttribute == nsWidgetAtoms::image) {
    SetupIcon();
  }

  return NS_OK;
  
} 


NS_IMETHODIMP
nsMenuX :: ContentRemoved(nsIDocument *aDocument, nsIContent *aChild, PRInt32 aIndexInContainer)
{  
  if (gConstructingMenu)
    return NS_OK;

    SetRebuild(PR_TRUE);

  RemoveItem(aIndexInContainer);
  mManager->Unregister(aChild);

  return NS_OK;
  
} 


NS_IMETHODIMP
nsMenuX :: ContentInserted(nsIDocument *aDocument, nsIContent *aChild, PRInt32 aIndexInContainer)
{  
  if(gConstructingMenu)
    return NS_OK;

    SetRebuild(PR_TRUE);
  
  return NS_OK;
  
} 


NS_IMETHODIMP
nsMenuX::SetupIcon()
{
  
  

  if (!mIcon) return NS_ERROR_OUT_OF_MEMORY;

  return mIcon->SetupIcon();
}
