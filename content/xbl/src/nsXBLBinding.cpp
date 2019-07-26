




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
  xpc::DeferredRelease(static_cast<nsIScriptGlobalObjectOwner*>(docInfo));
  
  nsXBLJSClass* c = static_cast<nsXBLJSClass*>(::JS_GetClass(obj));
  c->Drop();
}























static const uint32_t XBLPROTO_SLOT = 0;
static const uint32_t FIELD_SLOT = 1;

bool
ValueHasISupportsPrivate(const JS::Value &v)
{
  if (!v.isObject()) {
    return false;
  }

  JSClass* clasp = ::JS_GetClass(&v.toObject());
  const uint32_t HAS_PRIVATE_NSISUPPORTS =
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS;
  return (clasp->flags & HAS_PRIVATE_NSISUPPORTS) == HAS_PRIVATE_NSISUPPORTS;
}





static JSBool
InstallXBLField(JSContext* cx,
                JS::Handle<JSObject*> callee, JS::Handle<JSObject*> thisObj,
                jsid* idp, bool* installed)
{
  *installed = false;

  
  
  
  
  MOZ_ASSERT(ValueHasISupportsPrivate(JS::ObjectValue(*thisObj)));

  
  
  
  nsCOMPtr<nsIXPConnectWrappedNative> xpcWrapper =
    do_QueryInterface(static_cast<nsISupports*>(::JS_GetPrivate(thisObj)));
  if (!xpcWrapper) {
    
    
    
    
    
    
    return true;
  }

  nsCOMPtr<nsIContent> xblNode = do_QueryWrappedNative(xpcWrapper);
  if (!xblNode) {
    xpc::Throw(cx, NS_ERROR_UNEXPECTED);
    return false;
  }

  
  
  
  

  
  
  
  
  
  
  
  nsXBLPrototypeBinding* protoBinding;
  nsDependentJSString fieldName;
  {
    JSAutoCompartment ac(cx, callee);

    js::Rooted<JSObject*> xblProto(cx);
    xblProto = &js::GetFunctionNativeReserved(callee, XBLPROTO_SLOT).toObject();

    JS::Value name = js::GetFunctionNativeReserved(callee, FIELD_SLOT);
    JSFlatString* fieldStr = JS_ASSERT_STRING_IS_FLAT(name.toString());
    fieldName.init(fieldStr);

    MOZ_ALWAYS_TRUE(JS_ValueToId(cx, name, idp));

    JS::Value slotVal = ::JS_GetReservedSlot(xblProto, 0);
    protoBinding = static_cast<nsXBLPrototypeBinding*>(slotVal.toPrivate());
    MOZ_ASSERT(protoBinding);
  }

  nsXBLProtoImplField* field = protoBinding->FindField(fieldName);
  MOZ_ASSERT(field);

  
  nsIScriptGlobalObject* global = xblNode->OwnerDoc()->GetScriptGlobalObject();
  if (!global) {
    return true;
  }

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) {
    return true;
  }

  nsresult rv = field->InstallField(context, thisObj, xblNode->NodePrincipal(),
                                    protoBinding->DocURI(), installed);
  if (NS_SUCCEEDED(rv)) {
    return true;
  }

  if (!::JS_IsExceptionPending(cx)) {
    xpc::Throw(cx, rv);
  }
  return false;
}

bool
FieldGetterImpl(JSContext *cx, JS::CallArgs args)
{
  const JS::Value &thisv = args.thisv();
  MOZ_ASSERT(ValueHasISupportsPrivate(thisv));

  js::Rooted<JSObject*> thisObj(cx, &thisv.toObject());

  bool installed = false;
  js::Rooted<JSObject*> callee(cx, &args.calleev().toObject());
  js::Rooted<jsid> id(cx);
  if (!InstallXBLField(cx, callee, thisObj, id.address(), &installed)) {
    return false;
  }

  if (!installed) {
    args.rval().setUndefined();
    return true;
  }

  js::Rooted<JS::Value> v(cx);
  if (!JS_GetPropertyById(cx, thisObj, id, v.address())) {
    return false;
  }
  args.rval().set(v);
  return true;
}

static JSBool
FieldGetter(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  return JS::CallNonGenericMethod<ValueHasISupportsPrivate, FieldGetterImpl>
                                 (cx, args);
}

bool
FieldSetterImpl(JSContext *cx, JS::CallArgs args)
{
  const JS::Value &thisv = args.thisv();
  MOZ_ASSERT(ValueHasISupportsPrivate(thisv));

  js::Rooted<JSObject*> thisObj(cx, &thisv.toObject());

  bool installed = false;
  js::Rooted<JSObject*> callee(cx, &args.calleev().toObject());
  js::Rooted<jsid> id(cx);
  if (!InstallXBLField(cx, callee, thisObj, id.address(), &installed)) {
    return false;
  }

  if (installed) {
    js::Rooted<JS::Value> v(cx,
                            args.length() > 0 ? args[0] : JS::UndefinedValue());
    if (!::JS_SetPropertyById(cx, thisObj, id, v.address())) {
      return false;
    }
  }
  args.rval().setUndefined();
  return true;
}

static JSBool
FieldSetter(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  return JS::CallNonGenericMethod<ValueHasISupportsPrivate, FieldSetterImpl>
                                 (cx, args);
}

static JSBool
XBLResolve(JSContext *cx, JSHandleObject obj, JSHandleId id, unsigned flags,
           JSMutableHandleObject objp)
{
  objp.set(NULL);

  if (!JSID_IS_STRING(id)) {
    return true;
  }

  nsXBLPrototypeBinding* protoBinding =
    static_cast<nsXBLPrototypeBinding*>(::JS_GetReservedSlot(obj, 0).toPrivate());
  MOZ_ASSERT(protoBinding);

  
  
  
  nsDependentJSString fieldName(id);
  nsXBLProtoImplField* field = protoBinding->FindField(fieldName);
  if (!field || field->IsEmpty()) {
    return true;
  }

  
  
  js::Rooted<JSObject*> global(cx, JS_GetGlobalForObject(cx, obj));

  js::Rooted<JSObject*> get(cx);
  get = ::JS_GetFunctionObject(js::NewFunctionByIdWithReserved(cx, FieldGetter,
                                                               0, 0, global,
                                                               id));
  if (!get) {
    return false;
  }
  js::SetFunctionNativeReserved(get, XBLPROTO_SLOT, JS::ObjectValue(*obj));
  js::SetFunctionNativeReserved(get, FIELD_SLOT,
                                JS::StringValue(JSID_TO_STRING(id)));

  js::Rooted<JSObject*> set(cx);
  set = ::JS_GetFunctionObject(js::NewFunctionByIdWithReserved(cx, FieldSetter,
                                                               1, 0, global,
                                                               id));
  if (!set) {
    return false;
  }
  js::SetFunctionNativeReserved(set, XBLPROTO_SLOT, JS::ObjectValue(*obj));
  js::SetFunctionNativeReserved(set, FIELD_SLOT,
                                JS::StringValue(JSID_TO_STRING(id)));

  if (!::JS_DefinePropertyById(cx, obj, id, JS::UndefinedValue(),
                               JS_DATA_TO_FUNC_PTR(JSPropertyOp, get.get()),
                               JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, set.get()),
                               field->AccessorAttributes())) {
    return false;
  }

  objp.set(obj);
  return true;
}

static JSBool
XBLEnumerate(JSContext *cx, JS::Handle<JSObject*> obj)
{
  nsXBLPrototypeBinding* protoBinding =
    static_cast<nsXBLPrototypeBinding*>(::JS_GetReservedSlot(obj, 0).toPrivate());
  MOZ_ASSERT(protoBinding);

  return protoBinding->ResolveAllFields(cx, obj);
}

nsXBLJSClass::nsXBLJSClass(const nsAFlatCString& aClassName)
{
  memset(this, 0, sizeof(nsXBLJSClass));
  next = prev = static_cast<JSCList*>(this);
  name = ToNewCString(aClassName);
  flags =
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    JSCLASS_NEW_RESOLVE |
    
    JSCLASS_HAS_RESERVED_SLOTS(1);
  addProperty = delProperty = getProperty = ::JS_PropertyStub;
  setProperty = ::JS_StrictPropertyStub;
  enumerate = XBLEnumerate;
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
    mInsertionPointTable(nullptr)
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
    ImplCycleCollectionTraverse(cb, *aData, "mInsertionPointTable value");
  }
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLBinding)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXBLBinding)
  
  
  if (tmp->mContent) {
    nsXBLBinding::UninstallAnonymousContent(tmp->mContent->OwnerDoc(),
                                            tmp->mContent);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mContent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNextBinding)
  delete tmp->mInsertionPointTable;
  tmp->mInsertionPointTable = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXBLBinding)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb,
                                     "mPrototypeBinding->XBLDocumentInfo()");
  cb.NoteXPCOMChild(static_cast<nsIScriptGlobalObjectOwner*>(
                      tmp->mPrototypeBinding->XBLDocumentInfo()));
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNextBinding)
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

  int32_t count = aData->Length();
  
  if (count == 0)
    return PL_DHASH_NEXT;

  
  nsXBLInsertionPoint* currPoint = aData->ElementAt(0);
  nsCOMPtr<nsIContent> parent = currPoint->GetInsertionParent();
  if (!parent) {
    data->mRv = NS_ERROR_FAILURE;
    return PL_DHASH_STOP;
  }
  int32_t currIndex = currPoint->GetInsertionIndex();

  
  
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

  nsXBLInsertionPoint* pseudoPoint = nullptr;
  uint32_t childCount;
  nodeList->GetLength(&childCount);
  int32_t j = 0;

  for (uint32_t i = 0; i < childCount; i++) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(i, getter_AddRefs(node));
    nsCOMPtr<nsIContent> child(do_QueryInterface(node));
    if (((int32_t)i) == currIndex) {
      
      contentList->AppendElement(currPoint);

      
      j++;
      if (j < count) {
        currPoint = aData->ElementAt(j);
        currIndex = currPoint->GetInsertionIndex();
      }

      
      pseudoPoint = nullptr;
    }
    
    if (!pseudoPoint) {
      pseudoPoint = new nsXBLInsertionPoint(parent, (uint32_t) -1, nullptr);
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

  int32_t count = aData->Length();
 
  for (int32_t i = 0; i < count; i++) {
    nsXBLInsertionPoint* currPoint = aData->ElementAt(i);
    int32_t insCount = currPoint->ChildCount();
    
    if (insCount == 0) {
      nsCOMPtr<nsIContent> defContent = currPoint->GetDefaultContentTemplate();
      if (defContent) {
        
        
        
        nsCOMPtr<nsIContent> insParent = currPoint->GetInsertionParent();
        if (!insParent) {
          data->mRv = NS_ERROR_FAILURE;
          return PL_DHASH_STOP;
        }
        nsIDocument *document = insParent->OwnerDoc();
        nsCOMPtr<nsINode> clonedNode;
        nsCOMArray<nsINode> nodesWithProperties;
        nsNodeUtils::Clone(defContent, true, document->NodeInfoManager(),
                           nodesWithProperties, getter_AddRefs(clonedNode));

        
        
        nsCOMPtr<nsIContent> clonedContent(do_QueryInterface(clonedNode));
        binding->InstallAnonymousContent(clonedContent, insParent,
                                         binding->PrototypeBinding()->
                                           ChromeOnlyContent());

        
        
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
  int32_t count = aData->Length();
  for (int32_t i = 0; i < count; i++) {
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
     
  
  
  uint32_t contentCount = content->GetChildCount();

  
  bool hasContent = (contentCount > 0);
  bool hasInsertionPoints = mPrototypeBinding->HasInsertionPoints();

#ifdef DEBUG
  
  if (nsContentUtils::HasNonEmptyAttr(content, kNameSpaceID_None,
                                      nsGkAtoms::includes)) {
    nsAutoCString message("An XBL Binding with URI ");
    nsAutoCString uri;
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
    uint32_t length;
    children->GetLength(&length);
    if (length > 0 && !hasInsertionPoints) {
      
      
      
      
      for (uint32_t i = 0; i < length; i++) {
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
      nsCOMPtr<nsINode> clonedNode;
      nsCOMArray<nsINode> nodesWithProperties;
      nsNodeUtils::Clone(content, true, doc->NodeInfoManager(),
                         nodesWithProperties, getter_AddRefs(clonedNode));

      mContent = do_QueryInterface(clonedNode);
      InstallAnonymousContent(mContent, mBoundElement,
                              mPrototypeBinding->ChromeOnlyContent());

      if (hasInsertionPoints) {
        
        
      
        
        
        mPrototypeBinding->InstantiateInsertionPoints(this);

        
        
        
        
        
        
        ContentListData data(this, bindingManager);
        mInsertionPointTable->Enumerate(BuildContentLists, &data);
        if (NS_FAILED(data.mRv)) {
          return;
        }

        
        
        uint32_t index = 0;
        bool multiplePoints = false;
        nsIContent *singlePoint = GetSingleInsertionPoint(&index,
                                                          &multiplePoints);
      
        if (children) {
          if (multiplePoints) {
            
            
            children->GetLength(&length);
            for (uint32_t i = 0; i < length; i++) {
              children->Item(i, getter_AddRefs(node));
              childContent = do_QueryInterface(node);

              
              uint32_t index;
              nsIContent *point = GetInsertionPoint(childContent, &index);
              bindingManager->SetInsertionParent(childContent, point);

              
              nsInsertionPointList* arr = nullptr;
              GetInsertionPointsFor(point, &arr);
              nsXBLInsertionPoint* insertionPoint = nullptr;
              int32_t arrCount = arr->Length();
              for (int32_t j = 0; j < arrCount; j++) {
                insertionPoint = arr->ElementAt(j);
                if (insertionPoint->Matches(point, index))
                  break;
                insertionPoint = nullptr;
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

                  
                  mContent = nullptr;
                  bindingManager->SetContentListFor(mBoundElement, nullptr);
                  bindingManager->SetAnonymousNodesFor(mBoundElement, nullptr);
                  return;
                }
              }
            }
          }
          else {
            
            nsInsertionPointList* arr = nullptr;
            GetInsertionPointsFor(singlePoint, &arr);
            nsXBLInsertionPoint* insertionPoint = arr->ElementAt(0);
        
            nsCOMPtr<nsIDOMNode> node;
            nsCOMPtr<nsIContent> content;
            uint32_t length;
            children->GetLength(&length);
          
            for (uint32_t i = 0; i < length; i++) {
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
          
          int32_t flags = (curr->GetPhase() == NS_PHASE_CAPTURING) ?
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
      int32_t i;
      for (i = 0; i < keyHandlers->Count(); ++i) {
        nsXBLKeyEventHandler* handler = keyHandlers->ObjectAt(i);
        handler->SetIsBoundToChrome(isChromeDoc);

        nsAutoString type;
        handler->GetEventName(type);

        
        

        
        int32_t flags = (handler->GetPhase() == NS_PHASE_CAPTURING) ?
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

      
      int32_t flags = (curr->GetPhase() == NS_PHASE_CAPTURING) ?
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
    int32_t i;
    for (i = 0; i < keyHandlers->Count(); ++i) {
      nsXBLKeyEventHandler* handler = keyHandlers->ObjectAt(i);

      nsAutoString type;
      handler->GetEventName(type);

      
      int32_t flags = (handler->GetPhase() == NS_PHASE_CAPTURING) ?
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
                scriptObject = nullptr;

            if (scriptObject) {
              
              
              
              
              

              
              JSObject* base = scriptObject;
              JSObject* proto;
              JSAutoRequest ar(cx);
              JSAutoCompartment ac(cx, scriptObject);

              for ( ; true; base = proto) { 
                if (!JS_GetPrototype(cx, base, &proto)) {
                  return;
                }
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

                
                
                JSObject* grandProto;
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
                                          nullptr);

        nsXBLBinding::UninstallAnonymousContent(aOldDocument, anonymous);
      }

      
      
      nsBindingManager* bindingManager = aOldDocument->BindingManager();
      for (nsIContent* child = mBoundElement->GetLastChild();
           child;
           child = child->GetPreviousSibling()) {
        bindingManager->SetInsertionParent(child, nullptr);
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
  
  nsAutoCString className(aClassName);
  JSObject* parent_proto = nullptr;  
  JSAutoRequest ar(cx);

  JSAutoCompartment ac(cx, global);

  if (obj) {
    
    if (!JS_GetPrototype(cx, obj, &parent_proto)) {
      return NS_ERROR_FAILURE;
    }
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
  if ((!::JS_LookupPropertyWithFlags(cx, global, className.get(), 0, &val)) ||
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

  nsIDocument* doc = mBoundElement ? mBoundElement->OwnerDoc() : nullptr;
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
  return NS_SUCCEEDED(rv) && canExecute;
}

void
nsXBLBinding::RemoveInsertionParent(nsIContent* aParent)
{
  if (mNextBinding) {
    mNextBinding->RemoveInsertionParent(aParent);
  }
  if (mInsertionPointTable) {
    nsInsertionPointList* list = nullptr;
    mInsertionPointTable->Get(aParent, &list);
    if (list) {
      int32_t count = list->Length();
      for (int32_t i = 0; i < count; ++i) {
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
    nsInsertionPointList* list = nullptr;
    mInsertionPointTable->Get(aParent, &list);
    if (list) {
      return true;
    }
  }
  return mNextBinding ? mNextBinding->HasInsertionParent(aParent) : false;
}

void
nsXBLBinding::GetInsertionPointsFor(nsIContent* aParent,
                                    nsInsertionPointList** aResult)
{
  if (!mInsertionPointTable) {
    mInsertionPointTable =
      new nsClassHashtable<nsISupportsHashKey, nsInsertionPointList>;
    mInsertionPointTable->Init(4);
  }

  mInsertionPointTable->Get(aParent, aResult);

  if (!*aResult) {
    *aResult = new nsInsertionPointList;
    mInsertionPointTable->Put(aParent, *aResult);
    if (aParent) {
      aParent->SetFlags(NODE_IS_INSERTION_PARENT);
    }
  }
}

nsInsertionPointList*
nsXBLBinding::GetExistingInsertionPointsFor(nsIContent* aParent)
{
  if (!mInsertionPointTable) {
    return nullptr;
  }

  nsInsertionPointList* result = nullptr;
  mInsertionPointTable->Get(aParent, &result);
  return result;
}

nsIContent*
nsXBLBinding::GetInsertionPoint(const nsIContent* aChild, uint32_t* aIndex)
{
  if (mContent) {
    return mPrototypeBinding->GetInsertionPoint(mBoundElement, mContent,
                                                aChild, aIndex);
  }

  if (mNextBinding)
    return mNextBinding->GetInsertionPoint(aChild, aIndex);

  return nullptr;
}

nsIContent*
nsXBLBinding::GetSingleInsertionPoint(uint32_t* aIndex,
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

  return nullptr;
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

  return mNextBinding ? mNextBinding->GetFirstStyleBinding() : nullptr;
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
    return mContent->ChildNodes();
  }

  if (mNextBinding)
    return mNextBinding->GetAnonymousNodes();

  return nullptr;
}
