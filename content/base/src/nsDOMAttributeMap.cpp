








#include "nsDOMAttributeMap.h"
#include "mozilla/dom/Attr.h"
#include "nsIDOMDocument.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsError.h"
#include "nsContentUtils.h"
#include "nsNodeInfoManager.h"
#include "nsAttrName.h"
#include "nsUnicharUtils.h"

using namespace mozilla;
using namespace mozilla::dom;



nsDOMAttributeMap::nsDOMAttributeMap(Element* aContent)
  : mContent(aContent)
{
  
  
  mAttributeCache.Init();
}




PLDHashOperator
RemoveMapRef(nsAttrHashKey::KeyType aKey, nsRefPtr<Attr>& aData,
             void* aUserArg)
{
  aData->SetMap(nullptr);

  return PL_DHASH_REMOVE;
}

nsDOMAttributeMap::~nsDOMAttributeMap()
{
  mAttributeCache.Enumerate(RemoveMapRef, nullptr);
}

void
nsDOMAttributeMap::DropReference()
{
  mAttributeCache.Enumerate(RemoveMapRef, nullptr);
  mContent = nullptr;
}

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMAttributeMap)
  tmp->DropReference();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


PLDHashOperator
TraverseMapEntry(nsAttrHashKey::KeyType aKey, nsRefPtr<Attr>& aData,
                 void* aUserArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aUserArg);

  cb->NoteXPCOMChild(static_cast<nsINode*>(aData.get()));

  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMAttributeMap)
  tmp->mAttributeCache.Enumerate(TraverseMapEntry, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(MozNamedAttrMap, nsDOMAttributeMap)


NS_INTERFACE_TABLE_HEAD(nsDOMAttributeMap)
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsDOMAttributeMap)
    NS_INTERFACE_TABLE_ENTRY(nsDOMAttributeMap, nsIDOMMozNamedAttrMap)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDOMAttributeMap)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozNamedAttrMap)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMAttributeMap)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMAttributeMap)

PLDHashOperator
SetOwnerDocumentFunc(nsAttrHashKey::KeyType aKey,
                     nsRefPtr<Attr>& aData,
                     void* aUserArg)
{
  nsresult rv = aData->SetOwnerDocument(static_cast<nsIDocument*>(aUserArg));

  return NS_FAILED(rv) ? PL_DHASH_STOP : PL_DHASH_NEXT;
}

nsresult
nsDOMAttributeMap::SetOwnerDocument(nsIDocument* aDocument)
{
  uint32_t n = mAttributeCache.Enumerate(SetOwnerDocumentFunc, aDocument);
  NS_ENSURE_TRUE(n == mAttributeCache.Count(), NS_ERROR_FAILURE);

  return NS_OK;
}

void
nsDOMAttributeMap::DropAttribute(int32_t aNamespaceID, nsIAtom* aLocalName)
{
  nsAttrKey attr(aNamespaceID, aLocalName);
  Attr *node = mAttributeCache.GetWeak(attr);
  if (node) {
    
    node->SetMap(nullptr);

    
    mAttributeCache.Remove(attr);
  }
}

already_AddRefed<Attr>
nsDOMAttributeMap::RemoveAttribute(nsINodeInfo* aNodeInfo)
{
  NS_ASSERTION(aNodeInfo, "RemoveAttribute() called with aNodeInfo == nullptr!");

  nsAttrKey attr(aNodeInfo->NamespaceID(), aNodeInfo->NameAtom());

  nsRefPtr<Attr> node;
  if (!mAttributeCache.Get(attr, getter_AddRefs(node))) {
    nsAutoString value;
    
    
    mContent->GetAttr(aNodeInfo->NamespaceID(), aNodeInfo->NameAtom(), value);
    nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
    node = new Attr(nullptr, ni.forget(), value, true);
  }
  else {
    
    node->SetMap(nullptr);

    
    mAttributeCache.Remove(attr);
  }

  return node.forget();
}

Attr*
nsDOMAttributeMap::GetAttribute(nsINodeInfo* aNodeInfo, bool aNsAware)
{
  NS_ASSERTION(aNodeInfo, "GetAttribute() called with aNodeInfo == nullptr!");

  nsAttrKey attr(aNodeInfo->NamespaceID(), aNodeInfo->NameAtom());

  Attr* node = mAttributeCache.GetWeak(attr);
  if (!node) {
    nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
    nsRefPtr<Attr> newAttr =
      new Attr(this, ni.forget(), EmptyString(), aNsAware);
    mAttributeCache.Put(attr, newAttr);
    node = newAttr;
  }

  return node;
}

Attr*
nsDOMAttributeMap::GetNamedItem(const nsAString& aAttrName)
{
  NS_ENSURE_TRUE(mContent, nullptr);

  nsCOMPtr<nsINodeInfo> ni = mContent->GetExistingAttrNameFromQName(aAttrName);
  if (!ni) {
    return nullptr;
  }

  return GetAttribute(ni, false);
}

NS_IMETHODIMP
nsDOMAttributeMap::GetNamedItem(const nsAString& aAttrName,
                                nsIDOMAttr** aAttribute)
{
  NS_ENSURE_ARG_POINTER(aAttribute);

  NS_IF_ADDREF(*aAttribute = GetNamedItem(aAttrName));

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttributeMap::SetNamedItem(nsIDOMAttr* aAttr, nsIDOMAttr** aReturn)
{
  ErrorResult rv;
  *aReturn = SetNamedItemInternal(aAttr, false, rv).get();
  return rv.ErrorCode();
}

NS_IMETHODIMP
nsDOMAttributeMap::SetNamedItemNS(nsIDOMAttr* aAttr, nsIDOMAttr** aReturn)
{
  ErrorResult rv;
  *aReturn = SetNamedItemInternal(aAttr, true, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<Attr>
nsDOMAttributeMap::SetNamedItemInternal(nsIDOMAttr* aAttr,
                                        bool aWithNS,
                                        ErrorResult& aError)
{
  NS_ENSURE_TRUE(mContent, nullptr);

  
  
  
  nsCOMPtr<nsIAttribute> iAttribute(do_QueryInterface(aAttr));
  if (!iAttribute) {
    aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
    return nullptr;
  }

  Attr *attribute = static_cast<Attr*>(iAttribute.get());

  
  nsDOMAttributeMap* owner = iAttribute->GetMap();
  if (owner) {
    if (owner != this) {
      aError.Throw(NS_ERROR_DOM_INUSE_ATTRIBUTE_ERR);
      return nullptr;
    }

    
    
    NS_ADDREF(attribute);
    return attribute;
  }

  nsresult rv;
  if (!mContent->HasSameOwnerDoc(iAttribute)) {
    nsCOMPtr<nsIDOMDocument> domDoc =
      do_QueryInterface(mContent->OwnerDoc(), &rv);
    if (NS_FAILED(rv)) {
      aError.Throw(rv);
      return nullptr;
    }

    nsCOMPtr<nsIDOMNode> adoptedNode;
    rv = domDoc->AdoptNode(aAttr, getter_AddRefs(adoptedNode));
    if (NS_FAILED(rv)) {
      aError.Throw(rv);
      return nullptr;
    }

    NS_ASSERTION(adoptedNode == aAttr, "Uh, adopt node changed nodes?");
  }

  
  nsAutoString name;
  nsCOMPtr<nsINodeInfo> ni;

  nsRefPtr<Attr> attr;
  
  if (aWithNS) {
    
    ni = iAttribute->NodeInfo();

    if (mContent->HasAttr(ni->NamespaceID(), ni->NameAtom())) {
      attr = RemoveAttribute(ni);
    }
  }
  else { 
    attribute->GetName(name);

    
    ni = mContent->GetExistingAttrNameFromQName(name);
    if (ni) {
      attr = RemoveAttribute(ni);
    }
    else {
      if (mContent->IsInHTMLDocument() &&
          mContent->IsHTML()) {
        nsContentUtils::ASCIIToLower(name);
      }

      rv = mContent->NodeInfo()->NodeInfoManager()->
        GetNodeInfo(name, nullptr, kNameSpaceID_None,
                    nsIDOMNode::ATTRIBUTE_NODE, getter_AddRefs(ni));
      if (NS_FAILED(rv)) {
        aError.Throw(rv);
        return nullptr;
      }
      
    }
  }

  nsAutoString value;
  attribute->GetValue(value);

  
  
  nsAttrKey attrkey(ni->NamespaceID(), ni->NameAtom());
  mAttributeCache.Put(attrkey, attribute);
  iAttribute->SetMap(this);

  rv = mContent->SetAttr(ni->NamespaceID(), ni->NameAtom(),
                         ni->GetPrefixAtom(), value, true);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
    DropAttribute(ni->NamespaceID(), ni->NameAtom());
  }

  return attr.forget();
}

NS_IMETHODIMP
nsDOMAttributeMap::RemoveNamedItem(const nsAString& aName,
                                   nsIDOMAttr** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;

  NS_ENSURE_TRUE(mContent, NS_OK);

  nsCOMPtr<nsINodeInfo> ni = mContent->GetExistingAttrNameFromQName(aName);
  if (!ni) {
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  NS_ADDREF(*aReturn = GetAttribute(ni, true));

  
  return mContent->UnsetAttr(ni->NamespaceID(), ni->NameAtom(), true);
}


Attr*
nsDOMAttributeMap::GetItemAt(uint32_t aIndex)
{
  NS_ENSURE_TRUE(mContent, nullptr);

  const nsAttrName* name = mContent->GetAttrNameAt(aIndex);
  NS_ENSURE_TRUE(name, nullptr);

  
  
  nsCOMPtr<nsINodeInfo> ni = mContent->NodeInfo()->NodeInfoManager()->
    GetNodeInfo(name->LocalName(), name->GetPrefix(), name->NamespaceID(),
                nsIDOMNode::ATTRIBUTE_NODE);
  return GetAttribute(ni, true);
}

NS_IMETHODIMP
nsDOMAttributeMap::Item(uint32_t aIndex, nsIDOMAttr** aReturn)
{
  NS_IF_ADDREF(*aReturn = GetItemAt(aIndex));
  return NS_OK;
}

nsresult
nsDOMAttributeMap::GetLength(uint32_t *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  if (mContent) {
    *aLength = mContent->GetAttrCount();
  }
  else {
    *aLength = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttributeMap::GetNamedItemNS(const nsAString& aNamespaceURI,
                                  const nsAString& aLocalName,
                                  nsIDOMAttr** aReturn)
{
  NS_IF_ADDREF(*aReturn = GetNamedItemNS(aNamespaceURI, aLocalName));
  return NS_OK;
}

Attr*
nsDOMAttributeMap::GetNamedItemNS(const nsAString& aNamespaceURI,
                                  const nsAString& aLocalName)
{
  nsCOMPtr<nsINodeInfo> ni = GetAttrNodeInfo(aNamespaceURI, aLocalName);
  if (!ni) {
    return nullptr;
  }

  return GetAttribute(ni, true);
}

already_AddRefed<nsINodeInfo>
nsDOMAttributeMap::GetAttrNodeInfo(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName)
{
  if (!mContent) {
    return nullptr;
  }

  int32_t nameSpaceID = kNameSpaceID_None;

  if (!aNamespaceURI.IsEmpty()) {
    nameSpaceID =
      nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

    if (nameSpaceID == kNameSpaceID_Unknown) {
      return nullptr;
    }
  }

  uint32_t i, count = mContent->GetAttrCount();
  for (i = 0; i < count; ++i) {
    const nsAttrName* name = mContent->GetAttrNameAt(i);
    int32_t attrNS = name->NamespaceID();
    nsIAtom* nameAtom = name->LocalName();

    if (nameSpaceID == attrNS &&
        nameAtom->Equals(aLocalName)) {
      nsCOMPtr<nsINodeInfo> ni;
      ni = mContent->NodeInfo()->NodeInfoManager()->
        GetNodeInfo(nameAtom, name->GetPrefix(), nameSpaceID,
                    nsIDOMNode::ATTRIBUTE_NODE);

      return ni.forget();
    }
  }

  return nullptr;
}

NS_IMETHODIMP
nsDOMAttributeMap::RemoveNamedItemNS(const nsAString& aNamespaceURI,
                                     const nsAString& aLocalName,
                                     nsIDOMAttr** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;

  nsCOMPtr<nsINodeInfo> ni = GetAttrNodeInfo(aNamespaceURI, aLocalName);

  if (!ni) {
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  nsRefPtr<Attr> attr = RemoveAttribute(ni);
  nsINodeInfo *attrNi = attr->NodeInfo();
  mContent->UnsetAttr(attrNi->NamespaceID(), attrNi->NameAtom(), true);

  attr.forget(aReturn);
  return NS_OK;
}

uint32_t
nsDOMAttributeMap::Count() const
{
  return mAttributeCache.Count();
}

uint32_t
nsDOMAttributeMap::Enumerate(AttrCache::EnumReadFunction aFunc,
                             void *aUserArg) const
{
  return mAttributeCache.EnumerateRead(aFunc, aUserArg);
}

size_t
AttrCacheSizeEnumerator(const nsAttrKey& aKey,
                        const nsRefPtr<Attr>& aValue,
                        nsMallocSizeOfFun aMallocSizeOf,
                        void* aUserArg)
{
  return aMallocSizeOf(aValue.get());
}

size_t
nsDOMAttributeMap::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += mAttributeCache.SizeOfExcludingThis(AttrCacheSizeEnumerator,
                                           aMallocSizeOf);

  
  return n;
}
