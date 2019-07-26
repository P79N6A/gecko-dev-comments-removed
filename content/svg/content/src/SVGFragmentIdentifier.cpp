




#include "SVGFragmentIdentifier.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/dom/SVGViewElement.h"
#include "SVGAnimatedTransformList.h"

using namespace mozilla;

static bool
IsMatchingParameter(const nsAString &aString, const nsAString &aParameterName)
{
  
  
  return StringBeginsWith(aString, aParameterName) &&
         aString.Last() == ')' &&
         aString.CharAt(aParameterName.Length()) == '(';
}

inline bool
IgnoreWhitespace(PRUnichar aChar)
{
  return false;
}

static dom::SVGViewElement*
GetViewElement(nsIDocument *aDocument, const nsAString &aId)
{
  dom::Element* element = aDocument->GetElementById(aId);
  return (element && element->IsSVG(nsGkAtoms::view)) ?
            static_cast<dom::SVGViewElement*>(element) : nullptr;
}

void
SVGFragmentIdentifier::SaveOldPreserveAspectRatio(dom::SVGSVGElement *root)
{
  if (root->mPreserveAspectRatio.IsExplicitlySet()) {
    root->SetPreserveAspectRatioProperty(root->mPreserveAspectRatio.GetBaseValue());
  }
}

void 
SVGFragmentIdentifier::RestoreOldPreserveAspectRatio(dom::SVGSVGElement *root)
{
  const SVGPreserveAspectRatio *oldPARPtr = root->GetPreserveAspectRatioProperty();
  if (oldPARPtr) {
    root->mPreserveAspectRatio.SetBaseValue(*oldPARPtr, root);
  } else if (root->mPreserveAspectRatio.IsExplicitlySet()) {
    mozilla::ErrorResult error;
    root->RemoveAttribute(NS_LITERAL_STRING("preserveAspectRatio"), error);
  }
}

void 
SVGFragmentIdentifier::SaveOldViewBox(dom::SVGSVGElement *root)
{
  if (root->mViewBox.IsExplicitlySet()) {
    root->SetViewBoxProperty(root->mViewBox.GetBaseValue());
  }
}

void 
SVGFragmentIdentifier::RestoreOldViewBox(dom::SVGSVGElement *root)
{
  const nsSVGViewBoxRect *oldViewBoxPtr = root->GetViewBoxProperty();
  if (oldViewBoxPtr) {
    root->mViewBox.SetBaseValue(*oldViewBoxPtr, root);
  } else if (root->mViewBox.IsExplicitlySet()) {
    mozilla::ErrorResult error;
    root->RemoveAttribute(NS_LITERAL_STRING("viewBox"), error);
  }
}

void 
SVGFragmentIdentifier::SaveOldZoomAndPan(dom::SVGSVGElement *root)
{
  if (root->mEnumAttributes[dom::SVGSVGElement::ZOOMANDPAN].IsExplicitlySet()) {
    root->SetZoomAndPanProperty(root->mEnumAttributes[dom::SVGSVGElement::ZOOMANDPAN].GetBaseValue());
  }
}

void
SVGFragmentIdentifier::RestoreOldZoomAndPan(dom::SVGSVGElement *root)
{
  uint16_t oldZoomAndPan = root->GetZoomAndPanProperty();
  if (oldZoomAndPan != SVG_ZOOMANDPAN_UNKNOWN) {
    root->mEnumAttributes[dom::SVGSVGElement::ZOOMANDPAN].SetBaseValue(oldZoomAndPan, root);
  } else if (root->mEnumAttributes[dom::SVGSVGElement::ZOOMANDPAN].IsExplicitlySet()) {
    mozilla::ErrorResult error;
    root->RemoveAttribute(NS_LITERAL_STRING("zoomAndPan"), error);
  }
}

void 
SVGFragmentIdentifier::ClearTransform(dom::SVGSVGElement *root)
{
  root->mFragmentIdentifierTransform = nullptr;
  root->InvalidateTransformNotifyFrame();
}

bool
SVGFragmentIdentifier::ProcessSVGViewSpec(const nsAString &aViewSpec,
                                          dom::SVGSVGElement *root)
{
  if (!IsMatchingParameter(aViewSpec, NS_LITERAL_STRING("svgView"))) {
    return false;
  }

  
  
  
  
  
  bool viewBoxFound = false;
  bool preserveAspectRatioFound = false;
  bool transformFound = false;
  bool zoomAndPanFound = false;

  
  int32_t bracketPos = aViewSpec.FindChar('(');
  uint32_t lengthOfViewSpec = aViewSpec.Length() - bracketPos - 2;
  nsCharSeparatedTokenizerTemplate<IgnoreWhitespace> tokenizer(
    Substring(aViewSpec, bracketPos + 1, lengthOfViewSpec), ';');

  if (!tokenizer.hasMoreTokens()) {
    return false;
  }
  do {

    nsAutoString token(tokenizer.nextToken());

    bracketPos = token.FindChar('(');
    if (bracketPos < 1 || token.Last() != ')') {
      
      return false;
    }

    const nsAString &params =
      Substring(token, bracketPos + 1, token.Length() - bracketPos - 2);

    if (IsMatchingParameter(token, NS_LITERAL_STRING("viewBox"))) {
      if (viewBoxFound ||
          NS_FAILED(root->mViewBox.SetBaseValueString(
                      params, root, true))) {
        return false;
      }
      viewBoxFound = true;
    } else if (IsMatchingParameter(token, NS_LITERAL_STRING("preserveAspectRatio"))) {
      if (preserveAspectRatioFound ||
          NS_FAILED(root->mPreserveAspectRatio.SetBaseValueString(
                      params, root, true))) {
        return false;
      }
      preserveAspectRatioFound = true;
    } else if (IsMatchingParameter(token, NS_LITERAL_STRING("transform"))) {
      SVGAnimatedTransformList transforms;
      if (transformFound ||
          NS_FAILED(transforms.SetBaseValueString(params))) {
        return false;
      }
      if (!root->mFragmentIdentifierTransform) {
        root->mFragmentIdentifierTransform = new gfxMatrix();
      }
      *root->mFragmentIdentifierTransform =
        transforms.GetBaseValue().GetConsolidationMatrix();
      root->InvalidateTransformNotifyFrame();
      transformFound = true;
    } else if (IsMatchingParameter(token, NS_LITERAL_STRING("zoomAndPan"))) {
      if (zoomAndPanFound) {
        return false;
      }
      nsIAtom *valAtom = NS_GetStaticAtom(params);
      if (!valAtom) {
        return false;
      }
      const nsSVGEnumMapping *mapping = dom::SVGSVGElement::sZoomAndPanMap;
      while (mapping->mKey) {
        if (valAtom == *(mapping->mKey)) {
          
          if (NS_FAILED(root->mEnumAttributes[dom::SVGSVGElement::ZOOMANDPAN].SetBaseValue(
                          mapping->mVal, root))) {
            return false;
          }
          break;
        }
        mapping++;
      }
      if (!mapping->mKey) {
          
          return false;
      }
      zoomAndPanFound = true;
    } else {
      
      return false;
    }
  } while (tokenizer.hasMoreTokens());

  if (root->mUseCurrentView) {
    
    
    if (!transformFound) {
      ClearTransform(root);
    }
    if (!viewBoxFound) {
      RestoreOldViewBox(root);
    }
    if (!preserveAspectRatioFound) {
      RestoreOldPreserveAspectRatio(root);
    }
    if (!zoomAndPanFound) {
      RestoreOldZoomAndPan(root);
    }
  }

  return true;
}

bool
SVGFragmentIdentifier::ProcessFragmentIdentifier(nsIDocument *aDocument,
                                                 const nsAString &aAnchorName)
{
  NS_ABORT_IF_FALSE(aDocument->GetRootElement()->IsSVG(nsGkAtoms::svg),
                    "expecting an SVG root element");

  dom::SVGSVGElement *rootElement =
    static_cast<dom::SVGSVGElement*>(aDocument->GetRootElement());

  if (!rootElement->mUseCurrentView) {
    SaveOldViewBox(rootElement);
    SaveOldPreserveAspectRatio(rootElement);
    SaveOldZoomAndPan(rootElement);
  }

  const dom::SVGViewElement *viewElement = GetViewElement(aDocument, aAnchorName);

  if (viewElement) {
    if (!rootElement->mCurrentViewID) {
      rootElement->mCurrentViewID = new nsString();
    }
    *rootElement->mCurrentViewID = aAnchorName;
    rootElement->mUseCurrentView = true;
    rootElement->InvalidateTransformNotifyFrame();
    return true;
  }

  bool wasOverridden = !!rootElement->mCurrentViewID;
  rootElement->mCurrentViewID = nullptr;

  rootElement->mUseCurrentView = ProcessSVGViewSpec(aAnchorName, rootElement);
  if (rootElement->mUseCurrentView) {
    return true;
  }
  RestoreOldViewBox(rootElement);
  rootElement->ClearViewBoxProperty();
  RestoreOldPreserveAspectRatio(rootElement);
  rootElement->ClearPreserveAspectRatioProperty();
  RestoreOldZoomAndPan(rootElement);
  rootElement->ClearZoomAndPanProperty();
  ClearTransform(rootElement);
  if (wasOverridden) {
    rootElement->InvalidateTransformNotifyFrame();
  }
  return false;
}
