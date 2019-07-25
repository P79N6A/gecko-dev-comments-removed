









































#include "nsDOMAttributeMap.h"
#include "nsDOMAttribute.h"
#include "nsIDOMDocument.h"
#include "nsGenericElement.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"
#include "nsNodeInfoManager.h"
#include "nsAttrName.h"
#include "nsUnicharUtils.h"



nsDOMAttributeMap::nsDOMAttributeMap(Element* aContent)
  : mContent(aContent)
{
  
  
}

PRBool
nsDOMAttributeMap::Init()
{
  return mAttributeCache.Init();
}




PLDHashOperator
RemoveMapRef(nsAttrHashKey::KeyType aKey, nsRefPtr<nsDOMAttribute>& aData,
             void* aUserArg)
{
  aData->SetMap(nsnull);

  return PL_DHASH_REMOVE;
}

nsDOMAttributeMap::~nsDOMAttributeMap()
{
  mAttributeCache.Enumerate(RemoveMapRef, nsnull);
}

void
nsDOMAttributeMap::DropReference()
{
  mAttributeCache.Enumerate(RemoveMapRef, nsnull);
  mContent = nsnull;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMAttributeMap)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMAttributeMap)
  tmp->DropReference();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


PLDHashOperator
TraverseMapEntry(nsAttrHashKey::KeyType aKey, nsRefPtr<nsDOMAttribute>& aData,
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

DOMCI_DATA(NamedNodeMap, nsDOMAttributeMap)


NS_INTERFACE_TABLE_HEAD(nsDOMAttributeMap)
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsDOMAttributeMap)
    NS_INTERFACE_TABLE_ENTRY(nsDOMAttributeMap, nsIDOMNamedNodeMap)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDOMAttributeMap)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(NamedNodeMap)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMAttributeMap)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMAttributeMap)

PLDHashOperator
SetOwnerDocumentFunc(nsAttrHashKey::KeyType aKey,
                     nsRefPtr<nsDOMAttribute>& aData,
                     void* aUserArg)
{
  nsresult rv = aData->SetOwnerDocument(static_cast<nsIDocument*>(aUserArg));

  return NS_FAILED(rv) ? PL_DHASH_STOP : PL_DHASH_NEXT;
}

nsresult
nsDOMAttributeMap::SetOwnerDocument(nsIDocument* aDocument)
{
  PRUint32 n = mAttributeCache.Enumerate(SetOwnerDocumentFunc, aDocument);
  NS_ENSURE_TRUE(n == mAttributeCache.Count(), NS_ERROR_FAILURE);

  return NS_OK;
}

void
nsDOMAttributeMap::DropAttribute(PRInt32 aNamespaceID, nsIAtom* aLocalName)
{
  nsAttrKey attr(aNamespaceID, aLocalName);
  nsDOMAttribute *node = mAttributeCache.GetWeak(attr);
  if (node) {
    
    node->SetMap(nsnull);

    
    mAttributeCache.Remove(attr);
  }
}

nsresult
nsDOMAttributeMap::RemoveAttribute(nsINodeInfo* aNodeInfo, nsIDOMNode** aReturn)
{
  NS_ASSERTION(aNodeInfo, "RemoveAttribute() called with aNodeInfo == nsnull!");
  NS_ASSERTION(aReturn, "RemoveAttribute() called with aReturn == nsnull");

  *aReturn = nsnull;

  nsAttrKey attr(aNodeInfo->NamespaceID(), aNodeInfo->NameAtom());

  nsRefPtr<nsDOMAttribute> node;
  if (!mAttributeCache.Get(attr, getter_AddRefs(node))) {
    nsAutoString value;
    
    
    mContent->GetAttr(aNodeInfo->NamespaceID(), aNodeInfo->NameAtom(), value);
    nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
    nsCOMPtr<nsIDOMNode> newAttr =
      new nsDOMAttribute(nsnull, ni.forget(), value, PR_TRUE);
    if (!newAttr) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    newAttr.swap(*aReturn);
  }
  else {
    
    node->SetMap(nsnull);

    
    mAttributeCache.Remove(attr);

    node.forget(aReturn);
  }

  return NS_OK;
}

nsDOMAttribute*
nsDOMAttributeMap::GetAttribute(nsINodeInfo* aNodeInfo, PRBool aNsAware)
{
  NS_ASSERTION(aNodeInfo, "GetAttribute() called with aNodeInfo == nsnull!");

  nsAttrKey attr(aNodeInfo->NamespaceID(), aNodeInfo->NameAtom());

  nsDOMAttribute* node = mAttributeCache.GetWeak(attr);
  if (!node) {
    nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
    nsRefPtr<nsDOMAttribute> newAttr =
      new nsDOMAttribute(this, ni.forget(), EmptyString(), aNsAware);
    if (newAttr && mAttributeCache.Put(attr, newAttr)) {
      node = newAttr;
    }
  }

  return node;
}

nsDOMAttribute*
nsDOMAttributeMap::GetNamedItem(const nsAString& aAttrName, nsresult *aResult)
{
  *aResult = NS_OK;

  if (mContent) {
    nsCOMPtr<nsINodeInfo> ni =
      mContent->GetExistingAttrNameFromQName(aAttrName);
    if (ni) {
      return GetAttribute(ni, PR_FALSE);
    }
  }

  return nsnull;
}

NS_IMETHODIMP
nsDOMAttributeMap::GetNamedItem(const nsAString& aAttrName,
                                nsIDOMNode** aAttribute)
{
  NS_ENSURE_ARG_POINTER(aAttribute);

  nsresult rv;
  NS_IF_ADDREF(*aAttribute = GetNamedItem(aAttrName, &rv));

  return rv;
}

NS_IMETHODIMP
nsDOMAttributeMap::SetNamedItem(nsIDOMNode *aNode, nsIDOMNode **aReturn)
{
  return SetNamedItemInternal(aNode, aReturn, PR_FALSE);
}

NS_IMETHODIMP
nsDOMAttributeMap::SetNamedItemNS(nsIDOMNode *aNode, nsIDOMNode **aReturn)
{
  return SetNamedItemInternal(aNode, aReturn, PR_TRUE);
}

nsresult
nsDOMAttributeMap::SetNamedItemInternal(nsIDOMNode *aNode,
                                        nsIDOMNode **aReturn,
                                        PRBool aWithNS)
{
  NS_ENSURE_ARG_POINTER(aNode);
  NS_ENSURE_ARG_POINTER(aReturn);

  nsresult rv = NS_OK;
  *aReturn = nsnull;
  nsCOMPtr<nsIDOMNode> tmpReturn;

  if (mContent) {
    
    
    
    nsCOMPtr<nsIAttribute> iAttribute(do_QueryInterface(aNode));
    if (!iAttribute) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }

    nsDOMAttribute *attribute = static_cast<nsDOMAttribute*>(iAttribute.get());

    
    nsDOMAttributeMap* owner = iAttribute->GetMap();
    if (owner) {
      if (owner != this) {
        return NS_ERROR_DOM_INUSE_ATTRIBUTE_ERR;
      }

      
      
      NS_ADDREF(*aReturn = aNode);
      
      return NS_OK;
    }

    if (!mContent->HasSameOwnerDoc(iAttribute)) {
      nsCOMPtr<nsIDOMDocument> domDoc =
        do_QueryInterface(mContent->GetOwnerDoc(), &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIDOMNode> adoptedNode;
      rv = domDoc->AdoptNode(aNode, getter_AddRefs(adoptedNode));
      NS_ENSURE_SUCCESS(rv, rv);

      NS_ASSERTION(adoptedNode == aNode, "Uh, adopt node changed nodes?");
    }

    
    nsAutoString name;
    nsCOMPtr<nsINodeInfo> ni;

    
    if (aWithNS) {
      
      ni = iAttribute->NodeInfo();

      if (mContent->HasAttr(ni->NamespaceID(), ni->NameAtom())) {
        rv = RemoveAttribute(ni, getter_AddRefs(tmpReturn));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else { 
      attribute->GetName(name);

      
      ni = mContent->GetExistingAttrNameFromQName(name);
      if (ni) {
        rv = RemoveAttribute(ni, getter_AddRefs(tmpReturn));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        if (mContent->IsInHTMLDocument() &&
            mContent->IsHTML()) {
          nsAutoString lower;
          ToLowerCase(name, lower);
          name = lower;
        }

        rv = mContent->NodeInfo()->NodeInfoManager()->
          GetNodeInfo(name, nsnull, kNameSpaceID_None, getter_AddRefs(ni));
        NS_ENSURE_SUCCESS(rv, rv);
        
      }
    }

    nsAutoString value;
    attribute->GetValue(value);

    
    
    nsAttrKey attrkey(ni->NamespaceID(), ni->NameAtom());
    rv = mAttributeCache.Put(attrkey, attribute);
    NS_ENSURE_SUCCESS(rv, rv);
    iAttribute->SetMap(this);

    rv = mContent->SetAttr(ni->NamespaceID(), ni->NameAtom(),
                           ni->GetPrefixAtom(), value, PR_TRUE);
    if (NS_FAILED(rv)) {
      DropAttribute(ni->NamespaceID(), ni->NameAtom());
    }
  }

  tmpReturn.swap(*aReturn); 

  return rv;
}

NS_IMETHODIMP
nsDOMAttributeMap::RemoveNamedItem(const nsAString& aName,
                                   nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsresult rv = NS_OK;

  if (mContent) {
    nsCOMPtr<nsINodeInfo> ni = mContent->GetExistingAttrNameFromQName(aName);
    if (!ni) {
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    NS_ADDREF(*aReturn = GetAttribute(ni, PR_TRUE));

    
    rv = mContent->UnsetAttr(ni->NamespaceID(), ni->NameAtom(), PR_TRUE);
  }

  return rv;
}


nsDOMAttribute*
nsDOMAttributeMap::GetItemAt(PRUint32 aIndex, nsresult *aResult)
{
  *aResult = NS_OK;

  nsDOMAttribute* node = nsnull;

  const nsAttrName* name;
  if (mContent && (name = mContent->GetAttrNameAt(aIndex))) {
    
    
    nsCOMPtr<nsINodeInfo> ni;
    ni = mContent->NodeInfo()->NodeInfoManager()->
      GetNodeInfo(name->LocalName(), name->GetPrefix(), name->NamespaceID());
    if (ni) {
      node = GetAttribute(ni, PR_TRUE);
    }
    else {
      *aResult = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return node;
}

NS_IMETHODIMP
nsDOMAttributeMap::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsresult rv;
  NS_IF_ADDREF(*aReturn = GetItemAt(aIndex, &rv));
  return rv;
}

nsresult
nsDOMAttributeMap::GetLength(PRUint32 *aLength)
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
                                  nsIDOMNode** aReturn)
{
  return GetNamedItemNSInternal(aNamespaceURI, aLocalName, aReturn);
}

nsresult
nsDOMAttributeMap::GetNamedItemNSInternal(const nsAString& aNamespaceURI,
                                          const nsAString& aLocalName,
                                          nsIDOMNode** aReturn,
                                          PRBool aRemove)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  if (!mContent) {
    return NS_OK;
  }

  PRInt32 nameSpaceID = kNameSpaceID_None;

  if (!aNamespaceURI.IsEmpty()) {
    nameSpaceID =
      nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

    if (nameSpaceID == kNameSpaceID_Unknown) {
      return NS_OK;
    }
  }

  PRUint32 i, count = mContent->GetAttrCount();
  for (i = 0; i < count; ++i) {
    const nsAttrName* name = mContent->GetAttrNameAt(i);
    PRInt32 attrNS = name->NamespaceID();
    nsIAtom* nameAtom = name->LocalName();

    if (nameSpaceID == attrNS &&
        nameAtom->Equals(aLocalName)) {
      nsCOMPtr<nsINodeInfo> ni;
      ni = mContent->NodeInfo()->NodeInfoManager()->
        GetNodeInfo(nameAtom, name->GetPrefix(), nameSpaceID);
      NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

      if (aRemove) {
        return RemoveAttribute(ni, aReturn);
      }

      NS_ADDREF(*aReturn = GetAttribute(ni, PR_TRUE));

      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttributeMap::RemoveNamedItemNS(const nsAString& aNamespaceURI,
                                     const nsAString& aLocalName,
                                     nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsresult rv = GetNamedItemNSInternal(aNamespaceURI,
                                       aLocalName,
                                       aReturn,
                                       PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!*aReturn) {
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  nsCOMPtr<nsIAttribute> attr = do_QueryInterface(*aReturn);
  NS_ASSERTION(attr, "attribute returned from nsDOMAttributeMap::GetNameItemNS "
               "didn't implement nsIAttribute");
  NS_ENSURE_TRUE(attr, NS_ERROR_UNEXPECTED);

  nsINodeInfo *ni = attr->NodeInfo();
  mContent->UnsetAttr(ni->NamespaceID(), ni->NameAtom(), PR_TRUE);

  return NS_OK;
}

PRUint32
nsDOMAttributeMap::Count() const
{
  return mAttributeCache.Count();
}

PRUint32
nsDOMAttributeMap::Enumerate(AttrCache::EnumReadFunction aFunc,
                             void *aUserArg) const
{
  return mAttributeCache.EnumerateRead(aFunc, aUserArg);
}
