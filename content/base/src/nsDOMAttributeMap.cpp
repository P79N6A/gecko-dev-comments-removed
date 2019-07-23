









































#include "nsDOMAttributeMap.h"
#include "nsDOMAttribute.h"
#include "nsIDOM3Document.h"
#include "nsGenericElement.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"
#include "nsNodeInfoManager.h"
#include "nsAttrName.h"



nsDOMAttributeMap::nsDOMAttributeMap(nsIContent* aContent)
  : mContent(aContent)
{
  
  
}

PRBool
nsDOMAttributeMap::Init()
{
  return mAttributeCache.Init();
}




PLDHashOperator
RemoveMapRef(nsAttrHashKey::KeyType aKey, nsCOMPtr<nsIDOMNode>& aData, void* aUserArg)
{
  nsCOMPtr<nsIAttribute> attr(do_QueryInterface(aData));
  NS_ASSERTION(attr, "non-nsIAttribute somehow made it into the hashmap?!");
  attr->SetMap(nsnull);

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
TraverseMapEntry(nsAttrHashKey::KeyType aKey, nsCOMPtr<nsIDOMNode>& aData, void* aUserArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aUserArg);

  cb->NoteXPCOMChild(aData.get());

  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMAttributeMap)
  tmp->mAttributeCache.Enumerate(TraverseMapEntry, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END



NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMAttributeMap)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNamedNodeMap)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(NamedNodeMap)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMAttributeMap)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMAttributeMap)

PLDHashOperator
SetOwnerDocumentFunc(nsAttrHashKey::KeyType aKey, nsCOMPtr<nsIDOMNode>& aData,
                     void* aUserArg)
{
  nsCOMPtr<nsIAttribute> attr(do_QueryInterface(aData));
  NS_ASSERTION(attr, "non-nsIAttribute somehow made it into the hashmap?!");
  nsresult rv = attr->SetOwnerDocument(static_cast<nsIDocument*>(aUserArg));

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
  nsIDOMNode *node = mAttributeCache.GetWeak(attr);
  if (node) {
    nsCOMPtr<nsIAttribute> iAttr(do_QueryInterface(node));
    NS_ASSERTION(iAttr, "non-nsIAttribute somehow made it into the hashmap?!");

    
    iAttr->SetMap(nsnull);

    
    mAttributeCache.Remove(attr);
  }
}

nsresult
nsDOMAttributeMap::GetAttribute(nsINodeInfo* aNodeInfo,
                                nsIDOMNode** aReturn,
                                PRBool aRemove)
{
  NS_ASSERTION(aNodeInfo, "GetAttribute() called with aNodeInfo == nsnull!");
  NS_ASSERTION(aReturn, "GetAttribute() called with aReturn == nsnull");

  *aReturn = nsnull;

  nsAttrKey attr(aNodeInfo->NamespaceID(), aNodeInfo->NameAtom());

  if (!mAttributeCache.Get(attr, aReturn)) {
    nsAutoString value;
    if (aRemove) {
      
      
      mContent->GetAttr(aNodeInfo->NamespaceID(), aNodeInfo->NameAtom(), value);
    }
    nsCOMPtr<nsIDOMNode> newAttr = new nsDOMAttribute(aRemove ? nsnull : this,
                                                      aNodeInfo, value);
    if (!newAttr) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!aRemove && !mAttributeCache.Put(attr, newAttr)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    newAttr.swap(*aReturn);
  }
  else if (aRemove) {
    nsCOMPtr<nsIAttribute> iAttr(do_QueryInterface(*aReturn));
    NS_ASSERTION(iAttr, "non-nsIAttribute somehow made it into the hashmap?!");

    
    iAttr->SetMap(nsnull);

    
    mAttributeCache.Remove(attr);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttributeMap::GetNamedItem(const nsAString& aAttrName,
                                nsIDOMNode** aAttribute)
{
  NS_ENSURE_ARG_POINTER(aAttribute);
  *aAttribute = nsnull;

  nsresult rv = NS_OK;
  if (mContent) {
    nsCOMPtr<nsINodeInfo> ni =
      mContent->GetExistingAttrNameFromQName(aAttrName);
    if (ni) {
      rv = GetAttribute(ni, aAttribute);
    }
  }

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
    
    
    
    nsCOMPtr<nsIDOMAttr> attribute(do_QueryInterface(aNode));
    nsCOMPtr<nsIAttribute> iAttribute(do_QueryInterface(aNode));
    if (!attribute || !iAttribute) {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }

    
    nsDOMAttributeMap* owner = iAttribute->GetMap();
    if (owner) {
      if (owner != this) {
        return NS_ERROR_DOM_INUSE_ATTRIBUTE_ERR;
      }

      
      
      NS_ADDREF(*aReturn = aNode);
      
      return NS_OK;
    }

    if (!mContent->HasSameOwnerDoc(iAttribute)) {
      nsCOMPtr<nsIDOM3Document> domDoc =
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
        rv = GetAttribute(ni, getter_AddRefs(tmpReturn), PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else { 
      attribute->GetName(name);

      
      ni = mContent->GetExistingAttrNameFromQName(name);
      if (ni) {
        rv = GetAttribute(ni, getter_AddRefs(tmpReturn), PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
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

    if (!aWithNS && ni->NamespaceID() == kNameSpaceID_None &&
        mContent->IsNodeOfType(nsINode::eHTML)) {
      
      
      nsCOMPtr<nsIDOMElement> ourElement(do_QueryInterface(mContent));
      NS_ASSERTION(ourElement, "HTML content that's not an element?");
      rv = ourElement->SetAttribute(name, value);
    }
    else {
      
      rv = mContent->SetAttr(ni->NamespaceID(), ni->NameAtom(),
                             ni->GetPrefixAtom(), value, PR_TRUE);
    }
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

    rv = GetAttribute(ni, aReturn);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mContent->UnsetAttr(ni->NamespaceID(), ni->NameAtom(), PR_TRUE);
  }

  return rv;
}


NS_IMETHODIMP
nsDOMAttributeMap::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  const nsAttrName* name;
  if (mContent && (name = mContent->GetAttrNameAt(aIndex))) {
    
    
    nsCOMPtr<nsINodeInfo> ni;
    mContent->NodeInfo()->NodeInfoManager()->
      GetNodeInfo(name->LocalName(), name->GetPrefix(), name->NamespaceID(),
                  getter_AddRefs(ni));
    NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

    return GetAttribute(ni, aReturn);
  }

  *aReturn = nsnull;

  return NS_OK;
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

  NS_ConvertUTF16toUTF8 utf8Name(aLocalName);
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
        nameAtom->EqualsUTF8(utf8Name)) {
      nsCOMPtr<nsINodeInfo> ni;
      mContent->NodeInfo()->NodeInfoManager()->
        GetNodeInfo(nameAtom, name->GetPrefix(), nameSpaceID,
                    getter_AddRefs(ni));
      NS_ENSURE_TRUE(ni, NS_ERROR_FAILURE);

      return GetAttribute(ni, aReturn, aRemove);
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
