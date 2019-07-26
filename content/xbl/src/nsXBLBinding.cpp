





#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsXBLDocumentInfo.h"
#include "nsIInputStream.h"
#include "nsINameSpaceManager.h"
#include "nsHashtable.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "plstr.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "ChildIterator.h"
#include "nsCxPusher.h"
#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif
#include "nsIXMLContentSink.h"
#include "nsContentCID.h"
#include "mozilla/dom/XMLDocument.h"
#include "jsapi.h"
#include "nsXBLService.h"
#include "nsIXPConnect.h"
#include "nsIScriptContext.h"
#include "nsCRT.h"


#include "nsEventListenerManager.h"
#include "nsIDOMEventListener.h"
#include "nsAttrName.h"

#include "nsGkAtoms.h"

#include "nsXBLPrototypeHandler.h"

#include "nsXBLPrototypeBinding.h"
#include "nsXBLBinding.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "mozilla/dom/XBLChildrenElement.h"

#include "prprf.h"
#include "nsNodeUtils.h"
#include "nsJSUtils.h"



#include "nsDOMClassInfo.h"

#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::dom;







static void
XBLFinalize(JSFreeOp *fop, JSObject *obj)
{
  nsXBLDocumentInfo* docInfo =
    static_cast<nsXBLDocumentInfo*>(::JS_GetPrivate(obj));
  nsContentUtils::DeferredFinalize(docInfo);
  
  nsXBLJSClass* c = nsXBLJSClass::fromJSClass(::JS_GetClass(obj));
  c->Drop();
}

static bool
XBLEnumerate(JSContext *cx, JS::Handle<JSObject*> obj)
{
  nsXBLPrototypeBinding* protoBinding =
    static_cast<nsXBLPrototypeBinding*>(::JS_GetReservedSlot(obj, 0).toPrivate());
  MOZ_ASSERT(protoBinding);

  return protoBinding->ResolveAllFields(cx, obj);
}

uint64_t nsXBLJSClass::sIdCount = 0;

nsXBLJSClass::nsXBLJSClass(const nsAFlatCString& aClassName,
                           const nsCString& aKey)
  : LinkedListElement<nsXBLJSClass>()
  , mRefCnt(0)
  , mKey(aKey)
{
  memset(static_cast<JSClass*>(this), 0, sizeof(JSClass));
  name = ToNewCString(aClassName);
  flags =
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    JSCLASS_NEW_RESOLVE |
    
    JSCLASS_HAS_RESERVED_SLOTS(1);
  addProperty = getProperty = ::JS_PropertyStub;
  delProperty = ::JS_DeletePropertyStub;
  setProperty = ::JS_StrictPropertyStub;
  enumerate = XBLEnumerate;
  resolve = JS_ResolveStub;
  convert = ::JS_ConvertStub;
  finalize = XBLFinalize;
}

nsrefcnt
nsXBLJSClass::Destroy()
{
  NS_ASSERTION(!isInList(),
               "referenced nsXBLJSClass is on LRU list already!?");

  if (nsXBLService::gClassTable) {
    nsCStringKey key(mKey);
    (nsXBLService::gClassTable)->Remove(&key);
    mKey.Truncate();
  }

  if (nsXBLService::gClassLRUListLength >= nsXBLService::gClassLRUListQuota) {
    
    delete this;
  } else {
    
    nsXBLService::gClassLRUList->insertBack(this);
    nsXBLService::gClassLRUListLength++;
  }

  return 0;
}

nsXBLJSClass*
nsXBLService::getClass(const nsCString& k)
{
  nsCStringKey key(k);
  return getClass(&key);
}

nsXBLJSClass*
nsXBLService::getClass(nsCStringKey *k)
{
  return static_cast<nsXBLJSClass*>(nsXBLService::gClassTable->Get(k));
}




nsXBLBinding::nsXBLBinding(nsXBLPrototypeBinding* aBinding)
  : mMarkedForDeath(false),
    mPrototypeBinding(aBinding)
{
  NS_ASSERTION(mPrototypeBinding, "Must have a prototype binding!");
  
  NS_ADDREF(mPrototypeBinding->XBLDocumentInfo());
}


nsXBLBinding::~nsXBLBinding(void)
{
  if (mContent) {
    nsXBLBinding::UninstallAnonymousContent(mContent->OwnerDoc(), mContent);
  }
  nsXBLDocumentInfo* info = mPrototypeBinding->XBLDocumentInfo();
  NS_RELEASE(info);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLBinding)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXBLBinding)
  
  
  if (tmp->mContent) {
    nsXBLBinding::UninstallAnonymousContent(tmp->mContent->OwnerDoc(),
                                            tmp->mContent);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mContent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNextBinding)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDefaultInsertionPoint)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mInsertionPoints)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mAnonymousContentList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXBLBinding)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb,
                                     "mPrototypeBinding->XBLDocumentInfo()");
  cb.NoteXPCOMChild(tmp->mPrototypeBinding->XBLDocumentInfo());
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNextBinding)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDefaultInsertionPoint)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mInsertionPoints)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAnonymousContentList)
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

nsXBLBinding*
nsXBLBinding::GetBindingWithContent()
{
  if (mContent) {
    return this;
  }

  return mNextBinding ? mNextBinding->GetBindingWithContent() : nullptr;
}

void
nsXBLBinding::InstallAnonymousContent(nsIContent* aAnonParent, nsIContent* aElement,
                                      bool aChromeOnlyContent)
{
  
  
  
  
  
  
  
  
  
  nsIDocument* doc = aElement->GetCurrentDoc();
  bool allowScripts = AllowScripts();

  nsAutoScriptBlocker scriptBlocker;
  for (nsIContent* child = aAnonParent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    child->UnbindFromTree();
    if (aChromeOnlyContent) {
      child->SetFlags(NODE_CHROME_ONLY_ACCESS |
                      NODE_IS_ROOT_OF_CHROME_ONLY_ACCESS);
    }
    nsresult rv =
      child->BindToTree(doc, aElement, mBoundElement, allowScripts);
    if (NS_FAILED(rv)) {
      
      
      child->UnbindFromTree();
      return;
    }        

    child->SetFlags(NODE_IS_ANONYMOUS_ROOT);

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
     
  
  
  uint32_t contentCount = content->GetChildCount();

  
  bool hasContent = (contentCount > 0);
  if (hasContent) {
    nsIDocument* doc = mBoundElement->OwnerDoc();

    nsCOMPtr<nsINode> clonedNode;
    nsCOMArray<nsINode> nodesWithProperties;
    nsNodeUtils::Clone(content, true, doc->NodeInfoManager(),
                       nodesWithProperties, getter_AddRefs(clonedNode));
    mContent = clonedNode->AsElement();

    
    
    
    for (nsIContent* child = mContent; child; child = child->GetNextNode(mContent)) {
      if (child->NodeInfo()->Equals(nsGkAtoms::children, kNameSpaceID_XBL)) {
        XBLChildrenElement* point = static_cast<XBLChildrenElement*>(child);
        if (point->IsDefaultInsertion()) {
          mDefaultInsertionPoint = point;
        } else {
          mInsertionPoints.AppendElement(point);
        }
      }
    }

    
    
    InstallAnonymousContent(mContent, mBoundElement,
                            mPrototypeBinding->ChromeOnlyContent());

    
    if (mDefaultInsertionPoint && mInsertionPoints.IsEmpty()) {
      ExplicitChildIterator iter(mBoundElement);
      for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
        mDefaultInsertionPoint->AppendInsertedChild(child);
      }
    } else {
      
      
      
      ExplicitChildIterator iter(mBoundElement);
      for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
        XBLChildrenElement* point = FindInsertionPointForInternal(child);
        if (point) {
          point->AppendInsertedChild(child);
        } else {
          nsINodeInfo *ni = child->NodeInfo();
          if (ni->NamespaceID() != kNameSpaceID_XUL ||
              (!ni->Equals(nsGkAtoms::_template) &&
               !ni->Equals(nsGkAtoms::observes))) {
            
            
            
            

            
            UninstallAnonymousContent(doc, mContent);

            
            ClearInsertionPoints();

            
            mContent = nullptr;
            return;
          }
        }
      }
    }

    
    if (mDefaultInsertionPoint) {
      mDefaultInsertionPoint->MaybeSetupDefaultContent();
    }
    for (uint32_t i = 0; i < mInsertionPoints.Length(); ++i) {
      mInsertionPoints[i]->MaybeSetupDefaultContent();
    }

    mPrototypeBinding->SetInitialAttributes(mBoundElement, mContent);
  }

  
  
  
  const nsAttrName* attrName;
  for (uint32_t i = 0; (attrName = content->GetAttrNameAt(i)); ++i) {
    int32_t namespaceID = attrName->NamespaceID();
    
    
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

XBLChildrenElement*
nsXBLBinding::FindInsertionPointFor(nsIContent* aChild)
{
  
  
  if (mContent) {
    return FindInsertionPointForInternal(aChild);
  }
  
  return mNextBinding ? mNextBinding->FindInsertionPointFor(aChild)
                      : nullptr;
}

XBLChildrenElement*
nsXBLBinding::FindInsertionPointForInternal(nsIContent* aChild)
{
  for (uint32_t i = 0; i < mInsertionPoints.Length(); ++i) {
    XBLChildrenElement* point = mInsertionPoints[i];
    if (point->Includes(aChild)) {
      return point;
    }
  }
  
  return mDefaultInsertionPoint;
}

void
nsXBLBinding::ClearInsertionPoints()
{
  if (mDefaultInsertionPoint) {
    mDefaultInsertionPoint->ClearInsertedChildren();
  }

  for (uint32_t i = 0; i < mInsertionPoints.Length(); ++i) {
    mInsertionPoints[i]->ClearInsertedChildren();
  }
}

nsAnonymousContentList*
nsXBLBinding::GetAnonymousNodeList()
{
  if (!mContent) {
    return mNextBinding ? mNextBinding->GetAnonymousNodeList() : nullptr;
  }

  if (!mAnonymousContentList) {
    mAnonymousContentList = new nsAnonymousContentList(mContent);
  }

  return mAnonymousContentList;
}

void
nsXBLBinding::InstallEventHandlers()
{
  
  if (AllowScripts()) {
    
    nsXBLPrototypeHandler* handlerChain = mPrototypeBinding->GetPrototypeHandlers();

    if (handlerChain) {
      nsEventListenerManager* manager = mBoundElement->GetOrCreateListenerManager();
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
          
          dom::EventListenerFlags flags;
          flags.mCapture = (curr->GetPhase() == NS_PHASE_CAPTURING);

          
          if ((curr->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND |
                                  NS_HANDLER_TYPE_SYSTEM)) &&
              (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
            flags.mInSystemGroup = true;
          }

          bool hasAllowUntrustedAttr = curr->HasAllowUntrustedAttr();
          if ((hasAllowUntrustedAttr && curr->AllowUntrustedEvents()) ||
              (!hasAllowUntrustedAttr && !isChromeDoc)) {
            flags.mAllowUntrustedEvents = true;
          }

          manager->AddEventListenerByType(handler,
                                          nsDependentAtomString(eventAtom),
                                          flags);
        }
      }

      const nsCOMArray<nsXBLKeyEventHandler>* keyHandlers =
        mPrototypeBinding->GetKeyEventHandlers();
      int32_t i;
      for (i = 0; i < keyHandlers->Count(); ++i) {
        nsXBLKeyEventHandler* handler = keyHandlers->ObjectAt(i);
        handler->SetIsBoundToChrome(isChromeDoc);

        nsAutoString type;
        handler->GetEventName(type);

        
        

        
        dom::EventListenerFlags flags;
        flags.mCapture = (handler->GetPhase() == NS_PHASE_CAPTURING);

        if ((handler->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND |
                                   NS_HANDLER_TYPE_SYSTEM)) &&
            (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
          flags.mInSystemGroup = true;
        }

        
        
        
        flags.mAllowUntrustedEvents = true;

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
    return mPrototypeBinding->InstallImplementation(this);

  return NS_OK;
}

nsIAtom*
nsXBLBinding::GetBaseTag(int32_t* aNameSpaceID)
{
  nsIAtom *tag = mPrototypeBinding->GetBaseTag(aNameSpaceID);
  if (!tag && mNextBinding)
    return mNextBinding->GetBaseTag(aNameSpaceID);

  return tag;
}

void
nsXBLBinding::AttributeChanged(nsIAtom* aAttribute, int32_t aNameSpaceID,
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
      mBoundElement->GetExistingListenerManager();
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

      
      dom::EventListenerFlags flags;
      flags.mCapture = (curr->GetPhase() == NS_PHASE_CAPTURING);

      
      

      if ((curr->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND |
                              NS_HANDLER_TYPE_SYSTEM)) &&
          (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
        flags.mInSystemGroup = true;
      }

      manager->RemoveEventListenerByType(handler,
                                         nsDependentAtomString(eventAtom),
                                         flags);
    }

    const nsCOMArray<nsXBLKeyEventHandler>* keyHandlers =
      mPrototypeBinding->GetKeyEventHandlers();
    int32_t i;
    for (i = 0; i < keyHandlers->Count(); ++i) {
      nsXBLKeyEventHandler* handler = keyHandlers->ObjectAt(i);

      nsAutoString type;
      handler->GetEventName(type);

      
      dom::EventListenerFlags flags;
      flags.mCapture = (handler->GetPhase() == NS_PHASE_CAPTURING);

      
      

      if ((handler->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND | NS_HANDLER_TYPE_SYSTEM)) &&
          (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
        flags.mInSystemGroup = true;
      }

      manager->RemoveEventListenerByType(handler, type, flags);
    }
  }
}

static void
UpdateInsertionParent(XBLChildrenElement* aPoint,
                      nsIContent* aOldBoundElement)
{
  if (aPoint->IsDefaultInsertion()) {
    return;
  }

  for (size_t i = 0; i < aPoint->InsertedChildrenLength(); ++i) {
    nsIContent* child = aPoint->mInsertedChildren[i];

    MOZ_ASSERT(child->GetParentNode());

    
    
    
    
    
    
    
    if (child->GetParentNode() == aOldBoundElement) {
      child->SetXBLInsertionParent(nullptr);
    } else {
      child->SetXBLInsertionParent(aOldBoundElement);
    }
  }
}

void
nsXBLBinding::ChangeDocument(nsIDocument* aOldDocument, nsIDocument* aNewDocument)
{
  if (aOldDocument == aNewDocument)
    return;

  
  if (mPrototypeBinding->HasImplementation()) {
    nsCOMPtr<nsIScriptGlobalObject> global =  do_QueryInterface(
                                                                aOldDocument->GetScopeObject());
    if (global) {
      nsCOMPtr<nsIScriptContext> context = global->GetContext();
      if (context) {
        JSContext *cx = context->GetNativeContext();

        nsCxPusher pusher;
        pusher.Push(cx);

        
        
        
        
        
        JS::Rooted<JSObject*> scope(cx, global->GetGlobalJSObject());
        JS::Rooted<JSObject*> scriptObject(cx, mBoundElement->GetWrapper());
        if (scope && scriptObject) {
          
          
          
          
          

          
          JSAutoCompartment ac(cx, scriptObject);

          JS::Rooted<JSObject*> base(cx, scriptObject);
          JS::Rooted<JSObject*> proto(cx);
          for ( ; true; base = proto) { 
            if (!JS_GetPrototype(cx, base, &proto)) {
              return;
            }
            if (!proto) {
              break;
            }

            const JSClass* clazz = ::JS_GetClass(proto);
            if (!clazz ||
                (~clazz->flags &
                 (JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS)) ||
                JSCLASS_RESERVED_SLOTS(clazz) != 1 ||
                clazz->finalize != XBLFinalize) {
              
              continue;
            }

            nsRefPtr<nsXBLDocumentInfo> docInfo =
              static_cast<nsXBLDocumentInfo*>(::JS_GetPrivate(proto));
            if (!docInfo) {
              
              continue;
            }

            JS::Value protoBinding = ::JS_GetReservedSlot(proto, 0);

            if (JSVAL_TO_PRIVATE(protoBinding) != mPrototypeBinding) {
              
              continue;
            }

            
            
            JS::Rooted<JSObject*> grandProto(cx);
            if (!JS_GetPrototype(cx, proto, &grandProto)) {
              return;
            }
            ::JS_SetPrototype(cx, base, grandProto);
            break;
          }

          mPrototypeBinding->UndefineFields(cx, scriptObject);

          
          
          
        }
      }
    }
  }

  
  UnhookEventHandlers();

  {
    nsAutoScriptBlocker scriptBlocker;

    
    
    if (mNextBinding) {
      mNextBinding->ChangeDocument(aOldDocument, aNewDocument);
    }

    
    
    if (mContent) {
      nsXBLBinding::UninstallAnonymousContent(aOldDocument, mContent);
    }

    
    
    
    if (mDefaultInsertionPoint) {
      UpdateInsertionParent(mDefaultInsertionPoint, mBoundElement);
    }

    for (size_t i = 0; i < mInsertionPoints.Length(); ++i) {
      UpdateInsertionParent(mInsertionPoints[i], mBoundElement);
    }

    
    
    ClearInsertionPoints();
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
nsXBLBinding::DoInitJSClass(JSContext *cx, JS::Handle<JSObject*> global,
                            JS::Handle<JSObject*> obj,
                            const nsAFlatCString& aClassName,
                            nsXBLPrototypeBinding* aProtoBinding,
                            JS::MutableHandle<JSObject*> aClassObject,
                            bool* aNew)
{
  
  nsAutoCString className(aClassName);
  nsAutoCString xblKey(aClassName);

  JSAutoCompartment ac(cx, global);

  JS::Rooted<JSObject*> parent_proto(cx, nullptr);
  nsXBLJSClass* c = nullptr;
  if (obj) {
    
    if (!JS_GetPrototype(cx, obj, &parent_proto)) {
      return NS_ERROR_FAILURE;
    }
    if (parent_proto) {
      
      
      
      
      JS::Rooted<jsid> parent_proto_id(cx);
      if (!::JS_GetObjectId(cx, parent_proto, parent_proto_id.address())) {
        
        return NS_ERROR_OUT_OF_MEMORY;
      }

      
      
      
      
      char buf[20];
      if (sizeof(jsid) == 4) {
        PR_snprintf(buf, sizeof(buf), " %lx", parent_proto_id.get());
      } else {
        MOZ_ASSERT(sizeof(jsid) == 8);
        PR_snprintf(buf, sizeof(buf), " %llx", parent_proto_id.get());
      }
      xblKey.Append(buf);

      c = nsXBLService::getClass(xblKey);
      if (c) {
        className.Assign(c->name);
      } else {
        char buf[20];
        PR_snprintf(buf, sizeof(buf), " %llx", nsXBLJSClass::NewId());
        className.Append(buf);
      }
    }
  }

  JS::Rooted<JSObject*> proto(cx);
  JS::Rooted<JS::Value> val(cx);

  if (!::JS_LookupPropertyWithFlags(cx, global, className.get(), 0, &val))
    return NS_ERROR_OUT_OF_MEMORY;

  if (val.isObject()) {
    *aNew = false;
    proto = &val.toObject();
  } else {
    
    *aNew = true;

    nsCStringKey key(xblKey);
    if (!c) {
      c = nsXBLService::getClass(&key);
    }
    if (c) {
      
      if (c->isInList()) {
        c->remove();
        nsXBLService::gClassLRUListLength--;
      }
    } else {
      if (nsXBLService::gClassLRUList->isEmpty()) {
        
        c = new nsXBLJSClass(className, xblKey);
      } else {
        
        c = nsXBLService::gClassLRUList->popFirst();
        nsXBLService::gClassLRUListLength--;

        
        nsCStringKey oldKey(c->Key());
        (nsXBLService::gClassTable)->Remove(&oldKey);

        
        nsMemory::Free((void*) c->name);
        c->name = ToNewCString(className);
        c->SetKey(xblKey);
      }

      
      (nsXBLService::gClassTable)->Put(&key, (void*)c);
    }

    
    c->Hold();

    
    proto = ::JS_InitClass(cx,                  
                           global,              
                           parent_proto,        
                           c,                   
                           nullptr,              
                           0,                   
                           nullptr,              
                           nullptr,              
                           nullptr,              
                           nullptr);             
    if (!proto) {
      
      

      (nsXBLService::gClassTable)->Remove(&key);

      c->Drop();

      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    
    
    
    nsXBLDocumentInfo* docInfo = aProtoBinding->XBLDocumentInfo();
    ::JS_SetPrivate(proto, docInfo);
    NS_ADDREF(docInfo);

    ::JS_SetReservedSlot(proto, 0, PRIVATE_TO_JSVAL(aProtoBinding));
  }

  aClassObject.set(proto);

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

  nsIDocument* doc = mBoundElement ? mBoundElement->OwnerDoc() : nullptr;
  if (!doc) {
    return false;
  }

  nsCOMPtr<nsIScriptGlobalObject> global = do_QueryInterface(doc->GetWindow());
  if (!global) {
    return false;
  }

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) {
    return false;
  }
  
  AutoPushJSContext cx(context->GetNativeContext());

  nsCOMPtr<nsIDocument> ourDocument =
    mPrototypeBinding->XBLDocumentInfo()->GetDocument();
  bool canExecute;
  nsresult rv =
    mgr->CanExecuteScripts(cx, ourDocument->NodePrincipal(), &canExecute);
  return NS_SUCCEEDED(rv) && canExecute;
}

nsXBLBinding*
nsXBLBinding::RootBinding()
{
  if (mNextBinding)
    return mNextBinding->RootBinding();

  return this;
}

bool
nsXBLBinding::ResolveAllFields(JSContext *cx, JS::Handle<JSObject*> obj) const
{
  if (!mPrototypeBinding->ResolveAllFields(cx, obj)) {
    return false;
  }

  if (mNextBinding) {
    return mNextBinding->ResolveAllFields(cx, obj);
  }

  return true;
}

bool
nsXBLBinding::LookupMember(JSContext* aCx, JS::HandleId aId,
                           JS::MutableHandle<JSPropertyDescriptor> aDesc)
{
  
  MOZ_ASSERT(!aDesc.object());

  
  
  if (!JSID_IS_STRING(aId)) {
    return true;
  }
  nsDependentJSString name(aId);

  
  if (!mBoundElement || !mBoundElement->GetWrapper()) {
    return false;
  }

  
  
  JS::Rooted<JSObject*> boundScope(aCx,
    js::GetGlobalForObjectCrossCompartment(mBoundElement->GetWrapper()));
  JS::Rooted<JSObject*> xblScope(aCx, xpc::GetXBLScope(aCx, boundScope));
  NS_ENSURE_TRUE(xblScope, false);
  MOZ_ASSERT(boundScope != xblScope);

  
  {
    JSAutoCompartment ac(aCx, xblScope);
    JS::RootedId id(aCx, aId);
    if (!JS_WrapId(aCx, id.address()) ||
        !LookupMemberInternal(aCx, name, id, aDesc, xblScope))
    {
      return false;
    }
  }

  
  return JS_WrapPropertyDescriptor(aCx, aDesc);
}

bool
nsXBLBinding::LookupMemberInternal(JSContext* aCx, nsString& aName,
                                   JS::HandleId aNameAsId,
                                   JS::MutableHandle<JSPropertyDescriptor> aDesc,
                                   JS::Handle<JSObject*> aXBLScope)
{
  
  
  if (!mJSClass) {
    if (!mNextBinding) {
      return true;
    }
    return mNextBinding->LookupMemberInternal(aCx, aName, aNameAsId,
                                              aDesc, aXBLScope);
  }

  
  
  JS::RootedValue classObject(aCx);
  if (!JS_GetProperty(aCx, aXBLScope, mJSClass->name, &classObject)) {
    return false;
  }

  
  
  
  
  if (classObject.isUndefined()) {
    return true;
  }

  MOZ_ASSERT(classObject.isObject());

  
  
  nsXBLProtoImpl* impl = mPrototypeBinding->GetImplementation();
  if (impl && !impl->LookupMember(aCx, aName, aNameAsId, aDesc,
                                  &classObject.toObject()))
  {
    return false;
  }
  if (aDesc.object() || !mNextBinding) {
    return true;
  }

  return mNextBinding->LookupMemberInternal(aCx, aName, aNameAsId, aDesc,
                                            aXBLScope);
}

bool
nsXBLBinding::HasField(nsString& aName)
{
  
  return mPrototypeBinding->FindField(aName) ||
    (mNextBinding && mNextBinding->HasField(aName));
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
