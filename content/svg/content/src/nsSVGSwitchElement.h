





































#ifndef __NS_SVGSWITCHELEMENT_H__
#define __NS_SVGSWITCHELEMENT_H__

#include "DOMSVGTests.h"
#include "nsIDOMSVGSwitchElement.h"
#include "nsSVGGraphicElement.h"

typedef nsSVGGraphicElement nsSVGSwitchElementBase;

class nsSVGSwitchElement : public nsSVGSwitchElementBase,
                           public nsIDOMSVGSwitchElement,
                           public DOMSVGTests
{
  friend class nsSVGSwitchFrame;
protected:
  friend nsresult NS_NewSVGSwitchElement(nsIContent **aResult,
                                         already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGSwitchElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  nsIContent * GetActiveChild() const
  { return mActiveChild; }
  void MaybeInvalidate();
    
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsSVGSwitchElement,
                                           nsSVGSwitchElementBase)
  NS_DECL_NSIDOMSVGSWITCHELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGSwitchElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGSwitchElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGSwitchElementBase::)

  
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 bool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, bool aNotify);

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
private:
  void UpdateActiveChild()
  { mActiveChild = FindActiveChild(); }
  nsIContent* FindActiveChild() const;

  
  nsCOMPtr<nsIContent> mActiveChild;
};

#endif 
