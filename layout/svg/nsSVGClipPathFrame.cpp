





#include "nsSVGClipPathFrame.h"


#include "gfxContext.h"
#include "mozilla/dom/SVGClipPathElement.h"
#include "nsGkAtoms.h"
#include "nsSVGEffects.h"
#include "nsSVGPathGeometryElement.h"
#include "nsSVGPathGeometryFrame.h"
#include "nsSVGUtils.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;




nsIFrame*
NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGClipPathFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGClipPathFrame)

nsresult
nsSVGClipPathFrame::ApplyClipOrPaintClipMask(gfxContext& aContext,
                                             nsIFrame* aClippedFrame,
                                             const gfxMatrix& aMatrix)
{
  DrawTarget& aDrawTarget = *aContext.GetDrawTarget();

  
  
  
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return NS_OK;
  }
  AutoClipPathReferencer clipRef(this);

  mMatrixForChildren = GetClipPathTransform(aClippedFrame) * aMatrix;

  nsISVGChildFrame* singleClipPathChild = nullptr;

  if (IsTrivial(&singleClipPathChild)) {
    gfxContextMatrixAutoSaveRestore autoRestore(&aContext);
    RefPtr<Path> clipPath;
    if (singleClipPathChild) {
      nsSVGPathGeometryFrame* pathFrame = do_QueryFrame(singleClipPathChild);
      if (pathFrame) {
        nsSVGPathGeometryElement* pathElement =
          static_cast<nsSVGPathGeometryElement*>(pathFrame->GetContent());
        gfxMatrix toChildsUserSpace = pathElement->
          PrependLocalTransformsTo(mMatrixForChildren,
                                   nsSVGElement::eUserSpaceToParent);
        gfxMatrix newMatrix =
          aContext.CurrentMatrix().PreMultiply(toChildsUserSpace).NudgeToIntegers();
        if (!newMatrix.IsSingular()) {
          aContext.SetMatrix(newMatrix);
          clipPath = pathElement->GetOrBuildPath(aDrawTarget,
                                                 nsSVGUtils::ToFillRule(pathFrame->StyleSVG()->mClipRule));
        }
      }
    }
    if (clipPath) {
      aContext.Clip(clipPath);
    } else {
      
      
      aContext.Clip(Rect());
    }
    return NS_OK;
  }

  
  
  
  

  
  nsSVGClipPathFrame *clipPathFrame =
    nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nullptr);
  bool referencedClipIsTrivial;
  if (clipPathFrame) {
    referencedClipIsTrivial = clipPathFrame->IsTrivial();
    aContext.Save();
    if (referencedClipIsTrivial) {
      clipPathFrame->ApplyClipOrPaintClipMask(aContext, aClippedFrame, aMatrix);
    } else {
      aContext.PushGroup(gfxContentType::ALPHA);
    }
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      
      SVGFrame->NotifySVGChanged(nsISVGChildFrame::TRANSFORM_CHANGED);

      bool isOK = true;
      nsSVGClipPathFrame *clipPathFrame =
        nsSVGEffects::GetEffectProperties(kid).GetClipPathFrame(&isOK);
      if (!isOK) {
        continue;
      }

      bool isTrivial;

      if (clipPathFrame) {
        isTrivial = clipPathFrame->IsTrivial();
        aContext.Save();
        if (isTrivial) {
          clipPathFrame->ApplyClipOrPaintClipMask(aContext, aClippedFrame, aMatrix);
        } else {
          aContext.PushGroup(gfxContentType::ALPHA);
        }
      }

      gfxMatrix toChildsUserSpace = mMatrixForChildren;
      nsIFrame* child = do_QueryFrame(SVGFrame);
      nsIContent* childContent = child->GetContent();
      if (childContent->IsSVGElement()) {
        toChildsUserSpace =
          static_cast<const nsSVGElement*>(childContent)->
            PrependLocalTransformsTo(mMatrixForChildren,
                                     nsSVGElement::eUserSpaceToParent);
      }
      SVGFrame->PaintSVG(aContext, toChildsUserSpace);

      if (clipPathFrame) {
        if (!isTrivial) {
          aContext.PopGroupToSource();

          aContext.PushGroup(gfxContentType::ALPHA);

          clipPathFrame->ApplyClipOrPaintClipMask(aContext, aClippedFrame, aMatrix);
          Matrix maskTransform;
          RefPtr<SourceSurface> clipMaskSurface = aContext.PopGroupToSurface(&maskTransform);

          if (clipMaskSurface) {
            aContext.Mask(clipMaskSurface, maskTransform);
          }
        }
        aContext.Restore();
      }
    }
  }

  if (clipPathFrame) {
    if (!referencedClipIsTrivial) {
      aContext.PopGroupToSource();

      aContext.PushGroup(gfxContentType::ALPHA);

      clipPathFrame->ApplyClipOrPaintClipMask(aContext, aClippedFrame, aMatrix);
      Matrix maskTransform;
      RefPtr<SourceSurface> clipMaskSurface = aContext.PopGroupToSurface(&maskTransform);

      if (clipMaskSurface) {
        aContext.Mask(clipMaskSurface, maskTransform);
      }
    }
    aContext.Restore();
  }

  return NS_OK;
}

bool
nsSVGClipPathFrame::PointIsInsideClipPath(nsIFrame* aClippedFrame,
                                          const gfxPoint &aPoint)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return false;
  }
  AutoClipPathReferencer clipRef(this);

  gfxMatrix matrix = GetClipPathTransform(aClippedFrame);
  if (!matrix.Invert()) {
    return false;
  }
  gfxPoint point = matrix.Transform(aPoint);

  
  
  
  
  
  nsSVGClipPathFrame *clipPathFrame =
    nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nullptr);
  if (clipPathFrame &&
      !clipPathFrame->PointIsInsideClipPath(aClippedFrame, aPoint)) {
    return false;
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      gfxPoint pointForChild = point;
      gfxMatrix m = static_cast<nsSVGElement*>(kid->GetContent())->
        PrependLocalTransformsTo(gfxMatrix(), nsSVGElement::eUserSpaceToParent);
      if (!m.IsIdentity()) {
        if (!m.Invert()) {
          return false;
        }
        pointForChild = m.Transform(point);
      }
      if (SVGFrame->GetFrameForPoint(pointForChild)) {
        return true;
      }
    }
  }
  return false;
}

bool
nsSVGClipPathFrame::IsTrivial(nsISVGChildFrame **aSingleChild)
{
  
  if (nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(nullptr))
    return false;

  if (aSingleChild) {
    *aSingleChild = nullptr;
  }

  nsISVGChildFrame *foundChild = nullptr;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame *svgChild = do_QueryFrame(kid);
    if (svgChild) {
      
      
      if (foundChild || svgChild->IsDisplayContainer())
        return false;

      
      if (nsSVGEffects::GetEffectProperties(kid).GetClipPathFrame(nullptr))
        return false;

      foundChild = svgChild;
    }
  }
  if (aSingleChild) {
    *aSingleChild = foundChild;
  }
  return true;
}

bool
nsSVGClipPathFrame::IsValid()
{
  if (mInUse) {
    NS_WARNING("Clip loop detected!");
    return false;
  }
  AutoClipPathReferencer clipRef(this);

  bool isOK = true;
  nsSVGEffects::GetEffectProperties(this).GetClipPathFrame(&isOK);
  if (!isOK) {
    return false;
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {

    nsIAtom *type = kid->GetType();

    if (type == nsGkAtoms::svgUseFrame) {
      for (nsIFrame* grandKid = kid->GetFirstPrincipalChild(); grandKid;
           grandKid = grandKid->GetNextSibling()) {

        nsIAtom *type = grandKid->GetType();

        if (type != nsGkAtoms::svgPathGeometryFrame &&
            type != nsGkAtoms::svgTextFrame) {
          return false;
        }
      }
      continue;
    }
    if (type != nsGkAtoms::svgPathGeometryFrame &&
        type != nsGkAtoms::svgTextFrame) {
      return false;
    }
  }
  return true;
}

nsresult
nsSVGClipPathFrame::AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::transform) {
      nsSVGEffects::InvalidateDirectRenderingObservers(this);
      nsSVGUtils::NotifyChildrenOfSVGChange(this,
                                            nsISVGChildFrame::TRANSFORM_CHANGED);
    }
    if (aAttribute == nsGkAtoms::clipPathUnits) {
      nsSVGEffects::InvalidateDirectRenderingObservers(this);
    }
  }

  return nsSVGClipPathFrameBase::AttributeChanged(aNameSpaceID,
                                                  aAttribute, aModType);
}

void
nsSVGClipPathFrame::Init(nsIContent*       aContent,
                         nsContainerFrame* aParent,
                         nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVGElement(nsGkAtoms::clipPath),
               "Content is not an SVG clipPath!");

  AddStateBits(NS_STATE_SVG_CLIPPATH_CHILD);
  nsSVGClipPathFrameBase::Init(aContent, aParent, aPrevInFlow);
}

nsIAtom *
nsSVGClipPathFrame::GetType() const
{
  return nsGkAtoms::svgClipPathFrame;
}

gfxMatrix
nsSVGClipPathFrame::GetCanvasTM()
{
  return mMatrixForChildren;
}

gfxMatrix
nsSVGClipPathFrame::GetClipPathTransform(nsIFrame* aClippedFrame)
{
  SVGClipPathElement *content = static_cast<SVGClipPathElement*>(mContent);

  gfxMatrix tm = content->PrependLocalTransformsTo(gfxMatrix());

  nsSVGEnum* clipPathUnits =
    &content->mEnumAttributes[SVGClipPathElement::CLIPPATHUNITS];

  return nsSVGUtils::AdjustMatrixForUnits(tm, clipPathUnits, aClippedFrame);
}

SVGBBox
nsSVGClipPathFrame::GetBBoxForClipPathFrame(const SVGBBox &aBBox, 
                                            const gfxMatrix &aMatrix)
{
  nsIContent* node = GetContent()->GetFirstChild();
  SVGBBox unionBBox, tmpBBox;
  for (; node; node = node->GetNextSibling()) {
    nsIFrame *frame = 
      static_cast<nsSVGElement*>(node)->GetPrimaryFrame();
    if (frame) {
      nsISVGChildFrame *svg = do_QueryFrame(frame);
      if (svg) {
        tmpBBox = svg->GetBBoxContribution(mozilla::gfx::ToMatrix(aMatrix), 
                                         nsSVGUtils::eBBoxIncludeFill);
        nsSVGEffects::EffectProperties effectProperties =
                              nsSVGEffects::GetEffectProperties(frame);
        bool isOK = true;
        nsSVGClipPathFrame *clipPathFrame = 
                              effectProperties.GetClipPathFrame(&isOK);
        if (clipPathFrame && isOK) {
          tmpBBox = clipPathFrame->GetBBoxForClipPathFrame(tmpBBox, aMatrix);
        } 
        tmpBBox.Intersect(aBBox);
        unionBBox.UnionEdges(tmpBBox);
      }
    }
  }
  nsSVGEffects::EffectProperties props = 
    nsSVGEffects::GetEffectProperties(this);    
  if (props.mClipPath) {
    bool isOK = true;
    nsSVGClipPathFrame *clipPathFrame = props.GetClipPathFrame(&isOK);
    if (clipPathFrame && isOK) {
      tmpBBox = clipPathFrame->GetBBoxForClipPathFrame(aBBox, aMatrix);                                                       
      unionBBox.Intersect(tmpBBox);
    } else if (!isOK) {
      unionBBox = SVGBBox();
    }
  }
  return unionBBox;
}
