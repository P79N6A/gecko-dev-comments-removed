








#ifndef DocumentType_h
#define DocumentType_h

#include "nsCOMPtr.h"
#include "nsIDOMDocumentType.h"
#include "nsIContent.h"
#include "nsGenericDOMDataNode.h"
#include "nsString.h"

namespace mozilla {
namespace dom {






class DocumentTypeForward : public nsGenericDOMDataNode,
                            public nsIDOMDocumentType
{
public:
  DocumentTypeForward(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {
  }

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
};

class DocumentType : public DocumentTypeForward
{
public:
  DocumentType(already_AddRefed<nsINodeInfo> aNodeInfo,
               const nsAString& aPublicId,
               const nsAString& aSystemId,
               const nsAString& aInternalSubset);

  virtual ~DocumentType();

  
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

  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

  nsString mPublicId;
  nsString mSystemId;
  nsString mInternalSubset;
};

} 
} 

already_AddRefed<mozilla::dom::DocumentType>
NS_NewDOMDocumentType(nsNodeInfoManager* aNodeInfoManager,
                      nsIAtom *aName,
                      const nsAString& aPublicId,
                      const nsAString& aSystemId,
                      const nsAString& aInternalSubset,
                      mozilla::ErrorResult& rv);

nsresult
NS_NewDOMDocumentType(nsIDOMDocumentType** aDocType,
                      nsNodeInfoManager* aNodeInfoManager,
                      nsIAtom *aName,
                      const nsAString& aPublicId,
                      const nsAString& aSystemId,
                      const nsAString& aInternalSubset);

#endif
