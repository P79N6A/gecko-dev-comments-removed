




#ifndef mozilla_dom_SVGSwitchElement_h
#define mozilla_dom_SVGSwitchElement_h

#include "mozilla/dom/SVGGraphicsElement.h"

class nsSVGSwitchFrame;

nsresult NS_NewSVGSwitchElement(nsIContent **aResult,
                                already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGGraphicsElement SVGSwitchElementBase;

class SVGSwitchElement MOZ_FINAL : public SVGSwitchElementBase
{
  friend class nsSVGSwitchFrame;
protected:
  friend nsresult (::NS_NewSVGSwitchElement(nsIContent **aResult,
                                            already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGSwitchElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  ~SVGSwitchElement();
  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

public:
  nsIContent * GetActiveChild() const
  { return mActiveChild; }
  void MaybeInvalidate();

  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGSwitchElement,
                                           SVGSwitchElementBase)
  
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) MOZ_OVERRIDE;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) MOZ_OVERRIDE;

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;
private:
  void UpdateActiveChild()
  { mActiveChild = FindActiveChild(); }
  nsIContent* FindActiveChild() const;

  
  nsCOMPtr<nsIContent> mActiveChild;
};

} 
} 

#endif 
