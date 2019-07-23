





































#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIXBLDocumentInfo.h"
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
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsXBLService.h"
#include "nsXBLBinding.h"
#include "nsXBLInsertionPoint.h"
#include "nsXBLPrototypeBinding.h"
#include "nsFixedSizeAllocator.h"
#include "xptinfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIPresShell.h"
#include "nsIDocumentObserver.h"
#include "nsGkAtoms.h"
#include "nsXBLProtoImpl.h"
#include "nsCRT.h"
#include "nsContentUtils.h"

#include "nsIScriptContext.h"

#include "nsICSSLoader.h"
#include "nsIStyleRuleProcessor.h"
#include "nsXBLResourceLoader.h"






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
                    NS_ARRAY_LENGTH(kBucketSizes), aInitialSize);
      }
    }
  }
  static PRBool PoolInited()
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

  nsrefcnt AddRef() {
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsXBLInsertionPointEntry", sizeof(nsXBLInsertionPointEntry));
    return mRefCnt;
  }

  nsrefcnt Release() {
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsXBLInsertionPointEntry");
    if (mRefCnt == 0) {
      Destroy(this);
      return 0;
    }
    return mRefCnt;
  }

protected:
  nsCOMPtr<nsIContent> mInsertionParent;
  nsCOMPtr<nsIContent> mDefaultContent;
  PRUint32 mInsertionIndex;
  nsAutoRefCnt mRefCnt;

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
  mInheritStyle(PR_TRUE), 
  mHasBaseProto(PR_TRUE),
  mKeyHandlersRegistered(PR_FALSE),
  mResources(nsnull),
  mAttributeTable(nsnull),
  mInsertionPointTable(nsnull),
  mInterfaceTable(nsnull)
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
                            nsIXBLDocumentInfo* aInfo,
                            nsIContent* aElement,
                            PRBool aFirstBinding)
{
  if (!kAttrPool || !nsXBLInsertionPointEntry::PoolInited()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = aInfo->DocumentURI()->Clone(getter_AddRefs(mBindingURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIURL> bindingURL = do_QueryInterface(mBindingURI);
  if (bindingURL) {
    if (aFirstBinding) {
      rv = mBindingURI->Clone(getter_AddRefs(mAlternateBindingURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    bindingURL->SetRef(aID);
  }

  mXBLDocInfoWeak = aInfo;

  SetBindingElement(aElement);
  return NS_OK;
}

PRBool nsXBLPrototypeBinding::CompareBindingURI(nsIURI* aURI) const
{
  PRBool equal = PR_FALSE;
  mBindingURI->Equals(aURI, &equal);
  if (!equal && mAlternateBindingURI) {
    mAlternateBindingURI->Equals(aURI, &equal);
  }
  return equal;
}

static PRIntn
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

static PRBool
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
    mInheritStyle = PR_FALSE;
}

nsresult
nsXBLPrototypeBinding::GetAllowScripts(PRBool* aResult)
{
  return mXBLDocInfoWeak->GetScriptAccess(aResult);
}

PRBool
nsXBLPrototypeBinding::LoadResources()
{
  if (mResources) {
    PRBool result;
    mResources->LoadResources(&result);
    return result;
  }

  return PR_TRUE;
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
                                        PRBool aRemoveFlag, 
                                        nsIContent* aChangedElement,
                                        nsIContent* aAnonymousContent,
                                        PRBool aNotify)
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
        PRBool attrPresent = PR_TRUE;
        nsAutoString value;
        
        
        if (aAttribute == nsGkAtoms::text && aNameSpaceID == kNameSpaceID_XBL) {
          nsContentUtils::GetNodeTextContent(aChangedElement, PR_FALSE, value);
          value.StripChar(PRUnichar('\n'));
          value.StripChar(PRUnichar('\r'));
          nsAutoString stripVal(value);
          stripVal.StripWhitespace();
          if (stripVal.IsEmpty()) 
            attrPresent = PR_FALSE;
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

            textContent->SetText(value, PR_TRUE);
            realElement->AppendChildTo(textContent, PR_TRUE);
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

PRBool InstantiateInsertionPoint(nsHashKey* aKey, void* aData, void* aClosure)
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

  return PR_TRUE;
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
                                         nsIContent* aChild,
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
                                               PRBool* aMultipleInsertionPoints)
{ 
  *aMultipleInsertionPoints = PR_FALSE;
  *aIndex = 0;

  if (!mInsertionPointTable)
    return nsnull;

  if (mInsertionPointTable->Count() != 1) {
    *aMultipleInsertionPoints = PR_TRUE;
    return nsnull;
  }

  nsISupportsKey key(nsGkAtoms::children);
  nsXBLInsertionPointEntry* entry =
    static_cast<nsXBLInsertionPointEntry*>(mInsertionPointTable->Get(&key));

  if (!entry) {
    
    
    
    
    

    *aMultipleInsertionPoints = PR_TRUE;
    *aIndex = 0;
    return nsnull;
  }

  *aMultipleInsertionPoints = PR_FALSE;
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

PRBool
nsXBLPrototypeBinding::ImplementsInterface(REFNSIID aIID) const
{
  
  if (mInterfaceTable) {
    nsIIDKey key(aIID);
    nsCOMPtr<nsISupports> supports = getter_AddRefs(static_cast<nsISupports*>(mInterfaceTable->Get(&key)));
    return supports != nsnull;
  }

  return PR_FALSE;
}



nsIContent*
nsXBLPrototypeBinding::GetImmediateChild(nsIAtom* aTag)
{
  PRUint32 childCount = mBinding->GetChildCount();

  for (PRUint32 i = 0; i < childCount; i++) {
    nsIContent* child = mBinding->GetChildAt(i);
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
                                 void ** aClassObject)
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
    
    
    nsIDocument* doc = aBoundElement->GetOwnerDoc();
    nsXBLBinding *binding = doc->BindingManager()->GetBinding(aBoundElement);
    nsIContent *anonContent = nsnull;

    while (binding) {
      anonContent = binding->GetAnonymousContent();
      if (anonContent)
        break;

      binding = binding->GetBaseBinding();
    }

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


PRBool SetAttrs(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLAttributeEntry* entry = static_cast<nsXBLAttributeEntry*>(aData);
  nsXBLAttrChangeData* changeData = static_cast<nsXBLAttrChangeData*>(aClosure);

  nsIAtom* src = entry->GetSrcAttribute();
  PRInt32 srcNs = changeData->mSrcNamespace;
  nsAutoString value;
  PRBool attrPresent = PR_TRUE;

  if (src == nsGkAtoms::text && srcNs == kNameSpaceID_XBL) {
    nsContentUtils::GetNodeTextContent(changeData->mBoundElement, PR_FALSE,
                                       value);
    value.StripChar(PRUnichar('\n'));
    value.StripChar(PRUnichar('\r'));
    nsAutoString stripVal(value);
    stripVal.StripWhitespace();

    if (stripVal.IsEmpty()) 
      attrPresent = PR_FALSE;
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
        realElement->SetAttr(dstNs, dst, value, PR_FALSE);

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

          textContent->SetText(value, PR_FALSE);
          realElement->AppendChildTo(textContent, PR_FALSE);
        }
      }

      curr = curr->GetNext();
    }
  }

  return PR_TRUE;
}

PRBool SetAttrsNS(nsHashKey* aKey, void* aData, void* aClosure)
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
  return PR_TRUE;
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

nsCOMArray<nsICSSStyleSheet>*
nsXBLPrototypeBinding::GetStyleSheets()
{
  if (mResources) {
    return &mResources->mStyleSheetList;
  }

  return nsnull;
}

static PRBool
DeleteAttributeEntry(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLAttributeEntry::Destroy(static_cast<nsXBLAttributeEntry*>(aData));
  return PR_TRUE;
}

static PRBool
DeleteAttributeTable(nsHashKey* aKey, void* aData, void* aClosure)
{
  delete static_cast<nsObjectHashtable*>(aData);
  return PR_TRUE;
}

void
nsXBLPrototypeBinding::ConstructAttributeTable(nsIContent* aElement)
{
  
  
  if (!aElement->NodeInfo()->Equals(nsGkAtoms::children, kNameSpaceID_XBL)) {
    nsAutoString inherits;
    aElement->GetAttr(kNameSpaceID_XBL, nsGkAtoms::inherits, inherits);

    if (!inherits.IsEmpty()) {
      if (!mAttributeTable) {
        mAttributeTable = new nsObjectHashtable(nsnull, nsnull,
                                                DeleteAttributeTable,
                                                nsnull, 4);
        if (!mAttributeTable)
          return;
      }

      
      char* str = ToNewCString(inherits);
      char* newStr;
      
      

      char* token = nsCRT::strtok( str, ", ", &newStr );
      while( token != NULL ) {
        
        nsCOMPtr<nsIAtom> atom;
        PRInt32 atomNsID = kNameSpaceID_None;
        nsCOMPtr<nsIAtom> attribute;
        PRInt32 attributeNsID = kNameSpaceID_None;

        
        nsAutoString attrTok; attrTok.AssignWithConversion(token);
        PRInt32 index = attrTok.Find("=", PR_TRUE);
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

        nsPRUint32Key nskey(atomNsID);
        nsObjectHashtable* attributesNS =
          static_cast<nsObjectHashtable*>(mAttributeTable->Get(&nskey));
        if (!attributesNS) {
          attributesNS = new nsObjectHashtable(nsnull, nsnull,
                                               DeleteAttributeEntry,
                                               nsnull, 4);
          if (!attributesNS)
            return;

          mAttributeTable->Put(&nskey, attributesNS);
        }
      
        
        nsXBLAttributeEntry* xblAttr =
          nsXBLAttributeEntry::Create(atom, attribute, attributeNsID, aElement);

        
        
        nsISupportsKey key(atom);
        nsXBLAttributeEntry* entry = static_cast<nsXBLAttributeEntry*>
                                                (attributesNS->Get(&key));

        if (!entry) {
          
          attributesNS->Put(&key, xblAttr);
        } else {
          while (entry->GetNext())
            entry = entry->GetNext();

          entry->SetNext(xblAttr);
        }

        
        
        
        
        
        

        token = nsCRT::strtok( newStr, ", ", &newStr );
      }

      nsMemory::Free(str);
    }
  }

  
  PRUint32 childCount = aElement->GetChildCount();
  for (PRUint32 i = 0; i < childCount; i++) {
    ConstructAttributeTable(aElement->GetChildAt(i));
  }
}

static PRBool
DeleteInsertionPointEntry(nsHashKey* aKey, void* aData, void* aClosure)
{
  static_cast<nsXBLInsertionPointEntry*>(aData)->Release();
  return PR_TRUE;
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
    nsIContent* parent = child->GetParent(); 

    
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

    
    
    
    parent->RemoveChildAt(index, PR_FALSE);

    
    
    
    PRUint32 defaultCount = child->GetChildCount();
    if (defaultCount > 0) {
      nsAutoScriptBlocker scriptBlocker;
      
      xblIns->SetDefaultContent(child);

      
      
      
      
      nsresult rv =
        child->BindToTree(parent->GetCurrentDoc(), parent, nsnull, PR_FALSE);
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
  PRUint32 childCount = aContent->GetChildCount();

  for (PRUint32 i = 0; i < childCount; i++) {
    nsIContent *child = aContent->GetChildAt(i);

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
