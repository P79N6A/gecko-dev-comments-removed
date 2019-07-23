






































#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIXBLDocumentInfo.h"
#include "nsIInputStream.h"
#include "nsINameSpaceManager.h"
#include "nsHashtable.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIDOMEventTarget.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsNetUtil.h"
#include "plstr.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif
#include "nsIXMLContentSink.h"
#include "nsContentCID.h"
#include "nsXMLDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "jsapi.h"
#include "nsXBLService.h"
#include "nsXBLInsertionPoint.h"
#include "nsIXPConnect.h"
#include "nsIScriptContext.h"
#include "nsCRT.h"


#include "nsIEventListenerManager.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMXULListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMEventGroup.h"
#include "nsAttrName.h"

#include "nsGkAtoms.h"

#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"

#include "nsXBLPrototypeHandler.h"

#include "nsXBLPrototypeBinding.h"
#include "nsXBLBinding.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIEventListenerManager.h"
#include "nsGUIEvent.h"

#include "prprf.h"
#include "nsNodeUtils.h"







PR_STATIC_CALLBACK(void)
XBLFinalize(JSContext *cx, JSObject *obj)
{
  nsXBLJSClass* c = NS_STATIC_CAST(nsXBLJSClass*, ::JS_GetClass(cx, obj));
  c->Drop();
}

nsXBLJSClass::nsXBLJSClass(const nsAFlatCString& aClassName)
{
  memset(this, 0, sizeof(nsXBLJSClass));
  next = prev = NS_STATIC_CAST(JSCList*, this);
  name = ToNewCString(aClassName);
  addProperty = delProperty = setProperty = getProperty = ::JS_PropertyStub;
  enumerate = ::JS_EnumerateStub;
  resolve = ::JS_ResolveStub;
  convert = ::JS_ConvertStub;
  finalize = XBLFinalize;
}

nsrefcnt
nsXBLJSClass::Destroy()
{
  NS_ASSERTION(next == prev && prev == NS_STATIC_CAST(JSCList*, this),
               "referenced nsXBLJSClass is on LRU list already!?");

  if (nsXBLService::gClassTable) {
    nsCStringKey key(name);
    (nsXBLService::gClassTable)->Remove(&key);
  }

  if (nsXBLService::gClassLRUListLength >= nsXBLService::gClassLRUListQuota) {
    
    delete this;
  } else {
    
    JSCList* mru = NS_STATIC_CAST(JSCList*, this);
    JS_APPEND_LINK(mru, &nsXBLService::gClassLRUList);
    nsXBLService::gClassLRUListLength++;
  }

  return 0;
}




nsXBLBinding::nsXBLBinding(nsXBLPrototypeBinding* aBinding)
  : mPrototypeBinding(aBinding),
    mInsertionPointTable(nsnull),
    mIsStyleBinding(PR_TRUE),
    mMarkedForDeath(PR_FALSE)
{
  NS_ASSERTION(mPrototypeBinding, "Must have a prototype binding!");
  
  NS_ADDREF(mPrototypeBinding->XBLDocumentInfo());
}


nsXBLBinding::~nsXBLBinding(void)
{
  delete mInsertionPointTable;
  nsIXBLDocumentInfo* info = mPrototypeBinding->XBLDocumentInfo();
  NS_RELEASE(info);
}

PR_STATIC_CALLBACK(PLDHashOperator)
TraverseKey(nsISupports* aKey, nsInsertionPointList* aData, void* aClosure)
{
  nsCycleCollectionTraversalCallback &cb = 
    *NS_STATIC_CAST(nsCycleCollectionTraversalCallback*, aClosure);

  cb.NoteXPCOMChild(aKey);
  if (aData) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY(*aData, nsXBLInsertionPoint)
  }
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLBinding)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(nsXBLBinding)
  
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContent)
  
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(nsXBLBinding)
  cb.NoteXPCOMChild(tmp->mPrototypeBinding->XBLDocumentInfo());
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mNextBinding, nsXBLBinding)
  if (tmp->mInsertionPointTable)
    tmp->mInsertionPointTable->EnumerateRead(TraverseKey, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsXBLBinding, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsXBLBinding, Release)

void
nsXBLBinding::SetBaseBinding(nsXBLBinding* aBinding)
{
  if (mNextBinding) {
    NS_ERROR("Base XBL binding is already defined!");
    return;
  }

  mNextBinding = aBinding; 
}

void
nsXBLBinding::InstallAnonymousContent(nsIContent* aAnonParent, nsIContent* aElement)
{
  
  
  
  
  
  
  
  
  
  nsIDocument* doc = aElement->GetCurrentDoc();
  PRBool allowScripts = AllowScripts();

  PRUint32 childCount = aAnonParent->GetChildCount();
  for (PRUint32 i = 0; i < childCount; i++) {
    nsIContent *child = aAnonParent->GetChildAt(i);
    child->UnbindFromTree();
    nsresult rv =
      child->BindToTree(doc, aElement, mBoundElement, allowScripts);
    if (NS_FAILED(rv)) {
      
      
      child->UnbindFromTree();
      return;
    }        

#ifdef MOZ_XUL
    
    
    
    nsCOMPtr<nsIXULDocument> xuldoc(do_QueryInterface(doc));
    if (xuldoc)
      xuldoc->AddSubtreeToDocument(child);
#endif
  }
}

void
nsXBLBinding::SetBoundElement(nsIContent* aElement)
{
  mBoundElement = aElement;
  if (mNextBinding)
    mNextBinding->SetBoundElement(aElement);
}

nsXBLBinding*
nsXBLBinding::GetFirstBindingWithConstructor()
{
  if (mPrototypeBinding->GetConstructor())
    return this;

  if (mNextBinding)
    return mNextBinding->GetFirstBindingWithConstructor();

  return nsnull;
}

PRBool
nsXBLBinding::HasStyleSheets() const
{
  
  
  if (mPrototypeBinding->HasStyleSheets())
    return PR_TRUE;

  return mNextBinding ? mNextBinding->HasStyleSheets() : PR_FALSE;
}

struct EnumData {
  nsXBLBinding* mBinding;
 
  EnumData(nsXBLBinding* aBinding)
    :mBinding(aBinding)
  {}
};

struct ContentListData : public EnumData {
  nsBindingManager* mBindingManager;
  nsresult          mRv;

  ContentListData(nsXBLBinding* aBinding, nsBindingManager* aManager)
    :EnumData(aBinding), mBindingManager(aManager), mRv(NS_OK)
  {}
};

PR_STATIC_CALLBACK(PLDHashOperator)
BuildContentLists(nsISupports* aKey,
                  nsAutoPtr<nsInsertionPointList>& aData,
                  void* aClosure)
{
  ContentListData* data = (ContentListData*)aClosure;
  nsBindingManager* bm = data->mBindingManager;
  nsXBLBinding* binding = data->mBinding;

  nsIContent *boundElement = binding->GetBoundElement();

  PRInt32 count = aData->Length();
  
  if (count == 0)
    return PL_DHASH_NEXT;

  
  
  nsInsertionPointList* contentList = new nsInsertionPointList;
  if (!contentList) {
    data->mRv = NS_ERROR_OUT_OF_MEMORY;
    return PL_DHASH_STOP;
  }

  
  nsXBLInsertionPoint* currPoint = aData->ElementAt(0);
  nsCOMPtr<nsIContent> parent = currPoint->GetInsertionParent();
  PRInt32 currIndex = currPoint->GetInsertionIndex();

  nsCOMPtr<nsIDOMNodeList> nodeList;
  if (parent == boundElement) {
    
    nodeList = binding->GetAnonymousNodes();
  }
  else {
    
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(parent));
    node->GetChildNodes(getter_AddRefs(nodeList));
  }

  nsXBLInsertionPoint* pseudoPoint = nsnull;
  PRUint32 childCount;
  nodeList->GetLength(&childCount);
  PRInt32 j = 0;

  for (PRUint32 i = 0; i < childCount; i++) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(i, getter_AddRefs(node));
    nsCOMPtr<nsIContent> child(do_QueryInterface(node));
    if (((PRInt32)i) == currIndex) {
      
      contentList->AppendElement(currPoint);

      
      j++;
      if (j < count) {
        currPoint = aData->ElementAt(j);
        currIndex = currPoint->GetInsertionIndex();
      }

      
      pseudoPoint = nsnull;
    }
    
    if (!pseudoPoint) {
      pseudoPoint = new nsXBLInsertionPoint(parent, (PRUint32) -1, nsnull);
      if (pseudoPoint) {
        contentList->AppendElement(pseudoPoint);
      }
    }
    if (pseudoPoint) {
      pseudoPoint->AddChild(child);
    }
  }

  
  contentList->AppendElements(aData->Elements() + j, count - j);
  
  
  
  
  
  if (parent == boundElement)
    bm->SetAnonymousNodesFor(parent, contentList);
  else 
    bm->SetContentListFor(parent, contentList);
  return PL_DHASH_NEXT;
}

PR_STATIC_CALLBACK(PLDHashOperator)
RealizeDefaultContent(nsISupports* aKey,
                      nsAutoPtr<nsInsertionPointList>& aData,
                      void* aClosure)
{
  ContentListData* data = (ContentListData*)aClosure;
  nsBindingManager* bm = data->mBindingManager;
  nsXBLBinding* binding = data->mBinding;

  PRInt32 count = aData->Length();
 
  for (PRInt32 i = 0; i < count; i++) {
    nsXBLInsertionPoint* currPoint = aData->ElementAt(i);
    PRInt32 insCount = currPoint->ChildCount();
    
    if (insCount == 0) {
      nsCOMPtr<nsIContent> defContent = currPoint->GetDefaultContentTemplate();
      if (defContent) {
        
        
        
        nsCOMPtr<nsIContent> insParent = currPoint->GetInsertionParent();
        nsIDocument *document = insParent->GetOwnerDoc();
        if (!document) {
          data->mRv = NS_ERROR_FAILURE;
          return PL_DHASH_STOP;
        }

        nsCOMPtr<nsIDOMNode> clonedNode;
        nsCOMArray<nsINode> nodesWithProperties;
        nsNodeUtils::Clone(defContent, PR_TRUE, document->NodeInfoManager(),
                           nodesWithProperties, getter_AddRefs(clonedNode));

        
        
        nsCOMPtr<nsIContent> clonedContent(do_QueryInterface(clonedNode));
        binding->InstallAnonymousContent(clonedContent, insParent);

        
        
        currPoint->SetDefaultContent(clonedContent);

        
        
        PRUint32 cloneKidCount = clonedContent->GetChildCount();
        for (PRUint32 k = 0; k < cloneKidCount; k++) {
          nsIContent *cloneChild = clonedContent->GetChildAt(k);
          bm->SetInsertionParent(cloneChild, insParent);
          currPoint->AddChild(cloneChild);
        }
      }
    }
  }

  return PL_DHASH_NEXT;
}

PR_STATIC_CALLBACK(PLDHashOperator)
ChangeDocumentForDefaultContent(nsISupports* aKey,
                                nsAutoPtr<nsInsertionPointList>& aData,
                                void* aClosure)
{
  PRInt32 count = aData->Length();
  for (PRInt32 i = 0; i < count; i++) {
    nsXBLInsertionPoint* currPoint = aData->ElementAt(i);
    nsCOMPtr<nsIContent> defContent = currPoint->GetDefaultContent();
    if (defContent)
      defContent->UnbindFromTree();
  }

  return PL_DHASH_NEXT;
}

void
nsXBLBinding::GenerateAnonymousContent()
{
  
  nsIContent* content =
    mPrototypeBinding->GetImmediateChild(nsGkAtoms::content);

  if (!content) {
    
    if (mNextBinding)
      mNextBinding->GenerateAnonymousContent();

    return;
  }
     
  
  
  PRUint32 contentCount = content->GetChildCount();

  
  PRBool hasContent = (contentCount > 0);
  PRBool hasInsertionPoints = mPrototypeBinding->HasInsertionPoints();

#ifdef DEBUG
  
  if (nsContentUtils::HasNonEmptyAttr(content, kNameSpaceID_None,
                                      nsGkAtoms::includes)) {
    nsCAutoString message("An XBL Binding with URI ");
    nsCAutoString uri;
    mPrototypeBinding->BindingURI()->GetSpec(uri);
    message += uri;
    message += " is still using the deprecated\n<content includes=\"\"> syntax! Use <children> instead!\n"; 
    NS_WARNING(message.get());
  }
#endif

  if (hasContent || hasInsertionPoints) {
    nsIDocument* doc = mBoundElement->GetOwnerDoc();

    
    if (! doc)
      return;
    
    nsBindingManager *bindingManager = doc->BindingManager();

    nsCOMPtr<nsIDOMNodeList> children;
    bindingManager->GetContentListFor(mBoundElement, getter_AddRefs(children));
 
    nsCOMPtr<nsIDOMNode> node;
    nsCOMPtr<nsIContent> childContent;
    PRUint32 length;
    children->GetLength(&length);
    if (length > 0 && !hasInsertionPoints) {
      
      
      
      
      for (PRUint32 i = 0; i < length; i++) {
        children->Item(i, getter_AddRefs(node));
        childContent = do_QueryInterface(node);

        nsINodeInfo *ni = childContent->NodeInfo();
        nsIAtom *localName = ni->NameAtom();
        if (ni->NamespaceID() != kNameSpaceID_XUL ||
            (localName != nsGkAtoms::observes &&
             localName != nsGkAtoms::_template)) {
          hasContent = PR_FALSE;
          break;
        }
      }
    }

    if (hasContent || hasInsertionPoints) {
      nsIDocument *document = mBoundElement->GetOwnerDoc();
      if (!document) {
        return;
      }

      nsCOMPtr<nsIDOMNode> clonedNode;
      nsCOMArray<nsINode> nodesWithProperties;
      nsNodeUtils::Clone(content, PR_TRUE, document->NodeInfoManager(),
                         nodesWithProperties, getter_AddRefs(clonedNode));

      mContent = do_QueryInterface(clonedNode);
      InstallAnonymousContent(mContent, mBoundElement);

      if (hasInsertionPoints) {
        
        
      
        
        
        mPrototypeBinding->InstantiateInsertionPoints(this);

        
        
        
        
        
        
        ContentListData data(this, bindingManager);
        mInsertionPointTable->Enumerate(BuildContentLists, &data);
        if (NS_FAILED(data.mRv)) {
          return;
        }

        
        
        PRUint32 index = 0;
        PRBool multiplePoints = PR_FALSE;
        nsIContent *singlePoint = GetSingleInsertionPoint(&index,
                                                          &multiplePoints);
      
        if (children) {
          if (multiplePoints) {
            
            
            children->GetLength(&length);
            for (PRUint32 i = 0; i < length; i++) {
              children->Item(i, getter_AddRefs(node));
              childContent = do_QueryInterface(node);

              
              PRUint32 index;
              nsIContent *point = GetInsertionPoint(childContent, &index);
              bindingManager->SetInsertionParent(childContent, point);

              
              nsInsertionPointList* arr = nsnull;
              GetInsertionPointsFor(point, &arr);
              nsXBLInsertionPoint* insertionPoint = nsnull;
              PRInt32 arrCount = arr->Length();
              for (PRInt32 j = 0; j < arrCount; j++) {
                insertionPoint = arr->ElementAt(j);
                if (insertionPoint->Matches(point, index))
                  break;
                insertionPoint = nsnull;
              }

              if (insertionPoint) 
                insertionPoint->AddChild(childContent);
              else {
                
                

                nsINodeInfo *ni = childContent->NodeInfo();
                nsIAtom *localName = ni->NameAtom();
                if (ni->NamespaceID() != kNameSpaceID_XUL ||
                    (localName != nsGkAtoms::observes &&
                     localName != nsGkAtoms::_template)) {
                  
                  mContent = nsnull;
                  bindingManager->SetContentListFor(mBoundElement, nsnull);
                  bindingManager->SetAnonymousNodesFor(mBoundElement, nsnull);
                  return;
                }
              }
            }
          }
          else {
            
            nsInsertionPointList* arr = nsnull;
            GetInsertionPointsFor(singlePoint, &arr);
            nsXBLInsertionPoint* insertionPoint = arr->ElementAt(0);
        
            nsCOMPtr<nsIDOMNode> node;
            nsCOMPtr<nsIContent> content;
            PRUint32 length;
            children->GetLength(&length);
          
            for (PRUint32 i = 0; i < length; i++) {
              children->Item(i, getter_AddRefs(node));
              content = do_QueryInterface(node);
              bindingManager->SetInsertionParent(content, singlePoint);
              insertionPoint->AddChild(content);
            }
          }
        }

        
        
        
        mInsertionPointTable->Enumerate(RealizeDefaultContent, &data);
        if (NS_FAILED(data.mRv)) {
          return;
        }
      }
    }

    mPrototypeBinding->SetInitialAttributes(mBoundElement, mContent);
  }

  
  
  
  const nsAttrName* attrName;
  for (PRUint32 i = 0; (attrName = content->GetAttrNameAt(i)); ++i) {
    PRInt32 namespaceID = attrName->NamespaceID();
    nsIAtom* name = attrName->LocalName();

    if (name != nsGkAtoms::includes) {
      if (!nsContentUtils::HasNonEmptyAttr(mBoundElement, namespaceID, name)) {
        nsAutoString value2;
        content->GetAttr(namespaceID, name, value2);
        mBoundElement->SetAttr(namespaceID, name, attrName->GetPrefix(),
                               value2, PR_FALSE);
      }
    }

    
    if (mContent)
      mContent->UnsetAttr(namespaceID, name, PR_FALSE);
  }
}

void
nsXBLBinding::InstallEventHandlers()
{
  
  if (AllowScripts()) {
    
    nsXBLPrototypeHandler* handlerChain = mPrototypeBinding->GetPrototypeHandlers();

    if (handlerChain) {
      nsCOMPtr<nsIEventListenerManager> manager;
      mBoundElement->GetListenerManager(PR_TRUE, getter_AddRefs(manager));
      if (!manager)
        return;

      nsCOMPtr<nsIDOMEventGroup> systemEventGroup;
      PRBool isChromeDoc =
        nsContentUtils::IsChromeDoc(mBoundElement->GetOwnerDoc());
      nsXBLPrototypeHandler* curr;
      for (curr = handlerChain; curr; curr = curr->GetNextHandler()) {
        
        nsCOMPtr<nsIAtom> eventAtom = curr->GetEventName();
        if (!eventAtom ||
            eventAtom == nsGkAtoms::keyup ||
            eventAtom == nsGkAtoms::keydown ||
            eventAtom == nsGkAtoms::keypress)
          continue;

        nsAutoString type;
        eventAtom->ToString(type);

        
        

        
        
        nsIDOMEventGroup* eventGroup = nsnull;
        if (curr->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND | NS_HANDLER_TYPE_SYSTEM)) {
          if (!systemEventGroup)
            manager->GetSystemEventGroupLM(getter_AddRefs(systemEventGroup));
          eventGroup = systemEventGroup;
        }

        nsXBLEventHandler* handler = curr->GetEventHandler();
        if (handler) {
          
          PRInt32 flags = (curr->GetPhase() == NS_PHASE_CAPTURING) ?
            NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

          PRBool hasAllowUntrustedAttr = curr->HasAllowUntrustedAttr();
          if ((hasAllowUntrustedAttr && curr->AllowUntrustedEvents()) ||
              (!hasAllowUntrustedAttr && !isChromeDoc)) {
            flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
          }

          manager->AddEventListenerByType(handler, type, flags, eventGroup);
        }
      }

      const nsCOMArray<nsXBLKeyEventHandler>* keyHandlers =
        mPrototypeBinding->GetKeyEventHandlers();
      PRInt32 i;
      for (i = 0; i < keyHandlers->Count(); ++i) {
        nsXBLKeyEventHandler* handler = keyHandlers->ObjectAt(i);
        handler->SetIsBoundToChrome(isChromeDoc);

        nsAutoString type;
        handler->GetEventName(type);

        
        

        
        
        nsIDOMEventGroup* eventGroup = nsnull;
        if (handler->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND | NS_HANDLER_TYPE_SYSTEM)) {
          if (!systemEventGroup)
            manager->GetSystemEventGroupLM(getter_AddRefs(systemEventGroup));
          eventGroup = systemEventGroup;
        }

        
        PRInt32 flags = (handler->GetPhase() == NS_PHASE_CAPTURING) ?
          NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

        
        
        
        flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;

        manager->AddEventListenerByType(handler, type, flags, eventGroup);
      }
    }
  }

  if (mNextBinding)
    mNextBinding->InstallEventHandlers();
}

nsresult
nsXBLBinding::InstallImplementation()
{
  
  

  if (mNextBinding) {
    nsresult rv = mNextBinding->InstallImplementation();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  
  if (AllowScripts())
    return mPrototypeBinding->InstallImplementation(mBoundElement);

  return NS_OK;
}

nsIAtom*
nsXBLBinding::GetBaseTag(PRInt32* aNameSpaceID)
{
  nsIAtom *tag = mPrototypeBinding->GetBaseTag(aNameSpaceID);
  if (!tag && mNextBinding)
    return mNextBinding->GetBaseTag(aNameSpaceID);

  return tag;
}

void
nsXBLBinding::AttributeChanged(nsIAtom* aAttribute, PRInt32 aNameSpaceID,
                               PRBool aRemoveFlag, PRBool aNotify)
{
  
  if (!mContent) {
    if (mNextBinding)
      mNextBinding->AttributeChanged(aAttribute, aNameSpaceID,
                                     aRemoveFlag, aNotify);
  } else {
    mPrototypeBinding->AttributeChanged(aAttribute, aNameSpaceID, aRemoveFlag,
                                        mBoundElement, mContent, aNotify);
  }
}

void
nsXBLBinding::ExecuteAttachedHandler()
{
  if (mNextBinding)
    mNextBinding->ExecuteAttachedHandler();

  if (AllowScripts())
    mPrototypeBinding->BindingAttached(mBoundElement);
}

void
nsXBLBinding::ExecuteDetachedHandler()
{
  if (AllowScripts())
    mPrototypeBinding->BindingDetached(mBoundElement);

  if (mNextBinding)
    mNextBinding->ExecuteDetachedHandler();
}

void
nsXBLBinding::UnhookEventHandlers()
{
  nsXBLPrototypeHandler* handlerChain = mPrototypeBinding->GetPrototypeHandlers();

  if (handlerChain) {
    nsCOMPtr<nsPIDOMEventTarget> piTarget = do_QueryInterface(mBoundElement);
    nsCOMPtr<nsIDOM3EventTarget> target = do_QueryInterface(piTarget);
    nsCOMPtr<nsIDOMEventGroup> systemEventGroup;

    nsXBLPrototypeHandler* curr;
    for (curr = handlerChain; curr; curr = curr->GetNextHandler()) {
      nsXBLEventHandler* handler = curr->GetCachedEventHandler();
      if (handler) {
        nsCOMPtr<nsIAtom> eventAtom = curr->GetEventName();
        if (!eventAtom ||
            eventAtom == nsGkAtoms::keyup ||
            eventAtom == nsGkAtoms::keydown ||
            eventAtom == nsGkAtoms::keypress)
          continue;

        nsAutoString type;
        eventAtom->ToString(type);

        
        PRBool useCapture = (curr->GetPhase() == NS_PHASE_CAPTURING);

        
        

        
        
        nsIDOMEventGroup* eventGroup = nsnull;
        if (curr->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND | NS_HANDLER_TYPE_SYSTEM)) {
          if (!systemEventGroup)
            piTarget->GetSystemEventGroup(getter_AddRefs(systemEventGroup));
          eventGroup = systemEventGroup;
        }

        target->RemoveGroupedEventListener(type, handler, useCapture,
                                           eventGroup);
      }
    }

    const nsCOMArray<nsXBLKeyEventHandler>* keyHandlers =
      mPrototypeBinding->GetKeyEventHandlers();
    PRInt32 i;
    for (i = 0; i < keyHandlers->Count(); ++i) {
      nsXBLKeyEventHandler* handler = keyHandlers->ObjectAt(i);

      nsAutoString type;
      handler->GetEventName(type);

      
      PRBool useCapture = (handler->GetPhase() == NS_PHASE_CAPTURING);

      
      

      
      
      nsIDOMEventGroup* eventGroup = nsnull;
      if (handler->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND | NS_HANDLER_TYPE_SYSTEM)) {
        if (!systemEventGroup)
          piTarget->GetSystemEventGroup(getter_AddRefs(systemEventGroup));
        eventGroup = systemEventGroup;
      }

      target->RemoveGroupedEventListener(type, handler, useCapture,
                                         eventGroup);
    }
  }
}

void
nsXBLBinding::ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument)
{
  if (aOldDocument != aNewDocument) {
    if (mNextBinding)
      mNextBinding->ChangeDocument(aOldDocument, aNewDocument);

    
    if (mIsStyleBinding) {
      
      nsIContent* interfaceElement =
        mPrototypeBinding->GetImmediateChild(nsGkAtoms::implementation);

      if (interfaceElement) { 
        nsIScriptGlobalObject *global = aOldDocument->GetScriptGlobalObject();
        if (global) {
          nsIScriptContext *context = global->GetContext();
          if (context) {
            JSContext *jscontext = (JSContext *)context->GetNativeContext();
 
            nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
            nsresult rv = nsContentUtils::XPConnect()->
              WrapNative(jscontext, global->GetGlobalJSObject(),
                         mBoundElement, NS_GET_IID(nsISupports),
                         getter_AddRefs(wrapper));
            if (NS_FAILED(rv))
              return;

            JSObject* scriptObject = nsnull;
            rv = wrapper->GetJSObject(&scriptObject);
            if (NS_FAILED(rv))
              return;

            
            

            
            
            JSObject* ourProto = ::JS_GetPrototype(jscontext, scriptObject);
            if (ourProto)
            {
              JSObject* grandProto = ::JS_GetPrototype(jscontext, ourProto);
              ::JS_SetPrototype(jscontext, scriptObject, grandProto);
            }

            
            
            
          }
        }
      }
    }

    
    nsIContent *anonymous = mContent;
    if (anonymous) {
      
      if (mInsertionPointTable)
        mInsertionPointTable->Enumerate(ChangeDocumentForDefaultContent,
                                        nsnull);

#ifdef MOZ_XUL
      nsCOMPtr<nsIXULDocument> xuldoc(do_QueryInterface(aOldDocument));
#endif

      anonymous->UnbindFromTree(); 

#ifdef MOZ_XUL
      
      
      
      if (xuldoc)
        xuldoc->RemoveSubtreeFromDocument(anonymous);
#endif
    }

    
    
    nsBindingManager* bindingManager = aOldDocument->BindingManager();
    for (PRUint32 i = mBoundElement->GetChildCount(); i > 0; --i) {
      NS_ASSERTION(mBoundElement->GetChildAt(i-1),
                   "Must have child at i for 0 <= i < GetChildCount()!");
      bindingManager->SetInsertionParent(mBoundElement->GetChildAt(i-1),
                                         nsnull);
    }
  }
}

PRBool
nsXBLBinding::InheritsStyle() const
{
  
  

  
  if (mContent)
    return mPrototypeBinding->InheritsStyle();
  
  if (mNextBinding)
    return mNextBinding->InheritsStyle();

  return PR_TRUE;
}

void
nsXBLBinding::WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc, void* aData)
{
  if (mNextBinding)
    mNextBinding->WalkRules(aFunc, aData);

  nsIStyleRuleProcessor *rules = mPrototypeBinding->GetRuleProcessor();
  if (rules)
    (*aFunc)(rules, aData);
}




nsresult
nsXBLBinding::DoInitJSClass(JSContext *cx, JSObject *global, JSObject *obj,
                            const nsAFlatCString& aClassName,
                            void **aClassObject)
{
  
  jsval val;
  JSObject* proto;

  nsCAutoString className(aClassName);
  JSObject* parent_proto = nsnull;  
  JSAutoRequest ar(cx);
  if (obj) {
    
    parent_proto = ::JS_GetPrototype(cx, obj);
    if (parent_proto) {
      
      
      
      
      jsid parent_proto_id;
      if (!::JS_GetObjectId(cx, parent_proto, &parent_proto_id)) {
        
        return NS_ERROR_OUT_OF_MEMORY;
      }

      
      
      
      
      char buf[20];
      PR_snprintf(buf, sizeof(buf), " %lx", parent_proto_id);
      className.Append(buf);
    }
  }

  if ((!::JS_LookupPropertyWithFlags(cx, global, className.get(),
                                     JSRESOLVE_CLASSNAME,
                                     &val)) ||
      JSVAL_IS_PRIMITIVE(val)) {
    

    nsXBLJSClass* c;
    void* classObject;
    nsCStringKey key(className);
    classObject = (nsXBLService::gClassTable)->Get(&key);

    if (classObject) {
      c = NS_STATIC_CAST(nsXBLJSClass*, classObject);

      
      JSCList* link = NS_STATIC_CAST(JSCList*, c);
      if (c->next != link) {
        JS_REMOVE_AND_INIT_LINK(link);
        nsXBLService::gClassLRUListLength--;
      }
    } else {
      if (JS_CLIST_IS_EMPTY(&nsXBLService::gClassLRUList)) {
        
        c = new nsXBLJSClass(className);

        if (!c)
          return NS_ERROR_OUT_OF_MEMORY;
      } else {
        
        JSCList* lru = (nsXBLService::gClassLRUList).next;
        JS_REMOVE_AND_INIT_LINK(lru);
        nsXBLService::gClassLRUListLength--;

        
        c = NS_STATIC_CAST(nsXBLJSClass*, lru);
        nsCStringKey oldKey(c->name);
        (nsXBLService::gClassTable)->Remove(&oldKey);

        
        nsMemory::Free((void*) c->name);
        c->name = ToNewCString(className);
      }

      
      (nsXBLService::gClassTable)->Put(&key, (void*)c);
    }

    
    c->Hold();

    
    proto = ::JS_InitClass(cx,                  
                           global,              
                           parent_proto,        
                           c,                   
                           nsnull,              
                           0,                   
                           nsnull,              
                           nsnull,              
                           nsnull,              
                           nsnull);             
    if (!proto) {
      
      

      (nsXBLService::gClassTable)->Remove(&key);

      c->Drop();

      return NS_ERROR_OUT_OF_MEMORY;
    }

    *aClassObject = (void*)proto;
  }
  else {
    proto = JSVAL_TO_OBJECT(val);
  }

  if (obj) {
    
    if (!::JS_SetPrototype(cx, obj, proto)) {
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}


nsresult
nsXBLBinding::InitClass(const nsCString& aClassName,
                        nsIScriptContext* aContext, 
                        nsIDocument* aDocument, void** aScriptObject,
                        void** aClassObject)
{
  *aClassObject = nsnull;
  *aScriptObject = nsnull;

  nsresult rv;

  
  JSContext* cx = (JSContext*)aContext->GetNativeContext();

  nsIDocument *ownerDoc = mBoundElement->GetOwnerDoc();
  nsIScriptGlobalObject *sgo;

  if (!ownerDoc || !(sgo = ownerDoc->GetScriptGlobalObject())) {
    NS_ERROR("Can't find global object for bound content!");

    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
  rv = nsContentUtils::XPConnect()->WrapNative(cx, sgo->GetGlobalJSObject(),
                                               mBoundElement,
                                               NS_GET_IID(nsISupports),
                                               getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject* object = nsnull;
  rv = wrapper->GetJSObject(&object);
  NS_ENSURE_SUCCESS(rv, rv);

  *aScriptObject = object;

  

  rv = DoInitJSClass(cx, sgo->GetGlobalJSObject(), object, aClassName,
                     aClassObject);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsIDocument* doc = mBoundElement->GetOwnerDoc();

  if (doc) {
    nsCOMPtr<nsIXPConnectWrappedNative> native_wrapper =
      do_QueryInterface(wrapper);
    if (native_wrapper) {
      doc->AddReference(mBoundElement, native_wrapper);
    }
  }

  return NS_OK;
}

PRBool
nsXBLBinding::AllowScripts()
{
  PRBool result;
  mPrototypeBinding->GetAllowScripts(&result);
  if (!result) {
    return result;
  }

  
  
  
  nsIScriptSecurityManager* mgr = nsContentUtils::GetSecurityManager();
  if (!mgr) {
    return PR_FALSE;
  }

  nsIDocument* doc = mBoundElement->GetOwnerDoc();
  if (!doc) {
    return PR_FALSE;
  }

  nsIScriptGlobalObject* global = doc->GetScriptGlobalObject();
  if (!global) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) {
    return PR_FALSE;
  }
  
  JSContext* cx = (JSContext*) context->GetNativeContext();

  nsCOMPtr<nsIDocument> ourDocument;
  mPrototypeBinding->XBLDocumentInfo()->GetDocument(getter_AddRefs(ourDocument));
  PRBool canExecute;
  nsresult rv =
    mgr->CanExecuteScripts(cx, ourDocument->NodePrincipal(), &canExecute);
  return NS_SUCCEEDED(rv) && canExecute;
}

nsresult
nsXBLBinding::GetInsertionPointsFor(nsIContent* aParent,
                                    nsInsertionPointList** aResult)
{
  if (!mInsertionPointTable) {
    mInsertionPointTable =
      new nsClassHashtable<nsISupportsHashKey, nsInsertionPointList>;
    if (!mInsertionPointTable || !mInsertionPointTable->Init(4)) {
      delete mInsertionPointTable;
      mInsertionPointTable = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  mInsertionPointTable->Get(aParent, aResult);

  if (!*aResult) {
    *aResult = new nsInsertionPointList;
    if (!*aResult || !mInsertionPointTable->Put(aParent, *aResult)) {
      delete *aResult;
      *aResult = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
}

nsInsertionPointList*
nsXBLBinding::GetExistingInsertionPointsFor(nsIContent* aParent)
{
  if (!mInsertionPointTable) {
    return nsnull;
  }

  nsInsertionPointList* result = nsnull;
  mInsertionPointTable->Get(aParent, &result);
  return result;
}

nsIContent*
nsXBLBinding::GetInsertionPoint(nsIContent* aChild, PRUint32* aIndex)
{
  if (mContent) {
    return mPrototypeBinding->GetInsertionPoint(mBoundElement, mContent,
                                                aChild, aIndex);
  }

  if (mNextBinding)
    return mNextBinding->GetInsertionPoint(aChild, aIndex);

  return nsnull;
}

nsIContent*
nsXBLBinding::GetSingleInsertionPoint(PRUint32* aIndex,
                                      PRBool* aMultipleInsertionPoints)
{
  *aMultipleInsertionPoints = PR_FALSE;
  if (mContent) {
    return mPrototypeBinding->GetSingleInsertionPoint(mBoundElement, mContent, 
                                                      aIndex, 
                                                      aMultipleInsertionPoints);
  }

  if (mNextBinding)
    return mNextBinding->GetSingleInsertionPoint(aIndex,
                                                 aMultipleInsertionPoints);

  return nsnull;
}

nsXBLBinding*
nsXBLBinding::RootBinding()
{
  if (mNextBinding)
    return mNextBinding->RootBinding();

  return this;
}

nsXBLBinding*
nsXBLBinding::GetFirstStyleBinding()
{
  if (mIsStyleBinding)
    return this;

  return mNextBinding ? mNextBinding->GetFirstStyleBinding() : nsnull;
}

void
nsXBLBinding::MarkForDeath()
{
  mMarkedForDeath = PR_TRUE;
  ExecuteDetachedHandler();
}

PRBool
nsXBLBinding::ImplementsInterface(REFNSIID aIID) const
{
  return mPrototypeBinding->ImplementsInterface(aIID) ||
    (mNextBinding && mNextBinding->ImplementsInterface(aIID));
}

already_AddRefed<nsIDOMNodeList>
nsXBLBinding::GetAnonymousNodes()
{
  if (mContent) {
    nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(mContent));
    nsIDOMNodeList *nodeList = nsnull;
    elt->GetChildNodes(&nodeList);
    return nodeList;
  }

  if (mNextBinding)
    return mNextBinding->GetAnonymousNodes();

  return nsnull;
}

PRBool
nsXBLBinding::ShouldBuildChildFrames() const
{
  if (mContent)
    return mPrototypeBinding->ShouldBuildChildFrames();

  if (mNextBinding) 
    return mNextBinding->ShouldBuildChildFrames();

  return PR_TRUE;
}
