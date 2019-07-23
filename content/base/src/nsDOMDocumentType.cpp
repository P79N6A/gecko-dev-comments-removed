








































#include "nsDOMDocumentType.h"
#include "nsDOMAttributeMap.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsDOMString.h"
#include "nsIDOM3Node.h"
#include "nsNodeInfoManager.h"
#include "nsIDocument.h"
#include "nsIXPConnect.h"
#include "nsIDOMDocument.h"

nsresult
NS_NewDOMDocumentType(nsIDOMDocumentType** aDocType,
                      nsNodeInfoManager *aNodeInfoManager,
                      nsIPrincipal *aPrincipal,
                      nsIAtom *aName,
                      nsIDOMNamedNodeMap *aEntities,
                      nsIDOMNamedNodeMap *aNotations,
                      const nsAString& aPublicId,
                      const nsAString& aSystemId,
                      const nsAString& aInternalSubset)
{
  NS_PRECONDITION(aNodeInfoManager || aPrincipal,
                  "Must have a principal if no nodeinfo manager.");
  NS_ENSURE_ARG_POINTER(aDocType);
  NS_ENSURE_ARG_POINTER(aName);

  nsresult rv;

  nsRefPtr<nsNodeInfoManager> nimgr;
  if (aNodeInfoManager) {
    nimgr = aNodeInfoManager;
  }
  else {
    nimgr = new nsNodeInfoManager();
    NS_ENSURE_TRUE(nimgr, NS_ERROR_OUT_OF_MEMORY);
    
    rv = nimgr->Init(nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    nimgr->SetDocumentPrincipal(aPrincipal);
  }

  nsCOMPtr<nsINodeInfo> ni;
  rv = nimgr->GetNodeInfo(nsGkAtoms::documentTypeNodeName, nsnull,
                          kNameSpaceID_None, getter_AddRefs(ni));
  NS_ENSURE_SUCCESS(rv, rv);

  *aDocType = new nsDOMDocumentType(ni, aName, aEntities, aNotations,
                                    aPublicId, aSystemId, aInternalSubset);
  if (!*aDocType) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aDocType);

  return NS_OK;
}

nsDOMDocumentType::nsDOMDocumentType(nsINodeInfo *aNodeInfo,
                                     nsIAtom *aName,
                                     nsIDOMNamedNodeMap *aEntities,
                                     nsIDOMNamedNodeMap *aNotations,
                                     const nsAString& aPublicId,
                                     const nsAString& aSystemId,
                                     const nsAString& aInternalSubset) :
  nsGenericDOMDataNode(aNodeInfo),
  mName(aName),
  mEntities(aEntities),
  mNotations(aNotations),
  mPublicId(aPublicId),
  mSystemId(aSystemId),
  mInternalSubset(aInternalSubset)
{
}

nsDOMDocumentType::~nsDOMDocumentType()
{
}



NS_INTERFACE_MAP_BEGIN(nsDOMDocumentType)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentType)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DocumentType)
NS_INTERFACE_MAP_END_INHERITING(nsGenericDOMDataNode)


NS_IMPL_ADDREF_INHERITED(nsDOMDocumentType, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(nsDOMDocumentType, nsGenericDOMDataNode)

PRBool
nsDOMDocumentType::IsNodeOfType(PRUint32 aFlags) const
{
  
  
  
  
  return !(aFlags & ~eCONTENT);
}

const nsTextFragment*
nsDOMDocumentType::GetText()
{
  return nsnull;
}

NS_IMETHODIMP    
nsDOMDocumentType::GetName(nsAString& aName)
{
  return mName->ToString(aName);
}

NS_IMETHODIMP    
nsDOMDocumentType::GetEntities(nsIDOMNamedNodeMap** aEntities)
{
  NS_ENSURE_ARG_POINTER(aEntities);

  *aEntities = mEntities;

  NS_IF_ADDREF(*aEntities);

  return NS_OK;
}

NS_IMETHODIMP    
nsDOMDocumentType::GetNotations(nsIDOMNamedNodeMap** aNotations)
{
  NS_ENSURE_ARG_POINTER(aNotations);

  *aNotations = mNotations;

  NS_IF_ADDREF(*aNotations);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDocumentType::GetPublicId(nsAString& aPublicId)
{
  aPublicId = mPublicId;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDocumentType::GetSystemId(nsAString& aSystemId)
{
  aSystemId = mSystemId;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDocumentType::GetInternalSubset(nsAString& aInternalSubset)
{
  
  aInternalSubset = mInternalSubset;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDocumentType::GetNodeName(nsAString& aNodeName)
{
  return mName->ToString(aNodeName);
}

NS_IMETHODIMP
nsDOMDocumentType::GetNodeValue(nsAString& aNodeValue)
{
  SetDOMStringToNull(aNodeValue);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMDocumentType::SetNodeValue(const nsAString& aNodeValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDocumentType::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = nsIDOMNode::DOCUMENT_TYPE_NODE;

  return NS_OK;
}

nsGenericDOMDataNode*
nsDOMDocumentType::CloneDataNode(nsINodeInfo *aNodeInfo, PRBool aCloneText) const
{
  return new nsDOMDocumentType(aNodeInfo, mName, mEntities, mNotations,
                               mPublicId, mSystemId, mInternalSubset);
}

nsresult
nsDOMDocumentType::BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers)
{
  if (!HasSameOwnerDoc(NODE_FROM(aParent, aDocument))) {
    NS_ASSERTION(!GetOwnerDoc(), "Need to adopt or import first!");

    
    
    
    
    
    
    
    
    nsNodeInfoManager *nimgr = aParent ?
                               aParent->NodeInfo()->NodeInfoManager() :
                               aDocument->NodeInfoManager();
    nsCOMPtr<nsINodeInfo> newNodeInfo;
    nsresult rv = nimgr->GetNodeInfo(mNodeInfo->NameAtom(),
                                     mNodeInfo->GetPrefixAtom(),
                                     mNodeInfo->NamespaceID(),
                                     getter_AddRefs(newNodeInfo));
    NS_ENSURE_SUCCESS(rv, rv);

    mNodeInfo.swap(newNodeInfo);

    nsCOMPtr<nsIDocument> oldOwnerDoc =
      do_QueryInterface(nsContentUtils::GetDocumentFromContext());
    nsIDocument *newOwnerDoc = nimgr->GetDocument();
    if (oldOwnerDoc && newOwnerDoc) {
      nsIXPConnect *xpc = nsContentUtils::XPConnect();

      JSContext *cx = nsnull;
      JSObject *oldScope = nsnull, *newScope = nsnull;
      rv = nsContentUtils::GetContextAndScopes(oldOwnerDoc, newOwnerDoc, &cx,
                                               &oldScope, &newScope);
      if (cx && xpc) {
        nsISupports *node = NS_ISUPPORTS_CAST(nsIContent*, this);
        nsCOMPtr<nsIXPConnectJSObjectHolder> oldWrapper;
        rv = xpc->ReparentWrappedNativeIfFound(cx, oldScope, newScope, node,
                                               getter_AddRefs(oldWrapper));
      }

      if (NS_FAILED(rv)) {
        mNodeInfo.swap(newNodeInfo);

        return rv;
      }
    }
  }

  return nsGenericDOMDataNode::BindToTree(aDocument, aParent, aBindingParent,
                                          aCompileEventHandlers);
}
