









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
  explicit DocumentTypeForward(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {
  }

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
};

class DocumentType final : public DocumentTypeForward
{
public:
  DocumentType(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
               const nsAString& aPublicId,
               const nsAString& aSystemId,
               const nsAString& aInternalSubset);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  

  
  NS_DECL_NSIDOMDOCUMENTTYPE

  
  virtual bool IsNodeOfType(uint32_t aFlags) const override;
  virtual void GetNodeValueInternal(nsAString& aNodeValue) override
  {
    SetDOMStringToNull(aNodeValue);
  }
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError) override
  {
  }

  
  virtual const nsTextFragment* GetText() override;

  virtual nsGenericDOMDataNode* CloneDataNode(mozilla::dom::NodeInfo *aNodeInfo,
                                              bool aCloneText) const override;

  virtual nsIDOMNode* AsDOMNode() override { return this; }

protected:
  virtual ~DocumentType();

  virtual JSObject* WrapNode(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

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
