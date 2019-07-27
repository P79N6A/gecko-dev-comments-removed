




#ifndef mozilla_dom_SVGUseElement_h
#define mozilla_dom_SVGUseElement_h

#include "mozilla/dom/FromParser.h"
#include "nsReferencedElement.h"
#include "nsStubMutationObserver.h"
#include "mozilla/dom/SVGGraphicsElement.h"
#include "nsSVGLength2.h"
#include "nsSVGString.h"
#include "nsTArray.h"

class nsIContent;
class nsSVGUseFrame;

nsresult
NS_NewSVGSVGElement(nsIContent **aResult,
                    already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                    mozilla::dom::FromParser aFromParser);
nsresult NS_NewSVGUseElement(nsIContent **aResult,
                             already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGGraphicsElement SVGUseElementBase;

class SVGUseElement MOZ_FINAL : public SVGUseElementBase,
                                public nsStubMutationObserver
{
  friend class ::nsSVGUseFrame;
protected:
  friend nsresult (::NS_NewSVGUseElement(nsIContent **aResult,
                                         already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGUseElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  virtual ~SVGUseElement();
  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGUseElement, SVGUseElementBase)

  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

  
  nsIContent* CreateAnonymousContent();
  nsIContent* GetAnonymousContent() const { return mClone; }
  void DestroyAnonymousContent();

  
  virtual gfxMatrix PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                      TransformTypes aWhich = eAllTransforms) const MOZ_OVERRIDE;
  virtual bool HasValidDimensions() const MOZ_OVERRIDE;

  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedString> Href();
  already_AddRefed<SVGAnimatedLength> X();
  already_AddRefed<SVGAnimatedLength> Y();
  already_AddRefed<SVGAnimatedLength> Width();
  already_AddRefed<SVGAnimatedLength> Height();

protected:
  class SourceReference : public nsReferencedElement {
  public:
    explicit SourceReference(SVGUseElement* aContainer) : mContainer(aContainer) {}
  protected:
    virtual void ElementChanged(Element* aFrom, Element* aTo) MOZ_OVERRIDE {
      nsReferencedElement::ElementChanged(aFrom, aTo);
      if (aFrom) {
        aFrom->RemoveMutationObserver(mContainer);
      }
      mContainer->TriggerReclone();
    }
  private:
    SVGUseElement* mContainer;
  };

  virtual LengthAttributesInfo GetLengthInfo() MOZ_OVERRIDE;
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  




  bool OurWidthAndHeightAreUsed() const;
  void SyncWidthOrHeight(nsIAtom *aName);
  void LookupHref();
  void TriggerReclone();
  void UnlinkSource();

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

  nsCOMPtr<nsIContent> mOriginal; 
  nsCOMPtr<nsIContent> mClone;    
  SourceReference      mSource;   
};

} 
} 

#endif 
