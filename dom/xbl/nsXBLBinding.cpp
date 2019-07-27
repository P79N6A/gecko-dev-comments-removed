





#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsXBLDocumentInfo.h"
#include "nsIInputStream.h"
#include "nsNameSpaceManager.h"
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


#include "mozilla/EventListenerManager.h"
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

#include "mozilla/DeferredFinalize.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/ShadowRoot.h"

using namespace mozilla;
using namespace mozilla::dom;







static void
XBLFinalize(JSFreeOp *fop, JSObject *obj)
{
  nsXBLDocumentInfo* docInfo =
    static_cast<nsXBLDocumentInfo*>(::JS_GetPrivate(obj));
  DeferredFinalize(docInfo);
}

static bool
XBLEnumerate(JSContext *cx, JS::Handle<JSObject*> obj)
{
  nsXBLPrototypeBinding* protoBinding =
    static_cast<nsXBLPrototypeBinding*>(::JS_GetReservedSlot(obj, 0).toPrivate());
  MOZ_ASSERT(protoBinding);

  return protoBinding->ResolveAllFields(cx, obj);
}

static const JSClass gPrototypeJSClass = {
    "XBL prototype JSClass",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    
    JSCLASS_HAS_RESERVED_SLOTS(1),
    nullptr, nullptr, nullptr, nullptr,
    XBLEnumerate, nullptr, nullptr,
    nullptr, XBLFinalize,
    nullptr, nullptr, nullptr, nullptr
};




nsXBLBinding::nsXBLBinding(nsXBLPrototypeBinding* aBinding)
  : mMarkedForDeath(false)
  , mUsingContentXBLScope(false)
  , mIsShadowRootBinding(false)
  , mPrototypeBinding(aBinding)
{
  NS_ASSERTION(mPrototypeBinding, "Must have a prototype binding!");
  
  NS_ADDREF(mPrototypeBinding->XBLDocumentInfo());
}


nsXBLBinding::nsXBLBinding(ShadowRoot* aShadowRoot, nsXBLPrototypeBinding* aBinding)
  : mMarkedForDeath(false),
    mUsingContentXBLScope(false),
    mIsShadowRootBinding(true),
    mPrototypeBinding(aBinding),
    mContent(aShadowRoot)
{
  NS_ASSERTION(mPrototypeBinding, "Must have a prototype binding!");
  
  NS_ADDREF(mPrototypeBinding->XBLDocumentInfo());
}

nsXBLBinding::~nsXBLBinding(void)
{
  if (mContent && !mIsShadowRootBinding) {
    
    
    
    nsXBLBinding::UninstallAnonymousContent(mContent->OwnerDoc(), mContent);
  }
  nsXBLDocumentInfo* info = mPrototypeBinding->XBLDocumentInfo();
  NS_RELEASE(info);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLBinding)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXBLBinding)
  
  
  if (tmp->mContent && !tmp->mIsShadowRootBinding) {
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

  if (!mBoundElement) {
    return;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIGlobalObject> go = mBoundElement->OwnerDoc()->GetScopeObject();
  NS_ENSURE_TRUE_VOID(go && go->GetGlobalJSObject());
  mUsingContentXBLScope = xpc::UseContentXBLScope(js::GetObjectCompartment(go->GetGlobalJSObject()));
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
          NodeInfo *ni = child->NodeInfo();
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
      EventListenerManager* manager = mBoundElement->GetOrCreateListenerManager();
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
          
          EventListenerFlags flags;
          flags.mCapture = (curr->GetPhase() == NS_PHASE_CAPTURING);

          
          if ((curr->GetType() & (NS_HANDLER_TYPE_XBL_COMMAND |
                                  NS_HANDLER_TYPE_SYSTEM)) &&
              (isChromeBinding || mBoundElement->IsInNativeAnonymousSubtree())) {
            flags.mInSystemGroup = true;
          }

          bool hasAllowUntrustedAttr = curr->HasAllowUntrustedAttr();
          if ((hasAllowUntrustedAttr && curr->AllowUntrustedEvents()) ||
              (!hasAllowUntrustedAttr && !isChromeDoc && !mUsingContentXBLScope)) {
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
        handler->SetUsingContentXBLScope(mUsingContentXBLScope);

        nsAutoString type;
        handler->GetEventName(type);

        
        

        
        EventListenerFlags flags;
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
    EventListenerManager* manager = mBoundElement->GetExistingListenerManager();
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

      
      EventListenerFlags flags;
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

      
      EventListenerFlags flags;
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
    nsIContent* child = aPoint->InsertedChild(i);

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
    AutoJSAPI jsapi;
    
    
    
    
    if (jsapi.Init(aOldDocument->GetScopeObject())) {
      JSContext* cx = jsapi.cx();

      JS::Rooted<JSObject*> scriptObject(cx, mBoundElement->GetWrapper());
      if (scriptObject) {
        
        
        
        
        

        
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

          if (JS_GetClass(proto) != &gPrototypeJSClass) {
            
            continue;
          }

          nsRefPtr<nsXBLDocumentInfo> docInfo =
            static_cast<nsXBLDocumentInfo*>(::JS_GetPrivate(proto));
          if (!docInfo) {
            
            continue;
          }

          JS::Value protoBinding = ::JS_GetReservedSlot(proto, 0);

          if (protoBinding.toPrivate() != mPrototypeBinding) {
            
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

  
  UnhookEventHandlers();

  {
    nsAutoScriptBlocker scriptBlocker;

    
    
    if (mNextBinding) {
      mNextBinding->ChangeDocument(aOldDocument, aNewDocument);
    }

    
    
    if (mContent && !mIsShadowRootBinding) {
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











static JSObject*
GetOrCreateClassObjectMap(JSContext *cx, JS::Handle<JSObject*> scope, const char *mapName)
{
  AssertSameCompartment(cx, scope);
  MOZ_ASSERT(JS_IsGlobalObject(scope));
  MOZ_ASSERT(scope == xpc::GetXBLScopeOrGlobal(cx, scope));

  
  JS::Rooted<JSPropertyDescriptor> desc(cx);
  if (!JS_GetOwnPropertyDescriptor(cx, scope, mapName, &desc)) {
    return nullptr;
  }
  if (desc.object() && desc.value().isObject() &&
      JS::IsWeakMapObject(&desc.value().toObject())) {
    return &desc.value().toObject();
  }

  
  JS::Rooted<JSObject*> map(cx, JS::NewWeakMapObject(cx));
  if (!map || !JS_DefineProperty(cx, scope, mapName, map,
                                 JSPROP_PERMANENT | JSPROP_READONLY,
                                 JS_STUBGETTER, JS_STUBSETTER))
  {
    return nullptr;
  }
  return map;
}

static JSObject*
GetOrCreateMapEntryForPrototype(JSContext *cx, JS::Handle<JSObject*> proto)
{
  AssertSameCompartment(cx, proto);
  
  
  
  
  
  
  
  
  
  
  
  const char* name = xpc::IsInContentXBLScope(proto) ? "__ContentClassObjectMap__"
                                                     : "__XBLClassObjectMap__";

  
  
  
  JS::Rooted<JSObject*> scope(cx, xpc::GetXBLScopeOrGlobal(cx, proto));
  NS_ENSURE_TRUE(scope, nullptr);
  MOZ_ASSERT(js::GetGlobalForObjectCrossCompartment(scope) == scope);

  JS::Rooted<JSObject*> wrappedProto(cx, proto);
  JSAutoCompartment ac(cx, scope);
  if (!JS_WrapObject(cx, &wrappedProto)) {
    return nullptr;
  }

  
  JS::Rooted<JSObject*> map(cx, GetOrCreateClassObjectMap(cx, scope, name));
  if (!map) {
    return nullptr;
  }

  
  JS::Rooted<JS::Value> val(cx);
  if (!JS::GetWeakMapEntry(cx, map, wrappedProto, &val)) {
    return nullptr;
  }
  if (val.isObject()) {
    return &val.toObject();
  }

  
  JS::Rooted<JSObject*> entry(cx);
  entry = JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr());
  if (!entry) {
    return nullptr;
  }
  JS::Rooted<JS::Value> entryVal(cx, JS::ObjectValue(*entry));
  if (!JS::SetWeakMapEntry(cx, map, wrappedProto, entryVal)) {
    NS_WARNING("SetWeakMapEntry failed, probably due to non-preservable WeakMap "
               "key. XBL binding will fail for this element.");
    return nullptr;
  }
  return entry;
}


nsresult
nsXBLBinding::DoInitJSClass(JSContext *cx,
                            JS::Handle<JSObject*> obj,
                            const nsAFlatString& aClassName,
                            nsXBLPrototypeBinding* aProtoBinding,
                            JS::MutableHandle<JSObject*> aClassObject,
                            bool* aNew)
{
  MOZ_ASSERT(obj);

  
  
  
  
  
  
  JS::Rooted<JSObject*> global(cx, js::GetGlobalForObjectCrossCompartment(obj));

  
  JS::Rooted<JSObject*> xblScope(cx, xpc::GetXBLScopeOrGlobal(cx, global));
  NS_ENSURE_TRUE(xblScope, NS_ERROR_UNEXPECTED);

  JS::Rooted<JSObject*> parent_proto(cx);
  if (!JS_GetPrototype(cx, obj, &parent_proto)) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  JS::Rooted<JSObject*> holder(cx);
  if (parent_proto) {
    holder = GetOrCreateMapEntryForPrototype(cx, parent_proto);
  } else {
    JSAutoCompartment innerAC(cx, xblScope);
    holder = GetOrCreateClassObjectMap(cx, xblScope, "__ContentClassObjectMap__");
  }
  if (NS_WARN_IF(!holder)) {
    return NS_ERROR_FAILURE;
  }
  js::AssertSameCompartment(holder, xblScope);
  JSAutoCompartment ac(cx, holder);

  
  
  
  JS::Rooted<JSObject*> proto(cx);
  JS::Rooted<JSPropertyDescriptor> desc(cx);
  if (!JS_GetOwnUCPropertyDescriptor(cx, holder, aClassName.get(), &desc)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNew = !desc.object();
  if (desc.object()) {
    proto = &desc.value().toObject();
    MOZ_ASSERT(JS_GetClass(js::UncheckedUnwrap(proto)) == &gPrototypeJSClass);
  } else {

    
    
    JSAutoCompartment ac2(cx, global);
    proto = JS_NewObjectWithGivenProto(cx, &gPrototypeJSClass, parent_proto);
    if (!proto) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    
    
    
    nsXBLDocumentInfo* docInfo = aProtoBinding->XBLDocumentInfo();
    ::JS_SetPrivate(proto, docInfo);
    NS_ADDREF(docInfo);
    JS_SetReservedSlot(proto, 0, PRIVATE_TO_JSVAL(aProtoBinding));

    
    
    JSAutoCompartment ac3(cx, holder);
    if (!JS_WrapObject(cx, &proto) ||
        !JS_DefineUCProperty(cx, holder, aClassName.get(), -1, proto,
                             JSPROP_READONLY | JSPROP_PERMANENT,
                             JS_STUBGETTER, JS_STUBSETTER))
    {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  JSAutoCompartment ac4(cx, obj);
  if (!JS_WrapObject(cx, &proto) || !JS_SetPrototype(cx, obj, proto)) {
    return NS_ERROR_FAILURE;
  }
  aClassObject.set(proto);
  return NS_OK;
}

bool
nsXBLBinding::AllowScripts()
{
  return mBoundElement && mPrototypeBinding->GetAllowScripts();
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
nsXBLBinding::LookupMember(JSContext* aCx, JS::Handle<jsid> aId,
                           JS::MutableHandle<JSPropertyDescriptor> aDesc)
{
  
  MOZ_ASSERT(!aDesc.object());

  
  
  if (!JSID_IS_STRING(aId)) {
    return true;
  }
  nsAutoJSString name;
  if (!name.init(aCx, JSID_TO_STRING(aId))) {
    return false;
  }

  
  if (!mBoundElement || !mBoundElement->GetWrapper()) {
    return false;
  }

  
  
  
  
  
  
  
  
  
  
  
  JS::Rooted<JSObject*> boundScope(aCx,
    js::GetGlobalForObjectCrossCompartment(mBoundElement->GetWrapper()));
  MOZ_RELEASE_ASSERT(!xpc::IsInAddonScope(boundScope));
  MOZ_RELEASE_ASSERT(!xpc::IsInContentXBLScope(boundScope));
  JS::Rooted<JSObject*> xblScope(aCx, xpc::GetXBLScope(aCx, boundScope));
  NS_ENSURE_TRUE(xblScope, false);
  MOZ_ASSERT(boundScope != xblScope);

  
  {
    JSAutoCompartment ac(aCx, xblScope);
    JS::Rooted<jsid> id(aCx, aId);
    if (!LookupMemberInternal(aCx, name, id, aDesc, xblScope)) {
      return false;
    }
  }

  
  return JS_WrapPropertyDescriptor(aCx, aDesc);
}

bool
nsXBLBinding::LookupMemberInternal(JSContext* aCx, nsString& aName,
                                   JS::Handle<jsid> aNameAsId,
                                   JS::MutableHandle<JSPropertyDescriptor> aDesc,
                                   JS::Handle<JSObject*> aXBLScope)
{
  
  
  
  if (!PrototypeBinding()->HasImplementation()) {
    if (!mNextBinding) {
      return true;
    }
    return mNextBinding->LookupMemberInternal(aCx, aName, aNameAsId,
                                              aDesc, aXBLScope);
  }

  
  
  JS::Rooted<JS::Value> classObject(aCx);
  if (!JS_GetUCProperty(aCx, aXBLScope, PrototypeBinding()->ClassName().get(),
                        -1, &classObject)) {
    return false;
  }

  
  
  
  
  if (classObject.isUndefined()) {
    return true;
  }

  MOZ_ASSERT(classObject.isObject());

  
  
  nsXBLProtoImpl* impl = mPrototypeBinding->GetImplementation();
  JS::Rooted<JSObject*> object(aCx, &classObject.toObject());
  if (impl && !impl->LookupMember(aCx, aName, aNameAsId, aDesc, object)) {
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
