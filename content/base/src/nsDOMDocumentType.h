








































#ifndef nsDOMDocumentType_h
#define nsDOMDocumentType_h

#include "nsCOMPtr.h"
#include "nsIDOMDocumentType.h"
#include "nsIContent.h"
#include "nsGenericDOMDataNode.h"
#include "nsString.h"






class nsDOMDocumentTypeForward : public nsGenericDOMDataNode,
                                 public nsIDOMDocumentType
{
public:
  nsDOMDocumentTypeForward(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {
  }

  
  NS_FORWARD_NSIDOMNODE(nsGenericDOMDataNode::)
};

class nsDOMDocumentType : public nsDOMDocumentTypeForward
{
public:
  nsDOMDocumentType(already_AddRefed<nsINodeInfo> aNodeInfo,
                    const nsAString& aPublicId,
                    const nsAString& aSystemId,
                    const nsAString& aInternalSubset);

  virtual ~nsDOMDocumentType();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  

  
  NS_DECL_NSIDOMDOCUMENTTYPE

  NS_IMETHODIMP GetNodeValue(nsAString& aNodeValue)
  {
    SetDOMStringToNull(aNodeValue);
  
    return NS_OK;
  }
  NS_IMETHODIMP SetNodeValue(const nsAString& aNodeValue)
  {
    return NS_OK;
  }

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  
  virtual const nsTextFragment* GetText();
  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              PRBool aCloneText) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  nsString mPublicId;
  nsString mSystemId;
  nsString mInternalSubset;
};

nsresult
NS_NewDOMDocumentType(nsIDOMDocumentType** aDocType,
                      nsNodeInfoManager *aOwnerDoc,
                      nsIPrincipal *aPrincipal,
                      nsIAtom *aName,
                      const nsAString& aPublicId,
                      const nsAString& aSystemId,
                      const nsAString& aInternalSubset);

#endif
