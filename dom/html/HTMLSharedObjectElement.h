





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

class HTMLSharedObjectElement final : public nsGenericHTMLElement
                                    , public nsObjectLoadingContent
                                    , public nsIDOMHTMLAppletElement
                                    , public nsIDOMHTMLEmbedElement
{
public:
  explicit HTMLSharedObjectElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                                   mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual int32_t TabIndexDefault() override;

#ifdef XP_MACOSX
  
  NS_IMETHOD PostHandleEvent(EventChainPostVisitor& aVisitor) override;
#endif

  
  NS_DECL_NSIDOMHTMLAPPLETELEMENT

  
  

  
  NS_IMETHOD GetSrc(nsAString &aSrc) override;
  NS_IMETHOD SetSrc(const nsAString &aSrc) override;
  NS_IMETHOD GetType(nsAString &aType) override;
  NS_IMETHOD SetType(const nsAString &aType) override;

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom *aName,
                           nsIAtom *aPrefix, const nsAString &aValue,
                           bool aNotify) override;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex) override;
  virtual IMEState GetDesiredIMEState() override;

  virtual void DoneAddingChildren(bool aHaveNotified) override;
  virtual bool IsDoneAddingChildren() override;

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom *aAttribute,
                                const nsAString &aValue,
                                nsAttrValue &aResult) override;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom *aAttribute) const override;
  virtual EventStates IntrinsicState() const override;
  virtual void DestroyContent() override;

  
  virtual uint32_t GetCapabilities() const override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

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

  nsContentPolicyType GetContentPolicyType() const override;

  
  
  bool mIsDoneAddingChildren;

  virtual void GetItemValueText(DOMString& text) override;
  virtual void SetItemValueText(const nsAString& text) override;

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
