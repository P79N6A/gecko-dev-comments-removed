








#ifndef DocumentType_h
#define DocumentType_h

#include "mozilla/Attributes.h"
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
  DocumentTypeForward(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {
  }

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
};

class DocumentType MOZ_FINAL : public DocumentTypeForward
{
public:
  DocumentType(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
               const nsAString& aPublicId,
               const nsAString& aSystemId,
               const nsAString& aInternalSubset);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  

  
  NS_DECL_NSIDOMDOCUMENTTYPE

  
  virtual bool IsNodeOfType(uint32_t aFlags) const MOZ_OVERRIDE;
  virtual void GetNodeValueInternal(nsAString& aNodeValue) MOZ_OVERRIDE
  {
    SetDOMStringToNull(aNodeValue);
  }
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError) MOZ_OVERRIDE
  {
  }

  
  virtual const nsTextFragment* GetText() MOZ_OVERRIDE;

  virtual nsGenericDOMDataNode* CloneDataNode(mozilla::dom::NodeInfo *aNodeInfo,
                                              bool aCloneText) const MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

protected:
  virtual ~DocumentType();

  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;

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
