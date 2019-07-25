





































#ifndef __NS_SVGSWITCHELEMENT_H__
#define __NS_SVGSWITCHELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGSwitchElement.h"

typedef nsSVGGraphicElement nsSVGSwitchElementBase;

class nsSVGSwitchElement : public nsSVGSwitchElementBase,
                           public nsIDOMSVGSwitchElement
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
                                 PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
private:
  void UpdateActiveChild()
  { mActiveChild = FindActiveChild(); }
  nsIContent* FindActiveChild() const;

  
  nsCOMPtr<nsIContent> mActiveChild;
};

#endif 
