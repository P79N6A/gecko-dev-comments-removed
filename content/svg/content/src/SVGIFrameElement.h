




#include "mozilla/dom/DOMString.h"
#include "mozilla/dom/FromParser.h"
#include "mozilla/dom/SVGGraphicsElement.h"
#include "nsContentUtils.h"
#include "nsDOMSettableTokenList.h"
#include "nsFrameLoader.h"
#include "nsElementFrameLoaderOwner.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIFrameLoader.h"
#include "nsIWebNavigation.h"
#include "nsSVGElement.h"
#include "nsSVGLength2.h"
#include "SVGAnimatedPreserveAspectRatio.h"

nsresult NS_NewSVGIFrameElement(nsIContent **aResult,
                                already_AddRefed<nsINodeInfo>&& aNodeInfo,
                                mozilla::dom::FromParser aFromParser);

typedef mozilla::dom::SVGGraphicsElement SVGIFrameElementBase;

class nsSVGIFrameFrame;

namespace mozilla {
namespace dom {
class DOMSVGAnimatedPreserveAspectRatio;

class SVGIFrameElement MOZ_FINAL : public SVGIFrameElementBase,
                                   public nsElementFrameLoaderOwner
{
  friend class ::nsSVGIFrameFrame;
  friend nsresult (::NS_NewSVGIFrameElement(nsIContent **aResult,
                                  already_AddRefed<nsINodeInfo>&& aNodeInfo,
                                  mozilla::dom::FromParser aFromParser));

  SVGIFrameElement(already_AddRefed<nsINodeInfo>& aNodeInfo,
                   mozilla::dom::FromParser aFromParser);
  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual gfxMatrix PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                                             TransformTypes aWhich = eAllTransforms) const MOZ_OVERRIDE;

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) MOZ_OVERRIDE;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) MOZ_OVERRIDE;
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue,
                                bool aNotify) MOZ_OVERRIDE;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) MOZ_OVERRIDE;

  virtual void DestroyContent() MOZ_OVERRIDE;
  nsresult CopyInnerTo(mozilla::dom::Element* aDest);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedLength> X();
  already_AddRefed<SVGAnimatedLength> Y();
  already_AddRefed<SVGAnimatedLength> Width();
  already_AddRefed<SVGAnimatedLength> Height();
  already_AddRefed<DOMSVGAnimatedPreserveAspectRatio> PreserveAspectRatio();
  void GetName(DOMString& name);
  void GetSrc(DOMString& src);
  void GetSrcdoc(DOMString& srcdoc);
  already_AddRefed<nsDOMSettableTokenList> Sandbox();
  using nsElementFrameLoaderOwner::GetContentDocument;
  using nsElementFrameLoaderOwner::GetContentWindow;

  
  
  uint32_t GetSandboxFlags();

private:
  virtual LengthAttributesInfo GetLengthInfo();
  virtual SVGAnimatedPreserveAspectRatio *GetPreserveAspectRatio();
  virtual mozilla::dom::Element* ThisFrameElement() MOZ_OVERRIDE
  {
    return this;
  }

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  SVGAnimatedPreserveAspectRatio mPreserveAspectRatio;
};

} 
} 
