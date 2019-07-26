




#ifndef mozilla_dom_SVGStyleElement_h
#define mozilla_dom_SVGStyleElement_h

#include "nsSVGElement.h"
#include "nsStyleLinkElement.h"
#include "nsStubMutationObserver.h"

nsresult NS_NewSVGStyleElement(nsIContent **aResult,
                               already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGElement SVGStyleElementBase;

namespace mozilla {
namespace dom {

class SVGStyleElement MOZ_FINAL : public SVGStyleElementBase,
                                  public nsIDOMSVGElement,
                                  public nsStyleLinkElement,
                                  public nsStubMutationObserver
{
protected:
  friend nsresult (::NS_NewSVGStyleElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGStyleElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGStyleElement,
                                           SVGStyleElementBase)

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGStyleElementBase::)

  
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
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  void GetXmlspace(nsAString & aXmlspace);
  void SetXmlspace(const nsAString & aXmlspace, ErrorResult& rv);
  void GetMedia(nsAString & aMedia);
  void SetMedia(const nsAString& aMedia, ErrorResult& rv);
  bool Scoped() const;
  void SetScoped(bool aScoped, ErrorResult& rv);
  void GetType(nsAString & aType);
  void SetType(const nsAString& aType, ErrorResult& rv);
  void GetTitle(nsAString & aTitle);
  void SetTitle(const nsAString& aTitle, ErrorResult& rv);

protected:
  
  
  
  inline nsresult Init()
  {
    return NS_OK;
  }

  
  already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline);

  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         bool* aIsScoped,
                         bool* aIsAlternate);
  virtual CORSMode GetCORSMode() const;

  




  void ContentChanged(nsIContent* aContent);
};

} 
} 

#endif 
