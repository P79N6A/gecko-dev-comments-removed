






































#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsXBLDocumentInfo.h"
#include "nsIInputStream.h"
#include "nsINameSpaceManager.h"
#include "nsHashtable.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIDOMEventTarget.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
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
#include "jsapi.h"
#include "nsXBLService.h"
#include "nsXBLInsertionPoint.h"
#include "nsIXPConnect.h"
#include "nsIScriptContext.h"
#include "nsCRT.h"


#include "nsEventListenerManager.h"
#include "nsIDOMEventListener.h"
#include "nsAttrName.h"

#include "nsGkAtoms.h"

#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"

#include "nsXBLPrototypeHandler.h"

#include "nsXBLPrototypeBinding.h"
#include "nsXBLBinding.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsGUIEvent.h"

#include "prprf.h"
#include "nsNodeUtils.h"



#include "nsDOMClassInfo.h"
#include "nsJSUtils.h"

#include "mozilla/dom/Element.h"







static void
XBLFinalize(JSFreeOp *fop, JSObject *obj)
{
  nsXBLDocumentInfo* docInfo =
    static_cast<nsXBLDocumentInfo*>(::JS_GetPrivate(obj));
  NS_RELEASE(docInfo);
  
  nsXBLJSClass* c = static_cast<nsXBLJSClass*>(::JS_GetClass(obj));
  c->Drop();
}

static JSBool
XBLResolve(JSContext *cx, JSObject *obj, jsid id, unsigned flags,
           JSObject **objp)
{
  
  
  
  
  
  NS_ASSERTION(*objp, "Must have starting object");

  JSObject* origObj = *objp;
  *objp = NULL;

  if (!JSID_IS_STRING(id)) {
    return JS_TRUE;
  }

  nsDependentJSString fieldName(id);

  jsval slotVal = ::JS_GetReservedSlot(obj, 0);
  NS_ASSERTION(!JSVAL_IS_VOID(slotVal), "How did that happen?");
    
  nsXBLPrototypeBinding* protoBinding =
    static_cast<nsXBLPrototypeBinding*>(JSVAL_TO_PRIVATE(slotVal));
  NS_ASSERTION(protoBinding, "Must have prototype binding!");

  nsXBLProtoImplField* field = protoBinding->FindField(fieldName);
  if (!field) {
    return JS_TRUE;
  }

  
  JSClass* nodeClass = ::JS_GetClass(origObj);
  if (!nodeClass) {
    return JS_FALSE;
  }
  
  if (~nodeClass->flags &
      (JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS)) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_UNEXPECTED);
    return JS_FALSE;
  }

  nsCOMPtr<nsIXPConnectWrappedNative> xpcWrapper =
    do_QueryInterface(static_cast<nsISupports*>(::JS_GetPrivate(origObj)));
  if (!xpcWrapper) {
    
    
    
    
    
    return JS_TRUE;
  }

  nsCOMPtr<nsIContent> content = do_QueryWrappedNative(xpcWrapper);
  if (!content) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_UNEXPECTED);
    return JS_FALSE;
  }

  
  nsIDocument* doc = content->OwnerDoc();

  nsIScriptGlobalObject* global = doc->GetScriptGlobalObject();
  if (!global) {
    return JS_TRUE;
  }

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) {
    return JS_TRUE;
  }


  
  bool didInstall;
  nsresult rv = field->InstallField(context, origObj,
                                    content->NodePrincipal(),
                                    protoBinding->DocURI(),
                                    &didInstall);
  if (NS_FAILED(rv)) {
    if (!::JS_IsExceptionPending(cx)) {
      nsDOMClassInfo::ThrowJSException(cx, rv);
    }

    return JS_FALSE;
  }

  if (didInstall) {
    *objp = origObj;
  }
  

  return JS_TRUE;
}

nsXBLJSClass::nsXBLJSClass(const nsAFlatCString& aClassName)
{
  memset(this, 0, sizeof(nsXBLJSClass));
  next = prev = static_cast<JSCList*>(this);
  name = ToNewCString(aClassName);
  flags =
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    JSCLASS_NEW_RESOLVE | JSCLASS_NEW_RESOLVE_GETS_START |
    
    JSCLASS_HAS_RESERVED_SLOTS(1);
  addProperty = delProperty = getProperty = ::JS_PropertyStub;
  setProperty = ::JS_StrictPropertyStub;
  enumerate = ::JS_EnumerateStub;
  resolve = (JSResolveOp)XBLResolve;
  convert = ::JS_ConvertStub;
  finalize = XBLFinalize;
}

nsrefcnt
nsXBLJSClass::Destroy()
{
  NS_ASSERTION(next == prev && prev == static_cast<JSCList*>(this),
               "referenced nsXBLJSClass is on LRU list already!?");

  if (nsXBLService::gClassTable) {
    nsCStringKey key(name);
    (nsXBLService::gClassTable)->Remove(&key);
  }

  if (nsXBLService::gClassLRUListLength >= nsXBLService::gClassLRUListQuota) {
    
    delete this;
  } else {
    
    JSCList* mru = static_cast<JSCList*>(this);
    JS_APPEND_LINK(mru, &nsXBLService::gClassLRUList);
    nsXBLService::gClassLRUListLength++;
  }

  return 0;
}




nsXBLBinding::nsXBLBinding(nsXBLPrototypeBinding* aBinding)
  : mIsStyleBinding(true),
    mMarkedForDeath(false),
    mPrototypeBinding(aBinding),
    mInsertionPointTable(nsnull)
{
  NS_ASSERTION(mPrototypeBinding, "Must have a prototype binding!");
  
  NS_ADDREF(mPrototypeBinding->XBLDocumentInfo());
}


nsXBLBinding::~nsXBLBinding(void)
{
  if (mContent) {
    nsXBLBinding::UninstallAnonymousContent(mContent->OwnerDoc(), mContent);
  }
  delete mInsertionPointTable;
  nsXBLDocumentInfo* info = mPrototypeBinding->XBLDocumentInfo();
  NS_RELEASE(info);
}

static PLDHashOperator
TraverseKey(nsISupports* aKey, nsInsertionPointList* aData, void* aClosure)
{
  nsCycleCollectionTraversalCallback &cb = 
    *static_cast<nsCycleCollectionTraversalCallback*>(aClosure);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mInsertionPointTable key");
  cb.NoteXPCOMChild(aKey);
  if (aData) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY(*aData, nsXBLInsertionPoint,
                                               "mInsertionPointTable value")
  }
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLBinding)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(nsXBLBinding)
  
  
  if (tmp->mContent) {
    nsXBLBinding::UninstallAnonymousContent(tmp->mContent->OwnerDoc(),
                                            tmp->mContent);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNextBinding)
  delete tmp->mInsertionPointTable;
  tmp->mInsertionPointTable = nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(nsXBLBinding)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb,
                                     "mPrototypeBinding->XBLDocumentInfo()");
  cb.NoteXPCOMChild(static_cast<nsIScriptGlobalObjectOwner*>(
                      tmp->mPrototypeBinding->XBLDocumentInfo()));
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
  bool allowScripts = AllowScripts();

  nsAutoScriptBlocker scriptBlocker;
  for (nsIContent* child = aAnonParent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    child->UnbindFromTree();
    nsresult rv =
      child->BindToTree(doc, aElement, mBoundElement, allowScripts);
    if (NS_FAILED(rv)) {
      
      
      child->UnbindFromTree();
      return;
    }        

    child->SetFlags(NODE_IS_ANONYMOUS);

#ifdef MOZ_XUL
    
    
    
    nsCOMPtr<nsIXULDocument> xuldoc(do_QueryInterface(doc));
    if (xuldoc)
      xuldoc->AddSubtreeToDocument(child);
#endif
  }
}

void
nsXBLBinding::UninstallAnonymousContent(nsIDocument* aDocument,
                                        nsIContent* aAnonParent)
{
  nsAutoScriptBlocker scriptBlocker;
  
  nsCOMPtr<nsIContent> anonParent = aAnonParent;
#ifdef MOZ_XUL
  nsCOMPtr<nsIXULDocument> xuldoc =
    do_QueryInterface(aDocument);
#endif
  for (nsIContent* child = aAnonParent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    child->UnbindFromTree();
#ifdef MOZ_XUL
    if (xuldoc) {
      xuldoc->RemoveSubtreeFromDocument(child);
    }
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

bool
nsXBLBinding::HasStyleSheets() const
{
  
  
  if (mPrototypeBinding->HasStyleSheets())
    return true;

  return mNextBinding ? mNextBinding->HasStyleSheets() : false;
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

static PLDHashOperator
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

  
  nsXBLInsertionPoint* currPoint = aData->ElementAt(0);
  nsCOMPtr<nsIContent> parent = currPoint->GetInsertionParent();
  if (!parent) {
    data->mRv = NS_ERROR_FAILURE;
    return PL_DHASH_STOP;
  }
  PRInt32 currIndex = currPoint->GetInsertionIndex();

  
  
  nsInsertionPointList* contentList = new nsInsertionPointList;
  if (!contentList) {
    data->mRv = NS_ERROR_OUT_OF_MEMORY;
    return PL_DHASH_STOP;
  }

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

static PLDHashOperator
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
        if (!insParent) {
          data->mRv = NS_ERROR_FAILURE;
          return PL_DHASH_STOP;
        }
        nsIDocument *document = insParent->OwnerDoc();
        nsCOMPtr<nsIDOMNode> clonedNode;
        nsCOMArray<nsINode> nodesWithProperties;
        nsNodeUtils::Clone(defContent, true, document->NodeInfoManager(),
                           nodesWithProperties, getter_AddRefs(clonedNode));

        
        
        nsCOMPtr<nsIContent> clonedContent(do_QueryInterface(clonedNode));
        binding->InstallAnonymousContent(clonedContent, insParent);

        
        
        currPoint->SetDefaultContent(clonedContent);

        
        
        for (nsIContent* child = clonedContent->GetFirstChild();
             child;
             child = child->GetNextSibling()) {
          bm->SetInsertionParent(child, insParent);
          currPoint->AddChild(child);
        }
      }
    }
  }

  return PL_DHASH_NEXT;
}

static PLDHashOperator
ChangeDocumentForDefaultContent(nsISupports* aKey,
                                nsAutoPtr<nsInsertionPointList>& aData,
                                void* aClosure)
{
  PRInt32 count = aData->Length();
  for (PRInt32 i = 0; i < count; i++) {
    aData->ElementAt(i)->UnbindDefaultContent();
  }

  return PL_DHASH_NEXT;
}

void
nsXBLBinding::GenerateAnonymousContent()
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
               "Someone forgot a script blocker");

  
  nsIContent* content =
    mPrototypeBinding->GetImmediateChild(nsGkAtoms::content);

  if (!content) {
    
    if (mNextBinding)
      mNextBinding->GenerateAnonymousContent();

    return;
  }
     
  
  
  PRUint32 contentCount = content->GetChildCount();

  
  bool hasContent = (contentCount > 0);
  bool hasInsertionPoints = mPrototypeBinding->HasInsertionPoints();

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
    nsIDocument* doc = mBoundElement->OwnerDoc();
    
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
          hasContent = false;
          break;
        }
      }
    }

    if (hasContent || hasInsertionPoints) {
      nsCOMPtr<nsIDOMNode> clonedNode;
      nsCOMArray<nsINode> nodesWithProperties;
      nsNodeUtils::Clone(content, true, doc->NodeInfoManager(),
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
        bool multiplePoints = false;
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
                  
                  UninstallAnonymousContent(doc, mContent);

                  
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
    
    
    nsCOMPtr<nsIAtom> name = attrName->LocalName();

    if (name != nsGkAtoms::includes) {
      if (!nsContentUtils::HasNonEmptyAttr(mBoundElement, namespaceID, name)) {
        nsAutoString value2;
        content->GetAttr(namespaceID, name, value2);
        mBoundElement->SetAttr(namespaceID, name, attrName->GetPrefix(),
                               value2, false);
      }
    }

    
    if (mContent)
      mContent->UnsetAttr(namespaceID, name, false);
  }
}

void
nsXBLBinding::InstallEventHandlers()
{
  
  if (AllowScripts()) {
    
    nsXBLPrototypeHandler* handlerChain = mPrototypeBinding->GetPrototypeHandlers();

    if (handlerChain) {
      nsEventListenerManager* manager =
        mBoundElement->GetListenerManager(true);
      if (!manager)
        return;

      bool isChromeDoc =
        nsContentUtils::IsChromeDoc(mBoundElement->OwnerDoc());
      bool isChromeBinding = mPrototypeBinding->IsChrome();
      nsXBLPrototypeHandler* curr;
      for (curr = handlerChain; curr; curr = curr->GetNextHandler()) {
        
        nsCOMPtr<nsIAtom> eventAtom = curr->GetEventName();
        if (!eventAtom ||
            eventAtom == nsGkAtoms::keyup ||
            eventAtom == nsGkAtoms::keydown ||
            eventAtom == nsGkAtoms::keypress)
          continue;

        nsXBLEventHandler* handler = curr->GetEventHandler();
        if (handler) {
          
          PRInt32 flags = (curr->GetPhase() == NS_PHASE_CAPTURING) ?
            NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

          
          if ((curr->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND |
                                  NS_HANDLER_TYPE_SYSTEM)) &&
              (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
            flags |= NS_EVENT_FLAG_SYSTEM_EVENT;
          }

          bool hasAllowUntrustedAttr = curr->HasAllowUntrustedAttr();
          if ((hasAllowUntrustedAttr && curr->AllowUntrustedEvents()) ||
              (!hasAllowUntrustedAttr && !isChromeDoc)) {
            flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
          }

          manager->AddEventListenerByType(handler,
                                          nsDependentAtomString(eventAtom),
                                          flags);
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

        
        

        
        PRInt32 flags = (handler->GetPhase() == NS_PHASE_CAPTURING) ?
          NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

        if ((handler->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND |
                                   NS_HANDLER_TYPE_SYSTEM)) &&
            (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
          flags |= NS_EVENT_FLAG_SYSTEM_EVENT;
        }

        
        
        
        flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;

        manager->AddEventListenerByType(handler, type, flags);
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
                               bool aRemoveFlag, bool aNotify)
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
    nsEventListenerManager* manager =
      mBoundElement->GetListenerManager(false);
    if (!manager) {
      return;
    }
                                      
    bool isChromeBinding = mPrototypeBinding->IsChrome();
    nsXBLPrototypeHandler* curr;
    for (curr = handlerChain; curr; curr = curr->GetNextHandler()) {
      nsXBLEventHandler* handler = curr->GetCachedEventHandler();
      if (!handler) {
        continue;
      }
      
      nsCOMPtr<nsIAtom> eventAtom = curr->GetEventName();
      if (!eventAtom ||
          eventAtom == nsGkAtoms::keyup ||
          eventAtom == nsGkAtoms::keydown ||
          eventAtom == nsGkAtoms::keypress)
        continue;

      
      PRInt32 flags = (curr->GetPhase() == NS_PHASE_CAPTURING) ?
        NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

      
      

      if ((curr->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND |
                              NS_HANDLER_TYPE_SYSTEM)) &&
          (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
        flags |= NS_EVENT_FLAG_SYSTEM_EVENT;
      }

      manager->RemoveEventListenerByType(handler,
                                         nsDependentAtomString(eventAtom),
                                         flags);
    }

    const nsCOMArray<nsXBLKeyEventHandler>* keyHandlers =
      mPrototypeBinding->GetKeyEventHandlers();
    PRInt32 i;
    for (i = 0; i < keyHandlers->Count(); ++i) {
      nsXBLKeyEventHandler* handler = keyHandlers->ObjectAt(i);

      nsAutoString type;
      handler->GetEventName(type);

      
      PRInt32 flags = (handler->GetPhase() == NS_PHASE_CAPTURING) ?
        NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

      
      

      if ((handler->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND | NS_HANDLER_TYPE_SYSTEM)) &&
          (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
        flags |= NS_EVENT_FLAG_SYSTEM_EVENT;
      }

      manager->RemoveEventListenerByType(handler, type, flags);
    }
  }
}

void
nsXBLBinding::ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument)
{
  if (aOldDocument != aNewDocument) {
    
    if (mIsStyleBinding) {
      
      if (mPrototypeBinding->HasImplementation()) { 
        nsIScriptGlobalObject *global = aOldDocument->GetScopeObject();
        if (global) {
          JSObject *scope = global->GetGlobalJSObject();
          
          
          
          
          

          nsCOMPtr<nsIScriptContext> context = global->GetContext();
          if (context && scope) {
            JSContext *cx = context->GetNativeContext();
 
            nsCxPusher pusher;
            pusher.Push(cx);

            nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
            nsIXPConnect *xpc = nsContentUtils::XPConnect();
            nsresult rv =
              xpc->GetWrappedNativeOfNativeObject(cx, scope, mBoundElement,
                                                  NS_GET_IID(nsISupports),
                                                  getter_AddRefs(wrapper));
            if (NS_FAILED(rv))
              return;

            JSObject* scriptObject;
            if (wrapper)
                wrapper->GetJSObject(&scriptObject);
            else
                scriptObject = nsnull;

            if (scriptObject) {
              
              
              
              
              

              
              JSObject* base = scriptObject;
              JSObject* proto;
              JSAutoRequest ar(cx);
              JSAutoEnterCompartment ac;
              if (!ac.enter(cx, scriptObject)) {
                return;
              }

              for ( ; true; base = proto) { 
                proto = ::JS_GetPrototype(base);
                if (!proto) {
                  break;
                }

                JSClass* clazz = ::JS_GetClass(proto);
                if (!clazz ||
                    (~clazz->flags &
                     (JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS)) ||
                    JSCLASS_RESERVED_SLOTS(clazz) != 1 ||
                    clazz->resolve != (JSResolveOp)XBLResolve ||
                    clazz->finalize != XBLFinalize) {
                  
                  continue;
                }

                nsRefPtr<nsXBLDocumentInfo> docInfo =
                  static_cast<nsXBLDocumentInfo*>(::JS_GetPrivate(proto));
                if (!docInfo) {
                  
                  continue;
                }

                jsval protoBinding = ::JS_GetReservedSlot(proto, 0);

                if (JSVAL_TO_PRIVATE(protoBinding) != mPrototypeBinding) {
                  
                  continue;
                }

                
                
                JSObject* grandProto = ::JS_GetPrototype(proto);
                ::JS_SetPrototype(cx, base, grandProto);
                break;
              }

              mPrototypeBinding->UndefineFields(cx, scriptObject);

              
              
              
            }
          }
        }
      }

      
      UnhookEventHandlers();
    }

    {
      nsAutoScriptBlocker scriptBlocker;

      
      
      if (mNextBinding) {
        mNextBinding->ChangeDocument(aOldDocument, aNewDocument);
      }

      
      
      nsIContent *anonymous = mContent;
      if (anonymous) {
        
        if (mInsertionPointTable)
          mInsertionPointTable->Enumerate(ChangeDocumentForDefaultContent,
                                          nsnull);

        nsXBLBinding::UninstallAnonymousContent(aOldDocument, anonymous);
      }

      
      
      nsBindingManager* bindingManager = aOldDocument->BindingManager();
      for (nsIContent* child = mBoundElement->GetLastChild();
           child;
           child = child->GetPreviousSibling()) {
        bindingManager->SetInsertionParent(child, nsnull);
      }
    }
  }
}

bool
nsXBLBinding::InheritsStyle() const
{
  
  

  
  if (mContent)
    return mPrototypeBinding->InheritsStyle();
  
  if (mNextBinding)
    return mNextBinding->InheritsStyle();

  return true;
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
                            nsXBLPrototypeBinding* aProtoBinding,
                            JSObject** aClassObject)
{
  
  nsCAutoString className(aClassName);
  JSObject* parent_proto = nsnull;  
  JSAutoRequest ar(cx);

  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, global)) {
    return NS_ERROR_FAILURE;
  }

  if (obj) {
    
    parent_proto = ::JS_GetPrototype(obj);
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

  jsval val;
  JSObject* proto = NULL;
  if ((!::JS_LookupPropertyWithFlags(cx, global, className.get(),
                                     JSRESOLVE_CLASSNAME,
                                     &val)) ||
      JSVAL_IS_PRIMITIVE(val)) {
    

    nsXBLJSClass* c;
    void* classObject;
    nsCStringKey key(className);
    classObject = (nsXBLService::gClassTable)->Get(&key);

    if (classObject) {
      c = static_cast<nsXBLJSClass*>(classObject);

      
      JSCList* link = static_cast<JSCList*>(c);
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

        
        c = static_cast<nsXBLJSClass*>(lru);
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

    
    
    
    
    
    
    nsXBLDocumentInfo* docInfo = aProtoBinding->XBLDocumentInfo();
    ::JS_SetPrivate(proto, docInfo);
    NS_ADDREF(docInfo);

    ::JS_SetReservedSlot(proto, 0, PRIVATE_TO_JSVAL(aProtoBinding));

    *aClassObject = proto;
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

bool
nsXBLBinding::AllowScripts()
{
  if (!mPrototypeBinding->GetAllowScripts())
    return false;

  
  
  
  nsIScriptSecurityManager* mgr = nsContentUtils::GetSecurityManager();
  if (!mgr) {
    return false;
  }

  nsIDocument* doc = mBoundElement ? mBoundElement->OwnerDoc() : nsnull;
  if (!doc) {
    return false;
  }

  nsIScriptGlobalObject* global = doc->GetScriptGlobalObject();
  if (!global) {
    return false;
  }

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) {
    return false;
  }
  
  JSContext* cx = context->GetNativeContext();

  nsCOMPtr<nsIDocument> ourDocument =
    mPrototypeBinding->XBLDocumentInfo()->GetDocument();
  bool canExecute;
  nsresult rv =
    mgr->CanExecuteScripts(cx, ourDocument->NodePrincipal(), &canExecute);
  if (NS_FAILED(rv) || !canExecute) {
    return false;
  }

  
  
  bool haveCert;
  doc->NodePrincipal()->GetHasCertificate(&haveCert);
  if (!haveCert) {
    return true;
  }

  bool subsumes;
  rv = ourDocument->NodePrincipal()->Subsumes(doc->NodePrincipal(), &subsumes);
  return NS_SUCCEEDED(rv) && subsumes;
}

void
nsXBLBinding::RemoveInsertionParent(nsIContent* aParent)
{
  if (mNextBinding) {
    mNextBinding->RemoveInsertionParent(aParent);
  }
  if (mInsertionPointTable) {
    nsInsertionPointList* list = nsnull;
    mInsertionPointTable->Get(aParent, &list);
    if (list) {
      PRInt32 count = list->Length();
      for (PRInt32 i = 0; i < count; ++i) {
        nsRefPtr<nsXBLInsertionPoint> currPoint = list->ElementAt(i);
        currPoint->UnbindDefaultContent();
#ifdef DEBUG
        nsCOMPtr<nsIContent> parent = currPoint->GetInsertionParent();
        NS_ASSERTION(!parent || parent == aParent, "Wrong insertion parent!");
#endif
        currPoint->ClearInsertionParent();
      }
      mInsertionPointTable->Remove(aParent);
    }
  }
}

bool
nsXBLBinding::HasInsertionParent(nsIContent* aParent)
{
  if (mInsertionPointTable) {
    nsInsertionPointList* list = nsnull;
    mInsertionPointTable->Get(aParent, &list);
    if (list) {
      return true;
    }
  }
  return mNextBinding ? mNextBinding->HasInsertionParent(aParent) : false;
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
    if (aParent) {
      aParent->SetFlags(NODE_IS_INSERTION_PARENT);
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
nsXBLBinding::GetInsertionPoint(const nsIContent* aChild, PRUint32* aIndex)
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
                                      bool* aMultipleInsertionPoints)
{
  *aMultipleInsertionPoints = false;
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

bool
nsXBLBinding::ResolveAllFields(JSContext *cx, JSObject *obj) const
{
  if (!mPrototypeBinding->ResolveAllFields(cx, obj)) {
    return false;
  }

  if (mNextBinding) {
    return mNextBinding->ResolveAllFields(cx, obj);
  }

  return true;
}

void
nsXBLBinding::MarkForDeath()
{
  mMarkedForDeath = true;
  ExecuteDetachedHandler();
}

bool
nsXBLBinding::ImplementsInterface(REFNSIID aIID) const
{
  return mPrototypeBinding->ImplementsInterface(aIID) ||
    (mNextBinding && mNextBinding->ImplementsInterface(aIID));
}

nsINodeList*
nsXBLBinding::GetAnonymousNodes()
{
  if (mContent) {
    return mContent->GetChildNodesList();
  }

  if (mNextBinding)
    return mNextBinding->GetAnonymousNodes();

  return nsnull;
}
