





#ifndef mozilla_dom_HTMLSharedObjectElement_h
#define mozilla_dom_HTMLSharedObjectElement_h

#include "nsGenericHTMLElement.h"
#include "nsObjectLoadingContent.h"
#include "nsGkAtoms.h"
#include "nsError.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMGetSVGDocument.h"

namespace mozilla {
namespace dom {

class HTMLSharedObjectElement : public nsGenericHTMLElement
                              , public nsObjectLoadingContent
                              , public nsIDOMHTMLAppletElement
                              , public nsIDOMHTMLEmbedElement
                              , public nsIDOMGetSVGDocument
{
public:
  HTMLSharedObjectElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                          mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);
  virtual ~HTMLSharedObjectElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLAPPLETELEMENT

  
  

  
  NS_IMETHOD GetSrc(nsAString &aSrc);
  NS_IMETHOD SetSrc(const nsAString &aSrc);
  NS_IMETHOD GetType(nsAString &aType);
  NS_IMETHOD SetType(const nsAString &aType);

  
  NS_DECL_NSIDOMGETSVGDOCUMENT

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom *aName,
                           nsIAtom *aPrefix, const nsAString &aValue,
                           bool aNotify);

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex);
  virtual IMEState GetDesiredIMEState();

  virtual void DoneAddingChildren(bool aHaveNotified);
  virtual bool IsDoneAddingChildren();

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom *aAttribute,
                                const nsAString &aValue,
                                nsAttrValue &aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom *aAttribute) const;
  virtual nsEventStates IntrinsicState() const;
  virtual void DestroyContent();

  
  virtual uint32_t GetCapabilities() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(Element* aDest);

  void StartObjectLoad() { StartObjectLoad(true); }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(HTMLSharedObjectElement,
                                                     nsGenericHTMLElement)

  virtual nsXPCClassInfo* GetClassInfo()
  {
    return static_cast<nsXPCClassInfo*>(GetClassInfoInternal());
  }
  nsIClassInfo* GetClassInfoInternal();

  virtual nsIDOMNode* AsDOMNode()
  {
    return static_cast<nsIDOMHTMLAppletElement*>(this);
  }
private:
  


  NS_HIDDEN_(void) StartObjectLoad(bool aNotify);

  void GetTypeAttrValue(nsCString &aValue) const
  {
    if (mNodeInfo->Equals(nsGkAtoms::applet)) {
      aValue.AppendLiteral("application/x-java-vm");
    }
    else {
      nsAutoString type;
      GetAttr(kNameSpaceID_None, nsGkAtoms::type, type);

      CopyUTF16toUTF8(type, aValue);
    }
  }

  nsIAtom *URIAttrName() const
  {
    return mNodeInfo->Equals(nsGkAtoms::applet) ?
           nsGkAtoms::code :
           nsGkAtoms::src;
  }

  
  
  bool mIsDoneAddingChildren;

  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
};

} 
} 

#endif 
