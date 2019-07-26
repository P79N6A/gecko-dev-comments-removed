




#ifndef mozilla_dom_XMLStylesheetProcessingInstruction_h
#define mozilla_dom_XMLStylesheetProcessingInstruction_h

#include "mozilla/dom/ProcessingInstruction.h"
#include "nsStyleLinkElement.h"

namespace mozilla {
namespace dom {

class XMLStylesheetProcessingInstruction : public ProcessingInstruction,
                                           public nsStyleLinkElement
{
public:
  XMLStylesheetProcessingInstruction(already_AddRefed<nsINodeInfo> aNodeInfo,
                                     const nsAString& aData)
    : ProcessingInstruction(aNodeInfo, aData)
  {
  }
  virtual ~XMLStylesheetProcessingInstruction();

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XMLStylesheetProcessingInstruction,
                                           ProcessingInstruction)

  
  virtual void SetNodeValueInternal(const nsAString& aNodeValue,
                                    mozilla::ErrorResult& aError);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  
  virtual void OverrideBaseURI(nsIURI* aNewBaseURI);

  
  NS_IMETHOD GetCharset(nsAString& aCharset);

protected:
  nsCOMPtr<nsIURI> mOverriddenBaseURI;

  already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline);
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         bool* aIsScoped,
                         bool* aIsAlternate);
  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;
};

} 
} 

#endif 
