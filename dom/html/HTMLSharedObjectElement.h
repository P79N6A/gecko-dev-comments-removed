





#ifndef mozilla_dom_HTMLSharedObjectElement_h
#define mozilla_dom_HTMLSharedObjectElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsObjectLoadingContent.h"
#include "nsGkAtoms.h"
#include "nsError.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIDOMHTMLEmbedElement.h"

namespace mozilla {
namespace dom {

class HTMLSharedObjectElement MOZ_FINAL : public nsGenericHTMLElement
                                        , public nsObjectLoadingContent
                                        , public nsIDOMHTMLAppletElement
                                        , public nsIDOMHTMLEmbedElement
{
public:
  explicit HTMLSharedObjectElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                                   mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLAPPLETELEMENT

  
  

  
  NS_IMETHOD GetSrc(nsAString &aSrc) MOZ_OVERRIDE;
  NS_IMETHOD SetSrc(const nsAString &aSrc) MOZ_OVERRIDE;
  NS_IMETHOD GetType(nsAString &aType) MOZ_OVERRIDE;
  NS_IMETHOD SetType(const nsAString &aType) MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom *aName,
                           nsIAtom *aPrefix, const nsAString &aValue,
                           bool aNotify) MOZ_OVERRIDE;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex) MOZ_OVERRIDE;
  virtual IMEState GetDesiredIMEState() MOZ_OVERRIDE;

  virtual void DoneAddingChildren(bool aHaveNotified) MOZ_OVERRIDE;
  virtual bool IsDoneAddingChildren() MOZ_OVERRIDE;

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom *aAttribute,
                                const nsAString &aValue,
                                nsAttrValue &aResult) MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom *aAttribute) const MOZ_OVERRIDE;
  virtual EventStates IntrinsicState() const MOZ_OVERRIDE;
  virtual void DestroyContent() MOZ_OVERRIDE;

  
  virtual uint32_t GetCapabilities() const MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  nsresult CopyInnerTo(Element* aDest);

  void StartObjectLoad() { StartObjectLoad(true); }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(HTMLSharedObjectElement,
                                                     nsGenericHTMLElement)

  
  void GetAlign(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::align, aValue);
  }
  void SetAlign(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::align, aValue, aRv);
  }
  void GetAlt(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::alt, aValue);
  }
  void SetAlt(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::alt, aValue, aRv);
  }
  void GetArchive(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::archive, aValue);
  }
  void SetArchive(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::archive, aValue, aRv);
  }
  void GetCode(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::code, aValue);
  }
  void SetCode(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::code, aValue, aRv);
  }
  
  void SetCodeBase(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::codebase, aValue, aRv);
  }
  void GetHeight(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::height, aValue);
  }
  void SetHeight(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::height, aValue, aRv);
  }
  uint32_t Hspace()
  {
    return GetUnsignedIntAttr(nsGkAtoms::hspace, 0);
  }
  void SetHspace(uint32_t aValue, ErrorResult& aRv)
  {
    SetUnsignedIntAttr(nsGkAtoms::hspace, aValue, aRv);
  }
  void GetName(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::name, aValue);
  }
  void SetName(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aValue, aRv);
  }
  
  void SetObject(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::object, aValue, aRv);
  }
  uint32_t Vspace()
  {
    return GetUnsignedIntAttr(nsGkAtoms::vspace, 0);
  }
  void SetVspace(uint32_t aValue, ErrorResult& aRv)
  {
    SetUnsignedIntAttr(nsGkAtoms::vspace, aValue, aRv);
  }
  void GetWidth(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::width, aValue);
  }
  void SetWidth(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::width, aValue, aRv);
  }

  
  
  void SetSrc(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::src, aValue, aRv);
  }
  void GetType(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::type, aValue);
  }
  void SetType(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::type, aValue, aRv);
  }
  
  
  
  
  nsIDocument* GetSVGDocument()
  {
    return GetContentDocument();
  }

private:
  virtual ~HTMLSharedObjectElement();

  


  void StartObjectLoad(bool aNotify);

  nsIAtom *URIAttrName() const
  {
    return mNodeInfo->Equals(nsGkAtoms::applet) ?
           nsGkAtoms::code :
           nsGkAtoms::src;
  }

  
  
  bool mIsDoneAddingChildren;

  virtual void GetItemValueText(nsAString& text) MOZ_OVERRIDE;
  virtual void SetItemValueText(const nsAString& text) MOZ_OVERRIDE;

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
