




#ifndef mozilla_dom_XMLStylesheetProcessingInstruction_h
#define mozilla_dom_XMLStylesheetProcessingInstruction_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/ProcessingInstruction.h"
#include "nsIURI.h"
#include "nsStyleLinkElement.h"

namespace mozilla {
namespace dom {

class XMLStylesheetProcessingInstruction MOZ_FINAL
: public ProcessingInstruction
, public nsStyleLinkElement
{
public:
  XMLStylesheetProcessingInstruction(already_AddRefed<nsINodeInfo>&& aNodeInfo,
                                     const nsAString& aData)
    : ProcessingInstruction(Move(aNodeInfo), aData)
  {
  }

  XMLStylesheetProcessingInstruction(nsNodeInfoManager* aNodeInfoManager,
                                     const nsAString& aData)
    : ProcessingInstruction(aNodeInfoManager->GetNodeInfo(
                                       nsGkAtoms::processingInstructionTagName,
                                       nullptr, kNameSpaceID_None,
                                       nsIDOMNode::PROCESSING_INSTRUCTION_NODE,
                                       nsGkAtoms::xml_stylesheet), aData)
  {
  }

  virtual ~XMLStylesheetProcessingInstruction();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XMLStylesheetProcessingInstruction,
                                           ProcessingInstruction)

  
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError) MOZ_OVERRIDE;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;

  
  virtual void OverrideBaseURI(nsIURI* aNewBaseURI) MOZ_OVERRIDE;

  
  NS_IMETHOD GetCharset(nsAString& aCharset) MOZ_OVERRIDE;

  virtual void SetData(const nsAString& aData, mozilla::ErrorResult& rv) MOZ_OVERRIDE
  {
    nsGenericDOMDataNode::SetData(aData, rv);
    if (rv.Failed()) {
      return;
    }
    UpdateStyleSheetInternal(nullptr, nullptr, true);
  }
  using ProcessingInstruction::SetData; 

protected:
  nsCOMPtr<nsIURI> mOverriddenBaseURI;

  already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline) MOZ_OVERRIDE;
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         bool* aIsScoped,
                         bool* aIsAlternate) MOZ_OVERRIDE;
  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const MOZ_OVERRIDE;
};

} 
} 

#endif 
