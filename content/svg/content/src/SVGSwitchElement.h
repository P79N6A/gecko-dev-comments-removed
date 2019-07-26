




#ifndef mozilla_dom_SVGSwitchElement_h
#define mozilla_dom_SVGSwitchElement_h

#include "mozilla/dom/SVGGraphicsElement.h"

nsresult NS_NewSVGSwitchElement(nsIContent **aResult,
                                already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGGraphicsElement SVGSwitchElementBase;

class SVGSwitchElement MOZ_FINAL : public SVGSwitchElementBase
{
  friend class nsSVGSwitchFrame;
protected:
  friend nsresult (::NS_NewSVGSwitchElement(nsIContent **aResult,
                                            already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGSwitchElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

public:
  nsIContent * GetActiveChild() const
  { return mActiveChild; }
  void MaybeInvalidate();

  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGSwitchElement,
                                           SVGSwitchElementBase)
  
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
private:
  void UpdateActiveChild()
  { mActiveChild = FindActiveChild(); }
  nsIContent* FindActiveChild() const;

  
  nsCOMPtr<nsIContent> mActiveChild;
};

} 
} 

#endif 
