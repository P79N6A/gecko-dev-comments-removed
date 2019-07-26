





#ifndef mozilla_dom_HTMLAreaElement_h
#define mozilla_dom_HTMLAreaElement_h

#include "nsIDOMHTMLAreaElement.h"
#include "nsGenericHTMLElement.h"
#include "nsILink.h"
#include "nsGkAtoms.h"
#include "nsIURL.h"
#include "nsIDocument.h"
#include "Link.h"

namespace mozilla {
namespace dom {

class HTMLAreaElement : public nsGenericHTMLElement,
                        public nsIDOMHTMLAreaElement,
                        public nsILink,
                        public Link
{
public:
  HTMLAreaElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLAreaElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_SIZEOF_EXCLUDING_THIS

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLAREAELEMENT

  
  NS_IMETHOD LinkAdded() { return NS_OK; }
  NS_IMETHOD LinkRemoved() { return NS_OK; }

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual bool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);
  virtual nsLinkState GetLinkState() const;
  virtual already_AddRefed<nsIURI> GetHrefURI() const;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);

  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  virtual nsEventStates IntrinsicState() const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  

  
  void SetAlt(const nsAString& aAlt, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::alt, aAlt, aError);
  }

  
  void SetCoords(const nsAString& aCoords, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::coords, aCoords, aError);
  }

  
  void SetShape(const nsAString& aShape, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::shape, aShape, aError);
  }

  
  void SetHref(const nsAString& aHref, ErrorResult& aError)
  {
    aError = SetHref(aHref);
  }

  
  void SetTarget(const nsAString& aTarget, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::target, aTarget, aError);
  }

  
  void SetDownload(const nsAString& aDownload, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::download, aDownload, aError);
  }

  
  void SetPing(const nsAString& aPing, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::ping, aPing, aError);
  }

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  bool NoHref() const
  {
    return GetBoolAttr(nsGkAtoms::nohref);
  }

  void SetNoHref(bool aValue, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::nohref, aValue, aError);
  }

  void Stringify(nsAString& aResult)
  {
    GetHref(aResult);
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
};

} 
} 

#endif 
