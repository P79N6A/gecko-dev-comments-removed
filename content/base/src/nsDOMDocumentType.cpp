








































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
#include "xpcpublic.h"

nsresult
NS_NewDOMDocumentType(nsIDOMDocumentType** aDocType,
                      nsNodeInfoManager *aNodeInfoManager,
                      nsIPrincipal *aPrincipal,
                      nsIAtom *aName,
                      const nsAString& aPublicId,
                      const nsAString& aSystemId,
                      const nsAString& aInternalSubset)
{
  NS_PRECONDITION(aNodeInfoManager || aPrincipal,
                  "Must have a principal if no nodeinfo manager.");
  NS_ENSURE_ARG_POINTER(aDocType);
  NS_ENSURE_ARG_POINTER(aName);

  nsRefPtr<nsNodeInfoManager> nimgr;
  if (aNodeInfoManager) {
    nimgr = aNodeInfoManager;
  }
  else {
    nimgr = new nsNodeInfoManager();
    nsresult rv = nimgr->Init(nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    nimgr->SetDocumentPrincipal(aPrincipal);
  }

  nsCOMPtr<nsINodeInfo> ni;
  ni = nimgr->GetNodeInfo(nsGkAtoms::documentTypeNodeName, nsnull,
                          kNameSpaceID_None);
  NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

  *aDocType = new nsDOMDocumentType(ni.forget(), aName, aPublicId, aSystemId,
                                    aInternalSubset);
  NS_ADDREF(*aDocType);

  return NS_OK;
}

nsDOMDocumentType::nsDOMDocumentType(already_AddRefed<nsINodeInfo> aNodeInfo,
                                     nsIAtom *aName,
                                     const nsAString& aPublicId,
                                     const nsAString& aSystemId,
                                     const nsAString& aInternalSubset) :
  nsGenericDOMDataNode(aNodeInfo),
  mName(aName),
  mPublicId(aPublicId),
  mSystemId(aSystemId),
  mInternalSubset(aInternalSubset)
{
}

nsDOMDocumentType::~nsDOMDocumentType()
{
}

DOMCI_NODE_DATA(DocumentType, nsDOMDocumentType)


NS_INTERFACE_TABLE_HEAD(nsDOMDocumentType)
  NS_NODE_INTERFACE_TABLE2(nsDOMDocumentType, nsIDOMNode, nsIDOMDocumentType)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDOMDocumentType)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DocumentType)
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
  mName->ToString(aName);
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
  mName->ToString(aNodeName);
  return NS_OK;
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
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  return new nsDOMDocumentType(ni.forget(), mName, mPublicId, mSystemId,
                               mInternalSubset);
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
    newNodeInfo = nimgr->GetNodeInfo(mNodeInfo->NameAtom(),
                                     mNodeInfo->GetPrefixAtom(),
                                     mNodeInfo->NamespaceID());
    NS_ENSURE_TRUE(newNodeInfo, NS_ERROR_OUT_OF_MEMORY);

    mNodeInfo.swap(newNodeInfo);

    JSObject *oldScope = GetWrapper();
    if (oldScope) {
      nsIXPConnect *xpc = nsContentUtils::XPConnect();

      JSContext *cx = nsnull;
      JSObject *newScope = nsnull;
      nsresult rv = nsContentUtils::GetContextAndScope(nsnull,
                                                       nimgr->GetDocument(),
                                                       &cx, &newScope);
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
