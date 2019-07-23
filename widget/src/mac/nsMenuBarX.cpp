




































#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsINameSpaceManager.h"
#include "nsIMenu.h"
#include "nsIMenuItem.h"
#include "nsIContent.h"

#include "nsMenuBarX.h"
#include "nsMenuX.h"

#include "nsISupports.h"
#include "nsIWidget.h"
#include "nsString.h"
#include "nsIStringBundle.h"
#include "nsIDocument.h"
#include "nsIDocShell.h"
#include "nsIDocumentViewer.h"
#include "nsIMutationObserver.h"

#include "nsIDOMDocument.h"
#include "nsWidgetAtoms.h"

#include "nsMacResources.h"

#include "nsGUIEvent.h"


#include "nsWidgetsCID.h"
static NS_DEFINE_CID(kMenuCID, NS_MENU_CID);

NS_IMPL_ISUPPORTS6(nsMenuBarX, nsIMenuBar, nsIMenuListener, nsIMutationObserver, 
                    nsIChangeManager, nsIMenuCommandDispatcher, nsISupportsWeakReference)

MenuRef nsMenuBarX::sAppleMenu = nsnull;
EventHandlerUPP nsMenuBarX::sCommandEventHandler = nsnull;





nsMenuBarX::nsMenuBarX()
  : mNumMenus(0), mParent(nsnull), mIsMenuBarAdded(PR_FALSE), mCurrentCommandID(1), mDocument(nsnull)
{
  OSStatus status = ::CreateNewMenu(0, 0, &mRootMenu);
  NS_ASSERTION(status == noErr, "nsMenuBarX::nsMenuBarX:  creation of root menu failed.");
  
  
  if ( !sCommandEventHandler )
    sCommandEventHandler = ::NewEventHandlerUPP(CommandEventHandler);
}




nsMenuBarX::~nsMenuBarX()
{
  mMenusArray.Clear();    

  
  if ( mDocument ) {
    mDocument->RemoveMutationObserver(this);
  }

  if ( mRootMenu )
    ::ReleaseMenu(mRootMenu);
}

nsEventStatus 
nsMenuBarX::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

  PRUint32  numItems;
  mMenusArray.Count(&numItems);
  
  for (PRUint32 i = numItems; i > 0; --i) {
    nsCOMPtr<nsISupports>     menuSupports = getter_AddRefs(mMenusArray.ElementAt(i - 1));
    nsCOMPtr<nsIMenuListener> menuListener = do_QueryInterface(menuSupports);
    if (menuListener) {
      eventStatus = menuListener->MenuItemSelected(aMenuEvent);
      if (nsEventStatus_eIgnore != eventStatus)
        return eventStatus;
    }
  }
  return eventStatus;
}


nsEventStatus 
nsMenuBarX::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

  
  
  PRUint32  numItems;
  mMenusArray.Count(&numItems);
  for (PRUint32 i = numItems; i > 0; --i)
  {
    nsCOMPtr<nsISupports>     menuSupports = getter_AddRefs(mMenusArray.ElementAt(i - 1));
    nsCOMPtr<nsIMenuListener> thisListener = do_QueryInterface(menuSupports);
    if (thisListener)
    {
      
      
      eventStatus = thisListener->MenuItemSelected(aMenuEvent);
      if(nsEventStatus_eIgnore != eventStatus)
        return eventStatus;
    }
  }
  
  return eventStatus;
}


nsEventStatus 
nsMenuBarX::MenuDeselected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus 
nsMenuBarX::CheckRebuild(PRBool & aNeedsRebuild)
{
  aNeedsRebuild = PR_TRUE;
  return nsEventStatus_eIgnore;
}

nsEventStatus
nsMenuBarX::SetRebuild(PRBool aNeedsRebuild)
{
  return nsEventStatus_eIgnore;
}

void
nsMenuBarX :: GetDocument ( nsIDocShell* inDocShell, nsIDocument** outDocument )
{
  *outDocument = nsnull;
  
  if ( inDocShell ) {
    nsCOMPtr<nsIContentViewer> cv;
    inDocShell->GetContentViewer(getter_AddRefs(cv));
    if (cv) {
      
      nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
      if (!docv)
        return;
      docv->GetDocument(outDocument);    
    }
  }
}





void
nsMenuBarX :: RegisterAsDocumentObserver ( nsIDocShell* inDocShell )
{
  nsCOMPtr<nsIDocument> doc;
  GetDocument(inDocShell, getter_AddRefs(doc));
  if (!doc)
    return;

  
  doc->AddMutationObserver(this);
  
  
  mDocument = doc;
} 









void
nsMenuBarX :: AquifyMenuBar ( )
{
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(mMenuBarContent->GetDocument()));
  if ( domDoc ) {
    
    HideItem(domDoc, NS_LITERAL_STRING("menu_FileQuitSeparator"), nsnull);
    HideItem(domDoc, NS_LITERAL_STRING("menu_FileQuitItem"), getter_AddRefs(mQuitItemContent));

    
    
    HideItem(domDoc, NS_LITERAL_STRING("menu_PrefsSeparator"), nsnull);
    HideItem(domDoc, NS_LITERAL_STRING("menu_preferences"), getter_AddRefs(mPrefItemContent));
    if (mPrefItemContent)
      ::EnableMenuCommand(NULL, kHICommandPreferences);

    
    
    HideItem(domDoc, NS_LITERAL_STRING("menu_mac_services"), nsnull);
    HideItem(domDoc, NS_LITERAL_STRING("menu_mac_hide_app"), nsnull);
    HideItem(domDoc, NS_LITERAL_STRING("menu_mac_hide_others"), nsnull);
    HideItem(domDoc, NS_LITERAL_STRING("menu_mac_show_all"), nsnull);
  }
      
} 









OSStatus
nsMenuBarX :: InstallCommandEventHandler ( )
{
  OSStatus err = noErr;
  
  WindowRef myWindow = NS_REINTERPRET_CAST(WindowRef, mParent->GetNativeData(NS_NATIVE_DISPLAY));
  NS_ASSERTION ( myWindow, "Can't get WindowRef to install command handler!" );
  if ( myWindow && sCommandEventHandler ) {
    const EventTypeSpec commandEventList[] = {
     {kEventClassCommand, kEventCommandProcess},
    };
    err = ::InstallWindowEventHandler(myWindow,
                                      sCommandEventHandler,
                                      GetEventTypeCount(commandEventList),
                                      commandEventList,
                                      this,
                                      NULL);
    NS_ASSERTION ( err == noErr, "Uh oh, command handler not installed" );
  }

  return err;
  
} 







pascal OSStatus
nsMenuBarX :: CommandEventHandler ( EventHandlerCallRef inHandlerChain, EventRef inEvent, void* userData )
{
  OSStatus handled = eventNotHandledErr;

  HICommand command;
  OSErr err1 = ::GetEventParameter ( inEvent, kEventParamDirectObject, typeHICommand,
                                        NULL, sizeof(HICommand), NULL, &command );	
  if ( err1 )
    return handled;

  NS_ASSERTION(::GetEventKind(inEvent) == kEventCommandProcess,
               "CommandEventHandler asked to handle unknown event kind");

  nsMenuBarX* self = NS_REINTERPRET_CAST(nsMenuBarX*, userData);
  switch (command.commandID) {
    case kHICommandPreferences: {
      nsEventStatus status = self->ExecuteCommand(self->mPrefItemContent);
      if (status == nsEventStatus_eConsumeNoDefault)
        
        handled = noErr;
      break;
    }

    case kHICommandQuit: {
      nsEventStatus status = self->ExecuteCommand(self->mQuitItemContent);
      if (status == nsEventStatus_eConsumeNoDefault)
        
        handled = noErr;
      break;
    }

    case kHICommandAbout: {
      
      
      
      
      nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(self->mDocument);
      if (domDoc) {
        nsCOMPtr<nsIDOMElement> domElement;
        domDoc->GetElementById(NS_LITERAL_STRING("aboutName"),
                               getter_AddRefs(domElement));
        nsCOMPtr<nsIContent> aboutContent(do_QueryInterface(domElement));
        nsEventStatus status = self->ExecuteCommand(aboutContent);
        if (status == nsEventStatus_eConsumeNoDefault)
          
          handled = noErr;
      }
      break;
    }

    default: {
      
      
      
      nsPRUint32Key key(command.commandID);
      nsIMenuItem* content = NS_REINTERPRET_CAST(nsIMenuItem*,
                                               self->mObserverTable.Get(&key));
      if (content) {
        content->DoCommand();
        handled = noErr;
      }
      break;
    }
  }
  
  return handled;
  
} 








nsEventStatus
nsMenuBarX :: ExecuteCommand ( nsIContent* inDispatchTo )
{
  if (!inDispatchTo)
    return nsEventStatus_eIgnore;

  return MenuHelpersX::DispatchCommandTo(mDocShellWeakRef, inDispatchTo);
} 









void
nsMenuBarX :: HideItem ( nsIDOMDocument* inDoc, const nsAString & inID, nsIContent** outHiddenNode )
{
  nsCOMPtr<nsIDOMElement> menuItem;
  inDoc->GetElementById(inID, getter_AddRefs(menuItem));  
  nsCOMPtr<nsIContent> menuContent ( do_QueryInterface(menuItem) );
  if ( menuContent ) {
    menuContent->SetAttr ( kNameSpaceID_None, nsWidgetAtoms::hidden, NS_LITERAL_STRING("true"), PR_FALSE );
    if ( outHiddenNode ) {
      *outHiddenNode = menuContent.get();
      NS_IF_ADDREF(*outHiddenNode);
    }
  }

} 


nsEventStatus
nsMenuBarX::MenuConstruct( const nsMenuEvent & aMenuEvent, nsIWidget* aParentWindow, 
                            void * menubarNode, void * aDocShell )
{
  mDocShellWeakRef = do_GetWeakReference(NS_STATIC_CAST(nsIDocShell*, aDocShell));
  nsIDOMNode* aDOMNode  = NS_STATIC_CAST(nsIDOMNode*, menubarNode);
  mMenuBarContent = do_QueryInterface(aDOMNode);           
  NS_ASSERTION ( mMenuBarContent, "No content specified for this menubar" );
  if ( !mMenuBarContent )
    return nsEventStatus_eIgnore;
    
  Create(aParentWindow);
  
  
  AquifyMenuBar();
  
  OSStatus err = InstallCommandEventHandler();
  if ( err )
    return nsEventStatus_eIgnore;

  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocShellWeakRef);
  if (docShell) RegisterAsDocumentObserver(docShell);

  
  aParentWindow->AddMenuListener((nsIMenuListener *)this);

  PRUint32 count = mMenuBarContent->GetChildCount();
  for ( PRUint32 i = 0; i < count; ++i ) { 
    nsIContent *menu = mMenuBarContent->GetChildAt(i);
    if ( menu ) {
      if (menu->Tag() == nsWidgetAtoms::menu &&
          menu->IsNodeOfType(nsINode::eXUL)) {
        nsAutoString menuName;
        nsAutoString menuAccessKey(NS_LITERAL_STRING(" "));
        menu->GetAttr(kNameSpaceID_None, nsWidgetAtoms::label, menuName);
        menu->GetAttr(kNameSpaceID_None, nsWidgetAtoms::accesskey, menuAccessKey);
			  
        
              
        
        nsCOMPtr<nsIMenu> pnsMenu ( do_CreateInstance(kMenuCID) );
        if ( pnsMenu ) {
          pnsMenu->Create(NS_STATIC_CAST(nsIMenuBar*, this), menuName, menuAccessKey, 
                          NS_STATIC_CAST(nsIChangeManager *, this), 
                          NS_REINTERPRET_CAST(nsIDocShell*, aDocShell), menu);

          
          AddMenu(pnsMenu); 
                  
          if (menu->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::id,
                                NS_LITERAL_STRING("menu_Help"), eCaseMatters)) {
            nsMenuEvent event(PR_TRUE, 0, nsnull);
            nsCOMPtr<nsIMenuListener> listener(do_QueryInterface(pnsMenu));
            listener->MenuSelected(event);
          }          
        }
      } 
    }
  } 

  
  
  aParentWindow->SetMenuBar(this);

  return nsEventStatus_eIgnore;
}


nsEventStatus 
nsMenuBarX::MenuDestruct(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}







NS_METHOD nsMenuBarX::Create(nsIWidget *aParent)
{
  SetParent(aParent);
  return NS_OK;
}


NS_METHOD nsMenuBarX::GetParent(nsIWidget *&aParent)
{
  NS_IF_ADDREF(aParent = mParent);
  return NS_OK;
}



NS_METHOD nsMenuBarX::SetParent(nsIWidget *aParent)
{
  mParent = aParent;    
  return NS_OK;
}


NS_METHOD nsMenuBarX::AddMenu(nsIMenu * aMenu)
{
  
  mMenusArray.AppendElement(aMenu);    

  if (mNumMenus == 0) {
    
    if ( !sAppleMenu ) {
      nsresult rv = CreateAppleMenu(aMenu);
      NS_ASSERTION ( NS_SUCCEEDED(rv), "Can't create Apple menu" );
    }
    
    
    if ( sAppleMenu ) {
      
      
      
      mNumMenus = 1;
      ::InsertMenuItem(mRootMenu, "\pA", mNumMenus);
      OSStatus status = ::SetMenuItemHierarchicalMenu(mRootMenu, 1, sAppleMenu);
			NS_ASSERTION(status == noErr, "OS problem with SetMenuItemHierarchicalMenu");
    }
  }

  MenuRef menuRef = nsnull;
  aMenu->GetNativeData((void**)&menuRef);

  nsCOMPtr<nsIContent> menu;
  aMenu->GetMenuContent(getter_AddRefs(menu));
  if(!menu->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::hidden,
                        nsWidgetAtoms::_true, eCaseMatters) &&
     menu->GetChildCount() > 0) {
    
    
    mNumMenus++;
    
    ::InsertMenuItem(mRootMenu, "\pPlaceholder", mNumMenus);
    OSStatus status = ::SetMenuItemHierarchicalMenu(mRootMenu, mNumMenus, menuRef);
    NS_ASSERTION(status == noErr, "nsMenuBarX::AddMenu: SetMenuItemHierarchicalMenu failed.");
  }

  return NS_OK;
}







nsresult
nsMenuBarX :: CreateAppleMenu ( nsIMenu* inMenu )
{
  Str32 menuStr = { 1, kMenuAppleLogoFilledGlyph };
  OSStatus s = ::CreateNewMenu(kAppleMenuID, 0, &sAppleMenu);

  if ( s == noErr && sAppleMenu )  {
    ::SetMenuTitle(sAppleMenu, menuStr);
    
    
    
    nsAutoString label;
    nsCOMPtr<nsIContent> menu;
    inMenu->GetMenuContent(getter_AddRefs(menu));
    if (menu) {
      nsCOMPtr<nsIDocument> doc = menu->GetDocument();
      if (doc) {
        nsCOMPtr<nsIDOMDocument> domdoc ( do_QueryInterface(doc) );
        if ( domdoc ) {
          nsCOMPtr<nsIDOMElement> aboutMenuItem;
          domdoc->GetElementById(NS_LITERAL_STRING("aboutName"), getter_AddRefs(aboutMenuItem));
          if (aboutMenuItem)
            aboutMenuItem->GetAttribute(NS_LITERAL_STRING("label"), label);
        }
      }
    }

    CFStringRef labelRef = ::CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar*)label.get(), label.Length());
    if ( labelRef ) {
      ::InsertMenuItemTextWithCFString(sAppleMenu, labelRef, 1, 0, 0);
      ::CFRelease(labelRef);
    }
    
    ::SetMenuItemCommandID(sAppleMenu, 1, kHICommandAbout);

    ::AppendMenu(sAppleMenu, "\p-");
  }

  return (s == noErr && sAppleMenu) ? NS_OK : NS_ERROR_FAILURE;
}

        

NS_METHOD nsMenuBarX::GetMenuCount(PRUint32 &aCount)
{
  aCount = mNumMenus;
  return NS_OK;
}


NS_METHOD nsMenuBarX::GetMenuAt(const PRUint32 aCount, nsIMenu *& aMenu)
{ 
  aMenu = NULL;
  nsCOMPtr<nsISupports> supports = getter_AddRefs(mMenusArray.ElementAt(aCount));
  if (!supports) return NS_OK;
  
  return CallQueryInterface(supports, &aMenu); 
}


NS_METHOD nsMenuBarX::InsertMenuAt(const PRUint32 aCount, nsIMenu *& aMenu)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD nsMenuBarX::RemoveMenu(const PRUint32 aCount)
{
  mMenusArray.RemoveElementAt(aCount);
  ::DeleteMenuItem(mRootMenu, aCount + 1);    
  ::DrawMenuBar();
  return NS_OK;
}


NS_METHOD nsMenuBarX::RemoveAll()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD nsMenuBarX::GetNativeData(void *& aData)
{
  aData = (void *) mRootMenu;
  return NS_OK;
}


NS_METHOD nsMenuBarX::SetNativeData(void* aData)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_METHOD nsMenuBarX::Paint()
{
    
    
    ::SetRootMenu(mRootMenu);
    ::DrawMenuBar();
    return NS_OK;
}

#pragma mark -






void
nsMenuBarX::CharacterDataChanged( nsIDocument * aDocument,
                                  nsIContent * aContent,
                                  CharacterDataChangeInfo * aInfo)
{
}

void
nsMenuBarX::ContentAppended( nsIDocument * aDocument, nsIContent  * aContainer,
                              PRInt32 aNewIndexInContainer)
{
  if (aContainer != mMenuBarContent) {
    nsCOMPtr<nsIChangeObserver> obs;
    Lookup(aContainer, getter_AddRefs(obs));
    if (obs)
      obs->ContentInserted(aDocument, aContainer, aNewIndexInContainer);
    else {
      nsCOMPtr<nsIContent> parent = aContainer->GetParent();
      if (parent) {
        Lookup(parent, getter_AddRefs(obs));
        if (obs)
          obs->ContentInserted(aDocument, aContainer, aNewIndexInContainer);
      }
    }
  }
}

void
nsMenuBarX::NodeWillBeDestroyed(const nsINode* aNode)
{
  mDocument = nsnull;
}


void
nsMenuBarX::AttributeChanged( nsIDocument * aDocument, nsIContent * aContent,
                              PRInt32 aNameSpaceID, nsIAtom * aAttribute,
                              PRInt32 aModType )
{
  
  nsCOMPtr<nsIChangeObserver> obs;
  Lookup(aContent, getter_AddRefs(obs));
  if (obs)
    obs->AttributeChanged(aDocument, aNameSpaceID, aContent, aAttribute);
}

void
nsMenuBarX::ContentRemoved( nsIDocument * aDocument, nsIContent * aContainer,
                            nsIContent * aChild, PRInt32 aIndexInContainer )
{  
  if ( aContainer == mMenuBarContent ) {
    Unregister(aChild);
    RemoveMenu ( aIndexInContainer );
  }
  else {
    nsCOMPtr<nsIChangeObserver> obs;
    Lookup ( aContainer, getter_AddRefs(obs) );
    if ( obs )
      obs->ContentRemoved ( aDocument, aChild, aIndexInContainer );
    else {
      nsCOMPtr<nsIContent> parent = aContainer->GetParent();
      if(parent) {
        Lookup ( parent, getter_AddRefs(obs) );
        if ( obs )
          obs->ContentRemoved ( aDocument, aChild, aIndexInContainer );
      }
    }
  }
}

void
nsMenuBarX::ContentInserted( nsIDocument * aDocument, nsIContent * aContainer,
                             nsIContent * aChild, PRInt32 aIndexInContainer )
{  
  if (aContainer != mMenuBarContent) {
    nsCOMPtr<nsIChangeObserver> obs;
    Lookup (aContainer, getter_AddRefs(obs));
    if (obs)
      obs->ContentInserted(aDocument, aChild, aIndexInContainer);
    else {
      nsCOMPtr<nsIContent> parent = aContainer->GetParent();
      if (parent) {
        Lookup(parent, getter_AddRefs(obs));
        if (obs)
          obs->ContentInserted(aDocument, aChild, aIndexInContainer);
      }
    }
  }
}

#pragma mark - 









NS_IMETHODIMP 
nsMenuBarX :: Register ( nsIContent *aContent, nsIChangeObserver *aMenuObject )
{
  nsVoidKey key ( aContent );
  mObserverTable.Put ( &key, aMenuObject );
  
  return NS_OK;
}


NS_IMETHODIMP 
nsMenuBarX :: Unregister ( nsIContent *aContent )
{
  nsVoidKey key ( aContent );
  mObserverTable.Remove ( &key );
  
  return NS_OK;
}


NS_IMETHODIMP 
nsMenuBarX :: Lookup ( nsIContent *aContent, nsIChangeObserver **_retval )
{
  *_retval = nsnull;
  
  nsVoidKey key ( aContent );
  *_retval = NS_REINTERPRET_CAST(nsIChangeObserver*, mObserverTable.Get(&key));
  NS_IF_ADDREF ( *_retval );
  
  return NS_OK;
}


#pragma mark -













NS_IMETHODIMP 
nsMenuBarX :: Register ( nsIMenuItem* inMenuItem, PRUint32* outCommandID )
{
  
  
  
  
  
  
  nsPRUint32Key key ( mCurrentCommandID );
  mObserverTable.Put ( &key, inMenuItem );
  *outCommandID = mCurrentCommandID;

  
  ++mCurrentCommandID;
  
  return NS_OK;
}








NS_IMETHODIMP 
nsMenuBarX :: Unregister ( PRUint32 inCommandID )
{
  nsPRUint32Key key ( inCommandID );
  mObserverTable.Remove ( &key );

  return NS_OK;
}


#pragma mark -








nsresult
MenuHelpersX::DocShellToPresContext (nsIDocShell* inDocShell, nsPresContext** outContext )
{
  NS_ENSURE_ARG_POINTER(outContext);
  *outContext = nsnull;
  if (!inDocShell)
    return NS_ERROR_INVALID_ARG;
  
  nsresult retval = NS_OK;
  
  nsCOMPtr<nsIContentViewer> contentViewer;
  inDocShell->GetContentViewer(getter_AddRefs(contentViewer));
  if ( contentViewer ) {
    nsCOMPtr<nsIDocumentViewer> docViewer ( do_QueryInterface(contentViewer) );
    if ( docViewer )
      docViewer->GetPresContext(outContext);     
    else
      retval = NS_ERROR_FAILURE;
  }
  else
    retval = NS_ERROR_FAILURE;
  
  return retval;
  
} 

nsEventStatus
MenuHelpersX::DispatchCommandTo(nsIWeakReference* aDocShellWeakRef,
                                nsIContent* aTargetContent)
{
  NS_PRECONDITION(aTargetContent, "null ptr");

  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(aDocShellWeakRef);
  if (!docShell)
    return nsEventStatus_eConsumeNoDefault;
  nsCOMPtr<nsPresContext> presContext;
  MenuHelpersX::DocShellToPresContext(docShell, getter_AddRefs(presContext));

  nsEventStatus status = nsEventStatus_eConsumeNoDefault;
  nsXULCommandEvent event(PR_TRUE, NS_XUL_COMMAND, nsnull);

  
  

  aTargetContent->DispatchDOMEvent(&event, nsnull, presContext, &status);
  return status;
}
