








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

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
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

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;
  virtual void GetNodeValueInternal(nsAString& aNodeValue)
  {
    SetDOMStringToNull(aNodeValue);
  }
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError)
  {
  }

  
  virtual const nsTextFragment* GetText();

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  nsString mPublicId;
  nsString mSystemId;
  nsString mInternalSubset;
};

nsresult
NS_NewDOMDocumentType(nsIDOMDocumentType** aDocType,
                      nsNodeInfoManager* aNodeInfoManager,
                      nsIAtom *aName,
                      const nsAString& aPublicId,
                      const nsAString& aSystemId,
                      const nsAString& aInternalSubset);

#endif
