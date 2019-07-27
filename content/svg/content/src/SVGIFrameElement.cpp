




#include "SVGIFrameElement.h"

#include "GeckoProfiler.h"
#include "mozilla/ArrayUtils.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/SVGDocumentBinding.h"
#include "mozilla/dom/SVGIFrameElementBinding.h"
#include "mozilla/dom/SVGMatrix.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/Preferences.h"
#include "nsStyleConsts.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT_CHECK_PARSER(IFrame)

namespace mozilla {
namespace dom {

JSObject*
SVGIFrameElement::WrapNode(JSContext *aCx)
{
  return SVGIFrameElementBinding::Wrap(aCx, this);
}
  


nsSVGElement::LengthInfo SVGIFrameElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y }
};



NS_IMPL_ISUPPORTS_INHERITED(SVGIFrameElement, SVGIFrameElementBase,
                            nsIFrameLoaderOwner,
                            nsIDOMNode, nsIDOMElement,
                            nsIDOMSVGElement)



SVGIFrameElement::SVGIFrameElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                                   FromParser aFromParser)
  : SVGIFrameElementBase(aNodeInfo)
  , nsElementFrameLoaderOwner(aFromParser)
{
}

SVGIFrameElement::~SVGIFrameElement()
{
}




 gfxMatrix
SVGIFrameElement::PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                                           TransformTypes aWhich) const
{
  
  gfxMatrix fromUserSpace =
    SVGGraphicsElement::PrependLocalTransformsTo(aMatrix, aWhich);
  if (aWhich == eUserSpaceToParent) {
    return fromUserSpace;
  }
  
  float x, y;
  const_cast<SVGIFrameElement*>(this)->
    GetAnimatedLengthValues(&x, &y, nullptr);
  gfxMatrix toUserSpace = gfxMatrix::Translation(x, y);
  if (aWhich == eChildToUserSpace) {
    return toUserSpace;
  }
  NS_ABORT_IF_FALSE(aWhich == eAllTransforms, "Unknown TransformTypes");
  return toUserSpace * fromUserSpace;
}
  
  



nsresult
SVGIFrameElement::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;
  already_AddRefed<mozilla::dom::NodeInfo> ni = nsRefPtr<mozilla::dom::NodeInfo>(aNodeInfo).forget();
  SVGIFrameElement *it = new SVGIFrameElement(ni, NOT_FROM_PARSER);

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv1 = it->Init();
  nsresult rv2 = const_cast<SVGIFrameElement*>(this)->CopyInnerTo(it);
  if (NS_SUCCEEDED(rv1) && NS_SUCCEEDED(rv2)) {
    kungFuDeathGrip.swap(*aResult);
  }

  return NS_FAILED(rv1) ? rv1 : rv2;
}



  
nsSVGElement::LengthAttributesInfo
SVGIFrameElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

SVGAnimatedPreserveAspectRatio *
SVGIFrameElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}




already_AddRefed<SVGAnimatedLength>
SVGIFrameElement::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGIFrameElement::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGIFrameElement::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGIFrameElement::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}

already_AddRefed<DOMSVGAnimatedPreserveAspectRatio>
SVGIFrameElement::PreserveAspectRatio()
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio),
                                                        this);
  return ratio.forget();
}

void
SVGIFrameElement::GetName(DOMString& name)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
}

void
SVGIFrameElement::GetSrc(DOMString& src)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::src, src);
}

void
SVGIFrameElement::GetSrcdoc(DOMString& srcdoc)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::srcdoc, srcdoc);
}

nsDOMSettableTokenList*
SVGIFrameElement::Sandbox()
{
  return GetTokenList(nsGkAtoms::sandbox);
}

bool
SVGIFrameElement::ParseAttribute(int32_t aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::sandbox) {
      aResult.ParseAtomArray(aValue);
      return true;
    }
  }
  return SVGIFrameElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                              aValue, aResult);
}

nsresult
SVGIFrameElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                          nsIAtom* aPrefix, const nsAString& aValue,
                          bool aNotify)
{
  nsresult rv = nsSVGElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                      aValue, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNameSpaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::src &&
        !HasAttr(kNameSpaceID_None,nsGkAtoms::srcdoc)) {
      
      
      LoadSrc();
    }
    if (aName == nsGkAtoms::srcdoc) {
      
      
      LoadSrc();
    }
  }
  return NS_OK;
}

nsresult
SVGIFrameElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                               const nsAttrValue* aValue,
                               bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::sandbox && mFrameLoader) {
      
      
      
      mFrameLoader->ApplySandboxFlags(GetSandboxFlags());
    }
  }
  return nsSVGElement::AfterSetAttr(aNameSpaceID, aName, aValue, aNotify);
}

nsresult
SVGIFrameElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                            bool aNotify)
{
  
  nsresult rv = nsSVGElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::srcdoc) {
      
      LoadSrc();
    }
  }

  return NS_OK;
}

uint32_t
SVGIFrameElement::GetSandboxFlags()
{
  const nsAttrValue* sandboxAttr = GetParsedAttr(nsGkAtoms::sandbox);
  return nsContentUtils::ParseSandboxAttributeToFlags(sandboxAttr);
}

nsresult
SVGIFrameElement::BindToTree(nsIDocument* aDocument,
                             nsIContent* aParent,
                             nsIContent* aBindingParent,
                             bool aCompileEventHandlers)
{
  nsresult rv = nsSVGElement::BindToTree(aDocument, aParent,
                                         aBindingParent,
                                         aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
                 "Missing a script blocker!");

    PROFILER_LABEL("SVGIFrameElement", "BindToTree",
      js::ProfileEntry::Category::OTHER);

    
    LoadSrc();

    if (HasAttr(kNameSpaceID_None, nsGkAtoms::sandbox)) {
      if (mFrameLoader) {
        mFrameLoader->ApplySandboxFlags(GetSandboxFlags());
      }
    }
  }

  
  
  mNetworkCreated = false;
  return rv;
}

void
SVGIFrameElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  if (mFrameLoader) {
    
    
    
    
    
    
    mFrameLoader->Destroy();
    mFrameLoader = nullptr;
  }

  nsSVGElement::UnbindFromTree(aDeep, aNullParent);
}

void
SVGIFrameElement::DestroyContent()
{
  if (mFrameLoader) {
    mFrameLoader->Destroy();
    mFrameLoader = nullptr;
  }

  nsSVGElement::DestroyContent();
}

nsresult
SVGIFrameElement::CopyInnerTo(Element* aDest)
{
  nsresult rv = nsSVGElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIDocument* doc = aDest->OwnerDoc();
  if (doc->IsStaticDocument() && mFrameLoader) {
    SVGIFrameElement* dest =
      static_cast<SVGIFrameElement*>(aDest);
    nsFrameLoader* fl = nsFrameLoader::Create(dest, false);
    NS_ENSURE_STATE(fl);
    dest->mFrameLoader = fl;
    static_cast<nsFrameLoader*>(mFrameLoader.get())->CreateStaticClone(fl);
  }

  return rv;
}

} 
} 
