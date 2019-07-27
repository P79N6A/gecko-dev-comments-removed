





#ifndef mozilla_dom_SVGAnimationElement_h
#define mozilla_dom_SVGAnimationElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGTests.h"
#include "nsReferencedElement.h"
#include "nsSMILTimedElement.h"
#include "nsSVGElement.h"

typedef nsSVGElement SVGAnimationElementBase;

namespace mozilla {
namespace dom {

enum nsSMILTargetAttrType {
  eSMILTargetAttrType_auto,
  eSMILTargetAttrType_CSS,
  eSMILTargetAttrType_XML
};

class SVGAnimationElement : public SVGAnimationElementBase,
                            public SVGTests
{
protected:
  explicit SVGAnimationElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  nsresult Init();
  virtual ~SVGAnimationElement();

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGAnimationElement,
                                           SVGAnimationElementBase)

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override = 0;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep, bool aNullParent) override;

  virtual nsresult UnsetAttr(int32_t aNamespaceID, nsIAtom* aAttribute,
                             bool aNotify) override;

  virtual bool IsNodeOfType(uint32_t aFlags) const override;

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) override;
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) override;

  const nsAttrValue* GetAnimAttr(nsIAtom* aName) const;
  bool GetAnimAttr(nsIAtom* aAttName, nsAString& aResult) const;
  bool HasAnimAttr(nsIAtom* aAttName) const;
  Element* GetTargetElementContent();
  virtual bool GetTargetAttributeName(int32_t* aNamespaceID,
                                      nsIAtom** aLocalName) const;
  virtual nsSMILTargetAttrType GetTargetAttributeType() const;
  nsSMILTimedElement& TimedElement();
  nsSMILTimeContainer* GetTimeContainer();
  virtual nsSMILAnimationFunction& AnimationFunction() = 0;

  virtual bool IsEventAttributeName(nsIAtom* aName) override;

  
  void ActivateByHyperlink();

  
  nsSVGElement* GetTargetElement();
  float GetStartTime(ErrorResult& rv);
  float GetCurrentTime();
  float GetSimpleDuration(ErrorResult& rv);
  void BeginElement(ErrorResult& rv) { BeginElementAt(0.f, rv); }
  void BeginElementAt(float offset, ErrorResult& rv);
  void EndElement(ErrorResult& rv) { EndElementAt(0.f, rv); }
  void EndElementAt(float offset, ErrorResult& rv);

 protected:
  

  void UpdateHrefTarget(nsIContent* aNodeForContext,
                        const nsAString& aHrefStr);
  void AnimationTargetChanged();

  class TargetReference : public nsReferencedElement {
  public:
    explicit TargetReference(SVGAnimationElement* aAnimationElement) :
      mAnimationElement(aAnimationElement) {}
  protected:
    
    
    
    virtual void ElementChanged(Element* aFrom, Element* aTo) override {
      nsReferencedElement::ElementChanged(aFrom, aTo);
      mAnimationElement->AnimationTargetChanged();
    }

    
    
    virtual bool IsPersistent() override { return true; }
  private:
    SVGAnimationElement* const mAnimationElement;
  };

  TargetReference      mHrefTarget;
  nsSMILTimedElement   mTimedElement;
};

} 
} 

#endif 
