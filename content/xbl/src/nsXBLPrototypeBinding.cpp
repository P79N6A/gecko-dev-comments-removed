





































#include "mozilla/Util.h"

#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIInputStream.h"
#include "nsINameSpaceManager.h"
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
#include "nsContentCreatorFunctions.h"
#include "nsIDocument.h"
#include "nsIXMLContentSink.h"
#include "nsContentCID.h"
#include "nsXMLDocument.h"
#include "nsXBLService.h"
#include "nsXBLBinding.h"
#include "nsXBLInsertionPoint.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLContentSink.h"
#include "nsFixedSizeAllocator.h"
#include "xptinfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIDocumentObserver.h"
#include "nsGkAtoms.h"
#include "nsXBLProtoImpl.h"
#include "nsCRT.h"
#include "nsContentUtils.h"

#include "nsIScriptContext.h"
#include "nsIScriptError.h"

#include "nsIStyleRuleProcessor.h"
#include "nsXBLResourceLoader.h"
#include "mozilla/dom/Element.h"

#ifdef MOZ_XUL
#include "nsXULElement.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;




class nsIIDKey : public nsHashKey {
  public:
    nsIID mKey;

  public:
    nsIIDKey(REFNSIID key) : mKey(key) {}
    ~nsIIDKey(void) {}

    PRUint32 HashCode(void) const {
      
      return mKey.m0;
    }

    bool Equals(const nsHashKey *aKey) const {
      return mKey.Equals( ((nsIIDKey*) aKey)->mKey);
    }

    nsHashKey *Clone(void) const {
      return new nsIIDKey(mKey);
    }
};




class nsXBLAttributeEntry {
public:
  nsIAtom* GetSrcAttribute() { return mSrcAttribute; }
  nsIAtom* GetDstAttribute() { return mDstAttribute; }
  PRInt32 GetDstNameSpace() { return mDstNameSpace; }
  
  nsIContent* GetElement() { return mElement; }

  nsXBLAttributeEntry* GetNext() { return mNext; }
  void SetNext(nsXBLAttributeEntry* aEntry) { mNext = aEntry; }

  static nsXBLAttributeEntry*
  Create(nsIAtom* aSrcAtom, nsIAtom* aDstAtom, PRInt32 aDstNameSpace, nsIContent* aContent) {
    void* place = nsXBLPrototypeBinding::kAttrPool->Alloc(sizeof(nsXBLAttributeEntry));
    return place ? ::new (place) nsXBLAttributeEntry(aSrcAtom, aDstAtom, aDstNameSpace, 
                                                     aContent) : nsnull;
  }

  static void
  Destroy(nsXBLAttributeEntry* aSelf) {
    aSelf->~nsXBLAttributeEntry();
    nsXBLPrototypeBinding::kAttrPool->Free(aSelf, sizeof(*aSelf));
  }

protected:
  nsIContent* mElement;

  nsCOMPtr<nsIAtom> mSrcAttribute;
  nsCOMPtr<nsIAtom> mDstAttribute;
  PRInt32 mDstNameSpace;
  nsXBLAttributeEntry* mNext;

  nsXBLAttributeEntry(nsIAtom* aSrcAtom, nsIAtom* aDstAtom, PRInt32 aDstNameSpace,
                      nsIContent* aContent)
    : mElement(aContent),
      mSrcAttribute(aSrcAtom),
      mDstAttribute(aDstAtom),
      mDstNameSpace(aDstNameSpace),
      mNext(nsnull) { }

  ~nsXBLAttributeEntry() {
    NS_CONTENT_DELETE_LIST_MEMBER(nsXBLAttributeEntry, this, mNext);
  }

private:
  
  
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}
};






class nsXBLInsertionPointEntry {
public:
  ~nsXBLInsertionPointEntry() {
    if (mDefaultContent) {
      nsAutoScriptBlocker scriptBlocker;
      
      
      
      mDefaultContent->UnbindFromTree();
    }      
  }

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLInsertionPointEntry)

  nsIContent* GetInsertionParent() { return mInsertionParent; }
  PRUint32 GetInsertionIndex() { return mInsertionIndex; }
  void SetInsertionIndex(PRUint32 aIndex) { mInsertionIndex = aIndex; }

  nsIContent* GetDefaultContent() { return mDefaultContent; }
  void SetDefaultContent(nsIContent* aChildren) { mDefaultContent = aChildren; }


  
  
  
  
  

  static void InitPool(PRInt32 aInitialSize)
  {
    if (++gRefCnt == 1) {
      kPool = new nsFixedSizeAllocator();
      if (kPool) {
        static const size_t kBucketSizes[] = {
          sizeof(nsXBLInsertionPointEntry)
        };
        kPool->Init("XBL Insertion Point Entries", kBucketSizes,
                    ArrayLength(kBucketSizes), aInitialSize);
      }
    }
  }
  static bool PoolInited()
  {
    return kPool != nsnull;
  }
  static void ReleasePool()
  {
    if (--gRefCnt == 0) {
      delete kPool;
    }
  }

  static nsXBLInsertionPointEntry*
  Create(nsIContent* aParent) {
    void* place = kPool->Alloc(sizeof(nsXBLInsertionPointEntry));
    if (!place) {
      return nsnull;
    }
    ++gRefCnt;
    return ::new (place) nsXBLInsertionPointEntry(aParent);
  }

  static void
  Destroy(nsXBLInsertionPointEntry* aSelf) {
    aSelf->~nsXBLInsertionPointEntry();
    kPool->Free(aSelf, sizeof(*aSelf));
    nsXBLInsertionPointEntry::ReleasePool();
  }

  NS_INLINE_DECL_REFCOUNTING(nsXBLInsertionPointEntry)

protected:
  nsCOMPtr<nsIContent> mInsertionParent;
  nsCOMPtr<nsIContent> mDefaultContent;
  PRUint32 mInsertionIndex;

  nsXBLInsertionPointEntry(nsIContent* aParent)
    : mInsertionParent(aParent),
      mInsertionIndex(0) { }

private:
  
  
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}

  static nsFixedSizeAllocator* kPool;
  static PRUint32 gRefCnt;
};

PRUint32 nsXBLInsertionPointEntry::gRefCnt = 0;
nsFixedSizeAllocator* nsXBLInsertionPointEntry::kPool;

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLInsertionPointEntry)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(nsXBLInsertionPointEntry)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mInsertionParent)
  if (tmp->mDefaultContent) {
    nsAutoScriptBlocker scriptBlocker;
    
    
    
    tmp->mDefaultContent->UnbindFromTree();
    tmp->mDefaultContent = nsnull;
  }      
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(nsXBLInsertionPointEntry)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mInsertionParent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDefaultContent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsXBLInsertionPointEntry, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsXBLInsertionPointEntry, Release)




PRUint32 nsXBLPrototypeBinding::gRefCnt = 0;

nsFixedSizeAllocator* nsXBLPrototypeBinding::kAttrPool;

static const PRInt32 kNumElements = 128;

static const size_t kAttrBucketSizes[] = {
  sizeof(nsXBLAttributeEntry)
};

static const PRInt32 kAttrNumBuckets = sizeof(kAttrBucketSizes)/sizeof(size_t);
static const PRInt32 kAttrInitialSize = (NS_SIZE_IN_HEAP(sizeof(nsXBLAttributeEntry))) * kNumElements;

static const PRInt32 kInsInitialSize = (NS_SIZE_IN_HEAP(sizeof(nsXBLInsertionPointEntry))) * kNumElements;




nsXBLPrototypeBinding::nsXBLPrototypeBinding()
: mImplementation(nsnull),
  mBaseBinding(nsnull),
  mInheritStyle(true), 
  mCheckedBaseProto(false),
  mKeyHandlersRegistered(false),
  mResources(nsnull),
  mAttributeTable(nsnull),
  mInsertionPointTable(nsnull),
  mInterfaceTable(nsnull),
  mBaseNameSpaceID(kNameSpaceID_None)
{
  MOZ_COUNT_CTOR(nsXBLPrototypeBinding);
  gRefCnt++;

  if (gRefCnt == 1) {
    kAttrPool = new nsFixedSizeAllocator();
    if (kAttrPool) {
      kAttrPool->Init("XBL Attribute Entries", kAttrBucketSizes, kAttrNumBuckets, kAttrInitialSize);
    }
    nsXBLInsertionPointEntry::InitPool(kInsInitialSize);
  }
}

nsresult
nsXBLPrototypeBinding::Init(const nsACString& aID,
                            nsXBLDocumentInfo* aInfo,
                            nsIContent* aElement,
                            bool aFirstBinding)
{
  if (!kAttrPool || !nsXBLInsertionPointEntry::PoolInited()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = aInfo->DocumentURI()->Clone(getter_AddRefs(mBindingURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (aFirstBinding) {
    rv = mBindingURI->Clone(getter_AddRefs(mAlternateBindingURI));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  mBindingURI->SetRef(aID);

  mXBLDocInfoWeak = aInfo;

  
  
  if (aElement) {
    SetBindingElement(aElement);
  }
  return NS_OK;
}

bool nsXBLPrototypeBinding::CompareBindingURI(nsIURI* aURI) const
{
  bool equal = false;
  mBindingURI->Equals(aURI, &equal);
  if (!equal && mAlternateBindingURI) {
    mAlternateBindingURI->Equals(aURI, &equal);
  }
  return equal;
}

static bool
TraverseInsertionPoint(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsCycleCollectionTraversalCallback &cb = 
    *static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  nsXBLInsertionPointEntry* entry =
    static_cast<nsXBLInsertionPointEntry*>(aData);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(entry,
                                               nsXBLInsertionPointEntry,
                                               "[insertion point table] value")
  return kHashEnumerateNext;
}

static bool
TraverseBinding(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  cb->NoteXPCOMChild(static_cast<nsISupports*>(aData));
  return kHashEnumerateNext;
}

void
nsXBLPrototypeBinding::Traverse(nsCycleCollectionTraversalCallback &cb) const
{
  cb.NoteXPCOMChild(mBinding);
  if (mResources)
    cb.NoteXPCOMChild(mResources->mLoader);
  if (mInsertionPointTable)
    mInsertionPointTable->Enumerate(TraverseInsertionPoint, &cb);
  if (mInterfaceTable)
    mInterfaceTable->Enumerate(TraverseBinding, &cb);
}

void
nsXBLPrototypeBinding::UnlinkJSObjects()
{
  if (mImplementation)
    mImplementation->UnlinkJSObjects();
}

void
nsXBLPrototypeBinding::Trace(TraceCallback aCallback, void *aClosure) const
{
  if (mImplementation)
    mImplementation->Trace(aCallback, aClosure);
}

void
nsXBLPrototypeBinding::Initialize()
{
  nsIContent* content = GetImmediateChild(nsGkAtoms::content);
  if (content) {
    
    
    
    ConstructAttributeTable(content);
    ConstructInsertionTable(content);
  }
}

nsXBLPrototypeBinding::~nsXBLPrototypeBinding(void)
{
  delete mResources;
  delete mAttributeTable;
  delete mInsertionPointTable;
  delete mInterfaceTable;
  delete mImplementation;
  gRefCnt--;
  if (gRefCnt == 0) {
    delete kAttrPool;
    nsXBLInsertionPointEntry::ReleasePool();
  }
  MOZ_COUNT_DTOR(nsXBLPrototypeBinding);
}

void
nsXBLPrototypeBinding::SetBasePrototype(nsXBLPrototypeBinding* aBinding)
{
  if (mBaseBinding == aBinding)
    return;

  if (mBaseBinding) {
    NS_ERROR("Base XBL prototype binding is already defined!");
    return;
  }

  mBaseBinding = aBinding;
}

already_AddRefed<nsIContent>
nsXBLPrototypeBinding::GetBindingElement()
{
  nsIContent* result = mBinding;
  NS_IF_ADDREF(result);
  return result;
}

void
nsXBLPrototypeBinding::SetBindingElement(nsIContent* aElement)
{
  mBinding = aElement;
  if (mBinding->AttrValueIs(kNameSpaceID_None, nsGkAtoms::inheritstyle,
                            nsGkAtoms::_false, eCaseMatters))
    mInheritStyle = false;
}

bool
nsXBLPrototypeBinding::GetAllowScripts()
{
  return mXBLDocInfoWeak->GetScriptAccess();
}

bool
nsXBLPrototypeBinding::LoadResources()
{
  if (mResources) {
    bool result;
    mResources->LoadResources(&result);
    return result;
  }

  return true;
}

nsresult
nsXBLPrototypeBinding::AddResource(nsIAtom* aResourceType, const nsAString& aSrc)
{
  if (!mResources) {
    mResources = new nsXBLPrototypeResources(this);
    if (!mResources)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  mResources->AddResource(aResourceType, aSrc);
  return NS_OK;
}

nsresult
nsXBLPrototypeBinding::FlushSkinSheets()
{
  if (mResources)
    return mResources->FlushSkinSheets();
  return NS_OK;
}

nsresult
nsXBLPrototypeBinding::BindingAttached(nsIContent* aBoundElement)
{
  if (mImplementation && mImplementation->CompiledMembers() &&
      mImplementation->mConstructor)
    return mImplementation->mConstructor->Execute(aBoundElement);
  return NS_OK;
}

nsresult
nsXBLPrototypeBinding::BindingDetached(nsIContent* aBoundElement)
{
  if (mImplementation && mImplementation->CompiledMembers() &&
      mImplementation->mDestructor)
    return mImplementation->mDestructor->Execute(aBoundElement);
  return NS_OK;
}

nsXBLProtoImplAnonymousMethod*
nsXBLPrototypeBinding::GetConstructor()
{
  if (mImplementation)
    return mImplementation->mConstructor;

  return nsnull;
}

nsXBLProtoImplAnonymousMethod*
nsXBLPrototypeBinding::GetDestructor()
{
  if (mImplementation)
    return mImplementation->mDestructor;

  return nsnull;
}

nsresult
nsXBLPrototypeBinding::SetConstructor(nsXBLProtoImplAnonymousMethod* aMethod)
{
  if (!mImplementation)
    return NS_ERROR_FAILURE;
  mImplementation->mConstructor = aMethod;
  return NS_OK;
}

nsresult
nsXBLPrototypeBinding::SetDestructor(nsXBLProtoImplAnonymousMethod* aMethod)
{
  if (!mImplementation)
    return NS_ERROR_FAILURE;
  mImplementation->mDestructor = aMethod;
  return NS_OK;
}

nsresult
nsXBLPrototypeBinding::InstallImplementation(nsIContent* aBoundElement)
{
  if (mImplementation)
    return mImplementation->InstallImplementation(this, aBoundElement);
  return NS_OK;
}


void
nsXBLPrototypeBinding::AttributeChanged(nsIAtom* aAttribute,
                                        PRInt32 aNameSpaceID,
                                        bool aRemoveFlag, 
                                        nsIContent* aChangedElement,
                                        nsIContent* aAnonymousContent,
                                        bool aNotify)
{
  if (!mAttributeTable)
    return;
  nsPRUint32Key nskey(aNameSpaceID);
  nsObjectHashtable *attributesNS = static_cast<nsObjectHashtable*>(mAttributeTable->Get(&nskey));
  if (!attributesNS)
    return;

  nsISupportsKey key(aAttribute);
  nsXBLAttributeEntry* xblAttr = static_cast<nsXBLAttributeEntry*>
                                            (attributesNS->Get(&key));
  if (!xblAttr)
    return;

  
  nsCOMPtr<nsIContent> content = GetImmediateChild(nsGkAtoms::content);
  while (xblAttr) {
    nsIContent* element = xblAttr->GetElement();

    nsCOMPtr<nsIContent> realElement = LocateInstance(aChangedElement, content,
                                                      aAnonymousContent,
                                                      element);

    if (realElement) {
      
      
      nsCOMPtr<nsIAtom> dstAttr = xblAttr->GetDstAttribute();
      PRInt32 dstNs = xblAttr->GetDstNameSpace();

      if (aRemoveFlag)
        realElement->UnsetAttr(dstNs, dstAttr, aNotify);
      else {
        bool attrPresent = true;
        nsAutoString value;
        
        
        if (aAttribute == nsGkAtoms::text && aNameSpaceID == kNameSpaceID_XBL) {
          nsContentUtils::GetNodeTextContent(aChangedElement, false, value);
          value.StripChar(PRUnichar('\n'));
          value.StripChar(PRUnichar('\r'));
          nsAutoString stripVal(value);
          stripVal.StripWhitespace();
          if (stripVal.IsEmpty()) 
            attrPresent = false;
        }    
        else {
          attrPresent = aChangedElement->GetAttr(aNameSpaceID, aAttribute, value);
        }

        if (attrPresent)
          realElement->SetAttr(dstNs, dstAttr, value, aNotify);
      }

      
      
      

      if ((dstAttr == nsGkAtoms::text && dstNs == kNameSpaceID_XBL) ||
          realElement->NodeInfo()->Equals(nsGkAtoms::html,
                                          kNameSpaceID_XUL) &&
          dstAttr == nsGkAtoms::value) {
        
        PRUint32 childCount = realElement->GetChildCount();
        for (PRUint32 i = 0; i < childCount; i++)
          realElement->RemoveChildAt(0, aNotify);

        if (!aRemoveFlag) {
          
          nsAutoString value;
          aChangedElement->GetAttr(aNameSpaceID, aAttribute, value);
          if (!value.IsEmpty()) {
            nsCOMPtr<nsIContent> textContent;
            NS_NewTextNode(getter_AddRefs(textContent),
                           realElement->NodeInfo()->NodeInfoManager());
            if (!textContent) {
              continue;
            }

            textContent->SetText(value, true);
            realElement->AppendChildTo(textContent, true);
          }
        }
      }
    }

    xblAttr = xblAttr->GetNext();
  }
}

struct InsertionData {
  nsXBLBinding* mBinding;
  nsXBLPrototypeBinding* mPrototype;

  InsertionData(nsXBLBinding* aBinding,
                nsXBLPrototypeBinding* aPrototype) 
    :mBinding(aBinding), mPrototype(aPrototype) {}
};

bool InstantiateInsertionPoint(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLInsertionPointEntry* entry = static_cast<nsXBLInsertionPointEntry*>(aData);
  InsertionData* data = static_cast<InsertionData*>(aClosure);
  nsXBLBinding* binding = data->mBinding;
  nsXBLPrototypeBinding* proto = data->mPrototype;

  
  nsIContent* content = entry->GetInsertionParent();
  PRUint32 index = entry->GetInsertionIndex();
  nsIContent* defContent = entry->GetDefaultContent();

  
  nsIContent *instanceRoot = binding->GetAnonymousContent();
  nsIContent *templRoot = proto->GetImmediateChild(nsGkAtoms::content);
  nsIContent *realContent = proto->LocateInstance(nsnull, templRoot,
                                                  instanceRoot, content);
  if (!realContent)
    realContent = binding->GetBoundElement();

  
  nsInsertionPointList* points = nsnull;
  binding->GetInsertionPointsFor(realContent, &points);
  nsXBLInsertionPoint* insertionPoint = nsnull;
  PRInt32 count = points->Length();
  PRInt32 i = 0;
  PRInt32 currIndex = 0;  
  
  for ( ; i < count; i++) {
    nsXBLInsertionPoint* currPoint = points->ElementAt(i);
    currIndex = currPoint->GetInsertionIndex();
    if (currIndex == (PRInt32)index) {
      
      insertionPoint = currPoint;
      break;
    }
    
    if (currIndex > (PRInt32)index)
      
      break;
  }

  if (!insertionPoint) {
    
    insertionPoint = new nsXBLInsertionPoint(realContent, index, defContent);
    if (insertionPoint) {
      points->InsertElementAt(i, insertionPoint);
    }
  }

  return true;
}

void
nsXBLPrototypeBinding::InstantiateInsertionPoints(nsXBLBinding* aBinding)
{
  InsertionData data(aBinding, this);
  if (mInsertionPointTable)
    mInsertionPointTable->Enumerate(InstantiateInsertionPoint, &data);
}

nsIContent*
nsXBLPrototypeBinding::GetInsertionPoint(nsIContent* aBoundElement,
                                         nsIContent* aCopyRoot,
                                         const nsIContent* aChild,
                                         PRUint32* aIndex)
{
  if (!mInsertionPointTable)
    return nsnull;

  nsISupportsKey key(aChild->Tag());
  nsXBLInsertionPointEntry* entry = static_cast<nsXBLInsertionPointEntry*>(mInsertionPointTable->Get(&key));
  if (!entry) {
    nsISupportsKey key2(nsGkAtoms::children);
    entry = static_cast<nsXBLInsertionPointEntry*>(mInsertionPointTable->Get(&key2));
  }

  nsIContent *realContent = nsnull;
  if (entry) {
    nsIContent* content = entry->GetInsertionParent();
    *aIndex = entry->GetInsertionIndex();
    nsIContent* templContent = GetImmediateChild(nsGkAtoms::content);
    realContent = LocateInstance(nsnull, templContent, aCopyRoot, content);
  }
  else {
    
    return nsnull;
  }

  return realContent ? realContent : aBoundElement;
}

nsIContent*
nsXBLPrototypeBinding::GetSingleInsertionPoint(nsIContent* aBoundElement,
                                               nsIContent* aCopyRoot,
                                               PRUint32* aIndex,
                                               bool* aMultipleInsertionPoints)
{ 
  *aMultipleInsertionPoints = false;
  *aIndex = 0;

  if (!mInsertionPointTable)
    return nsnull;

  if (mInsertionPointTable->Count() != 1) {
    *aMultipleInsertionPoints = true;
    return nsnull;
  }

  nsISupportsKey key(nsGkAtoms::children);
  nsXBLInsertionPointEntry* entry =
    static_cast<nsXBLInsertionPointEntry*>(mInsertionPointTable->Get(&key));

  if (!entry) {
    
    
    
    
    

    *aMultipleInsertionPoints = true;
    *aIndex = 0;
    return nsnull;
  }

  *aMultipleInsertionPoints = false;
  *aIndex = entry->GetInsertionIndex();

  nsIContent* templContent = GetImmediateChild(nsGkAtoms::content);
  nsIContent* content = entry->GetInsertionParent();
  nsIContent *realContent = LocateInstance(nsnull, templContent, aCopyRoot,
                                           content);

  return realContent ? realContent : aBoundElement;
}

void
nsXBLPrototypeBinding::SetBaseTag(PRInt32 aNamespaceID, nsIAtom* aTag)
{
  mBaseNameSpaceID = aNamespaceID;
  mBaseTag = aTag;
}

nsIAtom*
nsXBLPrototypeBinding::GetBaseTag(PRInt32* aNamespaceID)
{
  if (mBaseTag) {
    *aNamespaceID = mBaseNameSpaceID;
    return mBaseTag;
  }

  return nsnull;
}

bool
nsXBLPrototypeBinding::ImplementsInterface(REFNSIID aIID) const
{
  
  if (mInterfaceTable) {
    nsIIDKey key(aIID);
    nsCOMPtr<nsISupports> supports = getter_AddRefs(static_cast<nsISupports*>(mInterfaceTable->Get(&key)));
    return supports != nsnull;
  }

  return false;
}



nsIContent*
nsXBLPrototypeBinding::GetImmediateChild(nsIAtom* aTag)
{
  for (nsIContent* child = mBinding->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->NodeInfo()->Equals(aTag, kNameSpaceID_XBL)) {
      return child;
    }
  }

  return nsnull;
}
 
nsresult
nsXBLPrototypeBinding::InitClass(const nsCString& aClassName,
                                 JSContext * aContext, JSObject * aGlobal,
                                 JSObject * aScriptObject,
                                 JSObject** aClassObject)
{
  NS_ENSURE_ARG_POINTER(aClassObject); 

  *aClassObject = nsnull;

  return nsXBLBinding::DoInitJSClass(aContext, aGlobal, aScriptObject,
                                     aClassName, this, aClassObject);
}

nsIContent*
nsXBLPrototypeBinding::LocateInstance(nsIContent* aBoundElement,
                                      nsIContent* aTemplRoot,
                                      nsIContent* aCopyRoot, 
                                      nsIContent* aTemplChild)
{
  
  
  if (aTemplChild == aTemplRoot || !aTemplChild)
    return nsnull;

  nsCOMPtr<nsIContent> templParent = aTemplChild->GetParent();
  nsCOMPtr<nsIContent> childPoint;

  
  if (!templParent)
    return nsnull;
  
  if (aBoundElement) {
    if (templParent->NodeInfo()->Equals(nsGkAtoms::children,
                                        kNameSpaceID_XBL)) {
      childPoint = templParent;
      templParent = childPoint->GetParent();
    }
  }

  if (!templParent)
    return nsnull;

  nsIContent* result = nsnull;
  nsIContent *copyParent;

  if (templParent == aTemplRoot)
    copyParent = aCopyRoot;
  else
    copyParent = LocateInstance(aBoundElement, aTemplRoot, aCopyRoot, templParent);
  
  if (childPoint && aBoundElement) {
    
    
    nsIDocument* doc = aBoundElement->OwnerDoc();
    nsXBLBinding *binding = doc->BindingManager()->GetBinding(aBoundElement);
    nsIContent *anonContent = nsnull;

    while (binding) {
      anonContent = binding->GetAnonymousContent();
      if (anonContent)
        break;

      binding = binding->GetBaseBinding();
    }

    NS_ABORT_IF_FALSE(binding, "Bug 620181 this is unexpected");
    if (!binding)
      return nsnull;

    nsInsertionPointList* points = nsnull;
    if (anonContent == copyParent)
      binding->GetInsertionPointsFor(aBoundElement, &points);
    else
      binding->GetInsertionPointsFor(copyParent, &points);
    PRInt32 count = points->Length();
    for (PRInt32 i = 0; i < count; i++) {
      
      
      
      nsXBLInsertionPoint* currPoint = static_cast<nsXBLInsertionPoint*>(points->ElementAt(i));
      nsCOMPtr<nsIContent> defContent = currPoint->GetDefaultContentTemplate();
      if (defContent == childPoint) {
        
        
        defContent = currPoint->GetDefaultContent();
        if (defContent) {
          
          PRInt32 index = childPoint->IndexOf(aTemplChild);
          
          
          
          result = defContent->GetChildAt(index);
        } 
        break;
      }
    }
  }
  else if (copyParent)
  {
    PRInt32 index = templParent->IndexOf(aTemplChild);
    result = copyParent->GetChildAt(index);
  }

  return result;
}

struct nsXBLAttrChangeData
{
  nsXBLPrototypeBinding* mProto;
  nsIContent* mBoundElement;
  nsIContent* mContent;
  PRInt32 mSrcNamespace;

  nsXBLAttrChangeData(nsXBLPrototypeBinding* aProto,
                      nsIContent* aElt, nsIContent* aContent) 
  :mProto(aProto), mBoundElement(aElt), mContent(aContent) {}
};


bool SetAttrs(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLAttributeEntry* entry = static_cast<nsXBLAttributeEntry*>(aData);
  nsXBLAttrChangeData* changeData = static_cast<nsXBLAttrChangeData*>(aClosure);

  nsIAtom* src = entry->GetSrcAttribute();
  PRInt32 srcNs = changeData->mSrcNamespace;
  nsAutoString value;
  bool attrPresent = true;

  if (src == nsGkAtoms::text && srcNs == kNameSpaceID_XBL) {
    nsContentUtils::GetNodeTextContent(changeData->mBoundElement, false,
                                       value);
    value.StripChar(PRUnichar('\n'));
    value.StripChar(PRUnichar('\r'));
    nsAutoString stripVal(value);
    stripVal.StripWhitespace();

    if (stripVal.IsEmpty()) 
      attrPresent = false;
  }
  else {
    attrPresent = changeData->mBoundElement->GetAttr(srcNs, src, value);
  }

  if (attrPresent) {
    nsIContent* content =
      changeData->mProto->GetImmediateChild(nsGkAtoms::content);

    nsXBLAttributeEntry* curr = entry;
    while (curr) {
      nsIAtom* dst = curr->GetDstAttribute();
      PRInt32 dstNs = curr->GetDstNameSpace();
      nsIContent* element = curr->GetElement();

      nsIContent *realElement =
        changeData->mProto->LocateInstance(changeData->mBoundElement, content,
                                           changeData->mContent, element);

      if (realElement) {
        realElement->SetAttr(dstNs, dst, value, false);

        
        if ((dst == nsGkAtoms::text && dstNs == kNameSpaceID_XBL) ||
            (realElement->NodeInfo()->Equals(nsGkAtoms::html,
                                             kNameSpaceID_XUL) &&
             dst == nsGkAtoms::value && !value.IsEmpty())) {

          nsCOMPtr<nsIContent> textContent;
          NS_NewTextNode(getter_AddRefs(textContent),
                         realElement->NodeInfo()->NodeInfoManager());
          if (!textContent) {
            continue;
          }

          textContent->SetText(value, false);
          realElement->AppendChildTo(textContent, false);
        }
      }

      curr = curr->GetNext();
    }
  }

  return true;
}

bool SetAttrsNS(nsHashKey* aKey, void* aData, void* aClosure)
{
  if (aData && aClosure) {
    nsPRUint32Key * key = static_cast<nsPRUint32Key*>(aKey);
    nsObjectHashtable* xblAttributes =
      static_cast<nsObjectHashtable*>(aData);
    nsXBLAttrChangeData * changeData = static_cast<nsXBLAttrChangeData *>
                                                  (aClosure);
    changeData->mSrcNamespace = key->GetValue();
    xblAttributes->Enumerate(SetAttrs, (void*)changeData);
  }
  return true;
}

void
nsXBLPrototypeBinding::SetInitialAttributes(nsIContent* aBoundElement, nsIContent* aAnonymousContent)
{
  if (mAttributeTable) {
    nsXBLAttrChangeData data(this, aBoundElement, aAnonymousContent);
    mAttributeTable->Enumerate(SetAttrsNS, (void*)&data);
  }
}

nsIStyleRuleProcessor*
nsXBLPrototypeBinding::GetRuleProcessor()
{
  if (mResources) {
    return mResources->mRuleProcessor;
  }
  
  return nsnull;
}

nsXBLPrototypeResources::sheet_array_type*
nsXBLPrototypeBinding::GetStyleSheets()
{
  if (mResources) {
    return &mResources->mStyleSheetList;
  }

  return nsnull;
}

static bool
DeleteAttributeEntry(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLAttributeEntry::Destroy(static_cast<nsXBLAttributeEntry*>(aData));
  return true;
}

static bool
DeleteAttributeTable(nsHashKey* aKey, void* aData, void* aClosure)
{
  delete static_cast<nsObjectHashtable*>(aData);
  return true;
}

void
nsXBLPrototypeBinding::EnsureAttributeTable()
{
  if (!mAttributeTable) {
    mAttributeTable = new nsObjectHashtable(nsnull, nsnull,
                                            DeleteAttributeTable,
                                            nsnull, 4);
  }
}

void
nsXBLPrototypeBinding::AddToAttributeTable(PRInt32 aSourceNamespaceID, nsIAtom* aSourceTag,
                                           PRInt32 aDestNamespaceID, nsIAtom* aDestTag,
                                           nsIContent* aContent)
{
    nsPRUint32Key nskey(aSourceNamespaceID);
    nsObjectHashtable* attributesNS =
      static_cast<nsObjectHashtable*>(mAttributeTable->Get(&nskey));
    if (!attributesNS) {
      attributesNS = new nsObjectHashtable(nsnull, nsnull,
                                           DeleteAttributeEntry,
                                           nsnull, 4);
      mAttributeTable->Put(&nskey, attributesNS);
    }
  
    nsXBLAttributeEntry* xblAttr =
      nsXBLAttributeEntry::Create(aSourceTag, aDestTag, aDestNamespaceID, aContent);

    nsISupportsKey key(aSourceTag);
    nsXBLAttributeEntry* entry = static_cast<nsXBLAttributeEntry*>
                                            (attributesNS->Get(&key));
    if (!entry) {
      attributesNS->Put(&key, xblAttr);
    } else {
      while (entry->GetNext())
        entry = entry->GetNext();
      entry->SetNext(xblAttr);
    }
}

void
nsXBLPrototypeBinding::ConstructAttributeTable(nsIContent* aElement)
{
  
  
  if (!aElement->NodeInfo()->Equals(nsGkAtoms::children, kNameSpaceID_XBL)) {
    nsAutoString inherits;
    aElement->GetAttr(kNameSpaceID_XBL, nsGkAtoms::inherits, inherits);

    if (!inherits.IsEmpty()) {
      EnsureAttributeTable();

      
      char* str = ToNewCString(inherits);
      char* newStr;
      
      

      char* token = nsCRT::strtok( str, ", ", &newStr );
      while( token != NULL ) {
        
        nsCOMPtr<nsIAtom> atom;
        PRInt32 atomNsID = kNameSpaceID_None;
        nsCOMPtr<nsIAtom> attribute;
        PRInt32 attributeNsID = kNameSpaceID_None;

        
        nsAutoString attrTok; attrTok.AssignWithConversion(token);
        PRInt32 index = attrTok.Find("=", true);
        nsresult rv;
        if (index != -1) {
          
          nsAutoString left, right;
          attrTok.Left(left, index);
          attrTok.Right(right, attrTok.Length()-index-1);

          rv = nsContentUtils::SplitQName(aElement, left, &attributeNsID,
                                          getter_AddRefs(attribute));
          if (NS_FAILED(rv))
            return;

          rv = nsContentUtils::SplitQName(aElement, right, &atomNsID,
                                          getter_AddRefs(atom));
          if (NS_FAILED(rv))
            return;
        }
        else {
          nsAutoString tok;
          tok.AssignWithConversion(token);
          rv = nsContentUtils::SplitQName(aElement, tok, &atomNsID, 
                                          getter_AddRefs(atom));
          if (NS_FAILED(rv))
            return;
          attribute = atom;
          attributeNsID = atomNsID;
        }

        AddToAttributeTable(atomNsID, atom, attributeNsID, attribute, aElement);

        
        
        
        
        
        

        token = nsCRT::strtok( newStr, ", ", &newStr );
      }

      nsMemory::Free(str);
    }
  }

  
  for (nsIContent* child = aElement->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    ConstructAttributeTable(child);
  }
}

static bool
DeleteInsertionPointEntry(nsHashKey* aKey, void* aData, void* aClosure)
{
  static_cast<nsXBLInsertionPointEntry*>(aData)->Release();
  return true;
}

void 
nsXBLPrototypeBinding::ConstructInsertionTable(nsIContent* aContent)
{
  nsCOMArray<nsIContent> childrenElements;
  GetNestedChildren(nsGkAtoms::children, kNameSpaceID_XBL, aContent,
                    childrenElements);

  PRInt32 count = childrenElements.Count();
  if (count == 0)
    return;

  mInsertionPointTable = new nsObjectHashtable(nsnull, nsnull,
                                               DeleteInsertionPointEntry,
                                               nsnull, 4);
  if (!mInsertionPointTable)
    return;

  PRInt32 i;
  for (i = 0; i < count; i++) {
    nsIContent* child = childrenElements[i];
    nsCOMPtr<nsIContent> parent = child->GetParent(); 

    
    nsXBLInsertionPointEntry* xblIns = nsXBLInsertionPointEntry::Create(parent);

    nsAutoString includes;
    child->GetAttr(kNameSpaceID_None, nsGkAtoms::includes, includes);
    if (includes.IsEmpty()) {
      nsISupportsKey key(nsGkAtoms::children);
      xblIns->AddRef();
      mInsertionPointTable->Put(&key, xblIns);
    }
    else {
      
      char* str = ToNewCString(includes);
      char* newStr;
      
      

      char* token = nsCRT::strtok( str, "| ", &newStr );
      while( token != NULL ) {
        nsAutoString tok;
        tok.AssignWithConversion(token);

        
        nsCOMPtr<nsIAtom> atom = do_GetAtom(tok);

        nsISupportsKey key(atom);
        xblIns->AddRef();
        mInsertionPointTable->Put(&key, xblIns);
          
        token = nsCRT::strtok( newStr, "| ", &newStr );
      }

      nsMemory::Free(str);
    }

    
    
    
    
    
    
    
    PRInt32 index = parent->IndexOf(child);
    xblIns->SetInsertionIndex((PRUint32)index);

    
    
    
    parent->RemoveChildAt(index, false);

    
    
    
    PRUint32 defaultCount = child->GetChildCount();
    if (defaultCount > 0) {
      nsAutoScriptBlocker scriptBlocker;
      
      xblIns->SetDefaultContent(child);

      
      
      
      
      nsresult rv =
        child->BindToTree(parent->GetCurrentDoc(), parent, nsnull, false);
      if (NS_FAILED(rv)) {
        
        
        child->UnbindFromTree();
        return;
      }
    }
  }
}

nsresult
nsXBLPrototypeBinding::ConstructInterfaceTable(const nsAString& aImpls)
{
  if (!aImpls.IsEmpty()) {
    
    
    nsCOMPtr<nsIInterfaceInfoManager>
      infoManager(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
    if (!infoManager)
      return NS_ERROR_FAILURE;

    
    if (!mInterfaceTable)
      mInterfaceTable = new nsSupportsHashtable(4);

    
    NS_ConvertUTF16toUTF8 utf8impl(aImpls);
    char* str = utf8impl.BeginWriting();
    char* newStr;
    
    

    char* token = nsCRT::strtok( str, ", ", &newStr );
    while( token != NULL ) {
      
      nsCOMPtr<nsIInterfaceInfo> iinfo;
      infoManager->GetInfoForName(token, getter_AddRefs(iinfo));

      if (iinfo) {
        
        const nsIID* iid = nsnull;
        iinfo->GetIIDShared(&iid);

        if (iid) {
          
          nsIIDKey key(*iid);
          mInterfaceTable->Put(&key, mBinding);

          
          
          nsCOMPtr<nsIInterfaceInfo> parentInfo;
          
          while (NS_SUCCEEDED(iinfo->GetParent(getter_AddRefs(parentInfo))) && parentInfo) {
            
            parentInfo->GetIIDShared(&iid);

            
            if (!iid || iid->Equals(NS_GET_IID(nsISupports)))
              break;

            
            nsIIDKey parentKey(*iid);
            mInterfaceTable->Put(&parentKey, mBinding);

            
            iinfo = parentInfo;
          }
        }
      }

      token = nsCRT::strtok( newStr, ", ", &newStr );
    }
  }

  return NS_OK;
}

void
nsXBLPrototypeBinding::GetNestedChildren(nsIAtom* aTag, PRInt32 aNamespace,
                                         nsIContent* aContent,
                                         nsCOMArray<nsIContent> & aList)
{
  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {

    if (child->NodeInfo()->Equals(aTag, aNamespace)) {
      aList.AppendObject(child);
    }
    else
      GetNestedChildren(aTag, aNamespace, child, aList);
  }
}

nsresult
nsXBLPrototypeBinding::AddResourceListener(nsIContent* aBoundElement)
{
  if (!mResources)
    return NS_ERROR_FAILURE; 
                             

  mResources->AddResourceListener(aBoundElement);
  return NS_OK;
}

void
nsXBLPrototypeBinding::CreateKeyHandlers()
{
  nsXBLPrototypeHandler* curr = mPrototypeHandler;
  while (curr) {
    nsCOMPtr<nsIAtom> eventAtom = curr->GetEventName();
    if (eventAtom == nsGkAtoms::keyup ||
        eventAtom == nsGkAtoms::keydown ||
        eventAtom == nsGkAtoms::keypress) {
      PRUint8 phase = curr->GetPhase();
      PRUint8 type = curr->GetType();

      PRInt32 count = mKeyHandlers.Count();
      PRInt32 i;
      nsXBLKeyEventHandler* handler = nsnull;
      for (i = 0; i < count; ++i) {
        handler = mKeyHandlers[i];
        if (handler->Matches(eventAtom, phase, type))
          break;
      }

      if (i == count) {
        nsRefPtr<nsXBLKeyEventHandler> newHandler;
        NS_NewXBLKeyEventHandler(eventAtom, phase, type,
                                 getter_AddRefs(newHandler));
        if (newHandler)
          mKeyHandlers.AppendObject(newHandler);
        handler = newHandler;
      }

      if (handler)
        handler->AddProtoHandler(curr);
    }

    curr = curr->GetNextHandler();
  }
}

nsresult
nsXBLPrototypeBinding::Read(nsIObjectInputStream* aStream,
                            nsXBLDocumentInfo* aDocInfo,
                            nsIDocument* aDocument,
                            PRUint8 aFlags)
{
  mInheritStyle = (aFlags & XBLBinding_Serialize_InheritStyle) ? true : false;

  
  
  nsCAutoString id;
  nsresult rv = aStream->ReadCString(id);

  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(!id.IsEmpty(), NS_ERROR_FAILURE);

  nsCAutoString baseBindingURI;
  rv = aStream->ReadCString(baseBindingURI);
  NS_ENSURE_SUCCESS(rv, rv);
  mCheckedBaseProto = true;

  if (!baseBindingURI.IsEmpty()) {
    rv = NS_NewURI(getter_AddRefs(mBaseBindingURI), baseBindingURI);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = ReadNamespace(aStream, mBaseNameSpaceID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString baseTag;
  rv = aStream->ReadString(baseTag);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!baseTag.IsEmpty()) {
    mBaseTag = do_GetAtom(baseTag);
  }

  aDocument->CreateElem(NS_LITERAL_STRING("binding"), nsnull, kNameSpaceID_XBL,
                        getter_AddRefs(mBinding));

  nsCOMPtr<nsIContent> child;
  rv = ReadContentNode(aStream, aDocument, aDocument->NodeInfoManager(), getter_AddRefs(child));
  NS_ENSURE_SUCCESS(rv, rv);

  Element* rootElement = aDocument->GetRootElement();
  if (rootElement)
    rootElement->AppendChildTo(mBinding, false);

  if (child) {
    mBinding->AppendChildTo(child, false);
  }

  PRUint32 interfaceCount;
  rv = aStream->Read32(&interfaceCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (interfaceCount > 0) {
    NS_ASSERTION(!mInterfaceTable, "non-null mInterfaceTable");
    mInterfaceTable = new nsSupportsHashtable(interfaceCount);
    NS_ENSURE_TRUE(mInterfaceTable, NS_ERROR_OUT_OF_MEMORY);

    for (; interfaceCount > 0; interfaceCount--) {
      nsIID iid;
      aStream->ReadID(&iid);
      nsIIDKey key(iid);
      mInterfaceTable->Put(&key, mBinding);
    }
  }

  nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner(do_QueryObject(aDocInfo));
  nsIScriptGlobalObject* globalObject = globalOwner->GetScriptGlobalObject();
  NS_ENSURE_TRUE(globalObject, NS_ERROR_UNEXPECTED);

  nsIScriptContext *context = globalObject->GetContext();
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);

  bool isFirstBinding = aFlags & XBLBinding_Serialize_IsFirstBinding;
  rv = Init(id, aDocInfo, nsnull, isFirstBinding);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = aDocInfo->SetPrototypeBinding(id, this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString className;
  rv = aStream->ReadCString(className);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!className.IsEmpty()) {
    nsXBLProtoImpl* impl; 
    NS_NewXBLProtoImpl(this, NS_ConvertUTF8toUTF16(className).get(), &impl);

    
    
    
    
    rv = mImplementation->Read(context, aStream, this, globalObject);
    if (NS_FAILED(rv)) {
      aDocInfo->RemovePrototypeBinding(id);
      return rv;
    }
  }

  
  nsXBLPrototypeHandler* previousHandler = nsnull;

  do {
    XBLBindingSerializeDetails type;
    rv = aStream->Read8(&type);
    NS_ENSURE_SUCCESS(rv, rv);

    if (type == XBLBinding_Serialize_NoMoreItems)
      break;

    NS_ASSERTION((type & XBLBinding_Serialize_Mask) == XBLBinding_Serialize_Handler,
                 "invalid handler type");

    nsXBLPrototypeHandler* handler = new nsXBLPrototypeHandler(this);
    rv = handler->Read(context, aStream);
    if (NS_FAILED(rv)) {
      delete handler;
      return rv;
    }

    if (previousHandler) {
      previousHandler->SetNextHandler(handler);
    }
    else {
      SetPrototypeHandlers(handler);
    }
    previousHandler = handler;
  } while (1);

  
  do {
    XBLBindingSerializeDetails type;
    rv = aStream->Read8(&type);
    NS_ENSURE_SUCCESS(rv, rv);

    if (type == XBLBinding_Serialize_NoMoreItems)
      break;

    NS_ASSERTION((type & XBLBinding_Serialize_Mask) == XBLBinding_Serialize_Stylesheet ||
                 (type & XBLBinding_Serialize_Mask) == XBLBinding_Serialize_Image, "invalid resource type");

    nsAutoString src;
    rv = aStream->ReadString(src);
    NS_ENSURE_SUCCESS(rv, rv);

    AddResource(type == XBLBinding_Serialize_Stylesheet ? nsGkAtoms::stylesheet :
                                                          nsGkAtoms::image, src);
  } while (1);

  if (isFirstBinding) {
    aDocInfo->SetFirstPrototypeBinding(this);
  }

  return NS_OK;
}

static
bool
GatherInsertionPoints(nsHashKey *aKey, void *aData, void* aClosure)
{
  ArrayOfInsertionPointsByContent* insertionPointsByContent =
    static_cast<ArrayOfInsertionPointsByContent *>(aClosure);

  nsXBLInsertionPointEntry* entry = static_cast<nsXBLInsertionPointEntry *>(aData);

  
  nsAutoTArray<InsertionItem, 1>* list;
  if (!insertionPointsByContent->Get(entry->GetInsertionParent(), &list)) {
    list = new nsAutoTArray<InsertionItem, 1>;
    insertionPointsByContent->Put(entry->GetInsertionParent(), list);
  }

  
  nsIAtom* atom = static_cast<nsIAtom *>(
                    static_cast<nsISupportsKey *>(aKey)->GetValue());
  InsertionItem newitem(entry->GetInsertionIndex(), atom, entry->GetDefaultContent());
  list->InsertElementSorted(newitem);

  return kHashEnumerateNext;
}

static
bool
WriteInterfaceID(nsHashKey *aKey, void *aData, void* aClosure)
{
  
  
  nsID iid = ((nsIIDKey *)aKey)->mKey;
  static_cast<nsIObjectOutputStream *>(aClosure)->WriteID(iid);
  return kHashEnumerateNext;
}

nsresult
nsXBLPrototypeBinding::Write(nsIObjectOutputStream* aStream)
{
  
  
  

  nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner(do_QueryObject(mXBLDocInfoWeak));
  nsIScriptGlobalObject* globalObject = globalOwner->GetScriptGlobalObject();
  NS_ENSURE_TRUE(globalObject, NS_ERROR_UNEXPECTED);

  nsIScriptContext *context = globalObject->GetContext();
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);

  PRUint8 flags = mInheritStyle ? XBLBinding_Serialize_InheritStyle : 0;

  
  if (mAlternateBindingURI) {
    flags |= XBLBinding_Serialize_IsFirstBinding;
  }

  nsresult rv = aStream->Write8(flags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString id;
  mBindingURI->GetRef(id);
  rv = aStream->WriteStringZ(id.get());
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString extends;
  ResolveBaseBinding();
  if (mBaseBindingURI)
    mBaseBindingURI->GetSpec(extends);

  rv = aStream->WriteStringZ(extends.get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = WriteNamespace(aStream, mBaseNameSpaceID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString baseTag;
  if (mBaseTag) {
    mBaseTag->ToString(baseTag);
  }
  rv = aStream->WriteWStringZ(baseTag.get());
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  ArrayOfInsertionPointsByContent insertionPointsByContent;
  insertionPointsByContent.Init();
  if (mInsertionPointTable) {
    mInsertionPointTable->Enumerate(GatherInsertionPoints, &insertionPointsByContent);
  }

  nsIContent* content = GetImmediateChild(nsGkAtoms::content);
  if (content) {
    rv = WriteContentNode(aStream, content, insertionPointsByContent);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    rv = aStream->Write8(XBLBinding_Serialize_NoContent);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (mInterfaceTable) {
    rv = aStream->Write32(mInterfaceTable->Count());
    NS_ENSURE_SUCCESS(rv, rv);

    mInterfaceTable->Enumerate(WriteInterfaceID, aStream);
  }
  else {
    rv = aStream->Write32(0);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (mImplementation) {
    rv = mImplementation->Write(context, aStream, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    
    rv = aStream->WriteWStringZ(EmptyString().get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsXBLPrototypeHandler* handler = mPrototypeHandler;
  while (handler) {
    rv = handler->Write(context, aStream);
    NS_ENSURE_SUCCESS(rv, rv);

    handler = handler->GetNextHandler();
  }

  aStream->Write8(XBLBinding_Serialize_NoMoreItems);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mResources) {
    rv = mResources->Write(aStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  return aStream->Write8(XBLBinding_Serialize_NoMoreItems);
}

nsresult
nsXBLPrototypeBinding::ReadContentNode(nsIObjectInputStream* aStream,
                                       nsIDocument* aDocument,
                                       nsNodeInfoManager* aNim,
                                       nsIContent** aContent)
{
  *aContent = nsnull;

  PRInt32 namespaceID;
  nsresult rv = ReadNamespace(aStream, namespaceID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (namespaceID == XBLBinding_Serialize_NoContent)
    return NS_OK;

  nsCOMPtr<nsIContent> content;

  
  if (namespaceID == XBLBinding_Serialize_TextNode ||
      namespaceID == XBLBinding_Serialize_CDATANode ||
      namespaceID == XBLBinding_Serialize_CommentNode) {
    switch (namespaceID) {
      case XBLBinding_Serialize_TextNode:
        rv = NS_NewTextNode(getter_AddRefs(content), aNim);
        break;
      case XBLBinding_Serialize_CDATANode:
        rv = NS_NewXMLCDATASection(getter_AddRefs(content), aNim);
        break;
      case XBLBinding_Serialize_CommentNode:
        rv = NS_NewCommentNode(getter_AddRefs(content), aNim);
        break;
      default:
        break;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString text;
    rv = aStream->ReadString(text);
    NS_ENSURE_SUCCESS(rv, rv);

    content->SetText(text, false);
    content.swap(*aContent);
    return NS_OK;
  }

  
  nsAutoString prefix, tag;
  rv = aStream->ReadString(prefix);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAtom> prefixAtom;
  if (!prefix.IsEmpty())
    prefixAtom = do_GetAtom(prefix);

  rv = aStream->ReadString(tag);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAtom> tagAtom = do_GetAtom(tag);
  nsCOMPtr<nsINodeInfo> nodeInfo =
    aNim->GetNodeInfo(tagAtom, prefixAtom, namespaceID, nsIDOMNode::ELEMENT_NODE);

  PRUint32 attrCount;
  rv = aStream->Read32(&attrCount);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
#ifdef MOZ_XUL
  if (namespaceID == kNameSpaceID_XUL) {
    nsIURI* documentURI = aDocument->GetDocumentURI();

    nsRefPtr<nsXULPrototypeElement> prototype = new nsXULPrototypeElement();
    NS_ENSURE_TRUE(prototype, NS_ERROR_OUT_OF_MEMORY);

    prototype->mNodeInfo = nodeInfo;
    prototype->mScriptTypeID = nsIProgrammingLanguage::JAVASCRIPT;

    nsCOMPtr<Element> result;
    nsresult rv =
      nsXULElement::Create(prototype, aDocument, false, getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);
    content = result;

    nsXULPrototypeAttribute* attrs = nsnull;
    if (attrCount > 0) {
      attrs = new nsXULPrototypeAttribute[attrCount];
    }

    prototype->mAttributes = attrs;
    prototype->mNumAttributes = attrCount;

    for (PRUint32 i = 0; i < attrCount; i++) {
      rv = ReadNamespace(aStream, namespaceID);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString prefix, name, val;
      rv = aStream->ReadString(prefix);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aStream->ReadString(name);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aStream->ReadString(val);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(name);
      if (namespaceID == kNameSpaceID_None) {
        attrs[i].mName.SetTo(nameAtom);
      }
      else {
        nsCOMPtr<nsIAtom> prefixAtom;
        if (!prefix.IsEmpty())
          prefixAtom = do_GetAtom(prefix);

        nsCOMPtr<nsINodeInfo> ni =
          aNim->GetNodeInfo(nameAtom, prefixAtom,
                            namespaceID, nsIDOMNode::ATTRIBUTE_NODE);
        attrs[i].mName.SetTo(ni);
      }
      
      rv = prototype->SetAttrAt(i, val, documentURI);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
#endif
    NS_NewElement(getter_AddRefs(content), nodeInfo.forget(), NOT_FROM_PARSER);

    for (PRUint32 i = 0; i < attrCount; i++) {
      rv = ReadNamespace(aStream, namespaceID);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString prefix, name, val;
      rv = aStream->ReadString(prefix);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aStream->ReadString(name);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aStream->ReadString(val);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIAtom> prefixAtom;
      if (!prefix.IsEmpty())
        prefixAtom = do_GetAtom(prefix);

      nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(name);
      content->SetAttr(namespaceID, nameAtom, prefixAtom, val, false);
    }

#ifdef MOZ_XUL
  }
#endif

  

  PRInt32 srcNamespaceID, destNamespaceID;
  rv = ReadNamespace(aStream, srcNamespaceID);
  NS_ENSURE_SUCCESS(rv, rv);

  while (srcNamespaceID != XBLBinding_Serialize_NoMoreAttributes) {
    nsAutoString srcAttribute, destAttribute;
    rv = aStream->ReadString(srcAttribute);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = ReadNamespace(aStream, destNamespaceID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aStream->ReadString(destAttribute);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIAtom> srcAtom = do_GetAtom(srcAttribute);
    nsCOMPtr<nsIAtom> destAtom = do_GetAtom(destAttribute);

    EnsureAttributeTable();
    AddToAttributeTable(srcNamespaceID, srcAtom, destNamespaceID, destAtom, content);

    rv = ReadNamespace(aStream, srcNamespaceID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRUint32 insertionPointIndex;
  rv = aStream->Read32(&insertionPointIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  while (insertionPointIndex != XBLBinding_Serialize_NoMoreInsertionPoints) {
    nsRefPtr<nsXBLInsertionPointEntry> xblIns =
      nsXBLInsertionPointEntry::Create(content);
    xblIns->SetInsertionIndex(insertionPointIndex);

    
    nsCOMPtr<nsIContent> defaultContent;
    rv = ReadContentNode(aStream, aDocument, aNim, getter_AddRefs(defaultContent));
    NS_ENSURE_SUCCESS(rv, rv);

    if (defaultContent) {
      xblIns->SetDefaultContent(defaultContent);

      rv = defaultContent->BindToTree(nsnull, content, nsnull, false);
      if (NS_FAILED(rv)) {
        defaultContent->UnbindFromTree();
        return rv;
      }
    }

    if (!mInsertionPointTable) {
      mInsertionPointTable = new nsObjectHashtable(nsnull, nsnull,
                                                   DeleteInsertionPointEntry,
                                                   nsnull, 4);
    }

    
    
    
    PRUint32 count;
    rv = aStream->Read32(&count);
    NS_ENSURE_SUCCESS(rv, rv);

    for (; count > 0; count --) {
      aStream->ReadString(tag);
      nsCOMPtr<nsIAtom> tagAtom = do_GetAtom(tag);

      nsISupportsKey key(tagAtom);
      NS_ADDREF(xblIns.get());
      mInsertionPointTable->Put(&key, xblIns);
    }

    rv = aStream->Read32(&insertionPointIndex);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRUint32 childCount;
  rv = aStream->Read32(&childCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < childCount; i++) {
    nsCOMPtr<nsIContent> child;
    ReadContentNode(aStream, aDocument, aNim, getter_AddRefs(child));

    
    if (child) {
      content->AppendChildTo(child, false);
    }
  }

  content.swap(*aContent);
  return NS_OK;
}




struct WriteAttributeData
{
  nsXBLPrototypeBinding* binding;
  nsIObjectOutputStream* stream;
  nsIContent* content;
  PRInt32 srcNamespace;

  WriteAttributeData(nsXBLPrototypeBinding* aBinding,
                     nsIObjectOutputStream* aStream,
                     nsIContent* aContent)
    : binding(aBinding), stream(aStream), content(aContent)
  { }
};

static
bool
WriteAttribute(nsHashKey *aKey, void *aData, void* aClosure)
{
  WriteAttributeData* data = static_cast<WriteAttributeData *>(aClosure);
  nsIObjectOutputStream* stream = data->stream;
  const PRInt32 srcNamespace = data->srcNamespace;

  nsXBLAttributeEntry* entry = static_cast<nsXBLAttributeEntry *>(aData);
  do {
    if (entry->GetElement() == data->content) {
      data->binding->WriteNamespace(stream, srcNamespace);
      stream->WriteWStringZ(nsDependentAtomString(entry->GetSrcAttribute()).get());
      data->binding->WriteNamespace(stream, entry->GetDstNameSpace());
      stream->WriteWStringZ(nsDependentAtomString(entry->GetDstAttribute()).get());
    }

    entry = entry->GetNext();
  } while (entry);

  return kHashEnumerateNext;
}





static
bool
WriteAttributeNS(nsHashKey *aKey, void *aData, void* aClosure)
{
  WriteAttributeData* data = static_cast<WriteAttributeData *>(aClosure);
  data->srcNamespace = static_cast<nsPRUint32Key *>(aKey)->GetValue();

  nsObjectHashtable* attributes = static_cast<nsObjectHashtable*>(aData);
  attributes->Enumerate(WriteAttribute, data);

  return kHashEnumerateNext;
}

nsresult
nsXBLPrototypeBinding::WriteContentNode(nsIObjectOutputStream* aStream,
                                        nsIContent* aNode,
                                        ArrayOfInsertionPointsByContent& aInsertionPointsByContent)
{
  nsresult rv;

  if (!aNode->IsElement()) {
    
    PRUint8 type = XBLBinding_Serialize_NoContent;
    switch (aNode->NodeType()) {
      case nsIDOMNode::TEXT_NODE:
        type = XBLBinding_Serialize_TextNode;
        break;
      case nsIDOMNode::CDATA_SECTION_NODE:
        type = XBLBinding_Serialize_CDATANode;
        break;
      case nsIDOMNode::COMMENT_NODE:
        type = XBLBinding_Serialize_CommentNode;
        break;
      default:
        break;
    }

    rv = aStream->Write8(type);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString content;
    aNode->GetText()->AppendTo(content);
    return aStream->WriteWStringZ(content.get());
  }

  

  
  rv = WriteNamespace(aStream, aNode->GetNameSpaceID());
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString prefixStr;
  aNode->NodeInfo()->GetPrefix(prefixStr);
  rv = aStream->WriteWStringZ(prefixStr.get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->WriteWStringZ(nsDependentAtomString(aNode->Tag()).get());
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 count = aNode->GetAttrCount();
  rv = aStream->Write32(count);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 i;
  for (i = 0; i < count; i++) {
    
    

    const nsAttrName* attr = aNode->GetAttrNameAt(i);

    
    PRInt32 namespaceID = attr->NamespaceID();
    rv = WriteNamespace(aStream, namespaceID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString prefixStr;
    nsIAtom* prefix = attr->GetPrefix();
    if (prefix)
      prefix->ToString(prefixStr);
    rv = aStream->WriteWStringZ(prefixStr.get());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aStream->WriteWStringZ(nsDependentAtomString(attr->LocalName()).get());
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString val;
    aNode->GetAttr(attr->NamespaceID(), attr->LocalName(), val);
    rv = aStream->WriteWStringZ(val.get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (mAttributeTable) {
    WriteAttributeData data(this, aStream, aNode);
    mAttributeTable->Enumerate(WriteAttributeNS, &data);
  }
  rv = aStream->Write8(XBLBinding_Serialize_NoMoreAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoTArray<InsertionItem, 1>* list;
  if (aInsertionPointsByContent.Get(aNode, &list)) {
    PRUint32 lastInsertionIndex = 0xFFFFFFFF;

    
    
    for (PRUint32 l = 0; l < list->Length(); l++) {
      InsertionItem item = list->ElementAt(l);
      
      
      
      if (item.insertionIndex != lastInsertionIndex) {
        lastInsertionIndex = item.insertionIndex;
        aStream->Write32(item.insertionIndex);
        
        
        if (item.defaultContent) {
          rv = WriteContentNode(aStream, item.defaultContent,
                                aInsertionPointsByContent);
        }
        else {
          rv = aStream->Write8(XBLBinding_Serialize_NoContent);
        }
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        PRUint32 icount = 1;
        for (PRUint32 i = l + 1; i < list->Length(); i++) {
          if (list->ElementAt(i).insertionIndex != lastInsertionIndex)
            break;
          icount++;
        }

        rv = aStream->Write32(icount);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = aStream->WriteWStringZ(nsDependentAtomString(list->ElementAt(l).tag).get());
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  rv = aStream->Write32(XBLBinding_Serialize_NoMoreInsertionPoints);
  NS_ENSURE_SUCCESS(rv, rv);

  
  count = aNode->GetChildCount();
  rv = aStream->Write32(count);
  NS_ENSURE_SUCCESS(rv, rv);

  for (i = 0; i < count; i++) {
    rv = WriteContentNode(aStream, aNode->GetChildAt(i), aInsertionPointsByContent);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsXBLPrototypeBinding::ReadNamespace(nsIObjectInputStream* aStream,
                                     PRInt32& aNameSpaceID)
{
  PRUint8 namespaceID;
  nsresult rv = aStream->Read8(&namespaceID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (namespaceID == XBLBinding_Serialize_CustomNamespace) {
    nsAutoString namesp;
    rv = aStream->ReadString(namesp);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsContentUtils::NameSpaceManager()->RegisterNameSpace(namesp, aNameSpaceID);
  }
  else {
    aNameSpaceID = namespaceID;
  }

  return NS_OK;
}

nsresult
nsXBLPrototypeBinding::WriteNamespace(nsIObjectOutputStream* aStream,
                                      PRInt32 aNameSpaceID)
{
  
  
  
  
  
  nsresult rv;

  if (aNameSpaceID <= kNameSpaceID_LastBuiltin) {
    rv = aStream->Write8((PRInt8)aNameSpaceID);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aStream->Write8(XBLBinding_Serialize_CustomNamespace);
    NS_ENSURE_SUCCESS(rv, rv);
  
    nsAutoString namesp;
    nsContentUtils::NameSpaceManager()->GetNameSpaceURI(aNameSpaceID, namesp);
    aStream->WriteWStringZ(namesp.get());
  }

  return NS_OK;
}


bool CheckTagNameWhiteList(PRInt32 aNameSpaceID, nsIAtom *aTagName)
{
  static nsIContent::AttrValuesArray kValidXULTagNames[] =  {
    &nsGkAtoms::autorepeatbutton, &nsGkAtoms::box, &nsGkAtoms::browser,
    &nsGkAtoms::button, &nsGkAtoms::hbox, &nsGkAtoms::image, &nsGkAtoms::menu,
    &nsGkAtoms::menubar, &nsGkAtoms::menuitem, &nsGkAtoms::menupopup,
    &nsGkAtoms::row, &nsGkAtoms::slider, &nsGkAtoms::spacer,
    &nsGkAtoms::splitter, &nsGkAtoms::text, &nsGkAtoms::tree, nsnull};

  PRUint32 i;
  if (aNameSpaceID == kNameSpaceID_XUL) {
    for (i = 0; kValidXULTagNames[i]; ++i) {
      if (aTagName == *(kValidXULTagNames[i])) {
        return true;
      }
    }
  }
  else if (aNameSpaceID == kNameSpaceID_SVG &&
           aTagName == nsGkAtoms::generic) {
    return true;
  }

  return false;
}

nsresult
nsXBLPrototypeBinding::ResolveBaseBinding()
{
  if (mCheckedBaseProto)
    return NS_OK;
  mCheckedBaseProto = true;

  nsCOMPtr<nsIDocument> doc = mXBLDocInfoWeak->GetDocument();

  
  nsAutoString display, extends;
  mBinding->GetAttr(kNameSpaceID_None, nsGkAtoms::extends, extends);
  if (extends.IsEmpty())
    return NS_OK;

  mBinding->GetAttr(kNameSpaceID_None, nsGkAtoms::display, display);
  bool hasDisplay = !display.IsEmpty();

  nsAutoString value(extends);
       
  
  nsAutoString prefix;
  PRInt32 offset;
  if (hasDisplay) {
    offset = display.FindChar(':');
    if (-1 != offset) {
      display.Left(prefix, offset);
      display.Cut(0, offset+1);
    }
  }
  else {
    offset = extends.FindChar(':');
    if (-1 != offset) {
      extends.Left(prefix, offset);
      extends.Cut(0, offset+1);
      display = extends;
    }
  }

  nsAutoString nameSpace;

  if (!prefix.IsEmpty()) {
    mBinding->LookupNamespaceURI(prefix, nameSpace);
    if (!nameSpace.IsEmpty()) {
      PRInt32 nameSpaceID =
        nsContentUtils::NameSpaceManager()->GetNameSpaceID(nameSpace);

      nsCOMPtr<nsIAtom> tagName = do_GetAtom(display);
      
      if (!CheckTagNameWhiteList(nameSpaceID, tagName)) {
        const PRUnichar* params[] = { display.get() };
        nsContentUtils::ReportToConsole(nsContentUtils::eXBL_PROPERTIES,
                                       "InvalidExtendsBinding",
                                        params, NS_ARRAY_LENGTH(params),
                                        doc->GetDocumentURI(),
                                        EmptyString(), 0, 0,
                                        nsIScriptError::errorFlag,
                                        "XBL");
        NS_ASSERTION(!nsXBLService::IsChromeOrResourceURI(doc->GetDocumentURI()),
                     "Invalid extends value");
        return NS_ERROR_ILLEGAL_VALUE;
      }

      SetBaseTag(nameSpaceID, tagName);
    }
  }

  if (hasDisplay || nameSpace.IsEmpty()) {
    mBinding->UnsetAttr(kNameSpaceID_None, nsGkAtoms::extends, false);
    mBinding->UnsetAttr(kNameSpaceID_None, nsGkAtoms::display, false);

    return NS_NewURI(getter_AddRefs(mBaseBindingURI), value,
                     doc->GetDocumentCharacterSet().get(),
                     doc->GetDocBaseURI());
  }

  return NS_OK;
}
